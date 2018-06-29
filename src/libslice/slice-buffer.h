#ifndef _SLICE_BUFFER_H_
#define _SLICE_BUFFER_H_

#include "slice-mainloop.h"
#include "slice-object.h"

#define SLICE_BUFFER_BLOCK_SIZE             (32 * 1024)
#define SLICE_BUFFER_BUCKET_MAX             64

typedef struct slice_buffer SliceBuffer;

// loop struct definetion
struct slice_mainloop;

struct slice_buffer
{
    SliceObject obj;

    unsigned int size;
    unsigned int length;
    unsigned int current;
    char data[1];
};

#ifdef __cplusplus
extern "C" {
#endif

SliceBuffer *slice_buffer_create(struct slice_mainloop *mainloop, unsigned int size, char *err);
SliceReturnType slice_buffer_prepare(struct slice_mainloop *mainloop, SliceBuffer **buff, unsigned int need_size, char *err);
SliceReturnType slice_buffer_release(struct slice_mainloop *mainloop, SliceBuffer **buff, char *err);

#ifdef __cplusplus
}
#endif

#define SliceBufferCreate(_mainloop, _size, _err) slice_buffer_create(_mainloop, _size, _err)
#define SliceBufferPrepare(_mainloop, _buff, _need_size, _err) slice_buffer_prepare(_mainloop, (SliceBuffer**)_buff, _need_size, _err)
#define SliceBufferRelease(_mainloop, _buff, _err) slice_buffer_release(_mainloop, _buff, _err)

#endif
