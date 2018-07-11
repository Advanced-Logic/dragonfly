#ifndef _SLICE_MAINLOOP_H_
#define _SLICE_MAINLOOP_H_

#include <sys/epoll.h>

#include "slice-io.h"
#include "slice-buffer.h"

#define SLICE_MAINLOOP_MAX_EVENT            (63 * 1024)

typedef struct slice_mainloop SliceMainloop;
typedef enum slice_mainloop_callback_event SliceMainloopCallbackEvent;
typedef struct slice_mainloop_event SliceMainloopEvent;

typedef enum slice_mainloop_epoll_event_callback SliceMainloopEpollEventCallback;
typedef struct slice_mainloop_epoll SliceMainloopEpoll;
typedef struct slice_mainloop_epoll_element SliceMainloopEpollElement;

// loop struct definetion
typedef struct slice_buffer SliceBuffer;

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

struct slice_mainloop_event
{
    struct slice_io io;

    char name[128];

    SliceMainloop *mainloop;

    void *user_data;

    int destroyed;

    int(*remove_cb)(SliceMainloopEvent *mainloop_event, char *err);

    int(*init_mainloop_cb)(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);

    int(*pre_mainloop_cb)(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);

    int(*process_mainloop_cb)(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);

    int(*post_mainloop_cb)(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);

    int(*finish_mainloop_cb)(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);
};

#ifdef __cplusplus
extern "C" {
#endif

SliceMainloop *slice_mainloop_create(int epoll_max_fd, int epoll_max_fetch_event, int epoll_timeout, char *err);
SliceReturnType slice_mainloop_destroy(SliceMainloop *mainloop, char *err);
SliceReturnType slice_mainloop_event_add(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, SliceReturnType(*event_add_cb)(SliceMainloopEvent*, char*), SliceReturnType(*event_remove_cb)(SliceMainloopEvent*, char*), char *err);
SliceReturnType slice_mainloop_event_remove(SliceMainloop *mainloop, SliceMainloopEvent *mainloop_event, char *err);
SliceReturnType slice_mainloop_set_user_data(SliceMainloop *mainloop, void *user_data, char *err);
SliceReturnType slice_mainloop_set_callback(SliceMainloop *mainloop, SliceMainloopCallbackEvent event_num, int(*ev_callback)(SliceMainloop*, void*, char*), char *err);
SliceReturnType slice_mainloop_run(SliceMainloop *mainloop, char *err);
void slice_mainloop_quit(SliceMainloop *mainloop);
SliceBuffer *slice_mainloop_get_buffer_bucket(SliceMainloop *mainloop);
int slice_mainloop_get_buffer_bucket_count(SliceMainloop *mainloop);
SliceReturnType slice_mainloop_buffer_bucket_add(SliceMainloop *mainloop, SliceBuffer *buffer, char *err);
SliceReturnType slice_mainloop_buffer_bucket_remove(SliceMainloop *mainloop, SliceBuffer *buffer, char *err);

SliceMainloopEpollElement *slice_mainloop_epoll_get_event_element(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_set_callback(SliceMainloop *mainloop, int fd, SliceMainloopEpollEventCallback flag, void *callback, char *err);
//SliceReturnType slice_mainloop_epoll_event_update(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_add_read(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_add_write(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove_read(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove_write(SliceMainloop *mainloop, int fd, char *err);
SliceReturnType slice_mainloop_epoll_event_remove(SliceMainloop *mainloop, int fd, char *err);

SliceMainloopEvent *slice_mainloop_epoll_element_get_slice_mainloop_event(SliceMainloopEpollElement *mainloop_epoll_element);
SliceReturnType slice_mainloop_epoll_element_set_slice_mainloop_event(SliceMainloopEpollElement *mainloop_epoll_element, SliceMainloopEvent *slice_event, char *err);

SliceReturnType slice_mainloop_event_set_name(SliceMainloopEvent *mainloop_event, char *name, char *err);
SliceReturnType slice_mainloop_event_set_user_data(SliceMainloopEvent *mainloop_event, void *user_data, char *err);
SliceReturnType slice_mainloop_event_set_callback(SliceMainloopEvent *mainloop_event, SliceMainloopCallbackEvent event_num, SliceReturnType(*ev_callback)(SliceMainloop*, SliceMainloopEvent*, char*), char *err);

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
#define SliceMainloopGetBufferBucket(_mainloop) slice_mainloop_get_buffer_bucket(_mainloop)
#define SliceMainloopGetBufferBucketCount(_mainloop) slice_mainloop_get_buffer_bucket_count(_mainloop)
#define SliceMainloopBufferBucketAdd(_mainloop, _buffer, _err) slice_mainloop_buffer_bucket_add(_mainloop, _buffer, _err)
#define SliceMainloopBufferBucketRemove(_mainloop, _buffer, _err) slice_mainloop_buffer_bucket_remove(_mainloop, _buffer, _err)

#define SliceMainloopEpollGetEventElement(_mainloop, _fd, _err) slice_mainloop_epoll_get_event_element(_mainloop, _fd, _err)
#define SliceMainloopEpollEventSetCallback(_mainloop, _fd, _flag, _callback, _err) slice_mainloop_epoll_set_callback(_mainloop, _fd, _flag, _callback, _err)
//#define SliceMainloopEpollEventUpdate(_mainloop, _fd, _err) slice_mainloop_epoll_event_update(_mainloop, _fd, _err)
#define SliceMainloopEpollEventAddRead(_mainloop, _fd, _err) slice_mainloop_epoll_event_add_read(_mainloop, _fd, _err)
#define SliceMainloopEpollEventAddWrite(_mainloop, _fd, _err) slice_mainloop_epoll_event_add_write(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemoveRead(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove_read(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemoveWrite(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove_write(_mainloop, _fd, _err)
#define SliceMainloopEpollEventRemove(_mainloop, _fd, _err) slice_mainloop_epoll_event_remove(_mainloop, _fd, _err)

#define SliceMainloopEpollElementGetSliceMainloopEvent(_mainloop_epoll_element) slice_mainloop_epoll_element_get_slice_mainloop_event(_mainloop_epoll_element)
#define SliceMainloopEpollElementSetSliceMainloopEvent(_mainloop_epoll_element, _slice_event, _err) slice_mainloop_epoll_element_set_slice_mainloop_event(_mainloop_epoll_element, _slice_event, _err)

#define SliceMainloopEventSetName(_event, _name, _err) slice_mainloop_event_set_name((SliceMainloopEvent*)_event, _name, _err)
#define SliceMainloopEventSetUserData(_event, _data, _err) slice_mainloop_event_set_user_data((SliceMainloopEvent*)_event, (void*)_data, _err)
#define SliceMainloopEventSetCallback(_event, _ev_num, _callback, _err) slice_mainloop_event_set_callback((SliceMainloopEvent*)_event, _ev_num, _callback, _err)

#endif
