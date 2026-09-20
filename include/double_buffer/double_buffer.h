#ifndef DOUBLE_BUFFER_H
#define DOUBLE_BUFFER_H

#include "double_buffer/config.h"

typedef enum
{
    DOUBLE_BUFFER_OK = 0,
    DOUBLE_BUFFER_NO_NEW_DATA,
    DOUBLE_BUFFER_ERROR_NULL,
    DOUBLE_BUFFER_ERROR_STATE
} double_buffer_status_e;

float double_buffer_get_version(void);

#endif