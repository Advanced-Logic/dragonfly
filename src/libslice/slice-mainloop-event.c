
#include <stdio.h>
#include <string.h>

#include "slice-mainloop-event.h"

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

SliceReturnType slice_mainloop_event_set_callback(SliceMainloopEvent *mainloop_event, SliceMainloopCallbackEvent event_num, int(*ev_callback)(SliceMainloop*, SliceMainloopEvent*, char*), char *err)
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
