#include "time_port/time_port.h"

/* cppcheck-suppress misra-c2012-21.10 ; POSIX timespec, not C date/time */
#include <time.h>

#define TIME_PORT_US_IN_SECOND  (1000000U)
#define TIME_PORT_NS_IN_US      (1000U)

uint32_t time_port_get_time_us(void)
{
    struct timespec current_time = { 0 };
    uint32_t current_time_seconds = 0U;
    uint32_t current_time_nanoseconds = 0U;
    uint32_t seconds_to_us = 0U;
    uint32_t ns_to_us = 0U;

    (void)clock_gettime(CLOCK_MONOTONIC, &current_time);
    current_time_seconds = (uint32_t)current_time.tv_sec;
    current_time_nanoseconds = (uint32_t)current_time.tv_nsec;
    
    seconds_to_us = current_time_seconds * TIME_PORT_US_IN_SECOND;
    ns_to_us = current_time_nanoseconds / TIME_PORT_NS_IN_US;

    return seconds_to_us + ns_to_us;
}