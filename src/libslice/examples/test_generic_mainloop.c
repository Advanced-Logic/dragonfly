
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "slice-mainloop.h"
#include "slice-client.h"
#include "slice-ssl-client.h"

SliceMainloop *mainloop = NULL;

SliceReturnType client_read_callback(SliceClient *client, int length, void *user_data, char *err)
{
    printf("read callback length: %d\n", length);

    return 0;
}

void client_connection_close_callback(SliceConnection *conn, void *user_data, char *error)
{
    SliceBuffer *buffer;
    printf("Connection close callback\n");
    if ((buffer = SliceConnectionGetReadBuffer(conn))) {
        printf("Total read data[%u] [%.*s]\n", buffer->length, buffer->length, buffer->data);
    } else {
        printf("Connection [%p] buffer not found\n", conn);
    }
}

SliceReturnType connect_result_https_cb(SliceClient *client, int res, char *error)
{
    SliceSSLContext *ssl_ctx;
    SliceBuffer *buffer;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE]; err_buff[0] = 0;

    if (res == 0) {
        if (!(ssl_ctx = SliceSSLContextCreate(SLICE_SSL_CONNECTION_TYPE_CLIENT, SLICE_SSL_MODE_TLS_1_2, SLICE_SSL_FILE_TYPE_NONE, "", "", "", err_buff))) {
            printf("SliceSSLContextCreate return error [%s]\n", err_buff);
            return -1;
        }

        if (SliceClientStart(client, ssl_ctx, client_read_callback, client_connection_close_callback, NULL, err_buff) != SLICE_RETURN_NORMAL) {
            printf("SliceClientStart return error [%s]\n", err_buff);
            return -1;
        }

        if (!(buffer = SliceBufferCreate(mainloop, 1024 * 8, err_buff))) {
            printf("SliceBufferCreate return error [%s]\n", err_buff);
            return -1;
        }

        buffer->length = sprintf(buffer->data, "GET / HTTP/1.1\r\nHost: www.google.com\r\nUser-Agent: Test Client\r\nConnection: close\r\n\r\n");

        SliceClientWrite(client, buffer, err_buff);
    } else {
        printf("Connection connect error [%s]\n", error);
    }

    return 0;
}

SliceReturnType connect_result_http_cb(SliceClient *client, int res, char *error)
{
    SliceBuffer *buffer;
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE]; err_buff[0] = 0;

    if (res == 0) {
        if (SliceClientStart(client, NULL, client_read_callback, client_connection_close_callback, NULL, err_buff) != SLICE_RETURN_NORMAL) {
            printf("SliceClientStart return error [%s]\n", err_buff);
            return -1;
        }

        if (!(buffer = SliceBufferCreate(mainloop, 1024 * 8, err_buff))) {
            printf("SliceBufferCreate return error [%s]\n", err_buff);
            return -1;
        }

        buffer->length = sprintf(buffer->data, "GET / HTTP/1.1\r\nHost: www.google.com\r\nUser-Agent: Test Client\r\nConnection: close\r\n\r\n");

        SliceClientWrite(client, buffer, err_buff);
    } else {
        printf("Connection connect error [%s]\n", error);
    }

    return 0;
}

static void interupt_signal(int sig)
{
    printf("Quit\n");
    sleep(1);
    if (mainloop) SliceMainloopQuit(mainloop);
}

int main()
{
    
    SliceClient *client;

    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE];

    if (!(mainloop = SliceMainloopCreate(128, 128, 1000, err_buff))) {
        printf("SliceMainloopCreate return error [%s]\n", err_buff);
        return -1;
    }

    SliceSSLLoadLibrary();
    SliceSSLClientBucketInit(NULL);

    if (!(client = SliceClientCreate(mainloop, "www.google.com", 443, SLICE_CONNECTION_TYPE_IP4_TCP, connect_result_https_cb, err_buff))) {
        printf("SliceClientCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }

    if (!(client = SliceClientCreate(mainloop, "www.google.com", 80, SLICE_CONNECTION_TYPE_IP4_TCP, connect_result_http_cb, err_buff))) {
        printf("SliceClientCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }

    signal(SIGINT, interupt_signal);

    SliceMainloopRun(mainloop, NULL);

    printf("mainloop exit\n");

    SliceSSLClientBucketDestroy(NULL);

    SliceMainloopDestroy(mainloop, NULL);

    return 0;
}
