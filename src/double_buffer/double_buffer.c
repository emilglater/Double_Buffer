#include "double_buffer/double_buffer.h"

#include <stddef.h>
#include <string.h>

double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    (void)memset(p_buffer->slots, 0, sizeof(p_buffer->slots));
    p_buffer->next_sequence_number = 0;
    p_buffer->has_new_data = false;
    p_buffer->write_index = 0;
    p_buffer->read_index = 1;

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
                                                    double_buffer_slot_u** pp_out)
{
    if((p_buffer == NULL) || (pp_out == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    uint8_t write_index = p_buffer->write_index;
    *pp_out = &p_buffer->slots[write_index];
    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_commit(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    uint8_t write_index = p_buffer->write_index;
    p_buffer->slots[write_index].header.sequence_number = p_buffer->next_sequence_number;
    p_buffer->next_sequence_number++;
    p_buffer->write_index = p_buffer->read_index;
    p_buffer->read_index = write_index;
    p_buffer->has_new_data = true;

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_read(double_buffer_t* p_buffer,
                                            double_buffer_slot_u* p_out)
{
    if((p_buffer == NULL) || (p_out == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }
    if(!p_buffer->has_new_data)
    {
        return DOUBLE_BUFFER_NO_NEW_DATA;
    }

    uint8_t read_index = p_buffer->read_index;
    *p_out = p_buffer->slots[read_index];
    p_buffer->has_new_data = false;

    return DOUBLE_BUFFER_OK;
}