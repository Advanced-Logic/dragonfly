
#include <stdio.h>

#include "slice-object.h"

SliceReturnType slice_object_list_append(SliceObject **head, SliceObject *item, char *err)
{
    if (!head || !item) {
	    if (err) sprintf(err, "Invalid parameter");
	    return SLICE_RETURN_ERROR;
    }
	
    if (item->next || item->prev) {
        if (err) sprintf(err, "Item already in list");
        return SLICE_RETURN_ERROR;
    }
	
    if (!(*head)) {
        (*head) = item->next = item->prev = item;
    } else {
        item->prev = (*head)->prev;
        item->next = (*head);
        (*head)->prev->next = item;
        (*head)->prev = item;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_object_list_remove(SliceObject **head, SliceObject *item, char *err)
{
    if (!head || !(*head) || !item) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (!item->next || !item->prev) {
        if (err) sprintf(err, "Item not steal in list");
        return SLICE_RETURN_ERROR;
    }

    if ((*head) == item) {
        if ((*head)->next == (*head)) {
            (*head) = NULL;
        } else {
            (*head) = item->next;
            item->next->prev = item->prev;
            item->prev->next = item->next;
        }
    } else {
        item->next->prev = item->prev;
        item->prev->next = item->next;
    }

    item->next = item->prev = NULL;

    return SLICE_RETURN_NORMAL;
}

/*
SliceReturnType slice_object_set_user_data(SliceObject *obj, void *user_data, int(*cb_user_data_destroy)(SliceObject*, void*, char*), char *err)
{
    char err_buff[SLICE_DEFAULT_ERROR_BUFF_SIZE]; err_buff[0] = 0;

    if (!obj || !user_data || !cb_user_data_destroy) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    if (obj->user_data && obj->destroy_cb) {
        if (obj->destroy_cb(obj, obj->user_data, err_buff) != SLICE_RETURN_NORMAL) {
            if (err) sprintf(err, "User data destroy callback return error [%s]", err_buff);
            return SLICE_RETURN_ERROR;
        }
    }

    obj->user_data = user_data;
    obj->destroy_cb = cb_user_data_destroy;

    return SLICE_RETURN_NORMAL;
}

void slice_object_destroying(SliceObject *obj)
{
    if (obj->user_data && obj->destroy_cb) {
        obj->destroy_cb(obj, obj->user_data, NULL);
    }
}
*/