#ifndef _SLICE_MAINLOOP_H_
#define _SLICE_MAINLOOP_H_

#include <sys/epoll.h>

#include "slice-buffer.h"
#include "slice-mainloop-event.h"

#define SLICE_MAINLOOP_MAX_EVENT            8

enum slice_mainloop_callback_event
{
    SLICE_MAINLOOP_EVENT_INIT = 0,
    SLICE_MAINLOOP_EVENT_PRELOOP,
    SLICE_MAINLOOP_EVENT_PROCESS,
    SLICE_MAINLOOP_EVENT_POSTLOOP,
    SLICE_MAINLOOP_EVENT_FINISH
};

enum slice_mainloop_epoll_event_callback
{
    SLICE_MAINLOOP_EPOLL_EVENT_WRITE = 0,
    SLICE_MAINLOOP_EPOLL_EVENT_READ,
    SLICE_MAINLOOP_EPOLL_EVENT_CLOSE
};

struct slice_mainloop_epoll
{
    int epoll_fd;
    
    int timeout;
    int max_fetch_event;
    int max_fd;

    struct epoll_event *event_bucket;
    struct slice_mainloop_epoll_element *element_table;
};

struct slice_mainloop_epoll_element
{
    int fd;

    struct slice_mainloop_event *slice_event;

    int need_read;
    int need_write;
    //int adge_trigger_flag;

    int(*write_cb)(struct slice_mainloop_epoll*, struct slice_mainloop_epoll_element*, struct epoll_event, void*);
    int(*read_cb)(struct slice_mainloop_epoll*, struct slice_mainloop_epoll_element*, struct epoll_event, void*);
    int(*close_cb)(struct slice_mainloop_epoll*, struct slice_mainloop_epoll_element*, struct epoll_event, void*);
};

struct slice_mainloop
{
    struct slice_mainloop_event *event_list;
    int event_list_count;

    int quit;

    void *user_data;

    struct slice_mainloop_epoll *epoll;

    struct slice_buffer *buffer_bucket;
    int buffer_bucket_count;

    int(*init_mainloop_cb)(struct slice_mainloop *mainloop, void *user_data, char *err);

    int(*pre_loop_cb)(struct slice_mainloop *mainloop, void *user_data, char *err);

    int(*process_cb)(struct slice_mainloop *mainloop, void *user_data, char *err);

    int(*post_loop_cb)(struct slice_mainloop *mainloop, void *user_data, char *err);

    int(*finish_mainloop_cb)(struct slice_mainloop *mainloop, void *user_data, char *err);
};

typedef struct slice_mainloop SliceMainloop;
typedef enum slice_mainloop_callback_event SliceMainloopCallbackEvent;

typedef enum slice_mainloop_epoll_event_callback SliceMainloopEpollEventCallback;
typedef struct slice_mainloop_epoll SliceMainloopEpoll;
typedef struct slice_mainloop_epoll_element SliceMainloopEpollElement;

// loop definition
typedef struct slice_mainloop_event SliceMainloopEvent;
typedef struct slice_buffer SliceBuffer;

#ifdef __cplusplus
extern "C" {
#endif

SliceMainloop *slice_mainloop_create(int epoll_max_fd, int epoll_max_fetch_event, int epoll_timeout, char *err);
SliceReturnType slice_mainloop_destroy(SliceMainloop *mainloop, char *err);
SliceReturnType slice_mainloop_event_add(SliceMainloop *mainloop, SliceMainloopEvent *event, int(*event_add_cb)(SliceMainloopEvent*, char*), int(*event_remove_cb)(SliceMainloopEvent*, char*), char *err);
SliceReturnType slice_mainloop_event_remove(SliceMainloop *mainloop, SliceMainloopEvent *event, char *err);
SliceReturnType slice_mainloop_set_user_data(SliceMainloop *mainloop, void *user_data, char *err);
SliceReturnType slice_mainloop_set_callback(SliceMainloop *mainloop, SliceMainloopCallbackEvent event_num, int(*ev_callback)(SliceMainloop*, void*, char*), char *err);
SliceReturnType slice_mainloop_run(SliceMainloop *mainloop, char *err);
void slice_mainloop_quit(SliceMainloop *mainloop);

SliceMainloopEpollElement *slice_mainloop_epoll_get_event_element(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_set_callback(SliceMainloop *mainloop, int fd, SliceMainloopEpollEventCallback flag, void *callback, char *err);
//SliceReturnType slice_mainloop_epoll_event_update(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_add_read(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_add_write(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove_read(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove_write(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove(SliceMainloop *mainloop, int fd, char *err);

#ifdef __cplusplus
}
#endif

#define SliceMainloopCreate(_max_fd, _max_fetch_ev, _timeout, _err) slice_mainloop_create(_max_fd, _max_fetch_ev, _timeout, _err)
#define SliceMainloopDestroy(_mainloop, _err) slice_mainloop_destroy(_mainloop, _err)
#define SliceMainloopEventAdd(_mainloop, _event, _ev_add_cb, _ev_remove_cb, _err) slice_mainloop_event_add(_mainloop, (SliceMainloopEvent*)_event, _ev_add_cb, _ev_remove_cb, _err)
#define SliceMainloopEventRemove(_mainloop, _event, _err) slice_mainloop_event_remove(_mainloop, (SliceMainloopEvent*)_event, _err)
#define SliceMainloopSetUserData(_mainloop, _user_data, _err) slice_mainloop_set_user_data(_mainloop, _user_data, _err)
#define SliceMainloopSetCallback(_mainloop, _ev_num, _callback, _err) slice_mainloop_set_callback(_mainloop, _ev_num, _callback, _err)
#define SliceMainloopRun(_mainloop, _err) slice_mainloop_run(_mainloop, _err)
#define SliceMainloopQuit(_mainloop) slice_mainloop_quit(_mainloop)

#define SliceMainloopEpollGetEventElement(_mainloop, _fd, _err) slice_mainloop_epoll_get_event_element(_mainloop, _fd, _err)
#define SliceMainloopEpollEventSetCallback(_mainloop, _fd, _flag, _callback, _err) slice_mainloop_epoll_set_callback(_mainloop, _fd, _flag, _callback, _err)
//#define SliceMainloopEpollEventUpdate(_mainloop, _fd, _err) slice_mainloop_epoll_event_update(_mainloop, _fd, _err)
#define SliceMainloopEpollEventAddRead(_mainloop, _fd, _err) slice_mainloop_epoll_event_add_read(_mainloop, _fd, _err)
#define SliceMainloopEpollEventAddWrite(_mainloop, _fd, _err) slice_mainloop_epoll_event_add_write(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemoveRead(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove_read(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemoveWrite(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove_write(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemove(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove(_mainloop, _fd, _err)

#endif
