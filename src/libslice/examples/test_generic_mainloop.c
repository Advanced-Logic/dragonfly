
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "slice-mainloop.h"
#include "slice-client.h"
#include "slice-server.h"
#include "slice-ssl-client.h"
#include "slice-ssl-server.h"

SliceMainloop *mainloop = NULL;
SliceSSLContext *ssl_ctx = NULL;
/*
#define SERVER_HOST "www.starplustrips.com"
#define SERVER_PORT 443

SliceReturnType client_read_callback(SliceClient *client, int length, void *user_data, char *err)
{
    printf("read callback length: %d\n", length);

    return 0;
}

void client_connection_close_callback(SliceConnection *conn, void *user_data, char *error)
{
    SliceBuffer *buffer;
    if ((buffer = SliceConnectionGetReadBuffer(conn))) {
        //printf("Total read data[%u] [%.*s]\n", buffer->length, buffer->length, buffer->data);
        printf("Total read data[%u]\n", buffer->length);
    } else {
        printf("Connection [%p] buffer not found\n", conn);
    }
}

SliceReturnType connect_result_https_cb(SliceClient *client, SliceReturnType res, char *error)
{
    SliceBuffer *buffer;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE]; err_buff[0] = 0;

    if (res == SLICE_RETURN_NORMAL) {
        if (SliceClientStart(client, ssl_ctx, client_read_callback, client_connection_close_callback, NULL, err_buff) != SLICE_RETURN_NORMAL) {
            printf("SliceClientStart return error [%s]\n", err_buff);
            return SLICE_RETURN_ERROR;
        }

        if (!(buffer = SliceBufferCreate(mainloop, 1024 * 8, err_buff))) {
            printf("SliceBufferCreate return error [%s]\n", err_buff);
            return SLICE_RETURN_ERROR;
        }

        buffer->length = sprintf(buffer->data, "GET /tour_in HTTP/1.1\r\nHost: " SERVER_HOST "\r\nUser-Agent: Test Client\r\nConnection: close\r\n\r\n");

        SliceClientWrite(client, buffer, err_buff);
    } else {
        printf("Connection connect error [%s]\n", error);
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType connect_result_http_cb(SliceClient *client, SliceReturnType res, char *error)
{
    SliceBuffer *buffer;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE]; err_buff[0] = 0;

    if (res == SLICE_RETURN_NORMAL) {
        if (SliceClientStart(client, NULL, client_read_callback, client_connection_close_callback, NULL, err_buff) != SLICE_RETURN_NORMAL) {
            printf("SliceClientStart return error [%s]\n", err_buff);
            return SLICE_RETURN_ERROR;
        }

        if (!(buffer = SliceBufferCreate(mainloop, 1024 * 8, err_buff))) {
            printf("SliceBufferCreate return error [%s]\n", err_buff);
            return SLICE_RETURN_ERROR;
        }

        buffer->length = sprintf(buffer->data, "GET /tour_in HTTP/1.1\r\nHost: " SERVER_HOST "\r\nUser-Agent: Test Client\r\nConnection: close\r\n\r\n");

        SliceClientWrite(client, buffer, err_buff);
    } else {
        printf("Connection connect error [%s]\n", error);
    }

    return SLICE_RETURN_NORMAL;
}
*/

static SliceReturnType server_accept_callback(SliceSession *session, char *err)
{
    printf("Session [%p] sock [%d] [%s:%d] server_accept_callback\n", session, SliceIOGetFD(session), SliceSessionGetPeerIP(session), SliceSessionGetPeerPort(session));
    return SLICE_RETURN_NORMAL;
}

static SliceReturnType session_ready_callback(SliceSession *session, char *err)
{
    printf("Session [%p] sock [%d] [%s:%d] session_ready_callback\n", session, SliceIOGetFD(session), SliceSessionGetPeerIP(session), SliceSessionGetPeerPort(session));
    return SLICE_RETURN_NORMAL;
}

static SliceReturnType session_read_callback(SliceSession *session, int length, void *user_data, char *err)
{
    printf("Session [%p] sock [%d] [%s:%d] session_read_callback length [%d] data [%s]\n", session, SliceIOGetFD(session), SliceSessionGetPeerIP(session), SliceSessionGetPeerPort(session), length, (SliceSessionGetReadBuffer(session))->data);
    SliceSessionClearReadBuffer(session, NULL);

    return SLICE_RETURN_NORMAL;
}

static void connection_close_callback(SliceConnection *connection, void *user_data, char *err)
{
    printf("Connection sock [%d] [%s:%d] connection_close_callback\n", SliceIOGetFD(SliceConnectionGetMainloopEvent(connection)), SliceConnectionGetPeerIP(connection), SliceConnectionGetPeerPort(connection));
}

static void interupt_signal(int sig)
{
    printf("Quit\n");
    sleep(1);
    if (mainloop) SliceMainloopQuit(mainloop);
}

int main()
{
    
    //SliceClient *client;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!(mainloop = SliceMainloopCreate(128, 128, 1000, err_buff))) {
        printf("SliceMainloopCreate return error [%s]\n", err_buff);
        return -1;
    }
    
    SliceSSLLoadLibrary();
    //SliceSSLClientBucketInit(NULL);
    SliceSSLSessionBucketInit(NULL);
    
    if (!(ssl_ctx = SliceSSLContextCreate(SLICE_SSL_CONNECTION_TYPE_SERVER, SLICE_SSL_MODE_TLS_1_2, SLICE_SSL_FILE_TYPE_PEM, "./test.cert", "", "./test.key", err_buff))) {
        printf("SliceSSLContextCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }
    /*
    if (!(client = SliceClientCreate(mainloop, SERVER_HOST, SERVER_PORT, SLICE_CONNECTION_MODE_IP4_TCP, connect_result_https_cb, err_buff))) {
        printf("SliceClientCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }

    if (!(client = SliceClientCreate(mainloop, SERVER_HOST, SERVER_PORT, SLICE_CONNECTION_MODE_IP4_TCP, connect_result_https_cb, err_buff))) {
        printf("SliceClientCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }
    */

    SliceServer *server;

    if (!(server = SliceServerCreate(mainloop, SLICE_SERVER_MODE_IP4_TCP, "", 5678, ssl_ctx, server_accept_callback, session_ready_callback, session_read_callback, connection_close_callback, err_buff))) {
        printf("SliceServerCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }


    signal(SIGINT, interupt_signal);

    SliceMainloopRun(mainloop, NULL);

    printf("mainloop exit\n");

    SliceSSLContextDestroy(ssl_ctx, NULL);

    //SliceSSLClientBucketDestroy(NULL);
    SliceSSLSessionBucketDestroy(NULL);

    SliceMainloopDestroy(mainloop, NULL);

    return 0;
}
