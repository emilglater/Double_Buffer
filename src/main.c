#include "double_buffer/double_buffer.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
/* cppcheck-suppress misra-c2012-21.10 ; POSIX nanosleep, not C date/time */
#include <time.h>

#define DEMO_LENGTH_SECONDS     (5U)
#define DEMO_PRODUCER_PERIOD_MS (10U)
#define DEMO_CONSUMER_PERIOD_MS (33U)
#define DEMO_MS_IN_SECOND       (1000U)
#define DEMO_NS_IN_MS           (1000000L)
#define DEMO_ITERATIONS         ((DEMO_LENGTH_SECONDS * DEMO_MS_IN_SECOND) / \
                                 DEMO_PRODUCER_PERIOD_MS)

typedef struct
{
    uint32_t    published_count;
    uint32_t    consumed_count;
    uint32_t    dropped_count;
    uint32_t    corrupted_count;
} demo_stats_t;

static double_buffer_t g_buffer = { 0 };
static demo_stats_t g_stats = { 0 };

static pthread_mutex_t g_signal_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_signal_cond = PTHREAD_COND_INITIALIZER;
static bool g_is_data_pending = false;
static bool g_is_stopping = false;

static void demo_sleep_ms(uint32_t milliseconds)
{
    const uint32_t whole_seconds = milliseconds / DEMO_MS_IN_SECOND;
    const uint32_t remainder_ms = milliseconds % DEMO_MS_IN_SECOND;
    struct timespec request;

    request.tv_sec = (time_t)whole_seconds;
    request.tv_nsec = (long)remainder_ms * DEMO_NS_IN_MS;

    (void)nanosleep(&request, NULL);
}

static void on_data_ready(void* p_user_data)
{
    (void)p_user_data;

    (void)pthread_mutex_lock(&g_signal_lock);
    g_is_data_pending = true;
    (void)pthread_cond_signal(&g_signal_cond);
    (void)pthread_mutex_unlock(&g_signal_lock);
}

static void* producer_thread(void* p_arg)
{
    bool is_valid = true;
    uint32_t i = 0U;

    (void)p_arg;

    for(i = 0U; i < DEMO_ITERATIONS && is_valid; i++)
    {
        double_buffer_slot_u* p_slot = NULL;

        if(double_buffer_acquire_write(&g_buffer, &p_slot) != DOUBLE_BUFFER_OK)
        {
            is_valid = false;
        }
        else
        {
            p_slot->scalar.header.kind = (uint16_t)DOUBLE_BUFFER_KIND_SCALAR;
            p_slot->scalar.value = (float)i;
    
            if(double_buffer_commit(&g_buffer) != DOUBLE_BUFFER_OK)
            {
                is_valid = false;
            }
            else
            {
                g_stats.published_count++;
                demo_sleep_ms(DEMO_PRODUCER_PERIOD_MS);
            }
        }

    }

    (void)pthread_mutex_lock(&g_signal_lock);
    g_is_stopping = true;
    (void)pthread_cond_signal(&g_signal_cond);
    (void)pthread_mutex_unlock(&g_signal_lock);

    return NULL;
}

static void* consumer_thread(void* p_arg)
{
    double_buffer_slot_u slot = { 0 };
    bool is_running = true;
    bool is_first = true;
    uint32_t last_sequence = 0U;

    (void)p_arg;

    while(is_running)
    {
        (void)pthread_mutex_lock(&g_signal_lock);
        while(!g_is_data_pending && !g_is_stopping)
        {
            (void)pthread_cond_wait(&g_signal_cond, &g_signal_lock);
        }
        g_is_data_pending = false;
        is_running = !g_is_stopping;
        (void)pthread_mutex_unlock(&g_signal_lock);

        if(double_buffer_read(&g_buffer, &slot) == DOUBLE_BUFFER_OK)
        {
            const double_buffer_kind_e kind =
                (double_buffer_kind_e)slot.header.kind;

            g_stats.consumed_count++;

            if(kind == DOUBLE_BUFFER_KIND_SCALAR)
            {
                if((uint32_t)slot.scalar.value != slot.header.sequence_number)
                {
                    g_stats.corrupted_count++;
                }
            }

            if(!is_first)
            {
                g_stats.dropped_count += slot.header.sequence_number - 
                    last_sequence - 1U;
            }
            last_sequence = slot.header.sequence_number;
            is_first = false;
        }

        demo_sleep_ms(DEMO_CONSUMER_PERIOD_MS);
    }

    return NULL;
}

int main(void)
{
    pthread_t producer;
    pthread_t consumer;
    const double_buffer_callbacks_t callbacks = { on_data_ready, NULL};

    if(double_buffer_init(&g_buffer) != DOUBLE_BUFFER_OK)
    {
        (void)printf("Buffer init failed\n");
        return 1;
    }

    if(double_buffer_set_callbacks(&g_buffer,&callbacks) != DOUBLE_BUFFER_OK)
    {
        (void)printf(("Set callbacks failed\n"));
        return 1;
    }

    if(pthread_create(&producer, NULL, producer_thread, NULL) != 0)
    {
        (void)printf(("Producer creation failed\n"));
        return 1;
    }

    if(pthread_create(&consumer, NULL, consumer_thread, NULL) != 0)
    {
        (void)printf(("Consumer creation failed\n"));
        return 1;
    }

    (void)pthread_join(producer, NULL);
    (void)pthread_join(consumer, NULL);

    (void)printf("published %u, consumed %u, dropped %u, corrupted %u\n",
        g_stats.published_count, g_stats.consumed_count,
        g_stats.dropped_count, g_stats.corrupted_count);

    (void)double_buffer_deinit(&g_buffer);

    return (g_stats.corrupted_count == 0U) ? 0 : 1;
}