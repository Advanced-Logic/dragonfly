
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slice-buffer.h"

SliceBuffer *slice_buffer_create(SliceMainloop *mainloop, unsigned int size, char *err)
{
    SliceBuffer *buff = NULL, *tmp;
    int adj_size;

    if (size == 0) size = 1;

    adj_size = (((size - 1) / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

    if (mainloop && (tmp = mainloop->buffer_bucket)) {
        do {
            if (tmp->size >= adj_size) break;

            tmp = (SliceBuffer*)tmp->obj.next;
        } while(tmp != mainloop->buffer_bucket);

        SliceListRemove(&(mainloop->buffer_bucket), tmp, NULL);
        buff = tmp;

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

SliceReturnType slice_buffer_prepare(SliceMainloop *mainloop, SliceBuffer **buff, unsigned int need_size, char *err)
{
    SliceBuffer *new_buff;
    int adj_size;

    if (!buff || need_size == 0) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!(*buff)) {
        if (!((*buff) = slice_buffer_create(mainloop, need_size, err))) {
            return SLICE_RETURN_ERROR;
        }
    } else if (((*buff)->length + need_size) > (*buff)->size) {
        if ((*buff)->obj.next || (*buff)->obj.prev) {
            if (err) sprintf(err, "Can't re-allocate buffer while object already in list");
            return SLICE_RETURN_ERROR;
        }

        need_size += (*buff)->length;

        adj_size = (((need_size - 1) / SLICE_BUFFER_BLOCK_SIZE) + 1) * SLICE_BUFFER_BLOCK_SIZE;

        if (!(new_buff = (SliceBuffer*)realloc((*buff), adj_size))) {
            if (err) sprintf(err, "Can't re-allocate buffer memory");
            return SLICE_RETURN_ERROR;
        }

        new_buff->data[new_buff->length] = 0;
        new_buff->size = adj_size;

        (*buff) = new_buff;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_buffer_release(SliceMainloop *mainloop, SliceBuffer **buff, char *err)
{
    if (!buff || !(*buff)) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if ((*buff)->obj.next || (*buff)->obj.prev) {
        if (err) sprintf(err, "Buffer steal in list");
        return SLICE_RETURN_ERROR;
    }

    if (mainloop && mainloop->buffer_bucket_count < SLICE_BUFFER_BUCKET_MAX) {
        SliceListAppend(&(mainloop->buffer_bucket), (*buff), NULL);
        mainloop->buffer_bucket_count++;
    } else {
        free(*buff);
    }

    (*buff) = NULL;

    return SLICE_RETURN_NORMAL;
}
