#include "double_buffer/double_buffer.h"

#include <stddef.h>
#include <stdint.h>

#include "unity.h"

TEST_SOURCE_FILE("src/double_buffer/double_buffer.c")

typedef struct
{
    uint16_t    invoke_count;
} callback_record_t;

static double_buffer_t g_double_buffer = { 0 };
static callback_record_t g_callback_record = { 0 };

static void publish_scalar(float value)
{
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    
    status = double_buffer_acquire_write(&g_double_buffer, &p_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
    
    p_slot->scalar.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_SCALAR;
    p_slot->scalar.value = value;
    
    status = double_buffer_commit(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
}

static void on_data_ready_mock(void* p_user_data)
{
    callback_record_t* p_record = (callback_record_t*)p_user_data;

    if(p_record != NULL)
    {
        p_record->invoke_count++;
    }
}

void setUp(void)
{
    (void)double_buffer_init(&g_double_buffer);
    g_callback_record.invoke_count = 0;
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

void test_slot_holds_largest_payload(void)
{
    TEST_ASSERT_TRUE(sizeof(double_buffer_slot_u) >= sizeof(double_buffer_blob_t));
}

void test_kind_readable_through_any_member(void)
{
    double_buffer_slot_u slot;

    slot.vector3.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_VECTOR3;

    TEST_ASSERT_EQUAL_UINT16(DOUBLE_BUFFER_KIND_VECTOR3, slot.blob.header.kind);
    TEST_ASSERT_EQUAL_UINT16(DOUBLE_BUFFER_KIND_VECTOR3, slot.header.kind);
}

void test_read_right_after_init_has_no_new_data(void)
{
    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    double_buffer_slot_u out = { 0 };

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_NO_NEW_DATA, status);
}

void test_acquire_write_yields_slot(void)
{
    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    double_buffer_slot_u* p_slot = NULL;

    status = double_buffer_acquire_write(&g_double_buffer, &p_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
    TEST_ASSERT_NOT_NULL(p_slot);
}

void test_committed_scalar_is_read_back(void)
{
    double_buffer_slot_u* p_write = NULL;
    double_buffer_slot_u out = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    status = double_buffer_acquire_write(&g_double_buffer, &p_write);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    p_write->scalar.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_SCALAR;
    p_write->scalar.value = 24.5F;

    status = double_buffer_commit(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_EQUAL_UINT16(DOUBLE_BUFFER_KIND_SCALAR, out.header.kind);
    TEST_ASSERT_EQUAL_FLOAT(24.5F, out.scalar.value);
}

void test_second_read_reports_no_new_data(void)
{
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_slot_u out_slot = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    (void)double_buffer_acquire_write(&g_double_buffer, &p_slot);
    (void)double_buffer_commit(&g_double_buffer);   // only the `has_new_data` member interests us

    status = double_buffer_read(&g_double_buffer, &out_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    status = double_buffer_read(&g_double_buffer, &out_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_NO_NEW_DATA, status);
}

void test_failed_read_leaves_output_untouched(void)
{
    double_buffer_status_e status = DOUBLE_BUFFER_OK;
    double_buffer_slot_u out_slot = { 0 };

    out_slot.header.kind = 0x01A4U;

    status = double_buffer_read(&g_double_buffer, &out_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_NO_NEW_DATA, status);
    TEST_ASSERT_EQUAL_UINT16(0x01A4U, out_slot.header.kind);
}

void test_sequence_number_increments_per_commit(void)
{
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_slot_u first = { 0 };
    double_buffer_slot_u second = { 0 };

    (void)double_buffer_acquire_write(&g_double_buffer, &p_slot);
    (void)double_buffer_commit(&g_double_buffer);
    (void)double_buffer_read(&g_double_buffer, &first);

    (void)double_buffer_acquire_write(&g_double_buffer, &p_slot);
    (void)double_buffer_commit(&g_double_buffer);
    (void)double_buffer_read(&g_double_buffer, &second);

    TEST_ASSERT_EQUAL_UINT32(first.header.sequence_number + 1U,
                                second.header.sequence_number);
}

void test_null_arguments_are_rejected(void)
{
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_slot_u out_slot = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    status = double_buffer_init(NULL);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
    status = double_buffer_acquire_write(NULL, &p_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
    status = double_buffer_acquire_write(&g_double_buffer, NULL);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
    status = double_buffer_commit(NULL);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
    status = double_buffer_read(NULL, &out_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
    status = double_buffer_read(&g_double_buffer, NULL);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_ERROR_NULL, status);
}

void test_index_swap(void)
{
    double_buffer_slot_u* p_first = NULL;
    double_buffer_slot_u* p_second = NULL;
    double_buffer_slot_u* p_third = NULL;
    double_buffer_status_e status = DOUBLE_BUFFER_OK;


    status = double_buffer_acquire_write(&g_double_buffer, &p_first);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    status = double_buffer_commit(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    status = double_buffer_acquire_write(&g_double_buffer, &p_second);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_NOT_EQUAL(p_first, p_second);

    status = double_buffer_commit(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    status = double_buffer_acquire_write(&g_double_buffer, &p_third);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_EQUAL(p_first, p_third);
}

void test_double_buffer_property(void)
{
    const float first_value = 24.5F;
    const float second_value = 42.9F;

    double_buffer_slot_u* p_slot = NULL;
    double_buffer_slot_u out = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    publish_scalar(first_value);

    status = double_buffer_acquire_write(&g_double_buffer, &p_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    p_slot->scalar.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_SCALAR;
    p_slot->scalar.value = second_value;

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_EQUAL_FLOAT(first_value, out.scalar.value);
    TEST_ASSERT_NOT_EQUAL_FLOAT(second_value, out.scalar.value);
    TEST_ASSERT_EQUAL_UINT32(0U, out.header.sequence_number);
}

void test_drop_oldest_record(void)
{
    const float first_value = 24.5F;
    const float second_value = 42.9F;

    double_buffer_slot_u out = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    publish_scalar(first_value);
    publish_scalar(second_value);

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_NOT_EQUAL_FLOAT(first_value, out.scalar.value);
    TEST_ASSERT_EQUAL_FLOAT(second_value, out.scalar.value);
    TEST_ASSERT_EQUAL_UINT32(1U, out.header.sequence_number);
}

void test_no_queueing_for_records(void)
{
    const float first_value = 24.5F;
    const float second_value = 42.9F;
    const float third_value = 32.1F;

    double_buffer_slot_u out = { 0 };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    publish_scalar(first_value);
    publish_scalar(second_value);
    publish_scalar(third_value);

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    TEST_ASSERT_EQUAL_FLOAT(third_value, out.scalar.value);
    TEST_ASSERT_EQUAL_UINT32(2U, out.header.sequence_number);

    status = double_buffer_read(&g_double_buffer, &out);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_NO_NEW_DATA, status);
}

void test_callback_fired_every_commit(void)
{
    (void)on_data_ready_mock(&g_callback_record);
}