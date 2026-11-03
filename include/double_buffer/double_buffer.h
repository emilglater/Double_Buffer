#ifndef DOUBLE_BUFFER_H
#define DOUBLE_BUFFER_H

#include <stdbool.h>
#include <pthread.h>

#include "double_buffer/config.h"

typedef void (*double_buffer_data_ready_fp)(void* p_user_data);
typedef void (*double_buffer_write_failed_fp)(void* p_user_data);

typedef enum
{
    DOUBLE_BUFFER_OK = 0,
    DOUBLE_BUFFER_NO_NEW_DATA,
    DOUBLE_BUFFER_ERROR_NULL,
    DOUBLE_BUFFER_ERROR_STATE
} double_buffer_status_e;

typedef struct
{
    double_buffer_data_ready_fp     on_data_ready;
    double_buffer_write_failed_fp   on_write_failed;
    void*                           p_user_data;
} double_buffer_callbacks_t;

typedef struct
{
    double_buffer_slot_u        slots[2];
    uint32_t                    next_sequence_number;
    uint32_t                    last_commit_time_us;
    uint32_t                    write_timeout_us;
    bool                        has_new_data;
    bool                        is_initialized;
    bool                        is_timeout_reported;
    uint8_t                     write_index;
    uint8_t                     read_index;
    uint8_t                     reserved[7];
    double_buffer_callbacks_t   callbacks;
    pthread_mutex_t             lock;
} double_buffer_t;

/**
 * @brief   Initialize a double buffer, clearing both slots.
 * 
 *          Must be called before any other function on this buffer. Resets the
 *          sequence counbter and discards any data the buffer held.
 * @param   p_buffer A pointer to a double_buffer_t.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Buffer initialized successfully.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   Mutex initialization failed.
 */
double_buffer_status_e double_buffer_init(double_buffer_t* p_buffer);

/**
 * @brief   Deinitialize a double buffer.
 * 
 *          For now it just destroys the mutex of the buffer. The buffer is left
 *          unusable until re-initialized.
 * @param   p_buffer A pointer to a double_buffer_t.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Buffer deinitialized successfully.
 * @retval  DOUBLE_BUFFER_ENRROR_NULL   p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   Mutex destruction failed.
 */
double_buffer_status_e double_buffer_deinit(double_buffer_t* p_buffer);

/**
 * @brief   Obtain the slot the producer may write into.
 * 
 *          The returned slot is private to the producer. The consumer cannot
 *          see it until double_buffer_commit() publishes it. Fill it in place,
 *          then commit.
 * @param   p_buffer A pointer to the buffer to write into.
 * @param   pp_out A pointer that will point to the writable slot.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Slot returned through pp_out successfully.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer or pp_out is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   p_buffer is not initialized.
 * @note    Producer only.
 */
double_buffer_status_e double_buffer_acquire_write(double_buffer_t* p_buffer,
    double_buffer_slot_u** pp_out);

/**
 * @brief   Publish the slot most recently acquired for writing.
 * 
 *          Embeds the record's sequence number and exchanges the roles of the
 *          two slots, so the consumer's next read returns what was just written.
 *          If the consumer has not yet read the previously commited record, that
 *          record is discarded. The producer is never blocked by consumer timing.
 *          Consumers can detect discarded records as gaps in sequence_number.
 * @param   p_buffer A pointer to the buffer to publish into.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Record published successfully.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   p_buffer is not initialized.
 * @note    Producer only.
 * 
 *          If the buffer's callbacks.on_data_ready is set, it will be invoked
 *          in this function. 
 */                                                    
double_buffer_status_e double_buffer_commit(double_buffer_t* p_buffer);

/**
 * @brief   Copy the most recently commited record out of the buffer.
 * 
 *          On success the caller owns a private copy of the record, so the
 *          producer may commit again immediately without disturbing it.
 * @param   p_buffer  A pointer to the buffer to read from.
 * @param   p_out A pointer to the copy of the record read.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Record copied successfully.
 * @retval  DOUBLE_BUFFER_NO_NEW_DATA   Nothing commited since last read.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer or p_out is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   p_buffer is not initialized.
 * @note    Consumer only.
 */
double_buffer_status_e double_buffer_read(double_buffer_t* p_buffer,
    double_buffer_slot_u* p_out);

/**
 * @brief   Set the callback function pointers.
 * 
 *          On commit, on_data_ready will be called with p_user_data as it is.
 * @param   p_buffer A pointer to the buffer to set callbacks for.
 * @param   p_callbacks A pointer to the callbacks struct that will be set.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            Callbacks set successfully.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer or p_callbacks is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   p_buffer is not initialized.
 * @warning The functions must not call back into the same buffer, as a deadlock
 *          may occur.
 */
double_buffer_status_e double_buffer_set_callbacks(double_buffer_t* p_buffer,
    const double_buffer_callbacks_t* p_callbacks);

/**
 * @brief   Abandon the slot acquired for writing without commiting it.
 * 
 *          Called by the producer when it cannot complete a record. Calls
 *          on_write_failed if defined.
 * @param   p_buffer A pointer to the buffer whose write is being aborted.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            The write was aborted.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   The buffer is not initialized.
 */
double_buffer_status_e double_buffer_abort_write(double_buffer_t* p_buffer);

/**
 * @brief   Set the time length until a timeout occurs.
 * 
 *          A timeout of 0 disables timeout checking (which is the default 
 *          state). The timeout is measured from the last successful commit, or
 *          from the initialization of the buffer.
 * @param   p_buffer A pointer to the buffer to configure.
 * @param   timeout_us The duration in microseconds, or 0 to disable.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            The timeout was set.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   The buffer is not initialized.
 */
double_buffer_status_e double_buffer_set_write_timeout_us(double_buffer_t* p_buffer,
    uint32_t timeout_us);

/**
 * @brief   Report of write failure if commit timed-out.
 * 
 *          Intended to be used periodically (e.g. by a scheduler of sorts).
 *          Does nothing if timeout is set to 0.
 * @param   p_buffer A pointer to the buffer to check.
 * @returns A value from @ref double_buffer_status_e.
 * @retval  DOUBLE_BUFFER_OK            The check completed.
 * @retval  DOUBLE_BUFFER_ERROR_NULL    p_buffer is NULL.
 * @retval  DOUBLE_BUFFER_ERROR_STATE   The buffer is not initialized.
 */
double_buffer_status_e double_buffer_check_write_timeout(double_buffer_t* p_buffer);



#endif