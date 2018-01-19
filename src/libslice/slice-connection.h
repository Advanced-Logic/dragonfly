#ifndef _SLICE_CONNECTION_H_
#define _SLICE_CONNECTION_H_

#include <netinet/in.h>

#include "slice-buffer.h"
#include "slice-mainloop-event.h"

struct slice_connection_ip4_tcp
{
    struct sockaddr_in peer_addr;

    char peer_ip[16];
    int peer_port;
};

struct slice_connection_ip6_tcp
{
    struct sockaddr_in6 peer_addr;

    char peer_ip[64];
    int peer_port;
};

struct slice_connection_ip4_udp
{
    struct sockaddr_storage peer_addr;
};

enum slice_connection_type
{
    SLICE_CONNECTION_TYPE_IP4_TCP = 0,
    SLICE_CONNECTION_TYPE_IP6_TCP,
    SLICE_CONNECTION_TYPE_IP4_UDP
};

struct slice_connection
{
    struct slice_mainloop_event *mainloop_event;

    enum slice_connection_type type;

    union
    {
        struct slice_connection_ip4_tcp ip4_tcp;
        struct slice_connection_ip6_tcp ip6_tcp;

        struct slice_connection_ip4_udp ip4_udp;
    } args;

    SliceBuffer *read_buffer;
    SliceBuffer *write_buffer;
};

typedef struct slice_connection SliceConnection;
typedef struct slice_connection_ip4_tcp SliceConnectionIP4TCP;
typedef struct slice_connection_ip6_tcp SliceConnectionIP6TCP;
typedef struct slice_connection_ip4_udp SliceConnectionIP4UDP;

typedef enum slice_connection_type SliceConnectionType;

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_connection_init(SliceConnection *conn, int fd, char *err);
SliceConnection *slice_connection_create(SliceMainloopEvent *mainloop_event, int fd, SliceConnectionType type, char *err);
SliceReturnType slice_connection_destroy(SliceConnection *conn, char *err);

#ifdef __cplusplus
}
#endif

#define SliceConnectionInit(_conn, _fd, _err) slice_connection_init((SliceConnection*)_conn, _fd, _err)
#define SliceConnectionCreate(_event, _fd, _type, _err) slice_connection_create((SliceMainloopEvent*)_event, _fd, _type, _err)
#define SliceConnectionDestroy(_conn, _err) slice_connection_destroy(_conn, _err)

#endif
