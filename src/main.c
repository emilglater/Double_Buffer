#include "double_buffer/double_buffer.h"

#include <stdio.h>

int main(void)
{
    double_buffer_slot_u slot;

    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    (void)status;

    double_buffer_kind_e kind = DOUBLE_BUFFER_KIND_SCALAR;
    (void)kind;

    slot.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_SCALAR;
    slot.scalar.value = 1.0f;

    (void)printf("kind %u, version %f\n",
            (unsigned int)slot.header.kind,
            (double)double_buffer_get_version());
    
    return 0;
}