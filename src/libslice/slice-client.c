
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "slice-client.h"
#include "slice-ssl-client.h"

struct slice_client
{
    SliceMainloopEvent mainloop_event;

    SliceConnection *connection;

    char host[128];
    int port;

    SliceReturnType(*connect_result_cb)(SliceClient*, int, char*);
    SliceReturnType(*read_callback)(SliceClient*, SliceReturnType, void*, char*);
};

SliceReturnType slice_client_remove(SliceClient *client, char *err)
{
    SliceMainloopEvent *mainloop_event;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!client) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop_event = (SliceMainloopEvent*)client;

    if (mainloop_event->destroyed) return SLICE_RETURN_NORMAL;
    mainloop_event->destroyed = 1;

    if (SliceMainloopEventRemove(client->mainloop_event.mainloop, client, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceMainloopEventRemove return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    if (client->connection) {
        printf("Client [%p] connection [%p] closed\n", client, client->connection);
        SliceConnectionDestroy(client->connection, NULL);
        client->connection = NULL;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_connecting_write_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceMainloopEvent *mainloop_event;
    SliceClient *client;

    mainloop_event = SliceMainloopEpollElementGetSliceMainloopEvent(element);
    client = (SliceClient*)mainloop_event;

    int e;
    socklen_t elen;

    e = 0;
    elen = sizeof(e);

    if (getsockopt(mainloop_event->io.fd, SOL_SOCKET, SO_ERROR, (char*)&e, &elen) != 0 || e != 0) {
        client->connect_result_cb(client, SLICE_RETURN_ERROR, strerror(errno));
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    SliceMainloopEpollEventRemove(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
    SliceMainloopEpollElementSetSliceMainloopEvent(element, mainloop_event, NULL);

    if (client->connect_result_cb(client, SLICE_RETURN_NORMAL, "connected") != 0) {
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    if (!client->read_callback) {
        printf("Client callback not found [%p]\n", client->read_callback);
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_connecting_read_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceMainloopEvent *mainloop_event;
    SliceClient *client;

    mainloop_event = SliceMainloopEpollElementGetSliceMainloopEvent(element);
    client = (SliceClient*)mainloop_event;

    int e;
    socklen_t elen;

    e = 0;
    elen = sizeof(e);

    if (getsockopt(mainloop_event->io.fd, SOL_SOCKET, SO_ERROR, (char*)&e, &elen) != 0 || e != 0) {
        client->connect_result_cb(client, SLICE_RETURN_ERROR, strerror(errno));
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    SliceMainloopEpollEventRemove(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
    SliceMainloopEpollElementSetSliceMainloopEvent(element, mainloop_event, NULL);

    if (client->connect_result_cb(client, SLICE_RETURN_NORMAL, "connected") != SLICE_RETURN_NORMAL) {
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    if (!client->read_callback) {
        printf("Client callback not found [%p]\n", client->read_callback);
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_connecting_add_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    // replace read event callback
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_client_connecting_read_callback, NULL);
    SliceMainloopEpollEventAddRead(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    // add write event for check connection
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_WRITE, slice_client_connecting_write_callback, NULL);
    SliceMainloopEpollEventAddWrite(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_remove_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (mainloop_event->destroyed)  return SLICE_RETURN_NORMAL;

    if (slice_client_remove((SliceClient*)mainloop_event, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "slice_client_destroy return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_read_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceClient *client;
    SliceReturnType ret;
    int r;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!epoll || !element) {
        return SLICE_RETURN_ERROR;
    }

    client = (SliceClient*)SliceMainloopEpollElementGetSliceMainloopEvent(element);

    if (!client) {
        printf("Read callback but client not found\n");
        return SLICE_RETURN_ERROR;
    }

    if ((ret = slice_connection_socket_read(client->connection, &r, err_buff)) == SLICE_RETURN_ERROR) {
        printf("slice_connection_socket_read return error [%s]\n", err_buff);
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    } else if (ret == SLICE_RETURN_INFO) {
        return SLICE_RETURN_NORMAL;
    }

    if (r > 0 && client->read_callback(client, r, client->mainloop_event.user_data, "read") != 0) {
        printf("client read callback return error\n");
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    }
    
    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_client_write_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    if (!epoll || !element) {
        return SLICE_RETURN_ERROR;
    }

    SliceClient *client;
    SliceReturnType ret;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    client = (SliceClient*)SliceMainloopEpollElementGetSliceMainloopEvent(element);

    if ((ret = slice_connection_socket_write(client->connection, err_buff)) == SLICE_RETURN_ERROR) {
        printf("slice_connection_socket_write return error [%s]\n", err_buff);
        slice_client_remove(client, NULL);
        free(client);
        return SLICE_RETURN_ERROR;
    } else if (ret == SLICE_RETURN_INFO) {
        return SLICE_RETURN_NORMAL;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_client_start(SliceClient *client, SliceSSLContext *ssl_ctx, SliceReturnType(*read_callback)(SliceClient*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), void *user_data, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!client || !client->connection || !read_callback) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (ssl_ctx && SliceConnectionSetSSLContext(client->connection, ssl_ctx, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceConnectionSetSSLContext return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }
    
    if (SliceConnectionSetCloseCallback(client->connection, close_callback, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceConnectionSetCloseCallback return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    client->read_callback = read_callback;

    SliceMainloopEventSetUserData(client, user_data, NULL);

    SliceMainloopEpollEventSetCallback(client->mainloop_event.mainloop, client->mainloop_event.io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_client_read_callback, NULL);
    SliceMainloopEpollEventAddRead(client->mainloop_event.mainloop, client->mainloop_event.io.fd, NULL);

    SliceMainloopEpollEventSetCallback(client->mainloop_event.mainloop, client->mainloop_event.io.fd, SLICE_MAINLOOP_EPOLL_EVENT_WRITE, slice_client_write_callback, NULL);

    if (ssl_ctx) {
        if (SliceSSLClientConnect(client->mainloop_event.io.fd, ssl_ctx, err_buff) < 0) {
            if (err) sprintf(err, "SliceSSLClientConnect return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_client_write(SliceClient *client, SliceBuffer *buffer, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!client || !buffer) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (SliceConnectionWriteBuffer(client->connection, buffer, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceConnectionWriteBuffer return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    SliceMainloopEpollEventAddWrite(client->mainloop_event.mainloop, client->mainloop_event.io.fd, NULL);

    return SLICE_RETURN_NORMAL;
}

SliceClient *slice_client_create(SliceMainloop *mainloop, char *host, int port, SliceConnectionMode mode, SliceReturnType(*connect_result_cb)(SliceClient*, SliceReturnType, char*), char *err)
{
    SliceClient *client;

    struct addrinfo hints, *res;
 
    int sock, r;
    char buff[1024];
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop || !host || !host[0] || port < 0 || port > 0xffff || !connect_result_cb) {
        if (err) sprintf(err, "Invalid parameter");
        return NULL;
    }

    if (!(client = (SliceClient*)malloc(sizeof(SliceClient)))) {
        if (err) sprintf(err, "Can't allocate client memory");
        return NULL;
    }
    memset(client, 0, sizeof(SliceClient));

    if (mode & SLICE_CONNECTION_MODE_TCP) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = (mode == SLICE_CONNECTION_MODE_IP4_TCP) ? AF_INET : (mode == SLICE_CONNECTION_MODE_IP6_TCP) ? AF_INET6 : AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        sprintf(buff, "%d", port);

        if ((r = getaddrinfo(host, buff, &hints, &res)) != 0) {
            if (err) sprintf(err, "getaddrinfo return error [%s]", strerror(errno));
            free(client);
            return NULL;
        }

        mode = (res->ai_family == AF_INET) ? SLICE_CONNECTION_MODE_IP4_TCP : SLICE_CONNECTION_MODE_IP6_TCP;
        
        if ((sock = socket(res->ai_family, SOCK_STREAM, 0)) < 0) {
            if (err) sprintf(err, "socket return error [%s]", strerror(errno));
            free(client);
            return NULL;
        }

        if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
            if (errno != EINPROGRESS) {
                if (err) sprintf(err, "connect return error [%s]", strerror(errno));
                close(sock);
                freeaddrinfo(res);
                free(client);
                return NULL;
            }
        }

        freeaddrinfo(res);

    } else if (mode & SLICE_CONNECTION_MODE_IP4_UDP) {
        if (err) sprintf(err, "Connection type SLICE_CONNECTION_TYPE_IP4_UDP not implement yet");
        free(client);
        return NULL;
    } else {
        if (err) sprintf(err, "Unknown connection type [%d]", (int)mode);
        free(client);
        return NULL;
    }

    if (sock >= 0) {
        if (!(client->connection = SliceConnectionCreate(client, sock, mode, SLICE_CONNECTION_TYPE_CLIENT, err_buff))) {
            if (err) sprintf(err, "SliceConnectionCreate return error [%s]", err_buff);
            close(sock);
            free(client);
            return NULL;
        }

        client->connect_result_cb = connect_result_cb;

        if (SliceMainloopEventAdd(mainloop, client, slice_client_connecting_add_callback, slice_client_remove_callback, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "SliceMainloopEventAdd return error [%s]", err_buff);
            close(sock);
            free(client->connection);
            free(client);
            return NULL;
        }
    } else {
        if (err) sprintf(err, "Create socket error");
        free(client->connection);
        free(client);
        return NULL;
    }

    return client;
}

int slice_client_fetch_read_buffer(SliceClient *client, char *out, unsigned int out_size, char *err)
{
    if (!client) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    return SliceConnectionFetchReadBuffer(client->connection, out, out_size, err);
}

SliceBuffer *slice_client_get_read_buffer(SliceClient *client)
{
    if (!client) return NULL;

    return SliceConnectionGetReadBuffer(client->connection);
}

SliceReturnType slice_client_clear_read_buffer(SliceClient *client, char *err)
{
    if (!client) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    return SliceConnectionClearReadBuffer(client->connection, err);
}
