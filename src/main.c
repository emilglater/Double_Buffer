#include "double_buffer/double_buffer.h"

#include <stdio.h>

typedef struct
{
    uint16_t    data;
} callback_data_t;

static void on_data_ready_mey(void* p_user_data)
{
    /* cppcheck-suppress misra-c2012-11.5 ; callback context is void* by design */
    callback_data_t* p_data = (callback_data_t*)p_user_data;

    if(p_user_data == NULL)
    {
        (void)printf("Data in callback is NULL\n");
    }
    else
    {
        p_data->data++;
    }
}

int main(void)
{
    callback_data_t user_data = { .data = 15 };
    double_buffer_t double_buff = { 0 };
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_slot_u result = { 0 };
    double_buffer_kind_e kind = DOUBLE_BUFFER_KIND_SCALAR;
    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    double_buffer_callbacks_t callbacks = { .on_data_ready = on_data_ready_mey,
                                            .p_user_data = &user_data };

    status = double_buffer_init(&double_buff);
    if(status != DOUBLE_BUFFER_OK)
    {
        (void)printf("Failed to init buffer\n");
        return 1;
    }

    status = double_buffer_set_callbacks(&double_buff, &callbacks);
    if(status != DOUBLE_BUFFER_OK)
    {
        (void)printf("Failed to set callbacks\n");
        return 1;
    }

    status = double_buffer_acquire_write(&double_buff, &p_slot);
    if(status != DOUBLE_BUFFER_OK)
    {
        (void)printf("Failed to acquire write\n");
        return 1;
    }

    p_slot->scalar.header.kind = (uint16_t)kind;
    p_slot->scalar.value = 45.3F;

    status = double_buffer_commit(&double_buff);
    if(status != DOUBLE_BUFFER_OK)
    {
        (void)printf("Failed to commit buffer\n");
        return 1;
    }

    status = double_buffer_read(&double_buff, &result);
    if(status != DOUBLE_BUFFER_OK)
    {
        (void)printf("Failed to read buffer\n");
        return 1;
    }

    (void)printf("kind %u, seq_num %u, value %f\n",
                    (unsigned int)result.scalar.header.kind,
                    (unsigned int)result.scalar.header.sequence_number,
                    (double)result.scalar.value);

    (void)printf("callback data: %u\n", (unsigned int)user_data.data);
    
    return 0;
}