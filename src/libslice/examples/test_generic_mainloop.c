
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "../slice-mainloop.h"
#include "../slice-client.h"

SliceMainloop *mainloop = NULL;

static int slice_client_read_callback(struct slice_mainloop_epoll *epoll, struct slice_mainloop_epoll_element *element, struct epoll_event ev, void *user_data)
{
    SliceClient *client = user_data;

    char buff[2048];
    int r;

    if ((r = recv(element->fd, buff, sizeof(buff) - 1, 0)) < 0) {
        SliceClientRemove(client, NULL);
        free(client);

        return -1;
    } else if (r == 0) {
        SliceClientRemove(client, NULL);
        free(client);

        if (mainloop) SliceMainloopQuit(mainloop);

        return -1;
    }

    buff[r] = 0;

    printf("Recv [%s]\n", buff);

    return 0;
}

static int slice_client_write_callback(struct slice_mainloop_epoll *epoll, struct slice_mainloop_epoll_element *element, struct epoll_event ev, void *user_data)
{
    printf("application write callback\n");

    return 0;
}

int connect_result_cb(SliceClient *client, SliceConnection *conn, int res, char *err)
{
    SliceMainloopEpollEventSetCallback(client->mainloop_event.mainloop, client->mainloop_event.io.fd, SLICE_MAINLOOP_EPOLL_EVENT_READ, slice_client_read_callback, NULL);
    SliceMainloopEpollEventAddRead(client->mainloop_event.mainloop, client->mainloop_event.io.fd, NULL);

    SliceMainloopEpollEventSetCallback(client->mainloop_event.mainloop, client->mainloop_event.io.fd, SLICE_MAINLOOP_EPOLL_EVENT_WRITE, slice_client_write_callback, NULL);
    //SliceMainloopEpollEventAddWrite(client->mainloop_event.mainloop, client->mainloop_event.io.fd, NULL);

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

    if (!(client = SliceClientCreate(mainloop, "127.0.0.1", 5050, SLICE_CONNECTION_TYPE_IP4_TCP, connect_result_cb, err_buff))) {
        printf("SliceClientCreate return error [%s]\n", err_buff);
        SliceMainloopDestroy(mainloop, NULL);
        return -1;
    }

    signal(SIGINT, interupt_signal);

    SliceMainloopRun(mainloop, NULL);

    printf("mainloop exit\n");

    SliceMainloopDestroy(mainloop, NULL);

    return 0;
}
