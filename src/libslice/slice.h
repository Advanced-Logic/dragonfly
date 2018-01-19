#ifndef _SLICE_H_
#define _SLICE_H_

#define SLICE_DEFAULT_ERROR_BUFF_SIZE         4096

enum slice_return_type
{
    SLICE_RETURN_ERROR = -1,
    SLICE_RETURN_NORMAL = 0,
    SLICE_RETURN_INFO
};

typedef enum slice_return_type SliceReturnType;

#endif
