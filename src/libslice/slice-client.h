#ifndef _SLICE_CLIENT_H_
#define _SLICE_CLIENT_H_

#include "slice-mainloop-event.h"
#include "slice-connection.h"

struct slice_client
{
    struct slice_mainloop_event mainloop_event;

    struct slice_connection *connection;

    char host[128];
    int port;

    int(*connect_result_cb)(struct slice_client*, struct slice_connection*, int, char*);
};

typedef struct slice_client SliceClient;

SliceClient *slice_client_create(SliceMainloop *mainloop, char *host, int port, SliceConnectionType type, int(*connect_result_cb)(SliceClient*, SliceConnection*, int, char*), char *err);
SliceReturnType slice_client_remove(SliceClient *client, char *err);

#define SliceClientCreate(_mainloop, _host, _port, _type, _result_cb, _err) slice_client_create(_mainloop, _host, _port, _type, _result_cb, _err)
#define SliceClientRemove(_client, _err) slice_client_remove(_client, _err)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif