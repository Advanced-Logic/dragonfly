#ifndef _SLICE_SERVER_H_
#define _SLICE_SERVER_H_

#include "slice-mainloop.h"
#include "slice-session.h"
#include "slice-ssl-server.h"

typedef enum slice_server_mode SliceServerMode;
typedef struct slice_server SliceServer;

enum slice_server_mode
{
    SLICE_SERVER_MODE_IP4_TCP = SLICE_CONNECTION_MODE_IP4_TCP,
    SLICE_SERVER_MODE_IP6_TCP = SLICE_CONNECTION_MODE_IP6_TCP,
    SLICE_SERVER_MODE_IP4_UDP = SLICE_CONNECTION_MODE_IP4_UDP
};

#ifdef __cplusplus
extern "C" {
#endif

SliceServer *slice_server_create(SliceMainloop *mainloop, SliceServerMode mode, char *bind_ip, int bind_port, SliceSSLContext *ssl_ctx, SliceReturnType(*accept_cb)(SliceSession*, char*), SliceReturnType(*ready_cb)(SliceSession*, char*), SliceReturnType(*read_callback)(SliceSession*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), char *err);
void slice_server_remove_session(SliceServer *server, SliceSession *session);   // for session remove only

#ifdef __cplusplus
}
#endif

#define SliceServerCreate(_mainloop, _mode, _bind_ip, _bind_port, _ssl_ctx, _accept_cb, _ready_cb, _read_callabck, _close_callback, _err) slice_server_create(_mainloop, _mode, _bind_ip, _bind_port, _ssl_ctx, _accept_cb, _ready_cb, _read_callabck, _close_callback, _err)
#define SliceServerRemoveSession(_server, _session) slice_server_remove_session(_server, _session)

#endif
