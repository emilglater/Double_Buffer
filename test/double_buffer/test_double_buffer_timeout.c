#include "double_buffer/double_buffer.h"

#include <stdint.h>

#include "unity.h"
#include "mock_time_port.h"

TEST_SOURCE_FILE("src/double_buffer/double_buffer.c")


#define TEST_TIMEOUT_US     (1000U)
#define TEST_INIT_TIME_US   (5000U)

typedef struct
{
    uint32_t    write_failed_count;
} timeout_record_t;

static double_buffer_t g_double_buffer = { 0 };
static timeout_record_t g_timeout_record = { 0 };

static void on_write_failed_mock(void* p_user_data)
{
    timeout_record_t* p_record = (timeout_record_t*)p_user_data;

    if(p_record != NULL)
    {
        p_record->write_failed_count++;
    }
}

static void register_callbacks(void)
{
    const double_buffer_callbacks_t callbacks =
    {
        .on_data_ready = NULL,
        .on_write_failed = on_write_failed_mock,
        .p_user_data = &g_timeout_record
    };
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    status = double_buffer_set_callbacks(&g_double_buffer, &callbacks);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
}

static void arm_timeout(uint32_t timeout_us)
{
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    status = double_buffer_set_write_timeout_us(&g_double_buffer, timeout_us);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
}

static void commit_at(uint32_t now_us)
{
    double_buffer_slot_u* p_slot = NULL;
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    status = double_buffer_acquire_write(&g_double_buffer, &p_slot);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);

    time_port_get_time_us_ExpectAndReturn(now_us);
    status = double_buffer_commit(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
}

static void check_at(uint32_t now_us)
{
    double_buffer_status_e status = DOUBLE_BUFFER_OK;

    time_port_get_time_us_ExpectAndReturn(now_us);
    status = double_buffer_check_write_timeout(&g_double_buffer);
    TEST_ASSERT_EQUAL_INT(DOUBLE_BUFFER_OK, status);
}

void setUp(void)
{
    time_port_get_time_us_ExpectAndReturn(TEST_INIT_TIME_US);
    (void)double_buffer_init(&g_double_buffer);

    g_timeout_record.write_failed_count = 0U;

    register_callbacks();
}

void tearDown(void)
{
    (void)double_buffer_deinit(&g_double_buffer);
}

void test_timeout_fires_once_past_the_deadline(void)
{
    arm_timeout(TEST_TIMEOUT_US);

    check_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US + 1U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_timeout_record.write_failed_count);

    check_at(TEST_INIT_TIME_US + (TEST_TIMEOUT_US * 10U));
    TEST_ASSERT_EQUAL_UINT32(1U, g_timeout_record.write_failed_count);
}

void test_timeout_does_not_fire_inside_the_deadline(void)
{
    arm_timeout(TEST_TIMEOUT_US);

    check_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US - 1U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_timeout_record.write_failed_count);
}

void test_commit_rearms_the_deadline(void)
{
    arm_timeout(TEST_TIMEOUT_US);

    commit_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US);

    check_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US + 1U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_timeout_record.write_failed_count);
}

void test_commit_clears_a_reported_timeout(void)
{
    arm_timeout(TEST_TIMEOUT_US);

    check_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US + 1U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_timeout_record.write_failed_count);

    commit_at(TEST_INIT_TIME_US + TEST_TIMEOUT_US + 2U);

    check_at(TEST_INIT_TIME_US + (TEST_TIMEOUT_US * 3U) + 3U);
    TEST_ASSERT_EQUAL_UINT32(2U, g_timeout_record.write_failed_count);
}

void test_zero_timeout_never_fires(void)
{
    arm_timeout(0U);

    check_at(TEST_INIT_TIME_US + UINT32_MAX / 2U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_timeout_record.write_failed_count);
}

void test_timeout_survives_clock_wraparound(void)
{
    arm_timeout(TEST_TIMEOUT_US);

    commit_at(UINT32_MAX - 100U);

    check_at(UINT32_MAX - 100U + 500U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_timeout_record.write_failed_count);

    check_at(UINT32_MAX - 100U + TEST_TIMEOUT_US + 1U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_timeout_record.write_failed_count);
}
