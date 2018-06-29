#ifndef _SLICE_CLIENT_H_
#define _SLICE_CLIENT_H_

#include "slice-connection.h"

typedef struct slice_client SliceClient;

#ifdef __cplusplus
extern "C" {
#endif

SliceClient *slice_client_create(SliceMainloop *mainloop, char *host, int port, SliceConnectionType type, SliceReturnType(*connect_result_cb)(SliceClient*, int, char*), char *err);
SliceReturnType slice_client_remove(SliceClient *client, char *err);
SliceReturnType slice_client_start(SliceClient *client, SliceSSLContext *ssl_ctx, SliceReturnType(*read_callback)(SliceClient*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), void *user_data, char *err);
SliceReturnType slice_client_write(SliceClient *client, SliceBuffer *buffer, char *err);

#ifdef __cplusplus
}
#endif

#define SliceClientCreate(_mainloop, _host, _port, _type, _result_cb, _err) slice_client_create(_mainloop, _host, _port, _type, _result_cb, _err)
#define SliceClientRemove(_client, _err) slice_client_remove(_client, _err)
#define SliceClientStart(_client, _ssl_ctx, _read_callback, _close_callback, _user_data, _err) slice_client_start(_client, _ssl_ctx, _read_callback, _close_callback, _user_data, _err)
#define SliceClientWrite(_client, _buffer, _err) slice_client_write(_client, _buffer, _err)

#endif
