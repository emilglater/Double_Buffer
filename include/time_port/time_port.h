#ifndef TIME_PORT_H
#define TIME_PORT_H

#include <stdint.h>

/**
 * @brief   Returns time from a monotonic source.
 * 
 *          Wraps about every 71 minutes.
 * @returns A value in microseconds.
 */
uint32_t time_port_get_time_us(void);

#endif