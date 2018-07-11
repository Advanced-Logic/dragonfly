
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "slice-server.h"

struct slice_session
{
    SliceMainloopEvent mainloop_event;

    SliceSession *next;
    SliceSession *prev;

    SliceServer *server;
    SliceConnection *connection;

    SliceReturnType(*read_callback)(SliceSession*, int, void*, char*);
};

SliceReturnType slice_session_remove(SliceSession *session, char *err)
{
    SliceMainloopEvent *mainloop_event;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!session) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    mainloop_event = (SliceMainloopEvent*)session;

    if (mainloop_event->destroyed) return SLICE_RETURN_NORMAL;
    mainloop_event->destroyed = 1;

    if (SliceMainloopEventRemove(session->mainloop_event.mainloop, session, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceMainloopEventRemove return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    if (session->server) SliceServerRemoveSession(session->server, session);

    if (session->connection) {
        printf("Session [%p] connection [%p] closed\n", session, session->connection);
        SliceConnectionDestroy(session->connection, NULL);
        session->connection = NULL;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_session_read_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceSession *session;
    SliceReturnType ret;
    int r;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!epoll || !element) {
        return SLICE_RETURN_ERROR;
    }

    session = (SliceSession*)SliceMainloopEpollElementGetSliceMainloopEvent(element);

    if (!session) {
        printf("Read callback but session not found\n");
        return SLICE_RETURN_ERROR;
    }

    if ((ret = slice_connection_socket_read(session->connection, &r, err_buff)) == SLICE_RETURN_ERROR) {
        printf("slice_connection_socket_read return error [%s]\n", err_buff);
        slice_session_remove(session, NULL);
        free(session);
        return SLICE_RETURN_ERROR;
    } else if (ret == SLICE_RETURN_INFO) {
        return SLICE_RETURN_NORMAL;
    }

    if (r > 0 && session->read_callback(session, r, session->mainloop_event.user_data, "read") != 0) {
        printf("session read callback return error\n");
        slice_session_remove(session, NULL);
        free(session);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_session_write_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    if (!epoll || !element) {
        return SLICE_RETURN_ERROR;
    }

    SliceSession *session;
    SliceReturnType ret;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    session = (SliceSession*)SliceMainloopEpollElementGetSliceMainloopEvent(element);

    if ((ret = slice_connection_socket_write(session->connection, err_buff)) == SLICE_RETURN_ERROR) {
        printf("slice_connection_socket_write return error [%s]\n", err_buff);
        slice_session_remove(session, NULL);
        free(session);
        return SLICE_RETURN_ERROR;
    } else if (ret == SLICE_RETURN_INFO) {
        return SLICE_RETURN_NORMAL;
    }

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_session_add_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_session_read_callback, NULL);
    SliceMainloopEpollEventAddRead(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_WRITE, slice_session_write_callback, NULL);

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_session_remove_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (mainloop_event->destroyed)  return SLICE_RETURN_NORMAL;

    if (slice_session_remove((SliceSession*)mainloop_event, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "slice_session_remove return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

SliceSession *slice_session_create(SliceMainloop *mainloop, SliceServer *server, int sock, struct sockaddr *addr, SliceConnectionMode mode, SliceSSLContext *ssl_ctx, SliceReturnType(*read_callback)(SliceSession*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), void *user_data, char *err)
{
    SliceSession *session;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop || sock < 0 || sock > 0xffff || !read_callback || !close_callback) {
        if (err) sprintf(err, "Invalid parameter");
        return NULL;
    }

    if (!(session = (SliceSession*)malloc(sizeof(SliceSession)))) {
        if (err) sprintf(err, "Can't allocate session memory");
        return NULL;
    }
    memset(session, 0, sizeof(SliceSession));

    if (mode & SLICE_CONNECTION_MODE_TCP) {
        /*if (type == SLICE_CONNECTION_TYPE_IP4_TCP) {
            inet_ntop(AF_INET, &(((struct sockaddr_in*)addr)->sin_addr), session->ip, sizeof(session->ip));
            session->port = ((struct sockaddr_in*)addr)->sin_port;
        } else {
            inet_ntop(AF_INET6, &(((struct sockaddr_in6*)addr)->sin6_addr), session->ip, sizeof(session->ip));
            session->port = ((struct sockaddr_in6 *)addr)->sin6_port;
        }*/
    } else if (mode & SLICE_CONNECTION_MODE_IP4_UDP) {
        if (err) sprintf(err, "Connection mode SLICE_CONNECTION_TYPE_IP4_UDP not implement yet");
        free(session);
        return NULL;
    } else {
        if (err) sprintf(err, "Unknown connection mode [%d] (no permit or not implmented)", (int)mode);
        free(session);
        return NULL;
    }

    if (!(session->connection = SliceConnectionCreate(session, sock, mode, SLICE_CONNECTION_TYPE_SESSION, err_buff))) {
        printf("SliceConnectionCreate return error [%s]\n", err_buff);
        if (err) sprintf(err, "SliceConnectionCreate return error [%s]", err_buff);
        free(session);
        return NULL;
    }

    if (ssl_ctx && SliceConnectionSetSSLContext(session->connection, ssl_ctx, err_buff) != SLICE_RETURN_NORMAL) {
        printf("SliceConnectionSetSSLContext return error [%s]\n", err_buff);
        if (err) sprintf(err, "SliceConnectionSetSSLContext return error [%s]", err_buff);
        free(session->connection);
        free(session);
        return NULL;
    }

    session->server = server;
    session->read_callback = read_callback;
    SliceMainloopEventSetUserData(session, user_data, NULL);

    if (SliceConnectionSetCloseCallback(session->connection, close_callback, err_buff) != SLICE_RETURN_NORMAL) {
        printf("SliceConnectionSetCloseCallback return error [%s]\n", err_buff);
        if (err) sprintf(err, "SliceConnectionSetCloseCallback return error [%s]", err_buff);
        free(session->connection);
        free(session);
        return NULL;
    }

    if (SliceMainloopEventAdd(mainloop, session, slice_session_add_callback, slice_session_remove_callback, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceMainloopEventAdd return error [%s]", err_buff);
        free(session->connection);
        free(session);
        return NULL;
    }

    return session;
}

SliceReturnType slice_session_write(SliceSession *session, SliceBuffer *buffer, char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!session || !buffer) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (SliceConnectionWriteBuffer(session->connection, buffer, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceConnectionWriteBuffer return error [%s]", err_buff);
        return SLICE_RETURN_ERROR;
    }

    SliceMainloopEpollEventAddWrite(session->mainloop_event.mainloop, session->mainloop_event.io.fd, NULL);

    return SLICE_RETURN_NORMAL;
}

int slice_session_fetch_read_buffer(SliceSession *session, char *out, unsigned int out_size, char *err)
{
    if (!session) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    return SliceConnectionFetchReadBuffer(session->connection, out, out_size, err);
}

SliceBuffer *slice_session_get_read_buffer(SliceSession *session)
{
    if (!session) return NULL;

    return SliceConnectionGetReadBuffer(session->connection);
}

SliceReturnType slice_session_clear_read_buffer(SliceSession *session, char *err)
{
    if (!session) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    return SliceConnectionClearReadBuffer(session->connection, err);
}

SliceServer *slice_session_get_server(SliceSession *session)
{
    if (!session) return NULL;

    return session->server;
}

char *slice_session_get_peer_ip(SliceSession *session)
{
    return SliceConnectionGetPeerIP(session->connection);
}

int slice_session_get_peer_port(SliceSession *session)
{
    return SliceConnectionGetPeerPort(session->connection);
}

SliceReturnType slice_session_list_append(SliceSession **head, SliceSession *item, char *err)
{
    if (!head || !item) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (item->next || item->prev) {
        if (err) sprintf(err, "Session already in list");
        return SLICE_RETURN_ERROR;
    }

    if (!(*head)) {
        (*head) = item->next = item->prev = item;
    } else {
        item->prev = (*head)->prev;
        item->next = (*head);
        (*head)->prev->next = item;
        (*head)->prev = item;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_session_list_remove(SliceSession **head, SliceSession *item, char *err)
{
    if (!head || !(*head) || !item) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!item->next || !item->prev) {
        if (err) sprintf(err, "Session not steal in list");
        return SLICE_RETURN_ERROR;
    }

    if ((*head) == item) {
        if ((*head)->next == (*head)) {
            (*head) = NULL;
        } else {
            (*head) = item->next;
            item->next->prev = item->prev;
            item->prev->next = item->next;
        }
    } else {
        item->next->prev = item->prev;
        item->prev->next = item->next;
    }

    item->next = item->prev = NULL;

    return SLICE_RETURN_NORMAL;
}
