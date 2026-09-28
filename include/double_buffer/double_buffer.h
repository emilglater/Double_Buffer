#ifndef DOUBLE_BUFFER_H
#define DOUBLE_BUFFER_H

#include <stdbool.h>

#include "double_buffer/config.h"

typedef enum
{
    DOUBLE_BUFFER_OK = 0,
    DOUBLE_BUFFER_NO_NEW_DATA,
    DOUBLE_BUFFER_ERROR_NULL,
    DOUBLE_BUFFER_ERROR_STATE
} double_buffer_status_e;

typedef struct
{
    double_buffer_slot_u    slot;
    uint32_t                next_sequence_number;
    bool                    has_new_data;
    uint8_t                 reserved[3];
} double_buffer_t;

double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer);

double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
                                                    double_buffer_slot_u** pp_out);

double_buffer_status_e double_buffer_commit(double_buffer_t* p_buffer);

double_buffer_status_e double_buffer_read(double_buffer_t* p_buffer,
                                            double_buffer_slot_u* p_out);

#endif