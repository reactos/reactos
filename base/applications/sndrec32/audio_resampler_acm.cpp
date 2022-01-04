/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_resampler_acm.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#include "stdafx.h"
#include "audio_resampler_acm.hpp"

_AUDIO_NAMESPACE_START_

/* Private Functions */

void
audio_resampler_acm::init_(void)
{
    /* Zeroing structures */
    ZeroMemory(&acm_header, sizeof(ACMSTREAMHEADER));
    ZeroMemory(&wformat_src, sizeof(WAVEFORMATEX));
    ZeroMemory(&wformat_dst, sizeof(WAVEFORMATEX));

    /* Setting structures sizes */
    acm_header.cbStruct = sizeof(ACMSTREAMHEADER);
    wformat_src.cbSize = sizeof(WAVEFORMATEX);
    wformat_dst.cbSize = sizeof(WAVEFORMATEX);

    /* Setting WAVEFORMATEX structure parameters
       according to `audio_format' in/out classes */

    wformat_src.wFormatTag = WAVE_FORMAT_PCM;
    wformat_src.nSamplesPerSec = audfmt_in.sample_rate();
    wformat_src.nChannels = audfmt_in.channels();
    wformat_src.wBitsPerSample = audfmt_in.bits();
    wformat_src.nAvgBytesPerSec = audfmt_in.byte_rate();
    wformat_src.nBlockAlign = audfmt_in.block_align();

    wformat_dst.wFormatTag = WAVE_FORMAT_PCM;
    wformat_dst.nSamplesPerSec = audfmt_out.sample_rate();
    wformat_dst.nChannels = audfmt_out.channels();
    wformat_dst.wBitsPerSample = audfmt_out.bits();
    wformat_dst.nAvgBytesPerSec = audfmt_out.byte_rate();
    wformat_dst.nBlockAlign = audfmt_out.block_align();

    /* Init acm structures completed successful */
}

/* Public Functions */

void
audio_resampler_acm::open(void)
{
    MMRESULT err;

    /* Opens ACM stream */
    err = acmStreamOpen(&acm_stream,
                        0,
                        &wformat_src,
                        &wformat_dst,
                        0, 0, 0,
                        ACM_STREAMOPENF_NONREALTIME);

    if (err != MMSYSERR_NOERROR)
    {
        /* TODO: throw error */
        MessageBox(0, _T("acmOpen error: %i"), _T("ERROR"), MB_ICONERROR);
    }

    /* Calcs source buffer length */
    src_buflen = (unsigned int)((float)audfmt_in.byte_rate() * (float)buf_secs);

    /* Calcs destination source buffer length with help of ACM apis */
    err = acmStreamSize(acm_stream,
                        src_buflen,
                        &dst_buflen,
                        ACM_STREAMSIZEF_SOURCE);

    if (err != MMSYSERR_NOERROR)
    {
        /* TODO: throw error */
        MessageBox(0, _T("acmStreamSize error"), _T("ERROR"), MB_ICONERROR);
    }

    /* Initialize ACMSTREAMHEADER structure,
       and alloc memory for source and destination buffers */

    acm_header.fdwStatus = 0;
    acm_header.dwUser = 0;

    acm_header.pbSrc = (LPBYTE) new BYTE[src_buflen];
    acm_header.cbSrcLength = src_buflen;
    acm_header.cbSrcLengthUsed = 0;
    acm_header.dwSrcUser = src_buflen;

    acm_header.pbDst = (LPBYTE) new BYTE[dst_buflen];
    acm_header.cbDstLength = dst_buflen;
    acm_header.cbDstLengthUsed = 0;
    acm_header.dwDstUser = dst_buflen;

    /* Give ACMSTREAMHEADER initialized correctly to the driver */
    err = acmStreamPrepareHeader(acm_stream, &acm_header, 0L);
    if (err != MMSYSERR_NOERROR)
    {
        /* TODO: throw error */
        MessageBox(0, _T("acmStreamPrepareHeader error"), _T("ERROR"), MB_ICONERROR);
    }

    /* ACM stream successfully opened */
    stream_opened = true;
}

void
audio_resampler_acm::close(void)
{
    MMRESULT err;

    if (acm_stream)
    {
        if (acm_header.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
        {
            acm_header.cbSrcLength = src_buflen;
            acm_header.cbDstLength = dst_buflen;

            err = acmStreamUnprepareHeader(acm_stream, &acm_header, 0L);
            if (err != MMSYSERR_NOERROR)
            {
                /* Free buffer memory */
                if (acm_header.pbSrc != 0)
                {
                    delete[] acm_header.pbSrc;
                    acm_header.pbSrc = 0;
                }

                if (acm_header.pbDst != 0)
                {
                    delete[] acm_header.pbDst;
                    acm_header.pbDst = 0;
                }

                /* Re-init structures */
                init_();
                /* Updating status */
                stream_opened = false;
                /* TODO: throw error */
                MessageBox(0, _T("acmStreamUnPrepareHeader error"), _T("ERROR"), MB_ICONERROR);
            }
        }

        err = acmStreamClose(acm_stream, 0);
        acm_stream = 0;

        if (err != MMSYSERR_NOERROR)
        {
            /* Free buffer memory */
            if (acm_header.pbSrc != 0)
            {
                delete[] acm_header.pbSrc;
                acm_header.pbSrc = 0;
            }

            if (acm_header.pbDst != 0)
            {
                delete[] acm_header.pbDst;
                acm_header.pbDst = 0;
            }

            /* Re-init structures */
            init_();
            /* Updating status */
            stream_opened = false;
            /* TODO: throw error! */
            MessageBox(0, _T("acmStreamClose error"), _T("ERROR"), MB_ICONERROR);
        }

    } /* if acm_stream != 0 */

    /* Free buffer memory */
    if (acm_header.pbSrc != 0)
        delete[] acm_header.pbSrc;

    if (acm_header.pbDst != 0)
        delete[] acm_header.pbDst;

    /* Re-init structures */
    init_();
    /* Updating status */
    stream_opened = false;

    /* ACM sream successfully closed */
}

void
audio_resampler_acm::audio_receive(unsigned char *data, unsigned int size)
{
    MMRESULT err;

    /* Checking for acm stream opened */
    if (stream_opened)
    {
        /* Copy audio data from extern to internal source buffer */
        memcpy(acm_header.pbSrc, data, size);

        acm_header.cbSrcLength = size;
        acm_header.cbDstLengthUsed = 0;

        err = acmStreamConvert(acm_stream, &acm_header, ACM_STREAMCONVERTF_BLOCKALIGN);

        if (err != MMSYSERR_NOERROR)
        {
            /* TODO: throw error */
            MessageBox(0, _T("acmStreamConvert error"), _T("ERROR"), MB_ICONERROR);
        }

        /* Wait for sound conversion */
        while ((ACMSTREAMHEADER_STATUSF_DONE & acm_header.fdwStatus) == 0);

        /* Copy resampled audio, to destination buffer */
        //memcpy(pbOutputData, acm_header.pbDst, acm_header.cbDstLengthUsed);
    }
}

_AUDIO_NAMESPACE_END_
