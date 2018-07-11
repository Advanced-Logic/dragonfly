#ifndef _SLICE_OBJECT_H_
#define _SLICE_OBJECT_H_

#include "slice.h"

typedef struct slice_object SliceObject;

struct slice_object
{
    SliceObject *next;
    SliceObject *prev;
	
	//void *user_data;
	//int(*destroy_cb)(struct slice_object*, void*, char*);
};

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_object_list_append(SliceObject **head, SliceObject *item, char *err);
SliceReturnType slice_object_list_remove(SliceObject **head, SliceObject *item, char *err);

#ifdef __cplusplus
}
#endif

#define SliceListAppend(_head, _item, _err) slice_object_list_append((SliceObject**)_head, (SliceObject*)_item, _err)
#define SliceListRemove(_head, _item, _err) slice_object_list_remove((SliceObject**)_head, (SliceObject*)_item, _err)

#endif
