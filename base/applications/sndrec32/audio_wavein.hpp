/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_wavein.hpp
 * PURPOSE:         Windows MM wave in abstraction
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIOWAVEIN_H_
#define _AUDIOWAVEIN_H_

#include "audio_format.hpp"
#include "audio_receiver.hpp"

_AUDIO_NAMESPACE_START_

enum audio_wavein_status
{
    WAVEIN_NOTREADY,
    WAVEIN_READY,
    WAVEIN_RECORDING,
    WAVEIN_ERR,
    WAVEIN_STOP,
    WAVEIN_FLUSHING
};

class audio_wavein
{
    private:
        /* The new recording thread sends message to this procedure
           about open recording, close, and sound data recorded */
        static DWORD WINAPI recording_procedure(LPVOID);

        /* When this event is signaled, then the previously created
           recording thread will wake up and start recording audio
           and will pass audio data to an `audio_receiver' object. */
        HANDLE wakeup_recthread;
        HANDLE data_flushed_event;

    protected:
        /* TODO: puts these structs in private?! */

        /* Audio wavein device stuff */
        WAVEFORMATEX wave_format;
        WAVEHDR *wave_headers;
        HWAVEIN wavein_handle;

        audio_format aud_info;
        audio_receiver &audio_rcvd;

        /* Audio Recorder Thread id */
        DWORD recthread_id;

        /* Object status */
        audio_wavein_status status;

        /* How many seconds of audio can record the internal buffer before
           flushing audio data to the `audio_receiver' class? */
        float buf_secs;

        /* The temporary buffers for the audio data incoming from the wavein
           device and its size, and its total number */
        BYTE *main_buffer;
        unsigned int mb_size;
        unsigned int buffers;

        /* Protected Functions */

        /* initialize all structures and variables */
        void init_(void);

        void alloc_buffers_mem_(unsigned int, float);
        void free_buffers_mem_(void);

        void init_headers_(void);
        void prep_headers_(void);
        void unprep_headers_(void);
        void add_buffers_to_driver_(void);

    public:
        /* Ctors */
        audio_wavein(const audio_format &a_info,
                     audio_receiver &a_receiver) : wave_headers(0),
                                                   aud_info(a_info),
                                                   audio_rcvd(a_receiver),
                                                   status(WAVEIN_NOTREADY),
                                                   main_buffer(0),
                                                   mb_size(0),
                                                   buffers(_AUDIO_DEFAULT_WAVEINBUFFERS)
        {
            /* Initializing internal wavein data */
            init_();
            aud_info = a_info;
        }

        /* Dtor */
        ~audio_wavein(void)
        {
            //close(); TODO!
        }

        /* Public functions */

        void open(void);
        void close(void);

        void start_recording(void);
        void stop_recording(void);

        audio_wavein_status current_status (void) const
        {
            return status;
        }

        float buffer_secs(void) const
        {
            return buf_secs;
        }

        void buffer_secs(float bsecs)
        {
            /* Some checking */
            if (bsecs <= 0)
                return;

            /* Set seconds length for each buffer */
            buf_secs = bsecs;
        }

        unsigned int total_buffers(void) const
        {
            return buffers;
        }

        void total_buffers(unsigned int tot_bufs)
        {
            /* Some checking */
            if (tot_bufs == 0)
                return;

            /* Sets the number of total buffers */
            buffers = tot_bufs;
        }

        audio_format format(void) const
        {
            return aud_info;
        }

        BYTE *buf(void)
        {
            return main_buffer;
        }

        unsigned int bufsz(void)
        {
            return mb_size;
        }

        unsigned int samplevalue_max(void)
        {
            if (aud_info.bits() == 16)
                return (unsigned int)65535;

            else if (aud_info.bits() == 8)
                return (unsigned int)255;

            else
                return 0;
        }

        unsigned tot_samples_buf(void)
        {
            return aud_info.samples_in_bytes(mb_size);
        }

        unsigned int nsample(unsigned int nsamp)
        {
            unsigned int svalue;

            if (aud_info.bits() == 16)
                svalue = (unsigned int)abs(*((short *)(main_buffer + aud_info.bytes_in_samples(nsamp))));
            else if (aud_info.bits() == 8)
               svalue = (unsigned int)((ptrdiff_t) *(main_buffer + aud_info.bytes_in_samples(nsamp)));
            else
                svalue = 0;

            return svalue;
        }
};

_AUDIO_NAMESPACE_END_

#endif /* _AUDIOWAVEIN_H_ */
