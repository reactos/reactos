/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_wavein.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#include "stdafx.h"
#include "audio_wavein.hpp"

_AUDIO_NAMESPACE_START_

void
audio_wavein::init_(void)
{
    ZeroMemory((LPVOID)&wave_format, sizeof(WAVEFORMATEX));
    wave_format.cbSize = sizeof(WAVEFORMATEX);
    wavein_handle = 0;
    recthread_id = 0;
    wakeup_recthread = 0;
    data_flushed_event = 0;
    buf_secs = _AUDIO_DEFAULT_WAVEINBUFSECS;
    status = WAVEIN_NOTREADY;
}

void
audio_wavein::alloc_buffers_mem_(unsigned int buffs, float secs)
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
    wave_headers = (WAVEHDR *)new BYTE[sizeof(WAVEHDR) * buffs];
    /* Zeros memory */
    ZeroMemory(main_buffer, tot_size);
    ZeroMemory(wave_headers, sizeof(WAVEHDR) * buffs);
    /* Updates total size of the buffers */
    mb_size = tot_size;
}

void
audio_wavein::free_buffers_mem_(void)
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
audio_wavein::init_headers_(void)
{
    /* If there is no memory for memory or headers, simply return */
    if ((!wave_headers) || (!main_buffer))
        return;

    /* This is the size for one buffer */
    DWORD buf_sz = mb_size / buffers;
    /* This is the base address for one buffer */
    BYTE *buf_addr = main_buffer;
    /* Initializes headers */
    for (unsigned int i = 0; i < buffers; ++i)
    {
        wave_headers[i].dwBufferLength = mb_size / buffers;
        wave_headers[i].lpData = (LPSTR)buf_addr;
        buf_addr += buf_sz;
    }
}

void
audio_wavein::prep_headers_(void)
{
    MMRESULT err;
    bool error = false;

    /* If there is no memory for memory or headers, throw error */
    if ((!wave_headers) || (!main_buffer) || (!wavein_handle))
    {
        /* TODO: throw error! */
    }

    for (unsigned int i = 0; i < buffers; ++i)
    {
        err = waveInPrepareHeader(wavein_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
            error = true;
    }

    if (error)
        MessageBox(0, TEXT("waveInPrepareHeader Error."), 0, 0);
}

void
audio_wavein::unprep_headers_(void)
{
    MMRESULT err;
    bool error = false;

    /* If there is no memory for memory or headers, throw error */
    if ((!wave_headers) || (!main_buffer) || (!wavein_handle))
    {
        /* TODO: throw error! */
    }

    for (unsigned int i = 0; i < buffers; ++i)
    {
        err = waveInUnprepareHeader(wavein_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
            error = true;
    }

    if (error)
        MessageBox(0, TEXT("waveInUnPrepareHeader Error."), 0, 0);
}

void
audio_wavein::add_buffers_to_driver_(void)
{
    MMRESULT err;
    bool error = false;

    /* If there is no memory for memory or headers, throw error */
    if ((!wave_headers) || (!main_buffer) || (!wavein_handle))
    {
        /* TODO: throw error! */
    }

    for (unsigned int i = 0; i < buffers; ++i)
    {
        err = waveInAddBuffer(wavein_handle, &wave_headers[i], sizeof(WAVEHDR));
        if (err != MMSYSERR_NOERROR)
            error = true;
    }

    if (error)
        MessageBox(0, TEXT("waveInAddBuffer Error."), 0, 0);
}

void
audio_wavein::close(void)
{
    /* If wavein object is already in the status NOTREADY, nothing to do */
    if (status == WAVEIN_NOTREADY)
        return;

    /* If the wavein is recording, then stop recording and close it */
    if (status == WAVEIN_RECORDING)
        stop_recording();

    /* Updating status */
    status = WAVEIN_NOTREADY;

    /* Waking up recording thread, so it can receive
       the `MM_WIM_CLOSE' message then dies */
    if (wakeup_recthread)
        SetEvent(wakeup_recthread);

    /* Closing wavein stream */
    while ((waveInClose(wavein_handle)) != MMSYSERR_NOERROR)
        Sleep(1);

    /* Release buffers memory */
    free_buffers_mem_();

    /* Re-initialize variables to the initial state */
    init_();
}

void
audio_wavein::open(void)
{
    MMRESULT err;
    HANDLE recthread_handle = 0;

    /* Checkin the status of the object */
    if (status != WAVEIN_NOTREADY)
    {
        /* TODO: throw error */
    }

    /* Creating the EVENT object that will be signaled
       when the recording thread has to wake up */
    wakeup_recthread = CreateEvent(0, FALSE, FALSE, 0);

    data_flushed_event = CreateEvent(0, FALSE, FALSE, 0);

    if ((!wakeup_recthread) || (!data_flushed_event))
    {
        status = WAVEIN_ERR;
        MessageBox(0, TEXT("Thread Error."), 0, 0);
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
    recthread_handle = CreateThread(NULL,
                                    0,
                                    audio_wavein::recording_procedure,
                                    (PVOID)this,
                                    0,
                                    &recthread_id);
    /* Checking thread handle */
    if (!recthread_handle)
    {
        /* Updating status */
        status = WAVEIN_ERR;
        MessageBox(0, TEXT("Thread Error."), 0, 0);
        /* TODO: throw error */
    }

    /* We don't need the thread handle anymore, so we can close it from now.
       (We'll just need the thread ID for the `waveInOpen' API) */
    CloseHandle(recthread_handle);

    /* Opening audio line wavein */
    err = waveInOpen(&wavein_handle,
                     0,
                     &wave_format,
                     recthread_id,
                     0,
                     CALLBACK_THREAD);

    if (err != MMSYSERR_NOERROR)
    {
        /* Updating status */
        status = WAVEIN_ERR;

        if (err == WAVERR_BADFORMAT)
            MessageBox(0, TEXT("waveInOpen Error"), 0, 0);

        /* TODO: throw error */
    }

    /* Update object status */
    status = WAVEIN_READY;

    /* Now `audio_wavein' object is ready for audio recording! */
}

void
audio_wavein::start_recording(void)
{
    MMRESULT err;
    BOOL ev;

    if ((status != WAVEIN_READY) && (status != WAVEIN_STOP))
    {
        /* TODO: throw error */
    }

    /* Updating to the recording status */
    status = WAVEIN_RECORDING;

    /* Let's prepare header of type WAVEHDR that we will pass to the driver
       with our audio informations, and buffer informations */
    prep_headers_();

    /* The waveInAddBuffer function sends an input buffer to the given waveform-audio
       input device. When the buffer is filled, the application is notified. */
    add_buffers_to_driver_();

    /* Signaling event for waking up the recorder thread */
    ev = SetEvent(wakeup_recthread);
    if (!ev)
        MessageBox(0, TEXT("Event Error."), 0, 0);

    /* Start recording */
    err = waveInStart(wavein_handle);
    if (err != MMSYSERR_NOERROR)
    {
        /* Updating status */
        status = WAVEIN_ERR;
        MessageBox(0, TEXT("waveInStart Error."), 0, 0);
        /* TODO: throw error */
    }
}

void
audio_wavein::stop_recording(void)
{
    MMRESULT err;

    if (status != WAVEIN_RECORDING)
        return;

    status = WAVEIN_FLUSHING;

    /* waveInReset will make all pending buffer as done */
    err = waveInReset(wavein_handle);
    if ( err != MMSYSERR_NOERROR )
    {
        /* TODO: throw error */
        MessageBox(0, TEXT("waveInReset Error."), 0, 0);
    }

    if (data_flushed_event)
        WaitForSingleObject(data_flushed_event, INFINITE);

    /* Stop recording */
    err = waveInStop(wavein_handle);
    if (err != MMSYSERR_NOERROR)
    {
        /* TODO: throw error */
        MessageBox(0, TEXT("waveInStop Error."), 0, 0);
    }

    /* The waveInUnprepareHeader function cleans up the preparation performed
       by the waveInPrepareHeader function */
    unprep_headers_();

    status = WAVEIN_STOP;
}

DWORD WINAPI
audio_wavein::recording_procedure(LPVOID arg)
{
    MSG msg;
    WAVEHDR *phdr;
    audio_wavein *_this = (audio_wavein *)arg;

    /* Check the arg pointer */
    if (_this == 0)
        return 0;

    /* The thread can go to sleep for now. It will be wake up only when
       there is audio data to be recorded */
    if (_this->wakeup_recthread)
        WaitForSingleObject(_this->wakeup_recthread, INFINITE);

    /* If status of the `audio_wavein' object is not ready or recording the thread can exit */
    if ((_this->status != WAVEIN_READY) && (_this->status != WAVEIN_RECORDING))
        return 0;

    /* Entering main polling loop */
    while (GetMessage(&msg, 0, 0, 0))
    {
        switch (msg.message)
        {
            case MM_WIM_DATA:
                phdr = (WAVEHDR *)msg.lParam;

                if ((_this->status == WAVEIN_RECORDING) ||
                    (_this->status == WAVEIN_FLUSHING))
                {
                    if (phdr->dwFlags & WHDR_DONE)
                    {
                        /* Flushes recorded audio data to the `audio_receiver' object */
                        _this->audio_rcvd.audio_receive((unsigned char *)phdr->lpData,
                                                        phdr->dwBytesRecorded);

                        /* Updating `audio_receiver' total bytes received
                           _AFTER_ calling `audio_receive' function */
                        _this->audio_rcvd.bytes_received += phdr->dwBytesRecorded;
                    }

                    /* If status is not flushing data, then we can re-add the buffer
                       for reusing it. Otherwise, if we are flushing pending data,
                       we cannot re-add buffer because we don't need it anymore */
                    if (_this->status != WAVEIN_FLUSHING)
                    {
                        /* Let the audio driver reuse the buffer */
                        waveInAddBuffer(_this->wavein_handle, phdr, sizeof(WAVEHDR));
                    } else {
                        /* If we are flushing pending data, we have to prepare
                           to stop recording. Set WAVEHDR flag to 0, and fires
                           the event `data_flushed_event', that will wake up
                           the main thread that is sleeping into wavein_in::stop_recording()
                           member function, waiting the last `MM_WIM_DATA' message
                           that contain pending data */

                        phdr->dwFlags = 0;
                        SetEvent(_this->data_flushed_event);

                        /* The recording is going to stop, so the recording thread can go to sleep! */
                        WaitForSingleObject(_this->wakeup_recthread, INFINITE);
                    }
                } /* if WAVEIN_RECORDING || WAVEIN_FLUSHING */
                break;

            case MM_WIM_CLOSE:
                /* The thread can exit now */
                return 0;
                break;
        } /* end switch(msg.message) */
    } /* end while(GetMessage(...)) */

    return 0;
}

_AUDIO_NAMESPACE_END_
