
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slice-buffer.h"

SliceBuffer *slice_buffer_create(SliceMainloop *mainloop, unsigned int size, char *err)
{
    SliceBuffer *buffer = NULL, *tmp, *buffer_bucket, *new_buff;
    unsigned int adj_size;

    if (size == 0) size = 1;

    adj_size = ((size / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

    if (mainloop && (tmp = buffer_bucket = SliceMainloopGetBufferBucket(mainloop))) {
        do {
            if (tmp->size >= adj_size) break;

            tmp = (SliceBuffer*)tmp->obj.next;
        } while(tmp != buffer_bucket);

        SliceMainloopBufferBucketRemove(mainloop, tmp, NULL);
        buffer = tmp;

        if (buffer->size < adj_size) {
            //printf("realloc 1 [%lu]\n", (size_t)(sizeof(SliceBuffer) + adj_size));
            if (!(new_buff = (SliceBuffer*)realloc(buffer, (size_t)(sizeof(SliceBuffer) + adj_size)))) {
                if (err) sprintf(err, "Can't re-allocate buffer memory");
                free(buffer);
                return NULL;
            }

            buffer = new_buff;
        }

        buffer->size = adj_size;
        buffer->length = 0;
        buffer->current = 0;
        buffer->data[0] = 0;
    } else {
        //printf("malloc [%lu]\n", (size_t)(sizeof(SliceBuffer) + adj_size));
        if (!(buffer = (SliceBuffer*)malloc((size_t)(sizeof(SliceBuffer) + adj_size)))) {
            if (err) sprintf(err, "Can't allocate buffer memory");
            return NULL;
        }

        memset(buffer, 0, sizeof(SliceBuffer));
        buffer->data[0] = 0;
        buffer->size = adj_size;
    }

    return buffer;
}

SliceReturnType slice_buffer_prepare(SliceMainloop *mainloop, SliceBuffer **buffer, unsigned int need_size, char *err)
{
    SliceBuffer *new_buff;
    unsigned int adj_size;

    if (!buffer || need_size == 0) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!(*buffer)) {
        if (!((*buffer) = slice_buffer_create(mainloop, need_size, err))) {
            return SLICE_RETURN_ERROR;
        }
    } else if (((*buffer)->length + need_size) > (*buffer)->size) {
        if ((*buffer)->obj.next || (*buffer)->obj.prev) {
            if (err) sprintf(err, "Can't re-allocate buffer while object already in list");
            return SLICE_RETURN_ERROR;
        }

        need_size += (*buffer)->length;

        adj_size = ((need_size / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

        //printf("realloc 2 [%lu]\n", (size_t)(sizeof(SliceBuffer) + adj_size));
        if (!(new_buff = (SliceBuffer*)realloc((*buffer), (size_t)(sizeof(SliceBuffer) + adj_size)))) {
            if (err) sprintf(err, "Can't re-allocate buffer memory");
            return SLICE_RETURN_ERROR;
        }

        new_buff->data[new_buff->length] = 0;
        new_buff->size = adj_size;

        (*buffer) = new_buff;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_buffer_release(SliceMainloop *mainloop, SliceBuffer **buffer, char *err)
{
    if (!buffer || !(*buffer)) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((*buffer)->obj.next || (*buffer)->obj.prev) {
        if (err) sprintf(err, "Buffer steal in list");
        return SLICE_RETURN_ERROR;
    }

    if (mainloop && SliceMainloopGetBufferBucketCount(mainloop) < SLICE_BUFFER_BUCKET_MAX) {
        SliceMainloopBufferBucketAdd(mainloop, (*buffer), NULL);
    } else {
        free(*buffer);
    }

    (*buffer) = NULL;

    return SLICE_RETURN_NORMAL;
}
