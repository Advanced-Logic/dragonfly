#ifndef _SLICE_CLIENT_H_
#define _SLICE_CLIENT_H_

#include "slice-connection.h"

typedef struct slice_client SliceClient;

#ifdef __cplusplus
extern "C" {
#endif

SliceClient *slice_client_create(SliceMainloop *mainloop, char *host, int port, SliceConnectionMode mode, SliceReturnType(*connect_result_cb)(SliceClient*, SliceReturnType, char*), char *err);
SliceReturnType slice_client_remove(SliceClient *client, char *err);
SliceReturnType slice_client_start(SliceClient *client, SliceSSLContext *ssl_ctx, SliceReturnType(*read_callback)(SliceClient*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), void *user_data, char *err);
SliceReturnType slice_client_write(SliceClient *client, SliceBuffer *buffer, char *err);
int slice_client_fetch_read_buffer(SliceClient *client, char *out, unsigned int out_size, char *err);
SliceBuffer *slice_client_get_read_buffer(SliceClient *client);
SliceReturnType slice_client_clear_read_buffer(SliceClient *client, char *err);

#ifdef __cplusplus
}
#endif

#define SliceClientCreate(_mainloop, _host, _port, _type, _result_cb, _err) slice_client_create(_mainloop, _host, _port, _type, _result_cb, _err)
#define SliceClientRemove(_client, _err) slice_client_remove(_client, _err)
#define SliceClientStart(_client, _ssl_ctx, _read_callback, _close_callback, _user_data, _err) slice_client_start(_client, _ssl_ctx, _read_callback, _close_callback, _user_data, _err)
#define SliceClientWrite(_client, _buffer, _err) slice_client_write(_client, _buffer, _err)
#define SliceClientFetchReadBuffer(_client, _out, _out_size, _err) slice_client_fetch_read_buffer(_client, _out, _out_size, _err)
#define SliceClientGetReadBuffer(_client) slice_client_get_read_buffer(_client)
#define SliceClientClearReadBuffer(_client, _err) slice_client_clear_read_buffer(_client, _err)

#endif
