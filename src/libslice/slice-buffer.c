
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slice-buffer.h"

SliceBuffer *slice_buffer_create(struct slice_mainloop *mainloop, unsigned int size, char *err)
{
    SliceBuffer *buff = NULL, *tmp, *buffer_bucket;
    int adj_size;

    if (size == 0) size = 1;

    adj_size = (((size - 1) / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

    if (mainloop && (tmp = buffer_bucket = SliceMainloopGetBufferBucket(mainloop))) {
        do {
            if (tmp->size >= adj_size) break;

            tmp = (SliceBuffer*)tmp->obj.next;
        } while(tmp != buffer_bucket);

        SliceMainloopBufferBucketRemove(mainloop, tmp, NULL);
        buff = tmp;

        if (tmp->size < adj_size) {

        }

        buff->length = 0;
        buff->current = 0;
        buff->data[0] = 0;
    } else {
        if (!(buff = (SliceBuffer*)malloc(sizeof(SliceBuffer) + adj_size))) {
            if (err) sprintf(err, "Can't allocate buffer memory");
            return NULL;
        }

        memset(buff, 0, sizeof(SliceBuffer));
        buff->data[0] = 0;
        buff->size = adj_size;
    }

    return buff;
}

SliceReturnType slice_buffer_prepare(struct slice_mainloop *mainloop, SliceBuffer **buffer, unsigned int need_size, char *err)
{
    SliceBuffer *new_buff;
    int adj_size;

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

        adj_size = (((need_size - 1) / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

        if (!(new_buff = (SliceBuffer*)realloc((*buffer), adj_size))) {
            if (err) sprintf(err, "Can't re-allocate buffer memory");
            return SLICE_RETURN_ERROR;
        }

        new_buff->data[new_buff->length] = 0;
        new_buff->size = adj_size;

        (*buffer) = new_buff;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_buffer_release(struct slice_mainloop *mainloop, SliceBuffer **buffer, char *err)
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
