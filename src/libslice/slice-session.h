#ifndef _SLICE_SESSION_H_
#define _SLICE_SESSION_H_

#include "slice-mainloop.h"
#include "slice-connection.h"

struct slice_session
{
    struct slice_mainloop_event mainloop_event;

    struct slice_connection *connection;

    char ip[128];
    int port;

    SliceReturnType(*read_callback)(struct slice_session*, int, void*, char*);
};

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif
