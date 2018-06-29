#ifndef _SLICE_CONNECTION_H_
#define _SLICE_CONNECTION_H_

#include <netinet/in.h>

#include "slice-buffer.h"
#include "slice-mainloop.h"
#include "slice-ssl.h"

#define DEFAULT_READ_BUFFER_SIZE    (SLICE_BUFFER_BLOCK_SIZE - 1)
#define MIN_READ_BUFFER_SIZE        (2 * 1024)

typedef struct slice_connection SliceConnection;
typedef struct slice_connection_ip4_tcp SliceConnectionIP4TCP;
typedef struct slice_connection_ip6_tcp SliceConnectionIP6TCP;
typedef struct slice_connection_ip4_udp SliceConnectionIP4UDP;

typedef enum slice_connection_type SliceConnectionType;

enum slice_connection_type
{
    SLICE_CONNECTION_TYPE_TCP = 0,
    SLICE_CONNECTION_TYPE_IP4_TCP,
    SLICE_CONNECTION_TYPE_IP6_TCP,
    SLICE_CONNECTION_TYPE_IP4_UDP
};

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_connection_init(SliceConnection *conn, int fd, char *err);
SliceConnection *slice_connection_create(SliceMainloopEvent *mainloop_event, int fd, SliceConnectionType type, char *err);
SliceReturnType slice_connection_destroy(SliceConnection *conn, char *err);
SliceReturnType slice_connection_set_ssl_context(SliceConnection *conn, SliceSSLContext *ssl_ctx, char *err);
SliceReturnType slice_connection_set_close_callback(SliceConnection *conn, void(*close_callback)(SliceConnection*, void*, char*), char *err);
SliceReturnType slice_connection_socket_read(SliceConnection *connection, int *read_length, char *err);
SliceReturnType slice_connection_socket_write(SliceConnection *conn, char *err);
SliceReturnType slice_connection_set_read_buffer_size(SliceConnection *conn, unsigned int size, char *err);
SliceReturnType slice_connection_need_read_buffer_size(SliceConnection *conn, unsigned int need_size, char *err);
int slice_connection_fetch_read_buffer(SliceConnection *conn, char *out, unsigned int out_size, char *err);
SliceBuffer *slice_connection_get_read_buffer(SliceConnection *conn);
SliceReturnType slice_connection_clear_read_buffer(SliceConnection *conn, char *err);
SliceReturnType slice_connection_write_buffer(SliceConnection *conn, SliceBuffer *buffer, char *err);

#ifdef __cplusplus
}
#endif

#define SliceConnectionInit(_conn, _fd, _err) slice_connection_init((SliceConnection*)_conn, _fd, _err)
#define SliceConnectionCreate(_event, _fd, _type, _err) slice_connection_create((SliceMainloopEvent*)_event, _fd, _type, _err)
#define SliceConnectionDestroy(_conn, _err) slice_connection_destroy(_conn, _err)
#define SliceConnectionSetSSLContext(_conn, _ssl_ctx, _err) slice_connection_set_ssl_context(_conn, _ssl_ctx, _err)
#define SliceConnectionSetCloseCallback(_conn, _close_callback, _err) slice_connection_set_close_callback(_conn, _close_callback, _err)
//#define SliceConnectionSocketRead(_conn, _read_length, _err) slice_connection_read(_conn, _read_length, _err)
//#define SliceConnectionSocketWrite(_conn, _err) slice_connection_write(_conn, _err)
#define SliceConnectionSetReadBufferSize(_conn, _size, _err) slice_connection_set_read_buffer_size(_conn, _size, _err)
#define SliceConnectionNeedReadBufferSize(_conn, _need_size, _err) slice_connection_need_read_buffer_size(_conn, _need_size, _err)
#define SliceConnectionFetchReadBuffer(_conn, _out, _out_size, _err) slice_connection_fetch_read_buffer(_conn, _out, _out_size, _err)
#define SliceConnectionGetReadBuffer(_conn) slice_connection_get_read_buffer(_conn)
#define SliceConnectionClearReadBuffer(_conn, _err) slice_connection_clear_read_buffer(_conn, _err)
#define SliceConnectionWriteBuffer(_conn, _buffer, _err) slice_connection_write_buffer(_conn, _buffer, _err)

#endif
