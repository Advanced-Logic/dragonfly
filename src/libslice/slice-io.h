#ifndef _SLICE_IO_H_
#define _SLICE_IO_H_

#include "slice-object.h"

struct slice_io
{
    struct slice_object obj;

    int fd;
};

typedef struct slice_io SliceIO;

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_oi_init(SliceIO *io, int fd, char *err);
SliceReturnType slice_io_close(SliceIO *io, char *err);
int slice_io_read(SliceIO *io, void *buffer, unsigned int buffer_size, char *err);
int slice_io_write(SliceIO *io, void *buffer, unsigned int buffer_size, char *err);

#ifdef __cplusplus
}
#endif

#define SliceIOInit(_io, _fd, _err) slice_oi_init((SliceIO*)_io, _fd, _err)
#define SliceIOClose(_io, _err) slice_io_close((SliceIO*)_io, _err)
#define SliceIORead(_io, _buff, _size, _err) slice_io_read((SliceIO*)_io, _buff, _size, _err)
#define SliceIOWrite(_io, _buff, _size, _err) slice_io_write((SliceIO*)_io, _buff, _size, _err)

#endif
