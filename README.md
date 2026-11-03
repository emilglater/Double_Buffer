# Double Buffer Implementation
Implementing an SPSC double buffer in C99. The producer fills one slot with its 
data and the consumer reads the data contained in another slot. When ready, the 
producer swaps the slots, so now it writes to the slot the consumer read from 
earlier, and the consumer reads from the slot the producer just filled.

## API
All the functions return a `double_buffer_status_e` value for uniformity.

| Function | Side | Purpose |
| --- | --- | --- |
| `double_buffer_init` | setup | Clear both slots, reset buffer members, create the lock, start the timeout clock |
| `double_buffer_deinit` | setup | Destroy the lock |
| `double_buffer_set_callbacks` | setup | Add callback functions and user data to the buffer struct |
| `double_buffer_set_write_timeout_us` | setup | Set the commit deadline in microseconds (value 0 disables timeout) |
| `double_buffer_acquire_write` | producer | Retrieve the slot the producer may fill |
| `double_buffer_commit` | producer | Publishe the filled slot, swap the slots, and notify via callback (if needed) |
| `double_buffer_abort_write` | producer | Abandon the slot acquired for writing without commiting it |
| `double_buffer_read ` | consumer | Copy the most recently commited data of the buffer |
| `double_buffer_check_write_timeout` | any | Report of write failure if commit timed-out |

The setup functions must not run concurrently with any other operation on the
same buffer.

## Workflow
The double buffer is designed to be partially-generic: in `/include/double_buffer/` there is the header `config.h` in which we define the types of 
data we plan on using in our buffer. Every type should have 
`double_buffer_header_t` as its first member to allow the conversion. Several 
types can be used by a single buffer simultaneously. 

First we create a buffer (can be local) and initialize it. Then, if we want, we configure the callback functions, user data pointer and how long until timeout.
For the producer to publish data, it first needs to acquire the writing slot,
then fill the slot with ones of the data types defined by the user, and then
commit the slot. The commit swaps the slots and now the consumer can copy the 
data published by the producer, while the producer has a new slot to fill. 

At the end of the program, or when a buffer is no longer needed, the 
deinitialization destroys the buffer's lock and updates an internal flag so it
knows it is no longer in use.

## Lintra
This implementation uses Lintra (https://github.com/rtsworks/lintra) as its
base. Cool repo by a cool guy.