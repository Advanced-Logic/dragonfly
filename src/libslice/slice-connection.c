#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "slice-connection.h"

SliceReturnType slice_connection_init(SliceConnection *conn, int fd, char *err)
{
    int skflag;

    if (!conn || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((skflag = fcntl(fd, F_GETFL, 0)) < 0) {
        if (err) sprintf("fcntl get return error [%s]", strerror(errno));
        return SLICE_RETURN_ERROR;
    }

    if (fcntl(fd, F_SETFL, skflag | O_NONBLOCK) < 0) {
        if (err) sprintf("fcntl set return error [%s]", strerror(errno));
        return SLICE_RETURN_ERROR;
    }

    return SliceIOInit(conn, fd, err);
}

SliceConnection *slice_connection_create(SliceMainloopEvent *mainloop_event, int fd, SliceConnectionType type, char *err)
{
    SliceConnection *conn;
    socklen_t addr_len;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop_event || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return NULL;
    }

    if (!(conn = (SliceConnection*)malloc(sizeof(SliceConnection)))) {
        if (err) sprintf(err, "Can't allocate memory");
        return NULL;
    }

    memset(conn, 0, sizeof(SliceConnection));

    if (type == SLICE_CONNECTION_TYPE_IP4_TCP) {
        addr_len = sizeof(conn->args.ip4_tcp.peer_addr);

        if (getpeername(fd, (struct sockaddr*)&(conn->args.ip4_tcp.peer_addr), &addr_len) != 0) {
            if (err) sprintf(err, "getpeername return error [%s]", strerror(errno));
            free(conn);
            return NULL;
        }

        if (inet_ntop(AF_INET, &(conn->args.ip4_tcp.peer_addr.sin_addr), conn->args.ip4_tcp.peer_ip, INET_ADDRSTRLEN)) {
            conn->args.ip4_tcp.peer_port = conn->args.ip4_tcp.peer_addr.sin_port;
        } else {
            conn->args.ip4_tcp.peer_ip[0] = 0;
            conn->args.ip4_tcp.peer_port = 0;
        }
    } else if (type == SLICE_CONNECTION_TYPE_IP6_TCP) {
        addr_len = sizeof(conn->args.ip6_tcp.peer_addr);

        if (getpeername(fd, (struct sockaddr*)&(conn->args.ip6_tcp.peer_addr), &addr_len) != 0) {
            if (err) sprintf(err, "getpeername return error [%s]", strerror(errno));
            free(conn);
            return NULL;
        }

        if (inet_ntop(AF_INET6, &(conn->args.ip6_tcp.peer_addr.sin6_addr), conn->args.ip6_tcp.peer_ip, INET_ADDRSTRLEN)) {
            conn->args.ip6_tcp.peer_port = conn->args.ip6_tcp.peer_addr.sin6_port;
        } else {
            conn->args.ip6_tcp.peer_ip[0] = 0;
            conn->args.ip6_tcp.peer_port = 0;
        }
    } else if (type == SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (err) sprintf(err, "Connection type [%d] not implement yet", (int)type);
        free(conn);
        return NULL;
    } else {
        if (err) sprintf(err, "Connection type [%d] is invalid", (int)type);
        free(conn);
        return NULL;
    }

    if (slice_connection_init(conn, fd, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "Connection [%d] slice_connection_init return error [%s]", fd, err_buff);
        free(conn);
        return NULL;
    }
    
    mainloop_event->io.fd = fd;
    conn->mainloop_event = mainloop_event;

    return conn;
}

SliceReturnType slice_connection_destroy(SliceConnection *conn, char *err)
{
    SliceBuffer *buff;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!conn) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (conn->read_buffer) {
        SliceBufferRelease(conn->mainloop_event->mainloop, &(conn->read_buffer), NULL);
    }

    while ((buff = conn->write_buffer)) {
        SliceListRemove(&(conn->write_buffer), buff, NULL);
        SliceBufferRelease(conn->mainloop_event->mainloop, &buff, NULL);
    }

    if (SliceIOClose(conn, err_buff) != 0) {
        if (err) sprintf(err, "SliceIOClose return error [%s]", err_buff);
        // skip
    }

    free(conn);

    return 0;
}
