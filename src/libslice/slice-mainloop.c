
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "slice-mainloop.h"

struct slice_mainloop_epoll
{
    int epoll_fd;

    int timeout;
    int max_fetch_event;
    int max_fd;

    struct epoll_event *event_bucket;
    SliceMainloopEpollElement *element_table;
};

struct slice_mainloop_epoll_element
{
    int fd;

    SliceMainloopEvent *slice_event;

    int need_read;
    int need_write;

    SliceReturnType(*write_cb)(SliceMainloopEpoll*, SliceMainloopEpollElement*, struct epoll_event, void*);
    SliceReturnType(*read_cb)(SliceMainloopEpoll*, SliceMainloopEpollElement*, struct epoll_event, void*);
    SliceReturnType(*close_cb)(SliceMainloopEpoll*, SliceMainloopEpollElement*, struct epoll_event, void*);
};

struct slice_mainloop
{
    SliceMainloopEvent *event_list;
    int event_list_count;

    int quit;

    void *user_data;

    SliceMainloopEpoll *epoll;

    SliceBuffer *buffer_bucket;
    int buffer_bucket_count;

    SliceReturnType(*init_mainloop_cb)(SliceMainloop *mainloop, void *user_data, char *err);

    SliceReturnType(*pre_loop_cb)(SliceMainloop *mainloop, void *user_data, char *err);

    SliceReturnType(*process_cb)(SliceMainloop *mainloop, void *user_data, char *err);

    SliceReturnType(*post_loop_cb)(SliceMainloop *mainloop, void *user_data, char *err);

    SliceReturnType(*finish_mainloop_cb)(SliceMainloop *mainloop, void *user_data, char *err);
};

SliceMainloop *slice_mainloop_create(int epoll_max_fd, int epoll_max_fetch_event, int epoll_timeout, char *err)
{
    SliceMainloop *mainloop;
    SliceMainloopEpoll *epoll;
    SliceMainloopEpollElement *element_table;

    int i;

    if (!(mainloop = (SliceMainloop*)malloc(sizeof(SliceMainloop)))) {
        if (err) sprintf(err, "Can't allocate mainloop memory");
        return NULL;
    }

    memset(mainloop, 0, sizeof(SliceMainloop));

    if (!(epoll = mainloop->epoll = (SliceMainloopEpoll*)malloc(sizeof(SliceMainloopEpoll)))) {
        if (err) sprintf(err, "Can't allocate mainloop epoll memory");
        free(mainloop);
        return NULL;
    }

    memset(mainloop->epoll, 0, sizeof(SliceMainloopEpoll));

    epoll->max_fetch_event = epoll_max_fetch_event;
    epoll->max_fd = epoll_max_fd;
    epoll->timeout = epoll_timeout;

    if ((epoll->epoll_fd = epoll_create(epoll_max_fetch_event)) < 0) {
        if (err) sprintf(err, "epoll_create return error [%s]", strerror(errno));
        free(epoll);
        free(mainloop);
        return NULL;
    }

    if (!(element_table = epoll->element_table = (SliceMainloopEpollElement*)malloc(sizeof(SliceMainloopEpollElement) * epoll_max_fd))) {
        if (err) sprintf(err, "Can't allocate mainloop epoll element table memory");
        close(epoll->epoll_fd);
        free(epoll);
        free(mainloop);
        return NULL;
    }

    memset(epoll->element_table, 0, sizeof(SliceMainloopEpollElement) * epoll_max_fd);

    for (i = 0; i < epoll_max_fd; i++) {
        element_table[i].fd = -1;
    }

    if (!(epoll->event_bucket = (struct epoll_event*)malloc(sizeof(struct epoll_event) * epoll_max_fetch_event))) {
        if (err) sprintf(err, "Can't allocate mainloop epoll event bucket memory");
        close(epoll->epoll_fd);
        free(element_table);
        free(epoll);
        free(mainloop);
        return NULL;
    }

    memset(epoll->event_bucket, 0, sizeof(struct epoll_event) * epoll_max_fetch_event);

    return mainloop;
}

SliceReturnType slice_mainloop_destroy(SliceMainloop *mainloop, char *err)
{
    SliceMainloopEvent *mainloop_event;
    SliceBuffer *buffer;

    if (!mainloop) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    while ((mainloop_event = mainloop->event_list)) {
        slice_mainloop_event_remove(mainloop, mainloop_event, NULL);
        mainloop->event_list_count--;
        free(mainloop_event);
    }

    while ((buffer = mainloop->buffer_bucket)) {
        SliceListRemove(&(mainloop->buffer_bucket), buffer, NULL);
        free(buffer);
    }

    if (mainloop->epoll) {
        if (mainloop->epoll->element_table) {
            free(mainloop->epoll->element_table);
            mainloop->epoll->element_table = NULL;
        }

        if (mainloop->epoll->event_bucket) {
            free(mainloop->epoll->event_bucket);
            mainloop->epoll->event_bucket = NULL;
        }

        close(mainloop->epoll->epoll_fd);

        free(mainloop->epoll);
        mainloop->epoll = NULL;
    }

    free(mainloop);

    return SLICE_RETURN_NORMAL;
}

SliceMainloopEpollElement *slice_mainloop_epoll_get_event_element(SliceMainloop *mainloop, int fd, char *err)
{
    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return NULL;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return NULL;
    }

    return &(mainloop->epoll->element_table[fd]);
}

SliceReturnType slice_mainloop_epoll_set_callback(SliceMainloop *mainloop, int fd, SliceMainloopEpollEventCallback flag, void *callback, char *err)
{
    SliceMainloopEpollElement *element;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((element = slice_mainloop_epoll_get_event_element(mainloop, fd, err_buff))) {
        switch (flag) {
            case SLICE_MAINLOOP_EPOLL_EVENT_WRITE:
                element->write_cb = callback;
                break;

            case SLICE_MAINLOOP_EPOLL_EVENT_READ:
                element->read_cb = callback;
                break;

            case SLICE_MAINLOOP_EPOLL_EVENT_CLOSE:
                element->close_cb = callback;
                break;

            default:
                if (err) sprintf(err, "Invalid callback flag [%d]", (int)flag);
                return SLICE_RETURN_ERROR;
        }
    } else {
        if (err) sprintf(err, "slice_mainloop_epoll_get_event_element FD [%d] return NULL [%s]", fd, err_buff);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_update(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;
    uint32_t flags = 0;
    struct epoll_event ev;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (element->need_read) flags |= EPOLLIN;
    if (element->need_write) flags |= EPOLLOUT;
    if (flags) flags |= EPOLLET;

    //printf("Update [%d][%d]\n", element->need_read, element->need_write);

    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = flags;

    if (element->fd == fd) {
        if (epoll_ctl(mainloop->epoll->epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            if (err) sprintf(err, "epoll_ctl return error [%s]", strerror(errno));
            return SLICE_RETURN_ERROR;
        }
    } else {
        if (epoll_ctl(mainloop->epoll->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            if (err) sprintf(err, "epoll_ctl return error [%s]", strerror(errno));
            return SLICE_RETURN_ERROR;
        }
    }

    element->fd = fd;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_add_read(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (!element->need_read) {
        element->need_read = 1;
        return slice_mainloop_epoll_event_update(mainloop, fd, err);
    }
    
    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_add_write(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (!element->need_write) {
        element->need_write = 1;
        return slice_mainloop_epoll_event_update(mainloop, fd, err);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_remove_read(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (element->need_read) {
        element->need_read = 0;
        return slice_mainloop_epoll_event_update(mainloop, fd, err);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_remove_write(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (element->need_write) {
        element->need_write = 0;
        return slice_mainloop_epoll_event_update(mainloop, fd, err);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_epoll_event_remove(SliceMainloop *mainloop, int fd, char *err)
{
    SliceMainloopEpollElement *element;

    if (!mainloop || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (fd >= mainloop->epoll->max_fd) {
        if (err) sprintf(err, "FD [%d] table exceed, max [%d]", fd, mainloop->epoll->max_fd - 1);
        return SLICE_RETURN_ERROR;
    }

    element = &(mainloop->epoll->element_table[fd]);

    if (element->fd >= 0) {
        if (epoll_ctl(mainloop->epoll->epoll_fd, EPOLL_CTL_DEL, element->fd, NULL) < 0) {
            if (err) sprintf(err, "epoll_ctl return error [%s]", strerror(errno));
            return SLICE_RETURN_ERROR;
        }

        memset(element, 0, sizeof(SliceMainloopEpollElement));
        element->fd = -1;
    }

    return SLICE_RETURN_NORMAL;
}
/*
static int slice_mainloop_event_process_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event event, void *user_data)
{
    SliceMainloopEvent *slice_event = (SliceMainloopEvent*)user_data;
    int r;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (slice_event && slice_event->process_mainloop_cb) {
        if ((r = slice_event->process_mainloop_cb(slice_event->mainloop, slice_event, err_buff)) < SLICE_RETURN_NORMAL) {
            // skipp error buffer
        }

        return r;
    }

    return SLICE_RETURN_NORMAL;
}
*/
SliceReturnType slice_mainloop_event_add(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, SliceReturnType(*event_add_cb)(SliceMainloopEvent*, char*), SliceReturnType(*event_remove_cb)(SliceMainloopEvent*, char*), char *err)
{
    SliceMainloopEpollElement *element;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop || !mainloop_event || !event_add_cb || !event_remove_cb) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (mainloop->event_list_count >= SLICE_MAINLOOP_MAX_EVENT) {
        if (err) sprintf(err, "Event is full");
        return SLICE_RETURN_ERROR;
    }

    //if (!event->process_mainloop_cb) {
    //    if (err) sprintf(err, "Can't add mainloop event [%s] without main process callback", event->name);
    //    return SLICE_RETURN_ERROR;
    //}

    if (mainloop_event->io.fd < 0 || mainloop_event->io.fd > 0xffff) {
        if (err) sprintf(err, "Event io is not open/initialize");
        return SLICE_RETURN_ERROR;
    }

    if (!(element = slice_mainloop_epoll_get_event_element(mainloop, mainloop_event->io.fd, err_buff))) {
        if (err) sprintf(err, "slice_mainloop_epoll_get_event_element return [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    // add to epoll
    //if (slice_mainloop_epoll_event_add_read(mainloop, event->io.fd, err_buff) != SLICE_RETURN_NORMAL) {
    //    if (err) sprintf(err, "Add mainloop event to epoll return error [%s]", err_buff);
    //    return SLICE_RETURN_ERROR;
    //}

    //slice_mainloop_epoll_set_callback(mainloop, event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, (void*)slice_mainloop_event_process_callback, NULL);

    element->slice_event = mainloop_event;
    mainloop_event->mainloop = mainloop;

    if (event_add_cb && event_add_cb(mainloop_event, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "Add event callback is return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    mainloop_event->remove_cb = event_remove_cb;

    SliceListAppend(&(mainloop->event_list), mainloop_event, NULL);
    mainloop->event_list_count++;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_event_remove(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err)
{
    SliceMainloopEvent *tmp;
    SliceMainloopEpollElement *element;

    if (!mainloop || !mainloop_event) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((tmp = mainloop->event_list)) {
        do {
            if (tmp == mainloop_event) {
                SliceListRemove(&(mainloop->event_list), mainloop_event, NULL);
                mainloop->event_list_count--;

                // remove from epoll
                if ((element = slice_mainloop_epoll_get_event_element(mainloop, mainloop_event->io.fd, NULL))) {
                    element->slice_event = NULL;
                }
                slice_mainloop_epoll_event_remove(mainloop, mainloop_event->io.fd, NULL);

                if (mainloop_event->remove_cb && mainloop_event->remove_cb(mainloop_event, err) != SLICE_RETURN_NORMAL) {
                    return SLICE_RETURN_ERROR;
                }

                mainloop_event->mainloop = NULL;

                return SLICE_RETURN_NORMAL;
            }

            tmp = (SliceMainloopEvent*)tmp->io.obj.next;
        } while(tmp != mainloop->event_list);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_set_user_data(SliceMainloop *mainloop, void *user_data, char *err)
{
    if (!mainloop) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop->user_data = user_data;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_set_callback(SliceMainloop *mainloop, SliceMainloopCallbackEvent event_num, int(*ev_callback)(SliceMainloop*, void*, char*), char *err)
{
    if (!mainloop) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    switch (event_num) {
        case SLICE_MAINLOOP_EVENT_INIT:
            mainloop->init_mainloop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_PRELOOP:
            mainloop->pre_loop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_PROCESS:
            mainloop->process_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_POSTLOOP:
            mainloop->post_loop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_FINISH:
            mainloop->finish_mainloop_cb = ev_callback;
            break;

        default:
            if (err) sprintf(err, "Invalis event number [%d]", (int)event_num);
            return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_run(SliceMainloop *mainloop, char *err)
{
    SliceReturnType ret;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    int event_count, i;
    struct epoll_event* event_bucket;
    SliceMainloopEpollElement *element;
    
    err_buff[0] = 0;

    if (!mainloop) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop->quit = 0;

    event_bucket = mainloop->epoll->event_bucket;

    // main init
    if (mainloop->init_mainloop_cb) {
        if ((ret = mainloop->init_mainloop_cb(mainloop, (void*)mainloop->user_data, err_buff)) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "mainloop->init_mainloop_cb return [%d] [%s]", (int)ret, err_buff);
            return ret;
        }
    }

    // external init

    while (!mainloop->quit) {
        // main pre
        if (mainloop->pre_loop_cb) {
            if ((ret = mainloop->pre_loop_cb(mainloop, (void*)mainloop->user_data, err_buff)) != SLICE_RETURN_NORMAL) {
                if (err) sprintf(err, "mainloop->pre_loop_cb return [%d] [%s]", (int)ret, err_buff);
                return ret;
            }
        }

        // external pre


        // main process
        if (mainloop->process_cb) {
            if ((ret = mainloop->process_cb(mainloop, (void*)mainloop->user_data, err_buff)) != SLICE_RETURN_NORMAL) {
                if (err) sprintf(err, "mainloop->process_cb return [%d] [%s]", (int)ret, err_buff);
                return ret;
            }
        }

        // external epoll event
        if ((event_count = epoll_wait(mainloop->epoll->epoll_fd, event_bucket, mainloop->epoll->max_fetch_event, mainloop->epoll->timeout)) < 0) {
            if (err) sprintf(err, "epoll_wait return error [%s]", strerror(errno));
            return SLICE_RETURN_ERROR;
        }

        for (i = 0; i < event_count; i++) {
            if (event_bucket[i].data.fd < 0 || event_bucket[i].data.fd >= mainloop->epoll->max_fd) {
                // FD table exceed
                continue;
            }

            if (!(element = (SliceMainloopEpollElement*)&(mainloop->epoll->element_table[event_bucket[i].data.fd]))) {
                // Can't locate event FD
                continue;
            }

            if (event_bucket[i].events & EPOLLIN) {
                if (element->read_cb && element->read_cb(mainloop->epoll, element, event_bucket[i], (void*)element->slice_event) != SLICE_RETURN_NORMAL) {
                    continue;
                }
            }
            if (event_bucket[i].events & EPOLLOUT) {
                slice_mainloop_epoll_event_remove_write(mainloop, event_bucket[i].data.fd, NULL);

                if (element->write_cb && element->write_cb(mainloop->epoll, element, event_bucket[i], (void*)element->slice_event) != SLICE_RETURN_NORMAL) {
                    continue;
                }
            }
            if (event_bucket[i].events & EPOLLERR || event_bucket[i].events & EPOLLHUP) {
                if (element->close_cb) {
                    element->close_cb(mainloop->epoll, element, event_bucket[i], (void*)element->slice_event);
                }
            }
            slice_mainloop_epoll_event_update(mainloop, event_bucket[i].data.fd, err);
        }

        // main post
        if (mainloop->post_loop_cb) {
            if ((ret = mainloop->post_loop_cb(mainloop, (void*)mainloop->user_data, err_buff)) != SLICE_RETURN_NORMAL) {
                if (err) sprintf(err, "mainloop->post_loop_cb return [%d] [%s]", (int)ret, err_buff);
                return ret;
            }
        }

        // external post
    }

    // main finish
    if (mainloop->finish_mainloop_cb) {
        if ((ret = mainloop->finish_mainloop_cb(mainloop, (void*)mainloop->user_data, err_buff)) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "mainloop->finish_mainloop_cb return [%d] [%s]", (int)ret, err_buff);
            return ret;
        }
    }

    // external finish

    return SLICE_RETURN_NORMAL;
}

void slice_mainloop_quit(SliceMainloop *mainloop)
{
    if (!mainloop) return;

    mainloop->quit = 1;
}

struct slice_buffer *slice_mainloop_get_buffer_bucket(SliceMainloop *mainloop)
{
    if (!mainloop) return NULL;

    return mainloop->buffer_bucket;
}

int slice_mainloop_get_buffer_bucket_count(SliceMainloop *mainloop)
{
    if (!mainloop) return -1;

    return mainloop->buffer_bucket_count;
}

SliceReturnType slice_mainloop_buffer_bucket_add(SliceMainloop *mainloop, SliceBuffer *buffer, char *err)
{
    if (!mainloop || !buffer) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (buffer->obj.next || buffer->obj.prev) {
        if (err) sprintf(err, "Buffer is may be exist in lists");
        return SLICE_RETURN_ERROR;
    }

    SliceListAppend(&(mainloop->buffer_bucket), buffer, NULL);
    mainloop->buffer_bucket_count++;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_buffer_bucket_remove(SliceMainloop *mainloop, SliceBuffer *buffer, char *err)
{
    if (!mainloop || !buffer) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!buffer->obj.next || !buffer->obj.prev) {
        if (err) sprintf(err, "Buffer is may be not exist in lists");
        return SLICE_RETURN_ERROR;
    }

    SliceListRemove(&(mainloop->buffer_bucket), buffer, NULL);
    mainloop->buffer_bucket_count--;

    return SLICE_RETURN_NORMAL;
}

SliceMainloopEvent *slice_mainloop_epoll_element_get_slice_mainloop_event(SliceMainloopEpollElement *mainloop_epoll_element)
{
    if (!mainloop_epoll_element) return NULL;

    return mainloop_epoll_element->slice_event;
}

SliceReturnType slice_mainloop_epoll_element_set_slice_mainloop_event(SliceMainloopEpollElement *mainloop_epoll_element, SliceMainloopEvent *mainloop_event, char *err)
{
    if (!mainloop_epoll_element || !mainloop_event) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop_epoll_element->slice_event = mainloop_event;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_event_set_name(SliceMainloopEvent *mainloop_event, char *name, char *err)
{
    if (!mainloop_event || !name) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    strcpy(mainloop_event->name, name);

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_event_set_user_data(SliceMainloopEvent *mainloop_event, void *user_data, char *err)
{
    if (!mainloop_event) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop_event->user_data = user_data;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_mainloop_event_set_callback(SliceMainloopEvent *mainloop_event, SliceMainloopCallbackEvent event_num, SliceReturnType(*ev_callback)(SliceMainloop*, SliceMainloopEvent*, char*), char *err)
{
    if (!mainloop_event) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    switch (event_num) {
        case SLICE_MAINLOOP_EVENT_INIT:
            mainloop_event->init_mainloop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_PRELOOP:
            mainloop_event->pre_mainloop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_PROCESS:
            mainloop_event->process_mainloop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_POSTLOOP:
            mainloop_event->post_mainloop_cb = ev_callback;
            break;

        case SLICE_MAINLOOP_EVENT_FINISH:
            mainloop_event->finish_mainloop_cb = ev_callback;
            break;

        default:
            if (err) sprintf(err, "Invalid event number [%d]", (int)event_num);
            return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}
