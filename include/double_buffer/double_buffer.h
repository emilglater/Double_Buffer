#ifndef DOUBLE_BUFFER_H
#define DOUBLE_BUFFER_H

#include <stdbool.h>

#include "double_buffer/config.h"

typedef enum
{
    DOUBLE_BUFFER_OK = 0,
    DOUBLE_BUFFER_NO_NEW_DATA,
    DOUBLE_BUFFER_ERROR_NULL,
    DOUBLE_BUFFER_ERROR_STATE
} double_buffer_status_e;

typedef struct
{
    double_buffer_slot_u    slots[2];
    uint32_t                next_sequence_number;
    bool                    has_new_data;
    uint8_t                 write_index;
    uint8_t                 read_index;
    uint8_t                 reserved;
} double_buffer_t;

/**
 * @brief   Initialize a double buffer, clearing both slots.
 * @details Must be called before any other function on this buffer. Resets the
 *          sequence counbter and discards any data the buffer held.
 * @param   p_buffer A pointer to a double_buffer_t.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Buffer initialized.
 * @retval  DOUBLE_BUFFER_ENRROR_NULL   p_buffer is NULL.
 */
double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer);

/**
 * @brief   Obtain the slot the producer may write into.
 * @details The returned slot is private to the producer. The consumer cannot
 *          see it until double_buffer_commit() publishes it. Fill it in place,
 *          then commit.
 * @param   p_buffer A pointer to the buffer to write into.
 * @param   pp_out A pointer that will point to the writable slot.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Slot returned through pp_out.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer or pp_out is NULL.
 * @note    Producer only.
 */
double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
                                                    double_buffer_slot_u** pp_out);

/**
 * @brief   Publish the slot most recently acquired for writing.
 * @details Embeds the record's sequence number and exchanges the roles of the
 *          two slots, so the consumer's next read returns what was just written.
 *          If the consumer has not yet read the previously commited record, that
 *          record is discarded. The producer is never blocked by consumer timing.
 *          Consumers can detect discarded records as gaps in sequence_number.
 * @param   p_buffer A pointer to the buffer to publish into.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Record published.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @note    Producer only.
 */                                                    
double_buffer_status_e double_buffer_commit(double_buffer_t* p_buffer);

/**
 * @brief   Copy the most recently commited record out of the buffer.
 * @details On success the caller owns a private copy of the record, so the
 *          producer may commit again immediately without disturbing it.
 * @param   p_buffer  A pointer to the buffer to read from.
 * @param   p_out A pointer to the copy of the record read.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Record copied successfully.
 * @retval  DOUBLE_BUFFER_NO_NEW_DATA   Nothing commited since last read.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer or p_out is NULL.
 * @note    Consumer only.
 */
double_buffer_status_e double_buffer_read(double_buffer_t* p_buffer,
                                            double_buffer_slot_u* p_out);

#endif