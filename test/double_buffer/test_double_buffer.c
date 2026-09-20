#include "double_buffer/double_buffer.h"

#include <stddef.h>
#include <stdint.h>

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_header_is_first_in_payload(void)
{
    TEST_ASSERT_EQUAL_size_t(0U, offsetof(double_buffer_scalar_t, header));
    TEST_ASSERT_EQUAL_size_t(0U, offsetof(double_buffer_vector3_t, header));
    TEST_ASSERT_EQUAL_size_t(0U, offsetof(double_buffer_blob_t, header));
}

void test_slot_holds_larget_payload(void)
{
    TEST_ASSERT_TRUE(sizeof(double_buffer_slot_u) >= sizeof(double_buffer_blob_t));
}

void test_kind_readble_through_any_member(void)
{
    double_buffer_slot_u slot;

    slot.vector3.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_VECTOR3;

    TEST_ASSERT_EQUAL_UINT16(DOUBLE_BUFFER_KIND_VECTOR3, slot.blob.header.kind);
    TEST_ASSERT_EQUAL_UINT16(DOUBLE_BUFFER_KIND_VECTOR3, slot.header.kind);
}