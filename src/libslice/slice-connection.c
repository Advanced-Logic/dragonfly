#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "slice-connection.h"
#include "slice-ssl-client.h"

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

    SliceSSLContext *ssl_ctx;
    void(*close_callback)(struct slice_connection*, void*, char*);
};


SliceReturnType slice_connection_init(SliceConnection *conn, int fd, char *err)
{
    int skflag;

    if (!conn || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((skflag = fcntl(fd, F_GETFL, 0)) < 0) {
        if (err) sprintf(err, "fcntl get return error [%s]", strerror(errno));
        return SLICE_RETURN_ERROR;
    }

    if (fcntl(fd, F_SETFL, skflag | O_NONBLOCK) < 0) {
        if (err) sprintf(err, "fcntl set return error [%s]", strerror(errno));
        return SLICE_RETURN_ERROR;
    }

    return SliceIOInit(conn, fd, err);
}

SliceReturnType slice_connection_set_ssl_context(SliceConnection *conn, SliceSSLContext *ssl_ctx, char *err)
{
    if (!conn || !ssl_ctx) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (conn->type == SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (err) sprintf(err, "SSL on UDP is not implemetented");
        return SLICE_RETURN_ERROR;
    }

    if (conn->ssl_ctx) {
        if (err) sprintf(err, "SSL already set on this connection");
        return SLICE_RETURN_ERROR;
    }

    conn->ssl_ctx = ssl_ctx;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_connection_set_close_callback(SliceConnection *conn, void(*close_callback)(SliceConnection*, void*, char*), char *err)
{
    if (!conn) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    conn->close_callback = close_callback;

    return SLICE_RETURN_NORMAL;
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
    conn->type = type;

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

    if (conn->ssl_ctx) {
        SliceSSLClientShutdown(conn->mainloop_event->io.fd, NULL);
        conn->ssl_ctx = NULL;
    }

    printf("Connection [%p] sock [%d] closed\n", conn, conn->mainloop_event->io.fd);
    if (SliceIOClose(conn, err_buff) != 0) {
        if (err) sprintf(err, "SliceIOClose return error [%s]", err_buff);
        // skip
    }

    free(conn);

    return SLICE_RETURN_NORMAL;
}

// for event read callback
SliceReturnType slice_connection_socket_read(SliceConnection *conn, int *read_length, char *err)
{
    SliceMainloopEvent *mainloop_event;
    SliceBuffer *buffer = NULL;
    int r, ret, err_num = 0;
    unsigned int n;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    mainloop_event = (SliceMainloopEvent*)conn->mainloop_event;

    *read_length = 0;

    if (conn->type != SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (conn->ssl_ctx && SliceSSLClientGetState(mainloop_event->io.fd) != SLICE_SSL_STATE_CONNECTED) {
            if ((r = SliceSSLClientConnect(mainloop_event->io.fd, conn->ssl_ctx, err_buff)) == 1) {
                printf("SliceSSLClientConnect read return continue initial\n");
                return SLICE_RETURN_INFO;
            } else if (r != 0) {
                if (err) sprintf(err, "SliceSSLClientConnect return error [%s]", err_buff);
                if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
                return SLICE_RETURN_ERROR;
            }

            if (SliceSSLClientGetState(mainloop_event->io.fd) != SLICE_SSL_STATE_CONNECTED) {
                printf("Return while SSL connecting\n");
                return SLICE_RETURN_INFO;
            } else {
                if (err) sprintf(err, "SSL connected [%p]", conn->write_buffer);
            }

            if (conn->write_buffer) {
                printf("Add write while SSL connected and write buffer exists\n");
                SliceMainloopEpollEventAddWrite(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
            }
        }
    }

    if (!(buffer = conn->read_buffer)) {
        if (!(conn->read_buffer = buffer = SliceBufferCreate(mainloop_event->mainloop, DEFAULT_READ_BUFFER_SIZE, err_buff))) {
            if (err) sprintf(err, "SliceBufferCreate return error [%s]", err_buff);
            if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

#define SLICE_SSL_READ_SPEED_HACK
#ifdef SLICE_SSL_READ_SPEED_HACK
#define SLICE_SSL_READ_SPEED_HACK_MAX_RETRY     10
    int ssl_retry_count = 0;
ssl_retry_read:
#endif
    n = buffer->size - buffer->length;

    if (n < MIN_READ_BUFFER_SIZE) {
        if (SliceBufferPrepare(mainloop_event->mainloop, &(conn->read_buffer), MIN_READ_BUFFER_SIZE, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "SliceBufferPrepare return error [%s]", err_buff);
            if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
            return SLICE_RETURN_ERROR;
        }

        buffer = conn->read_buffer;
        n = buffer->size - buffer->length;
    }

    printf("Reading buffer size [%d]\n", n);

    if (conn->type != SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (conn->ssl_ctx) {
            if ((ret = SliceSSLClientRead(mainloop_event->io.fd, buffer->data + buffer->length, n, &r, &err_num, err_buff)) == SLICE_RETURN_ERROR) {
#ifdef SLICE_SSL_READ_SPEED_HACK
                if (*read_length > 0) {
                    // error/close after read
                    buffer->length += r;
                    buffer->data[buffer->length] = 0;

                    return SLICE_RETURN_NORMAL;
                }
#endif
                if (err_num == 0) {
                    if (err) sprintf(err, "SliceSSLClientRead return error [%s]", err_buff);
                } else {
                    if (err) sprintf(err, "SliceSSLClientRead system call [%s]", err_buff);
                }
                if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
                return SLICE_RETURN_ERROR;
            } else if (ret == SLICE_RETURN_INFO) {
#ifdef SLICE_SSL_READ_SPEED_HACK
                if (*read_length > 0) return SLICE_RETURN_NORMAL;
#endif
                printf("SliceSSLClientRead wait next read\n");
                return SLICE_RETURN_INFO;
            }
        } else {
            if ((r = recv(mainloop_event->io.fd, buffer->data + buffer->length, n, 0)) < 0) {
                if (err) sprintf(err, "recv return error [%s]", strerror(errno));
                if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, strerror(errno));
                return SLICE_RETURN_ERROR;
            } else if (r == 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    if (err) sprintf(err, "recv return connection closed");
                    if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, strerror(errno));
                    return SLICE_RETURN_ERROR;
                }
            }
        }

        buffer->length += r;
        buffer->data[buffer->length] = 0;

        *read_length += r;
#ifdef SLICE_SSL_READ_SPEED_HACK
        if (conn->ssl_ctx && (ssl_retry_count++) < SLICE_SSL_READ_SPEED_HACK_MAX_RETRY) goto ssl_retry_read;
#endif
    } else {
        if (err) sprintf(err, "UDP not implement yet");
        if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, "UDP is not implmented");
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

// for event write callback
SliceReturnType slice_connection_socket_write(SliceConnection *conn, char *err)
{
    SliceMainloopEvent *mainloop_event;
    SliceBuffer *buffer = NULL;
    int r, ret, err_num = 0;
    unsigned int n;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    mainloop_event = (SliceMainloopEvent*)conn->mainloop_event;

    if (conn->type != SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (conn->ssl_ctx && SliceSSLClientGetState(mainloop_event->io.fd) != SLICE_SSL_STATE_CONNECTED) {
            if ((r = SliceSSLClientConnect(mainloop_event->io.fd, conn->ssl_ctx, err_buff)) == 1) {
                printf("SliceSSLClientConnect write return continue initial\n");
                return SLICE_RETURN_INFO;
            } else if (r != 0) {
                if (err) sprintf(err, "SliceSSLClientConnect return error [%s]", err_buff);
                if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
                return SLICE_RETURN_ERROR;
            }

            if (SliceSSLClientGetState(mainloop_event->io.fd) != SLICE_SSL_STATE_CONNECTED) return SLICE_RETURN_INFO;
        }
    }

    printf("Write XXXXX [%p]\n", conn->write_buffer);

    while ((buffer = conn->write_buffer)) {
        n = buffer->length - buffer->current;

        printf("Writing [%d][%.*s]\n", buffer->length, buffer->length, buffer->data);

        if (n > 0) {
            if (conn->ssl_ctx) {
                if ((ret = SliceSSLClientWrite(mainloop_event->io.fd, buffer->data + buffer->current, n, &r, &err_num, err_buff)) == SLICE_RETURN_ERROR) {
                    if (err_num == 0) {
                        if (err) sprintf(err, "SliceSSLClientWrite return error [%s]", err_buff);
                    } else {
                        if (err) sprintf(err, "SliceSSLClientWrite system call [%s]", err_buff);
                    }
                    if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, err_buff);
                    return SLICE_RETURN_ERROR;
                } else if (ret == 1) {
                    printf("SSL_client_write return need re-write again\n");
                    break;
                }
            } else {
                if ((r = send(mainloop_event->io.fd, buffer->data + buffer->current, n, 0)) < 0) {
                    if (err) sprintf(err, "send return error [%s]", strerror(errno));
                    if (conn->close_callback) conn->close_callback(conn, mainloop_event->user_data, strerror(errno));
                    return SLICE_RETURN_ERROR;
                } else if (r == 0) {
                    // write buffer full or disconnected
                    break;
                }
            }

            buffer->current += r;

            if (buffer->current >= buffer->length) {
                SliceListRemove(&(conn->write_buffer), buffer, NULL);
                SliceBufferRelease(mainloop_event->mainloop, &buffer, NULL);
            }
        } else {
            SliceListRemove(conn->write_buffer, buffer, NULL);
            SliceBufferRelease(mainloop_event->mainloop, &buffer, NULL);
        }
    }

    if (conn->write_buffer) {
        SliceMainloopEpollEventAddWrite(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_connection_set_read_buffer_size(SliceConnection *conn, unsigned int size, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!conn || size > 0x8FFF) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!conn->read_buffer) {
        if (!(conn->read_buffer = SliceBufferCreate(conn->mainloop_event->mainloop, size, err_buff))) {
            if (err) sprintf(err, "SliceBufferCreate return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    if (size > conn->read_buffer->size) {
        if (SliceBufferPrepare(conn->mainloop_event->mainloop, &(conn->read_buffer), size - conn->read_buffer->size, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "SliceBufferPrepare return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_connection_need_read_buffer_size(SliceConnection *conn, unsigned int need_size, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!conn || (conn->read_buffer->length + need_size) > 0x8FFF) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!conn->read_buffer) {
        if (!(conn->read_buffer = SliceBufferCreate(conn->mainloop_event->mainloop, need_size, err_buff))) {
            if (err) sprintf(err, "SliceBufferCreate return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    if ((conn->read_buffer->length + need_size) > conn->read_buffer->size) {
        if (SliceBufferPrepare(conn->mainloop_event->mainloop, &(conn->read_buffer), need_size + MIN_READ_BUFFER_SIZE, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "SliceBufferPrepare return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    return SLICE_RETURN_NORMAL;
}

int slice_connection_fetch_read_buffer(SliceConnection *conn, char *out, unsigned int out_size, char *err)
{
    SliceBuffer *buffer;
    unsigned int read_length;

    if (!conn || !out || out_size == 0 || out_size > 0x8FFF) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    if (!(buffer = conn->read_buffer)) return 0;

    read_length = (out_size < buffer->length) ? out_size : buffer->length;

    if (read_length > 0) {
        memcpy(out, buffer->data, read_length);
        if (read_length < buffer->length) {
            memmove(buffer->data, buffer->data + read_length, buffer->length - read_length);
            buffer->length -= read_length;
            buffer->data[buffer->length] = 0;
        } else {
            buffer->length = 0;
        }
        buffer->current = 0;
    }
    
    return (int)read_length;
}

SliceBuffer *slice_connection_get_read_buffer(SliceConnection *conn)
{
    if (!conn) return NULL;

    return conn->read_buffer;
}

SliceReturnType slice_connection_clear_read_buffer(SliceConnection *conn, char *err)
{
    if (!conn) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (conn->read_buffer) {
        conn->read_buffer->length = 0;
        conn->read_buffer->current = 0;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_connection_write_buffer(SliceConnection *conn, SliceBuffer *buffer, char *err)
{
    if (!conn || !buffer) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    SliceListAppend(&(conn->write_buffer), buffer, NULL);

    return SLICE_RETURN_NORMAL;
}
