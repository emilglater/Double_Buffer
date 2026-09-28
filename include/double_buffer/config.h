#ifndef DOUBLE_BUFFER_CONFIG_H
#define DOUBLE_BUFFER_CONFIG_H

#include <stdint.h>

#define DOUBLE_BUFFER_BLOB_SIZE (32U)  // needs to be a multiple of 4

typedef enum
{
    DOUBLE_BUFFER_KIND_NONE = 0,
    DOUBLE_BUFFER_KIND_SCALAR,
    DOUBLE_BUFFER_KIND_VECTOR3,
    DOUBLE_BUFFER_KIND_BLOB
} double_buffer_kind_e;

typedef struct
{
    uint32_t timestamp_us;
    uint32_t sequence_number;
    uint16_t kind;
    uint16_t payload_len;
} double_buffer_header_t;

typedef struct
{
    double_buffer_header_t  header;
    float                   value;
} double_buffer_scalar_t;

typedef struct
{
    double_buffer_header_t  header;
    float                   value1;
    float                   value2;
    float                   value3;
} double_buffer_vector3_t;

typedef struct
{
    double_buffer_header_t  header;
    uint8_t                 payload[DOUBLE_BUFFER_BLOB_SIZE];
} double_buffer_blob_t;

typedef union
{
    double_buffer_header_t  header;
    double_buffer_scalar_t  scalar;
    double_buffer_vector3_t vector3;
    double_buffer_blob_t    blob;
} double_buffer_slot_u;

#endif