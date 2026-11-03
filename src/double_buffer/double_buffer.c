#include "double_buffer/double_buffer.h"
#include "time_port/time_port.h"

#include <stddef.h>
#include <string.h>

double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    (void)memset(p_buffer->slots, 0, sizeof(p_buffer->slots));
    p_buffer->next_sequence_number = 0U;
    p_buffer->last_commit_time_us = time_port_get_time_us();
    p_buffer->write_timeout_us = 0U;
    p_buffer->has_new_data = false;
    p_buffer->is_timeout_reported = false;
    p_buffer->write_index = 0U;
    p_buffer->read_index = 1U;
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

    uint32_t current_time_us = time_port_get_time_us();

    (void)pthread_mutex_lock(&p_buffer->lock);

    const uint8_t write_index = p_buffer->write_index;
    const double_buffer_callbacks_t callbacks = p_buffer->callbacks;
    p_buffer->slots[write_index].header.sequence_number = p_buffer->next_sequence_number;
    p_buffer->next_sequence_number++;
    p_buffer->slots[write_index].header.timestamp_us = current_time_us;
    p_buffer->last_commit_time_us = current_time_us;
    p_buffer->write_index = p_buffer->read_index;
    p_buffer->read_index = write_index;
    p_buffer->has_new_data = true;
    p_buffer->is_timeout_reported = false;

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

double_buffer_status_e double_buffer_abort_write(double_buffer_t* p_buffer)
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

    double_buffer_callbacks_t callbacks_copy = p_buffer->callbacks;

    (void)pthread_mutex_unlock(&p_buffer->lock);

    if(callbacks_copy.on_write_failed != NULL)
    {
        callbacks_copy.on_write_failed(callbacks_copy.p_user_data);
    }

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_set_write_timeout_us(double_buffer_t* p_buffer,
    uint32_t timeout_us)
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

    p_buffer->write_timeout_us = timeout_us;

    (void)pthread_mutex_unlock(&p_buffer->lock);

    return DOUBLE_BUFFER_OK;
}

double_buffer_status_e double_buffer_check_write_timeout(double_buffer_t* p_buffer)
{
    if(p_buffer == NULL)
    {
        return DOUBLE_BUFFER_ERROR_NULL;
    }

    if(!p_buffer->is_initialized)
    {
        return DOUBLE_BUFFER_ERROR_STATE;
    }

    uint32_t current_time_us = time_port_get_time_us();
    double_buffer_callbacks_t callbacks_copy = { 0 };
    bool is_callback_needed = false;

    (void)pthread_mutex_lock(&p_buffer->lock);

    if(p_buffer->write_timeout_us != 0U)
    {
        uint32_t interval = current_time_us - p_buffer->last_commit_time_us;
        if((interval >= p_buffer->write_timeout_us) && 
            (!p_buffer->is_timeout_reported))
        {
            callbacks_copy = p_buffer->callbacks;
            p_buffer->is_timeout_reported = true;
            is_callback_needed = true;
        }
    }

    (void)pthread_mutex_unlock(&p_buffer->lock);

    if(is_callback_needed)
    {
        if(callbacks_copy.on_write_failed != NULL)
        {
            callbacks_copy.on_write_failed(callbacks_copy.p_user_data);
        }
    }

    return DOUBLE_BUFFER_OK;
}