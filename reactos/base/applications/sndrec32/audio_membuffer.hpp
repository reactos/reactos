/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_membuffer.hpp
 * PURPOSE:         Allocs audio buffer
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIOMEMBUFFER__H_
#define _AUDIOMEMBUFFER__H_

#include "audio_receiver.hpp"
#include "audio_format.hpp"
#include "audio_producer.hpp"

_AUDIO_NAMESPACE_START_

class audio_membuffer : public audio_receiver, public audio_producer
{
    protected:
        BYTE * audio_data;
        audio_format aud_info;
        unsigned int buf_size;
        unsigned int init_size;

        /* Protected Functions */

        /* allocs N bytes for the audio buffer */
        void alloc_mem_(unsigned int);
        /* frees memory */
        void free_mem_(void);
        /* resizes memory, and copies old audio data to new-size memory */
        void resize_mem_(unsigned int);
        /* truncates and discards unused memory. `buf_size' will be the same as `bytes_received' */
        void truncate_(void);

    public:
        void (* audio_arrival)(unsigned int);
        void (* buffer_resized)(unsigned int);

        /* Ctors */
        audio_membuffer(void) : audio_data(0),
                                aud_info(_AUDIO_DEFAULT_FORMAT),
                                buf_size(0),
                                init_size(0)
        {
            /* Allocs memory for at least 1 or some seconds of recording */
            init_size = (unsigned int)((float)aud_info.byte_rate() * _AUDIO_DEFAULT_BUFSECS);
            alloc_mem_(init_size);
        }

        audio_membuffer(audio_format aud_fmt) : audio_data(0),
                                                aud_info(aud_fmt),
                                                buf_size(0),
                                                init_size(0)
        {
            /* Allocs memory for at least 1 or some seconds of recording */
            init_size = (unsigned int)((float)aud_info.byte_rate() * _AUDIO_DEFAULT_BUFSECS);
            alloc_mem_(init_size);
        }

        audio_membuffer(audio_format aud_fmt, unsigned int seconds) : audio_data(0),
                                                                      aud_info(aud_fmt),
                                                                      buf_size(0),
                                                                      init_size(0)
        {
            /* Allocs memory for audio recording the specified number of seconds */
            init_size = aud_info.byte_rate() * seconds;
            alloc_mem_(init_size);
        }

        audio_membuffer(audio_format aud_fmt, float seconds) : audio_data(0),
                                                               aud_info(aud_fmt),
                                                               buf_size(0),
                                                               init_size(0)
        {
            /* Allocs memory for audio recording the specified number of seconds */
            init_size = (unsigned int)((float)aud_info.byte_rate() * seconds <= 0 ? 1 : seconds);
            alloc_mem_(init_size);
        }

        audio_membuffer(unsigned int bytes) : audio_data(0),
                                              aud_info(_AUDIO_DEFAULT_FORMAT),
                                              buf_size(0),
                                              init_size(0)
        {
            /* Allocs memory for the specified bytes */
            init_size = bytes;
            alloc_mem_(init_size);
        }

        /* Dtor */
        virtual ~audio_membuffer(void)
        {
            /* Frees memory and reset values */
            clear();
        }

        /* Public functions */

        /* returns the audio buffer size in bytes */
        unsigned int mem_size(void) const
        {
            return buf_size;
        }

        /* returns how many audio data has been received, in bytes */
        unsigned int bytes_recorded(void) const
        {
            return bytes_received;
        }

        /* returns the integer number of seconds that the buffer can record */
        unsigned int seconds_total(void) const
        {
            return buf_size / aud_info.byte_rate();
        }

        /* returns the integer number of seconds that the buffer can record */
        unsigned int seconds_recorded(void) const
        {
            return bytes_received / aud_info.byte_rate();
        }

        /* returns the float number of seconds that the buffer can record */
        float fseconds_total(void) const
        {
            return (float)((float) buf_size / (float)aud_info.byte_rate());
        }

        /* returns the float number of seconds that has been recorded */
        float fseconds_recorded(void) const
        {
            return (float)((float)bytes_received / (float)aud_info.byte_rate());
        }

        unsigned int total_samples(void) const
        {
            return (aud_info.samples_in_seconds(fseconds_total()));
        }

        unsigned int samples_received(void) const
        {
            return (aud_info.samples_in_bytes(bytes_received));
        }

        /* returns a pointer to the audio buffer */
        BYTE * audio_buffer(void) const
        {
            return audio_data;
        }

        /* frees memory and resets values */
        void clear(void);

        audio_format & audinfo(void)
        {
            return aud_info;
        }

        /* discard audio data, resets values, but, instead of clear() which
           frees memory, reset the memory to the initial size, ready for
           receiving "new" audio data. */
        void reset(void);

        /* truncates and discards unused memory. `buf_size' will be the same as `bytes_received' */
        void truncate(void)
        {
            truncate_();
        } /* TODO: fare truncate N bytes */

        /* if there is a buffer, discards current buffer memory and realloc
           a new memory buffer with a new size expressed in bytes. */
        void alloc_bytes(unsigned int);

        /* if there is a buffer, discards current buffer memory and realloc
           a new memory buffer with a new size expressed in seconds, integer and float. */
        void alloc_seconds(unsigned int);
        void alloc_seconds(float);

        /* resizes in bytes the current buffer, without discarding
           previously audio data received */
        void resize_bytes(unsigned int);

        /* resizes in seconds the current buffer, without discarding
           previously audio data received */
        void resize_seconds( unsigned int );
        void resize_seconds( float );

        /* Inherited Functions from `audio_receiver' */
        void audio_receive(unsigned char *, unsigned int);

        /* Inherited Functions from `audio_buffer' */
        unsigned int read(BYTE *, unsigned int);
        bool finished(void);
};

_AUDIO_NAMESPACE_END_

#endif /* _AUDIOMEMBUFFER__H_ */
