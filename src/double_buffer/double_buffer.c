#include "double_buffer/double_buffer.h"

#include <stddef.h>
#include <string.h>

double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    (void)memset(&p_buffer->slot, 0, sizeof(p_buffer->slot));
    p_buffer->next_sequence_number = 0;
    p_buffer->has_new_data = false;

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
                                                    double_buffer_slot_u** pp_out)
{
    if((p_buffer == NULL) || (pp_out == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    *pp_out = &p_buffer->slot;
    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_commit(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    p_buffer->slot.header.sequence_number = p_buffer->next_sequence_number;
    p_buffer->next_sequence_number++;
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

    *p_out = p_buffer->slot;
    p_buffer->has_new_data = false;

    return DOUBLE_BUFFER_OK;
}