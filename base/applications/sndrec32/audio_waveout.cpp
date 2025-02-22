/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_waveout.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#include "stdafx.h"
#include "audio_waveout.hpp"

_AUDIO_NAMESPACE_START_

void
audio_waveout::init_(void)
{
    ZeroMemory((LPVOID)&wave_format, sizeof(WAVEFORMATEX));
    wave_format.cbSize = sizeof(WAVEFORMATEX);
    waveout_handle = 0;
    playthread_id = 0;
    wakeup_playthread = 0;
    buf_secs = _AUDIO_DEFAULT_WAVEOUTBUFSECS;
    status = WAVEOUT_NOTREADY;
}

void
audio_waveout::alloc_buffers_mem_(unsigned int buffs, float secs)
{
    unsigned int onebuf_size = 0, tot_size = 0;

    /* Release old memory */
    if (main_buffer)
        delete[] main_buffer;

    if (wave_headers)
        delete[] wave_headers;

    /* Calcs size of the buffers */
    onebuf_size = (unsigned int)((float)aud_info.byte_rate() * secs);
    tot_size = onebuf_size * buffs;
    /* Allocs memory for the audio buffers */
    main_buffer = new BYTE[tot_size];
    /* Allocs memory for the `WAVEHDR' structures */
    wave_headers = (WAVEHDR *) new BYTE[sizeof(WAVEHDR) * buffs];
    /* Zeros memory */
    ZeroMemory(main_buffer, tot_size);
    ZeroMemory(wave_headers, sizeof(WAVEHDR) * buffs);
    /* Updates total size of the buffers */
    mb_size = tot_size;
}

void
audio_waveout::init_headers_(void)
{
    /* If there is no memory for memory or headers, simply return */
    if ((!wave_headers) || (!main_buffer))
        return;

    /* This is the size for one buffer */
    DWORD buf_sz = mb_size / buffers;
    /* This is the base address for one buffer */
    BYTE *buf_addr = main_buffer;

    ZeroMemory(wave_headers, sizeof(WAVEHDR) * buffers);

    /* Initializes headers */
    for (unsigned int i = 0; i < buffers; ++i)
    {
        /* Sets the correct base address and length for the little buffer */
        wave_headers[i].dwBufferLength = mb_size / buffers;
        wave_headers[i].lpData = (LPSTR) buf_addr;

        /* Unsets the WHDR_DONE flag */
        wave_headers[i].dwFlags &= ~WHDR_DONE;

        /* Sets the WAVEHDR user data with an unique little buffer ID# */
        wave_headers[i].dwUser = (unsigned int)i;

        /* Increments little buffer base address */
        buf_addr += buf_sz;
    }
}

void
audio_waveout::prep_headers_(void)
{
    MMRESULT err;
    bool error = false;

    /* If there is no memory for memory or headers, throw error */
    if ((!wave_headers) || (!main_buffer) || (!waveout_handle))
    {
        /* TODO: throw error! */
    }

    for (unsigned int i = 0; i < buffers; ++i)
    {
        err = waveOutPrepareHeader(waveout_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
            error = true;
    }

    if (error)
    {
        /* TODO: throw error indicating which header i-th is errorneous */
    }
}

void
audio_waveout::unprep_headers_(void)
{
    MMRESULT err;
    bool error = false;

    /* If there is no memory for memory or headers, throw error */
    if ((!wave_headers) || (!main_buffer) || (!waveout_handle))
    {
        /* TODO: throw error! */
    }

    for (unsigned int i = 0; i < buffers; ++i)
    {
        err = waveOutUnprepareHeader(waveout_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
            error = true;
    }

    if (error)
    {
        /* TODO: throw error indicating which header i-th is errorneous */
    }
}

void
audio_waveout::free_buffers_mem_(void)
{
    /* Frees memory */
    if (main_buffer)
        delete[] main_buffer;

    if (wave_headers)
        delete[] wave_headers;

    main_buffer = 0;
    wave_headers = 0;
}

void
audio_waveout::open(void)
{
    MMRESULT err;
    HANDLE playthread_handle = 0;

    /* Checkin the status of the object */
    if (status != WAVEOUT_NOTREADY)
    {
        /* TODO: throw error */
    }

    /* Creating the EVENT object that will be signaled when
       the playing thread has to wake up */
    wakeup_playthread = CreateEvent(0, FALSE, FALSE, 0);
    if (!wakeup_playthread)
    {
        status = WAVEOUT_ERR;
        /* TODO: throw error */
    }

    /* Inialize buffers for recording audio data from the wavein audio line */
    alloc_buffers_mem_(buffers, buf_secs);
    init_headers_();

    /* Sound format that will be captured by wavein */
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = aud_info.channels();
    wave_format.nSamplesPerSec = aud_info.sample_rate();
    wave_format.wBitsPerSample = aud_info.bits();
    wave_format.nBlockAlign = aud_info.block_align();
    wave_format.nAvgBytesPerSec = aud_info.byte_rate();

    /* Creating the recording thread */
    playthread_handle = CreateThread(NULL,
                                     0,
                                     audio_waveout::playing_procedure,
                                     (PVOID)this,
                                     0,
                                     &playthread_id);
    /* Checking thread handle */
    if (!playthread_handle)
    {
        /* Updating status */
        status = WAVEOUT_ERR;
        /* TODO: throw error */
    }

    /* We don't need the thread handle anymore, so we can close it from now.
       (We'll just need the thread ID for the `waveInOpen' API) */
    CloseHandle(playthread_handle);

    /* Reset the `audio_source' to the start position */
    audio_buf.set_position_start();
    /* Opens the WAVE_OUT device */
    err = waveOutOpen(&waveout_handle,
                      WAVE_MAPPER,
                      &wave_format,
                      playthread_id,
                      0,
                      CALLBACK_THREAD | WAVE_ALLOWSYNC);
    if (err != MMSYSERR_NOERROR)
    {
        MessageBox(0, _T("waveOutOpen Error"), 0, 0);
        /* TODO: throw error */
    }

    status = WAVEOUT_READY;
}

void
audio_waveout::play(void)
{
    MMRESULT err;
    unsigned int i;

    if (!main_buffer)
    {
        /* TODO; throw error, or assert */
        return;
    }

    /* If the status is PAUSED, we have to resume the audio playing */
    if (status == WAVEOUT_PAUSED)
    {
        /* Updates status */
        status = WAVEOUT_PLAYING;
        /* Tells to the driver to resume audio playing */
        waveOutRestart(waveout_handle);
        /* Wakeup playing thread */
        SetEvent(wakeup_playthread);
        return;
    } /* if status == WAVEOUT_PAUSED */

    if (status != WAVEOUT_READY)
        return;

    /* Prepares WAVEHDR structures */
    prep_headers_();
    /* Sets correct status */
    status = WAVEOUT_PLAYING;
    /* Reads the audio from the start */
    //audio_buf.set_position_start();

    /* Reads the first N bytes from the audio buffer, where N = the total
       size of all little buffers */
    audio_buf.read(main_buffer, mb_size);

    /* Wakeup the playing thread */
    SetEvent(wakeup_playthread);

    /* Sends all the little buffers to the audio driver, so it can play
       the sound data */
    for (i = 0; i < buffers; ++i)
    {
        err = waveOutWrite(waveout_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
        {
            MessageBox(0, _T("waveOutWrite Error"), 0, 0);
            /* TODO: throw error */
        }
    }
}

void
audio_waveout::pause(void)
{
    MMRESULT err;

    /* If the waveout object is not playing audio, do nothing */
    if (status == WAVEOUT_PLAYING)
    {
        /* Updating status */
        status = WAVEOUT_PAUSED;
        /* Tells to audio driver to pause audio */
        err = waveOutPause(waveout_handle);
        if (err != MMSYSERR_NOERROR)
        {
            MessageBox(0, _T("waveOutPause Error"), 0, 0);
            /* TODO: throw error */
        }
    }
}

void
audio_waveout::stop(void)
{
    MMRESULT err;

    /* Checks the current status */
    if ((status != WAVEOUT_PLAYING) &&
        (status != WAVEOUT_FLUSHING) &&
        (status != WAVEOUT_PAUSED))
    {
        /* Do nothing */
        return;
    }

    /* Sets a new status */
    status = WAVEOUT_STOP;
    /* Flushes pending audio datas */
    err = waveOutReset( waveout_handle );
    if (err != MMSYSERR_NOERROR)
    {
        MessageBox(0, _T("err WaveOutReset.\n"),_T("ERROR"), 0);
        /* TODO: throw error */
    }

    /* Sets the start position of the audio buffer */
    audio_buf.set_position_start();
    /* Cleans little buffers */
    unprep_headers_();
    init_headers_();
    /* Refreshes the status */
    status = WAVEOUT_READY;
}

void
audio_waveout::close(void)
{
    MMRESULT err;

    /* If the `wave_out' object is playing audio, or it is in paused state,
       we have to call the `stop' member function, to flush pending buffers */
    if ((status == WAVEOUT_PLAYING) || (status== WAVEOUT_PAUSED))
    {
        stop();
    }

    /* When we have flushed all pending buffers, the wave out handle can be successfully closed */
    err = waveOutClose(waveout_handle);
    if (err != MMSYSERR_NOERROR)
    {
        MessageBox(0, _T("waveOutClose Error"), 0, 0);
        /* TODO: throw error */
    }

    free_buffers_mem_();
}

DWORD WINAPI
audio_waveout::playing_procedure(LPVOID arg)
{
    MSG msg;
    WAVEHDR *phdr;
    MMRESULT err;
    audio_waveout *_this = (audio_waveout *)arg;
    unsigned int read_size;

    /* Check the arg pointer */
    if (_this == 0)
        return 0;

    /* The thread can go to sleep for now. It will be wake up only when
       there is audio data to be recorded */
    if (_this->wakeup_playthread)
        WaitForSingleObject(_this->wakeup_playthread, INFINITE);

    /* Entering main polling loop */
    while (GetMessage(&msg, 0, 0, 0))
    {
        switch (msg.message)
        {
            case MM_WOM_DONE:
                phdr = (WAVEHDR *)msg.lParam;

                /* If the status of the `wave_out' object is different
                   than playing, then the thread can go to sleep */
                if ((_this->status != WAVEOUT_PLAYING) &&
                    (_this->status != WAVEOUT_FLUSHING) &&
                    (_this->wakeup_playthread))
                {
                    WaitForSingleObject(_this->wakeup_playthread, INFINITE);
                }

                /* The playing thread doesn't have to sleep, so let's checking
                   first if the little buffer has been sent to the audio driver
                   (it has the WHDR_DONE flag). If it is, we can read new audio
                   datas from the `audio_producer' object, refill the little buffer,
                   and resend it to the driver with waveOutWrite( ) API */
                if (phdr->dwFlags & WHDR_DONE)
                {
                    if (_this->status == WAVEOUT_PLAYING)
                    {
                        /* Here the thread is still playing a sound, so it can
                           read new audio data from the `audio_producer' object */
                        read_size = _this->audio_buf.read((BYTE *)phdr->lpData,
                                                          phdr->dwBufferLength);
                    } else
                        read_size = 0;

                    /* If the `audio_producer' object, has produced some
                       audio data, so `read_size' will be > 0 */
                    if (read_size)
                    {
                        /* Adjusts the correct effectively read size */
                        phdr->dwBufferLength = read_size;

                        /* Before sending the little buffer to the driver,
                           we have to remove the `WHDR_DONE' flag, because
                           the little buffer now contain new audio data that have to be played */
                        phdr->dwFlags &= ~WHDR_DONE;

                        /* Plays the sound of the little buffer */
                        err = waveOutWrite(_this->waveout_handle,
                                           phdr,
                                           sizeof(WAVEHDR));

                        /* Checking if any error has occured */
                        if (err != MMSYSERR_NOERROR)
                        {
                            MessageBox(0, _T("waveOutWrite Error"), 0, 0);
                            /* TODO: throw error */
                        }
                    }
                    else
                    {
                        /* Here `read_size' is 0, so the `audio_producer' object,
                           doesn't have any sound data to produce anymore. So,
                           now we have to see the little buffer #ID to establishing what to do */
                        if (phdr->dwUser == 0)
                        {
                            /* Here `read_size' is 0, and the buffer user data
                               contain 0, so this is the first of N little
                               buffers that came back with `WHDR_DONE' flag;
                               this means that this is the last little buffer
                               in which we have to read data to; so we can
                               _STOP_ reading data from the `audio_producer'
                               object: doing this is accomplished just setting
                               the current status as "WAVEOUT_FLUSHING" */
                            _this->status = WAVEOUT_FLUSHING;
                        }
                        else if (phdr->dwUser == (_this->buffers - 1))
                        {
                            /* Here `read_size' and the buffer user data, that
                               contain a buffer ID#, is equal to the number of
                               the total buffers - 1. This means that this is
                               the _LAST_ little buffer that has been played by
                               the audio driver. We can STOP the `wave_out'
                               object now, or restart the sound playing, if we have a infinite loop */
                            _this->stop();

                            /* Let the thread go to sleep */
                            if (_this->audio_buf.play_finished)
                                _this->audio_buf.play_finished();

                            if (_this->wakeup_playthread)
                                WaitForSingleObject(_this->wakeup_playthread,
                                                    INFINITE);

                        } /* if (phdr->dwUser == (_this->buffers - 1)) */
                    } /* if read_size != 0 */
                } /* (phdr->dwFlags & WHDR_DONE) */
                break; /* end case */
            case MM_WOM_CLOSE:
                /* The thread can exit now */
                return 0;
                break;
            case MM_WOM_OPEN:
                /* Do nothing */
                break;
        } /* end switch(msg.message) */
    } /* end while(GetMessage(...)) */

    return 0;
}

_AUDIO_NAMESPACE_END_
