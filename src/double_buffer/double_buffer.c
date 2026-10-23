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
    (void)memset(&p_buffer->callbacks, 0, sizeof(p_buffer->callbacks));

    if(!p_buffer->is_initialized)
    {
        int result = pthread_mutex_init(&p_buffer->lock, NULL);
        if(result != 0)
        {
            return DOUBLE_BUFFER_ERROR_STATE;
        }
    }

    p_buffer->is_initialized = true;

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_deinit(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    if(p_buffer->is_initialized)
    {
        int result = pthread_mutex_destroy(&p_buffer->lock);
        if(result != 0)
        {
            return DOUBLE_BUFFER_ERROR_STATE;
        }
    }


    p_buffer->is_initialized = false;

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
    double_buffer_slot_u** pp_out)
{
    if((p_buffer == NULL) || (pp_out == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    if(!p_buffer->is_initialized)
    {
        return DOUBLE_BUFFER_ERROR_STATE;
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

    if(!p_buffer->is_initialized)
    {
        return DOUBLE_BUFFER_ERROR_STATE;
    }

    (void)pthread_mutex_lock(&p_buffer->lock);

    const uint8_t write_index = p_buffer->write_index;
    const double_buffer_callbacks_t callbacks = p_buffer->callbacks;
    p_buffer->slots[write_index].header.sequence_number = p_buffer->next_sequence_number;
    p_buffer->next_sequence_number++;
    p_buffer->write_index = p_buffer->read_index;
    p_buffer->read_index = write_index;
    p_buffer->has_new_data = true;

    (void)pthread_mutex_unlock(&p_buffer->lock);

    if(callbacks.on_data_ready != NULL)
    {
        callbacks.on_data_ready(callbacks.p_user_data);
    }

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_read(double_buffer_t* p_buffer,
    double_buffer_slot_u* p_out)
{
    if((p_buffer == NULL) || (p_out == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    if(!p_buffer->is_initialized)
    {
        return DOUBLE_BUFFER_ERROR_STATE;
    }

    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    
    (void)pthread_mutex_lock(&p_buffer->lock);

    if(!p_buffer->has_new_data)
    {
        status = DOUBLE_BUFFER_NO_NEW_DATA;
    }
    else
    {
        uint8_t read_index = p_buffer->read_index;
        *p_out = p_buffer->slots[read_index];
        p_buffer->has_new_data = false;
        status = DOUBLE_BUFFER_OK;
    }

    (void)pthread_mutex_unlock(&p_buffer->lock);

    return status;
}

double_buffer_status_e double_buffer_set_callbacks(double_buffer_t* p_buffer,
    const double_buffer_callbacks_t* p_callbacks)
{
    if((p_buffer == NULL) || (p_callbacks == NULL))
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    if(!p_buffer->is_initialized)
    {
        return DOUBLE_BUFFER_ERROR_STATE;
    }

    p_buffer->callbacks = *p_callbacks;

    return DOUBLE_BUFFER_OK;
}