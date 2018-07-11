
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "slice-server.h"

struct slice_server
{
    SliceMainloopEvent mainloop_event;

    SliceServerMode mode;

    char bind_ip[128];
    int bind_port;
    int sock;

    SliceSSLContext *ssl_ctx;

    SliceReturnType(*accept_cb)(SliceSession*, char*);
    SliceReturnType(*ready_cb)(SliceSession*, char*);
    SliceReturnType(*read_callback)(SliceSession*, int, void*, char*);
    void(*close_callback)(SliceConnection*, void*, char*);

    SliceSession *sessions;
    int sessions_count;
};

static SliceReturnType slice_server_read_callback(SliceMainloopEpoll *epoll, SliceMainloopEpollElement *element, struct epoll_event ev, void *user_data)
{
    SliceServer *server;
    SliceSession *session;
    struct sockaddr *addr_p;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int sock, skflag;
    socklen_t socklen;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!epoll || !element) {
        return SLICE_RETURN_ERROR;
    }

    server = (SliceServer*)SliceMainloopEpollElementGetSliceMainloopEvent(element);

    if (!server) {
        printf("Read callback but server not found\n");
        return SLICE_RETURN_ERROR;
    }

    if (server->mode == SLICE_SERVER_MODE_IP4_TCP) {
ip4_tcp_accept_again:
        socklen = sizeof(addr);
        if ((sock = accept(server->sock, (struct sockaddr*)(&addr), &socklen)) < 0) {
            if (errno == EINTR) goto ip4_tcp_accept_again;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return SLICE_RETURN_NORMAL;
#ifdef EPROTO
            if (errno == EPROTO) return SLICE_RETURN_NORMAL;
#endif
#ifdef ECONNABORTED
            if (errno == ECONNABORTED) return SLICE_RETURN_NORMAL;
#endif
            return SLICE_RETURN_NORMAL;
            //return SLICE_RETURN_ERROR;
        }

        addr_p = (struct sockaddr*)&addr;
    } else if (server->mode == SLICE_SERVER_MODE_IP6_TCP) {
ip6_tcp_accept_again:
        socklen = sizeof(addr6);
        if ((sock = accept(server->sock, (struct sockaddr*)(&addr6), &socklen)) < 0) {
            if (errno == EINTR) goto ip6_tcp_accept_again;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return SLICE_RETURN_NORMAL;
#ifdef EPROTO
            if (errno == EPROTO) return SLICE_RETURN_NORMAL;
#endif
#ifdef ECONNABORTED
            if (errno == ECONNABORTED) return SLICE_RETURN_NORMAL;
#endif
            return SLICE_RETURN_NORMAL;
            //return SLICE_RETURN_ERROR;
        }

        addr_p = (struct sockaddr*)&addr6;
    } else if (server->mode == SLICE_SERVER_MODE_IP4_UDP) {
        printf("UDP server is not implemented\n");
        return SLICE_RETURN_ERROR;
    } else {
        printf("Invalid server mode [%d]\n", (int)server->mode);
        return SLICE_RETURN_ERROR;
    }

    if (server->mode == SLICE_SERVER_MODE_IP4_TCP || server->mode == SLICE_SERVER_MODE_IP6_TCP) {
        if ((skflag = fcntl(sock, F_GETFL, 0)) < 0) {
            printf("fcntl(F_GETFL) return error [%s]", strerror(errno));
            close(sock);
            return SLICE_RETURN_NORMAL;
            //return SLICE_RETURN_ERROR;
        }

        if (fcntl(sock, F_SETFL, skflag | O_NONBLOCK) < 0) {
            printf("fcntl(F_SETFL) return error [%s]", strerror(errno));
            close(sock);
            return SLICE_RETURN_NORMAL;
            //return SLICE_RETURN_ERROR;
        }
    }

    if (!(session = slice_session_create(server->mainloop_event.mainloop, server, sock, addr_p, server->mode, server->ssl_ctx, server->read_callback, server->close_callback, NULL, err_buff))) {
        printf("SliceSessionCreate return error [%s]\n", err_buff);
        close(sock);
        return SLICE_RETURN_NORMAL;
        //return SLICE_RETURN_ERROR;
    }

    SliceSessionListAppend(&(server->sessions), session, NULL);
    server->sessions_count++;
    
    if (server->accept_cb && server->accept_cb(session, err_buff) != SLICE_RETURN_NORMAL) {
        printf("Session sock [%d] accept callback return error [%s]\n", sock, err_buff);
        SliceSessionRemove(session, NULL);
        return SLICE_RETURN_NORMAL;
        //return SLICE_RETURN_ERROR;
    }

    if (!server->ssl_ctx) {
        if (server->ready_cb && server->ready_cb(session, err_buff) != SLICE_RETURN_NORMAL) {
            printf("Session sock [%d] ready callback return error [%s]\n", sock, err_buff);
            SliceSessionRemove(session, NULL);
            return SLICE_RETURN_NORMAL;
            //return SLICE_RETURN_ERROR;
        }
    }

    return SLICE_RETURN_NORMAL;
}

void slice_server_remove_session(SliceServer *server, SliceSession *session)
{
    if (!server || !session) return;

    SliceSessionListRemove(&(server->sessions), session, NULL);
    server->sessions_count--;
}

static SliceReturnType slice_server_remove_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    SliceServer *server;
    SliceSession *session;

    if (!(server = (SliceServer*)mainloop_event)) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    while ((session = server->sessions)) {
        SliceSessionRemove(session, NULL);
        free(session);
    }

    if (server->sock > 0) {
        SliceIOClose(server, NULL);
        server->sock = -1;
    }

    server->ssl_ctx = NULL;

    return SLICE_RETURN_NORMAL;
}

static SliceReturnType slice_server_add_callback(SliceMainloopEvent *mainloop_event, char *err)
{
    SliceMainloopEpollEventSetCallback(mainloop_event->mainloop, mainloop_event->io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_server_read_callback, NULL);
    SliceMainloopEpollEventAddRead(mainloop_event->mainloop, mainloop_event->io.fd, NULL);

    return SLICE_RETURN_NORMAL;
}

SliceServer *slice_server_create(SliceMainloop *mainloop, SliceServerMode mode, char *bind_ip, int bind_port, SliceSSLContext *ssl_ctx, SliceReturnType(*accept_cb)(SliceSession*, char*), SliceReturnType(*ready_cb)(SliceSession*, char*), SliceReturnType(*read_callback)(SliceSession*, int, void*, char*), void(*close_callback)(SliceConnection*, void*, char*), char *err)
{
    SliceServer *server;
    socklen_t socklen;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int sock = -1, reuse, skflag;
    char buff[1024];
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!mainloop || !bind_ip || bind_port < 0 || bind_port > 0xffff || !read_callback) {
        if (err) sprintf(err, "Invalid parameter");
        return NULL;
    }

    if (bind_ip[0] == 0) {
        if (mode != SLICE_SERVER_MODE_IP6_TCP) {
            bind_ip = "0.0.0.0";
        } else {
            bind_ip = "::0";
        }
    }

    if (mode == SLICE_SERVER_MODE_IP4_TCP) {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            if (err) sprintf(err, "sock return error [%s]", strerror(errno));
            return NULL;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(bind_port);

        if (inet_pton(AF_INET, bind_ip, &(addr.sin_addr)) != 1) {
            if (err) sprintf(err, "Invalid bind ip [%s]", bind_ip);
            close(sock);
            return NULL;
        }

        socklen = sizeof(addr);
        if (bind(sock, (struct sockaddr*)&addr, socklen) != 0) {
            if (err) sprintf(err, "bind to port [%d] return error [%s]", bind_port, strerror(errno));
            close(sock);
            return NULL;
        }
    } else if (mode == SLICE_SERVER_MODE_IP6_TCP) {
        if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
            if (err) sprintf(err, "sock return error [%s]", strerror(errno));
            return NULL;
        }

        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(bind_port);

        if (inet_pton(AF_INET6, bind_ip, &(addr6.sin6_addr)) != 1) {
            if (err) sprintf(err, "Invalid bind ip [%s]", bind_ip);
            close(sock);
            return NULL;
        }

        socklen = sizeof(addr6);
        if (bind(sock, (struct sockaddr*)&addr6, socklen) != 0) {
            if (err) sprintf(err, "bind to port [%d] return error [%s]", bind_port, strerror(errno));
            close(sock);
            return NULL;
        }
    } else if (mode == SLICE_SERVER_MODE_IP4_UDP) {
        if (err) sprintf(err, "UDP server is not implement");
        return NULL;
    } else {
        if (err) sprintf(err, "Invalid socket mode [%d]", (int)mode);
        return NULL;
    }

    reuse = 1;
    socklen = sizeof(reuse);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, socklen) != 0) {
        if (err) sprintf(err, "setsockopt return error [%s]", strerror(errno));
        close(sock);
        return NULL;
    }

    if ((skflag = fcntl(sock, F_GETFL, 0)) < 0) {
        if (err) sprintf(err, "fcntl(F_GETFL) return error [%s]", strerror(errno));
        close(sock);
        return NULL;
    }

    if (fcntl(sock, F_SETFL, skflag | O_NONBLOCK) < 0) {
        if (err) sprintf(err, "fcntl(F_SETFL) return error [%s]", strerror(errno));
        close(sock);
        return NULL;
    }

    if (mode == SLICE_SERVER_MODE_IP4_TCP || mode == SLICE_SERVER_MODE_IP6_TCP) {
        if (listen(sock, 256) != 0) {
            if (err) sprintf(err, "listen return error [%s]", strerror(errno));
            close(sock);
            return NULL;
        }
    }
    
    if (!(server = malloc(sizeof(*server)))) {
        if (err) sprintf(err, "Can't malloc server memory");
        close(sock);
        return NULL;
    }
    memset(server, 0, sizeof(*server));

    server->sock = sock;
    server->mode = mode;
    strcpy(server->bind_ip, bind_ip);
    server->bind_port = bind_port;

    server->accept_cb = accept_cb;
    server->ready_cb = ready_cb;
    server->read_callback = read_callback;
    server->close_callback = close_callback;
    server->ssl_ctx = ssl_ctx;

    SliceIOInit(server, sock, NULL);
    SliceMainloopEventSetUserData(server, NULL, NULL);

    sprintf(buff, "SERVER:[%s]:%d", bind_ip, bind_port);
    SliceMainloopEventSetName(server, buff, NULL);

    if (SliceMainloopEventAdd(mainloop, server, slice_server_add_callback, slice_server_remove_callback, err_buff) != SLICE_RETURN_NORMAL) {
        if (err) sprintf(err, "SliceMainloopEventAdd return error [%s]", err_buff);
        close(sock);
        free(server);
        return NULL;
    }

    printf("Server [%p] bind on [%s:%d]\n", server, bind_ip, bind_port);

    return server;
}
