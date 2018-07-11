#ifndef _SLICE_SESSION_H_
#define _SLICE_SESSION_H_

#include "slice-connection.h"

typedef struct slice_session SliceSession;

//
typedef struct slice_server SliceServer;

#ifdef __cplusplus
extern "C" {
#endif

SliceSession *slice_session_create(SliceMainloop *mainloop, SliceServer *server, int sock, struct sockaddr *addr, SliceConnectionMode mode, SliceSSLContext *ssl_ctx, SliceReturnType(*read_callback)(SliceSession*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), void *user_data, char *err);
SliceReturnType slice_session_remove(SliceSession *session, char *err);
SliceReturnType slice_session_write(SliceSession *session, SliceBuffer *buffer, char *err);
int slice_session_fetch_read_buffer(SliceSession *session, char *out, unsigned int out_size, char *err);
SliceBuffer *slice_session_get_read_buffer(SliceSession *session);
SliceReturnType slice_session_clear_read_buffer(SliceSession *session, char *err);
SliceServer *slice_session_get_server(SliceSession *session);
char *slice_session_get_peer_ip(SliceSession *session);
int slice_session_get_peer_port(SliceSession *session);
SliceReturnType slice_session_list_append(SliceSession **head, SliceSession *item, char *err);
SliceReturnType slice_session_list_remove(SliceSession **head, SliceSession *item, char *err);

#ifdef __cplusplus
}
#endif

#define SliceSessionCreate(_mainloop, _server, _sock, _addr, _mode, _ssl_ctx, _read_callback, _close_callback, _user_data, _err) slice_session_create(_mainloop, _server, _sock, _addr, _mode, _ssl_ctx, _read_callback, _close_callback, _user_data, _err)
#define SliceSessionRemove(_session, _err) slice_session_remove(_session, _err)
#define SliceSessionWrite(_session, _buffer, _err) slice_session_write(_session, _buffer, _err)
#define SliceSessionFetchReadBuffer(_session, _out, _out_size, _err) slice_session_fetch_read_buffer(_session, _out, _out_size, _err)
#define SliceSessionGetReadBuffer(_session) slice_session_get_read_buffer(_session)
#define SliceSessionClearReadBuffer(_session, _err) slice_session_clear_read_buffer(_session, _err)
#define SliceSessionGetServer(_session) slice_session_get_server(_session)
#define SliceSessionGetPeerIP(_session) slice_session_get_peer_ip(_session)
#define SliceSessionGetPeerPort(_session) slice_session_get_peer_port(_session)
#define SliceSessionListAppend(_head, _item, _err) slice_session_list_append(_head, _item, _err)
#define SliceSessionListRemove(_head, _item, _err) slice_session_list_remove(_head, _item, _err)

#endif
