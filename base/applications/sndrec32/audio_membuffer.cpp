/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_membuffer.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#include "stdafx.h"
#include "audio_membuffer.hpp"

_AUDIO_NAMESPACE_START_

/* Protected Functions */

void
audio_membuffer::alloc_mem_(unsigned int bytes)
{
    /* Some checking */
    if (bytes == 0)
        return;

    /* Checks previously alloc'd memory and frees it */
    if (audio_data)
        delete[] audio_data;

    /* Allocs new memory and zeros it */
    audio_data = new BYTE[bytes];
    memset(audio_data, 0, bytes * sizeof(BYTE));

    /* Sets the correct buffer size */
    buf_size = bytes;
    init_size = bytes;
}

void
audio_membuffer::free_mem_(void)
{
    if (audio_data)
        delete[] audio_data;

    buf_size = 0;
    audio_data = 0;
}

void
audio_membuffer::resize_mem_(unsigned int new_size)
{
    if (new_size == 0)
        return;

    /* The new_size, cannot be <= of the `bytes_received' member value of the
       parent class `audio_receiver'. We cannot touch received audio data,
       so we have to alloc at least bytes_received+1 bytes. But we can truncate
       unused memory, so `new_size' can be < of `buf_size' */
    if (new_size <= bytes_received)
        return;

    BYTE * new_mem;

    /* Allocs new memory and zeros it */
    new_mem = new BYTE[new_size];
    memset(new_mem, 0, new_size * sizeof(BYTE));

    if (audio_data)
    {
        /* Copies received audio data, and discard unused memory */
        memcpy(new_mem, audio_data, bytes_received);
        /* Frees old memory */
        delete[] audio_data;
        /* Commit new memory */
        audio_data = new_mem;
        buf_size = new_size;
    } else {
        audio_data = new_mem;
        buf_size = new_size;
    }

    if (buffer_resized)
        buffer_resized(new_size);
}

void
audio_membuffer::truncate_(void)
{
    /* If `buf_size' is already = to the `bytes_received' of audio data,
       then this operation is useless; simply return */
    if (bytes_received == buf_size)
        return;

    if (audio_data)
    {
        /* Allocs a new buffer */
        BYTE * newbuf = new BYTE[bytes_received];
        /* Copies audio data */
        memcpy(newbuf, audio_data, bytes_received);
        /* Frees old memory */
        delete[] audio_data;
        /* Commit the new buffer */
        audio_data = newbuf;
        buf_size = bytes_received;

        /* Buffer truncation successful. Now the buffer size is exactly big
           as much audio data was received */
    }
}

/* Public Functions */

void
audio_membuffer::clear(void)
{
    free_mem_();
    bytes_received = 0;
}

void
audio_membuffer::reset(void)
{
    /* Frees memory and reset to initial state */
    clear();
    /* Alloc memory of size specified at the constructor */
    alloc_mem_(init_size);
}

void
audio_membuffer::alloc_bytes(unsigned int bytes)
{
    alloc_mem_(bytes);
}

void
audio_membuffer::alloc_seconds(unsigned int secs)
{
    alloc_mem_(aud_info.byte_rate() * secs);
}

void
audio_membuffer::alloc_seconds(float secs)
{
    alloc_mem_((unsigned int)((float)aud_info.byte_rate() * secs));
}

void
audio_membuffer::resize_bytes(unsigned int bytes)
{
    resize_mem_(bytes);
}

void
audio_membuffer::resize_seconds(unsigned int secs)
{
    resize_mem_(aud_info.byte_rate() * secs);
}

void
audio_membuffer::resize_seconds(float secs)
{
    resize_mem_((unsigned int)((float)aud_info.byte_rate() * secs));
}

/* Inherited Functions */

void
audio_membuffer::audio_receive(unsigned char *data, unsigned int size)
{
    /* If there isn't a buffer, allocs memory for it of size*2, and copies audio data arrival */
    if ((audio_data == 0) || (buf_size == 0))
    {
        alloc_mem_(size * 2);
        memcpy(audio_data, data, size);
        return;
    }

    /* If buffer's free memory is < of `size', we have to realloc buffer memory
       of buf_size*2, while free memory is enough to contain `size' bytes.
       In this case free memory is represented by `buf_size - bytes_recorded' */
    unsigned int tot_mem = buf_size, free_mem = buf_size - bytes_received;
    if (free_mem < size)
    {
        /* Calcs new buffer size */
        /* TODO: flags for other behaviour? */
        while (free_mem < size)
        {
            tot_mem *= 2;
            free_mem = tot_mem - bytes_received;
        }

        /* Resize buffer memory */
        resize_mem_(tot_mem);
    }

    /* Now we have enough free space in the buffer, so let's copy audio data arrivals */
    memcpy(audio_data + bytes_received, data, size);

    if (audio_arrival)
        audio_arrival(aud_info.samples_in_bytes(size));
}

unsigned int
audio_membuffer::read(BYTE *out_buf, unsigned int bytes)
{
    /* Some checking */
    if (!audio_data)
        return 0;

    if (bytes_played_ >= bytes_received)
        return 0;

    unsigned int to_play = bytes_received - bytes_played_;
    unsigned int to_copy = bytes > to_play ? to_play : bytes;

    /* Copies the audio data out */
    if ((out_buf) && (to_copy) && (audio_data))
        memcpy(out_buf, audio_data + bytes_played_, to_copy);

    /* Increments the number of total bytes played (audio data gone out from
       the `audio_producer' object) */
    bytes_played_ += to_copy;

    if (audio_arrival)
        audio_arrival(aud_info.samples_in_bytes(to_copy));

    /* Returns the exact size of audio data produced */
    return to_copy;
}

bool
audio_membuffer::finished(void)
{
    if (bytes_played_ < bytes_received)
        return false;
    else
        return true;
}

_AUDIO_NAMESPACE_END_
