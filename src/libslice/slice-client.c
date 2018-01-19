
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
#include "slice-mainloop.h"

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
        SliceConnectionDestroy(client->connection, NULL);
        client->connection = NULL;
    }

    return SLICE_RETURN_NORMAL;
}

static int slice_client_write_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceMainloopEvent *mainloop_event;
    SliceClient *client;

    mainloop_event = (SliceMainloopEvent*)element->slice_event;
    client = (SliceClient*)element->slice_event;

    int e;
    socklen_t elen;

    e = 0;
    elen = sizeof(e);

    if (getsockopt(mainloop_event->io.fd, SOL_SOCKET, SO_ERROR, (char*)&e, &elen) != 0 || e != 0) {
        client->connect_result_cb(client, client->connection, -1, strerror(errno));
        slice_client_remove(client, NULL);
        free(client);
        return -1;
    }

    SliceMainloopEpollEventRemove(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
    element->slice_event = mainloop_event;

    if (client->connect_result_cb(client, client->connection, 0, "connected") != 0) {
        slice_client_remove(client, NULL);
        free(client);
        return -1;
    }

    return 0;
}

static int slice_client_read_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceMainloopEvent *mainloop_event;
    SliceClient *client;

    mainloop_event = (SliceMainloopEvent*)element->slice_event;
    client = (SliceClient*)element->slice_event;

    int e;
    socklen_t elen;

    e = 0;
    elen = sizeof(e);

    if (getsockopt(mainloop_event->io.fd, SOL_SOCKET, SO_ERROR, (char*)&e, &elen) != 0 || e != 0) {
        client->connect_result_cb(client, client->connection, -1, strerror(errno));
        slice_client_remove(client, NULL);
        free(client);
        return -1;
    }

    SliceMainloopEpollEventRemove(mainloop_event->mainloop, mainloop_event->io.fd, NULL);
    element->slice_event = mainloop_event;

    if (client->connect_result_cb(client, client->connection, 0, "connected") != 0) {
        slice_client_remove(client, NULL);
        free(client);
        return -1;
    }

    return 0;
}

static int slice_client_add_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    // replace read event callback
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_client_read_callback, NULL);
    SliceMainloopEpollEventAddRead(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    // add write event for check connection
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_WRITE, slice_client_write_callback, NULL);
    SliceMainloopEpollEventAddWrite(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    return 0;
}

static int slice_client_remove_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (mainloop_event->destroyed)  return 0;

    if (slice_client_remove((SliceClient*)mainloop_event, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "slice_client_destroy return error [%s]", err_buff);
        return -1;
    }

    return 0;
}

SliceClient *slice_client_create(SliceMainloop *mainloop, char *host, int port, SliceConnectionType type, int(*connect_result_cb)(SliceClient*, SliceConnection*, int, char*), char *err)
{
    SliceClient *client;

    struct addrinfo hints, *res;
 
    int sk = -1, r;
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

    if (type == SLICE_CONNECTION_TYPE_IP4_TCP) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        sprintf(buff, "%d", port);

        if ((r = getaddrinfo(host, buff, &hints, &res)) != 0) {
            if (err) sprintf(err, "getaddrinfo return error [%s]", strerror(errno));
            free(client);
            return NULL;
        }
        
        if ((sk = socket(res->ai_family, SOCK_STREAM, 0)) < 0) {
            if (err) sprintf(err, "socket return error [%s]", strerror(errno));
            free(client);
            return NULL;
        }

        if (connect(sk, res->ai_addr, res->ai_addrlen) != 0) {
            if (errno != EINPROGRESS) {
                if (err) sprintf(err, "connect return error [%s]", strerror(errno));
                close(sk);
                freeaddrinfo(res);
                free(client);
                return NULL;
            }
        }

        freeaddrinfo(res);

    } else if (type == SLICE_CONNECTION_TYPE_IP6_TCP) {
        if (err) sprintf(err, "Connection type SLICE_CONNECTION_TYPE_IP6_TCP not implement yet");
        free(client);
        return NULL;
    } else if (type == SLICE_CONNECTION_TYPE_IP4_UDP) {
        if (err) sprintf(err, "Connection type SLICE_CONNECTION_TYPE_IP4_UDP not implement yet");
        free(client);
        return NULL;
    } else {
        if (err) sprintf(err, "Unknown connection type [%d]", (int)type);
        free(client);
        return NULL;
    }

    if (sk >= 0) {
        if (!(client->connection = SliceConnectionCreate(client, sk, type, err_buff))) {
            if (err) sprintf(err, "SliceConnectionCreate return error [%s]", err_buff);
            free(client);
            return NULL;
        }

        client->connect_result_cb = connect_result_cb;

        if (SliceMainloopEventAdd(mainloop, client, slice_client_add_callback, slice_client_remove_callback, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "SliceMainloopEventAdd return error [%s]", err_buff);
            close(sk);
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
