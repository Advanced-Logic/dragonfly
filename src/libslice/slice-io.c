#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "slice-io.h"

SliceReturnType slice_oi_init(SliceIO *io, int fd, char *err)
{
    if (!io || fd < 0 || fd > 0xffff) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    io->fd = fd;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_io_close(SliceIO *io, char *err)
{
    if (!io) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (io->fd >= 0 && io->fd <= 0xffff) {
        close(io->fd);
        io->fd = -1;
    }

    return SLICE_RETURN_NORMAL;
}

int slice_io_read(SliceIO *io, void *buffer, unsigned int buffer_size, char *err)
{
    int r;

    if (!io) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    if (io->fd < 0 || io->fd > 0xffff) {
        if (err) sprintf(err, "Invalid IO file descriptor");
        return -1;
    }

    if ((r = read(io->fd, buffer, buffer_size)) < 0) {
        if (err) sprintf(err, "Read FD [%d] error [%s]", io->fd, strerror(errno));
    }

    return r;
}

int slice_io_write(SliceIO *io, void *buffer, unsigned int buffer_size, char *err)
{
    int r;

    if (!io) {
        if (err) sprintf(err, "Invalid parameter");
        return -1;
    }

    if (io->fd < 0 || io->fd > 0xffff) {
        if (err) sprintf(err, "Invalid IO file descriptor");
        return -1;
    }

    if ((r = write(io->fd, buffer, buffer_size)) < 0) {
        if (err) sprintf(err, "Write FD [%d] error [%s]", io->fd, strerror(errno));
    }

    return r;
}