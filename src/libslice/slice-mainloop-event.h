#ifndef _SLICE_MAINLOOP_EVENT_H_
#define _SLICE_MAINLOOP_EVENT_H_

#include "slice-io.h"
#include "slice-mainloop.h"

struct slice_mainloop_event
{
    struct slice_io io;

    char name[128];

    struct slice_mainloop *mainloop;

    void *user_data;

    int destroyed;

    int(*remove_cb)(struct slice_mainloop_event *mainloop_event, char *err);

    int(*init_mainloop_cb)(struct slice_mainloop *mainloop, struct slice_mainloop_event *mainloop_event, char *err);

    int(*pre_mainloop_cb)(struct slice_mainloop *mainloop, struct slice_mainloop_event *mainloop_event, char *err);

    int(*process_mainloop_cb)(struct slice_mainloop *mainloop, struct slice_mainloop_event *mainloop_event, char *err);

    int(*post_mainloop_cb)(struct slice_mainloop *mainloop, struct slice_mainloop_event *mainloop_event, char *err);

    int(*finish_mainloop_cb)(struct slice_mainloop *mainloop, struct slice_mainloop_event *mainloop_event, char *err);
};

typedef struct slice_mainloop_event SliceMainloopEvent;

// loop definition
typedef enum slice_mainloop_callback_event SliceMainloopCallbackEvent;
typedef struct slice_mainloop SliceMainloop;

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_mainloop_event_set_name(SliceMainloopEvent *mainloop_event, char *name, char *err);
SliceReturnType slice_mainloop_event_set_user_data(SliceMainloopEvent *mainloop_event, void *user_data, char *err);
SliceReturnType slice_mainloop_event_set_callback(SliceMainloopEvent *mainloop_event, SliceMainloopCallbackEvent event_num, int(*ev_callback)(SliceMainloop*, SliceMainloopEvent*, char*), char *err);

#ifdef __cplusplus
}
#endif

#define SliceMainloopEventSetName(_event, _name, _err) slice_mainloop_event_set_name((SliceMainloopEvent*)_event, _name, _err)
#define SliceMainloopEventSetUserData(_event, _data, _err) slice_mainloop_event_set_user_data((SliceMainloopEvent*)_event, (void*)_data, _err)
#define SliceMainloopEventSetCallback(_event, _ev_num, _callback, _err) slice_mainloop_event_set_callback((SliceMainloopEvent*)_event, _ev_num, _callback, _err)

#endif
