/*
 * MPEG Splitter Filter
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004-2005 Christian Costa
 * Copyright 2007 Chris Robinson
 * Copyright 2008 Maarten Lankhorst
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "quartz_private.h"

#include <strmif.h>


#define SEQUENCE_HEADER_CODE     0xB3
#define PACK_START_CODE          0xBA

#define SYSTEM_START_CODE        0xBB
#define AUDIO_ELEMENTARY_STREAM  0xC0
#define VIDEO_ELEMENTARY_STREAM  0xE0

#define MPEG_SYSTEM_HEADER 3
#define MPEG_VIDEO_HEADER 2
#define MPEG_AUDIO_HEADER 1
#define MPEG_NO_HEADER 0

typedef struct MPEGSplitterImpl
{
    ParserImpl Parser;
    IAMStreamSelect IAMStreamSelect_iface;
    LONGLONG EndOfFile;
    LONGLONG position;
    DWORD begin_offset;
    BYTE header[4];

    /* Whether we just seeked (or started playing) */
    BOOL seek;
} MPEGSplitterImpl;

static inline MPEGSplitterImpl *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, MPEGSplitterImpl, Parser.filter.IBaseFilter_iface);
}

static inline MPEGSplitterImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return CONTAINING_RECORD(iface, MPEGSplitterImpl, Parser.sourceSeeking.IMediaSeeking_iface);
}

static inline MPEGSplitterImpl *impl_from_IAMStreamSelect( IAMStreamSelect *iface )
{
    return CONTAINING_RECORD(iface, MPEGSplitterImpl, IAMStreamSelect_iface);
}

static int MPEGSplitter_head_check(const BYTE *header)
{
    /* If this is a possible start code, check for a system or video header */
    if (header[0] == 0 && header[1] == 0 && header[2] == 1)
    {
        /* Check if we got a system or elementary stream start code */
        if (header[3] == PACK_START_CODE ||
            header[3] == VIDEO_ELEMENTARY_STREAM ||
            header[3] == AUDIO_ELEMENTARY_STREAM)
            return MPEG_SYSTEM_HEADER;

        /* Check for a MPEG video sequence start code */
        if (header[3] == SEQUENCE_HEADER_CODE)
            return MPEG_VIDEO_HEADER;
    }

    /* This should give a good guess if we have an MPEG audio header */
    if(header[0] == 0xff && ((header[1]>>5)&0x7) == 0x7 &&
       ((header[1]>>1)&0x3) != 0 && ((header[2]>>4)&0xf) != 0xf &&
       ((header[2]>>2)&0x3) != 0x3)
        return MPEG_AUDIO_HEADER;

    /* Nothing yet.. */
    return MPEG_NO_HEADER;
}

static const WCHAR wszAudioStream[] = {'A','u','d','i','o',0};

static const DWORD freqs[10] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000,  8000, 0 };

static const DWORD tabsel_123[2][3][16] = {
    { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
      {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
      {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

    { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
      {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
      {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

static HRESULT parse_header(BYTE *header, LONGLONG *plen, LONGLONG *pduration)
{
    int bitrate_index, freq_index, lsf = 1, mpeg1, layer, padding, bitrate, length;
    LONGLONG duration;

    if (MPEGSplitter_head_check(header) != MPEG_AUDIO_HEADER)
    {
        FIXME("Not a valid header: %02x:%02x:%02x:%02x\n", header[0], header[1], header[2], header[3]);
        return E_INVALIDARG;
    }

    mpeg1 = (header[1]>>4)&0x1;
    if (mpeg1)
        lsf = ((header[1]>>3)&0x1)^1;

    layer = 4-((header[1]>>1)&0x3);
    bitrate_index = ((header[2]>>4)&0xf);
    freq_index = ((header[2]>>2)&0x3) + (mpeg1?(lsf*3):6);
    padding = ((header[2]>>1)&0x1);

    bitrate = tabsel_123[lsf][layer-1][bitrate_index] * 1000;
    if (!bitrate)
    {
        FIXME("Not a valid header: %02x:%02x:%02x:%02x\n", header[0], header[1], header[2], header[3]);
        return E_INVALIDARG;
    }

    if (layer == 1)
        length = 4 * (12 * bitrate / freqs[freq_index] + padding);
    else if (layer == 2)
        length = 144 * bitrate / freqs[freq_index] + padding;
    else if (layer == 3)
        length = 144 * bitrate / (freqs[freq_index]<<lsf) + padding;
    else
    {
        ERR("Impossible layer %d\n", layer);
        return E_INVALIDARG;
    }

    duration = (ULONGLONG)10000000 * (ULONGLONG)(length) / (ULONGLONG)(bitrate/8);
    *plen = length;
    if (pduration)
        *pduration += duration;
    return S_OK;
}

static HRESULT FillBuffer(MPEGSplitterImpl *This, IMediaSample *pCurrentSample)
{
    Parser_OutputPin * pOutputPin = unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1]);
    LONGLONG length = 0;
    LONGLONG pos = BYTES_FROM_MEDIATIME(This->Parser.pInputPin->rtNext);
    LONGLONG time = This->position, rtstop, rtstart;
    HRESULT hr;
    BYTE *fbuf = NULL;
    DWORD len = IMediaSample_GetActualDataLength(pCurrentSample);

    TRACE("Source length: %u\n", len);
    IMediaSample_GetPointer(pCurrentSample, &fbuf);

    /* Find the next valid header.. it <SHOULD> be right here */
    hr = parse_header(fbuf, &length, &This->position);
    assert(hr == S_OK);
    IMediaSample_SetActualDataLength(pCurrentSample, length);

    /* Queue the next sample */
    if (length + 4 == len)
    {
        PullPin *pin = This->Parser.pInputPin;
        LONGLONG stop = BYTES_FROM_MEDIATIME(pin->rtStop);

        hr = S_OK;
        memcpy(This->header, fbuf + length, 4);
        while (FAILED(hr = parse_header(This->header, &length, NULL)))
        {
            memmove(This->header, This->header+1, 3);
            if (pos + 4 >= stop)
                break;
            IAsyncReader_SyncRead(pin->pReader, ++pos, 1, This->header + 3);
        }
        pin->rtNext = MEDIATIME_FROM_BYTES(pos);

        if (SUCCEEDED(hr))
        {
            /* Remove 4 for the last header, which should hopefully work */
            IMediaSample *sample = NULL;
            LONGLONG rtSampleStart = pin->rtNext - MEDIATIME_FROM_BYTES(4);
            LONGLONG rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(length + 4);

            if (rtSampleStop > pin->rtStop)
                rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(pin->rtStop), pin->cbAlign));

            hr = IMemAllocator_GetBuffer(pin->pAlloc, &sample, NULL, NULL, 0);
            if (SUCCEEDED(hr))
            {
                IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);
                IMediaSample_SetPreroll(sample, FALSE);
                IMediaSample_SetDiscontinuity(sample, FALSE);
                IMediaSample_SetSyncPoint(sample, TRUE);
                hr = IAsyncReader_Request(pin->pReader, sample, 0);
                if (SUCCEEDED(hr))
                {
                    pin->rtCurrent = rtSampleStart;
                    pin->rtNext = rtSampleStop;
                }
                else
                    IMediaSample_Release(sample);
            }
            if (FAILED(hr))
                FIXME("o_Ox%08x\n", hr);
        }
    }
    /* If not, we're presumably at the end of file */

    TRACE("Media time : %u.%03u\n", (DWORD)(This->position/10000000), (DWORD)((This->position/10000)%1000));

    if (IMediaSample_IsDiscontinuity(pCurrentSample) == S_OK) {
        IPin *victim;
        EnterCriticalSection(&This->Parser.filter.csFilter);
        pOutputPin->pin.pin.tStart = time;
        pOutputPin->pin.pin.dRate = This->Parser.sourceSeeking.dRate;
        hr = IPin_ConnectedTo(&pOutputPin->pin.pin.IPin_iface, &victim);
        if (hr == S_OK)
        {
            hr = IPin_NewSegment(victim, time, This->Parser.sourceSeeking.llStop,
                                 This->Parser.sourceSeeking.dRate);
            if (hr != S_OK)
                FIXME("NewSegment returns %08x\n", hr);
            IPin_Release(victim);
        }
        LeaveCriticalSection(&This->Parser.filter.csFilter);
        if (hr != S_OK)
            return hr;
    }
    rtstart = (double)(time - pOutputPin->pin.pin.tStart) / pOutputPin->pin.pin.dRate;
    rtstop = (double)(This->position - pOutputPin->pin.pin.tStart) / pOutputPin->pin.pin.dRate;
    IMediaSample_SetTime(pCurrentSample, &rtstart, &rtstop);
    IMediaSample_SetMediaTime(pCurrentSample, &time, &This->position);

    hr = BaseOutputPinImpl_Deliver(&pOutputPin->pin, pCurrentSample);

    if (hr != S_OK)
    {
        if (hr != S_FALSE)
            TRACE("Error sending sample (%x)\n", hr);
        else
            TRACE("S_FALSE (%d), holding\n", IMediaSample_GetActualDataLength(pCurrentSample));
    }

    return hr;
}


static HRESULT MPEGSplitter_process_sample(LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie)
{
    MPEGSplitterImpl *This = iface;
    BYTE *pbSrcStream;
    DWORD cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop, tAviStart = This->position;
    HRESULT hr;

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (SUCCEEDED(hr))
    {
        cbSrcStream = IMediaSample_GetActualDataLength(pSample);
        hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    }

    /* Flush occurring */
    if (cbSrcStream == 0)
    {
        FIXME(".. Why do I need you?\n");
        return S_OK;
    }

    /* trace removed for performance reasons */
    /* TRACE("(%p), %llu -> %llu\n", pSample, tStart, tStop); */

    /* Now, try to find a new header */
    hr = FillBuffer(This, pSample);
    if (hr != S_OK)
    {
        WARN("Failed with hres: %08x!\n", hr);

        /* Unset progression if denied! */
        if (hr == VFW_E_WRONG_STATE || hr == S_FALSE)
        {
            memcpy(This->header, pbSrcStream, 4);
            This->Parser.pInputPin->rtCurrent = tStart;
            This->position = tAviStart;
        }
    }

    if (BYTES_FROM_MEDIATIME(tStop) >= This->EndOfFile || This->position >= This->Parser.sourceSeeking.llStop)
    {
        unsigned int i;

        TRACE("End of file reached\n");

        for (i = 0; i < This->Parser.cStreams; i++)
        {
            IPin* ppin;

            hr = IPin_ConnectedTo(This->Parser.ppPins[i+1], &ppin);
            if (SUCCEEDED(hr))
            {
                hr = IPin_EndOfStream(ppin);
                IPin_Release(ppin);
            }
            if (FAILED(hr))
                WARN("Error sending EndOfStream to pin %u (%x)\n", i, hr);
        }

        /* Force the pullpin thread to stop */
        hr = S_FALSE;
    }

    return hr;
}


static HRESULT MPEGSplitter_query_accept(LPVOID iface, const AM_MEDIA_TYPE *pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream))
        return S_FALSE;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Audio))
        return S_OK;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Video))
        FIXME("MPEG-1 video streams not yet supported.\n");
    else if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1System))
        FIXME("MPEG-1 system streams not yet supported.\n");
    else if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1VideoCD))
        FIXME("MPEG-1 VideoCD streams not yet supported.\n");
    else FIXME("%s\n", debugstr_guid(&pmt->subtype));

    return S_FALSE;
}


static HRESULT MPEGSplitter_init_audio(MPEGSplitterImpl *This, const BYTE *header, PIN_INFO *ppiOutput, AM_MEDIA_TYPE *pamt)
{
    WAVEFORMATEX *format;
    int bitrate_index;
    int freq_index;
    int mode_ext;
    int emphasis;
    int padding;
    int lsf = 1;
    int mpeg1;
    int layer;
    int mode;

    ZeroMemory(pamt, sizeof(*pamt));
    ppiOutput->dir = PINDIR_OUTPUT;
    ppiOutput->pFilter = &This->Parser.filter.IBaseFilter_iface;
    wsprintfW(ppiOutput->achName, wszAudioStream);

    pamt->formattype = FORMAT_WaveFormatEx;
    pamt->majortype = MEDIATYPE_Audio;
    pamt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pamt->lSampleSize = 0;
    pamt->bFixedSizeSamples = FALSE;
    pamt->bTemporalCompression = 0;

    mpeg1 = (header[1]>>4)&0x1;
    if (mpeg1)
        lsf = ((header[1]>>3)&0x1)^1;

    layer         = 4-((header[1]>>1)&0x3);
    bitrate_index =   ((header[2]>>4)&0xf);
    padding       =   ((header[2]>>1)&0x1);
    freq_index    =   ((header[2]>>2)&0x3) + (mpeg1?(lsf*3):6);
    mode          =   ((header[3]>>6)&0x3);
    mode_ext      =   ((header[3]>>4)&0x3);
    emphasis      =   ((header[3]>>0)&0x3);

    if (!bitrate_index)
    {
        /* Set to highest bitrate so samples will fit in for sure */
        FIXME("Variable-bitrate audio not fully supported.\n");
        bitrate_index = 15;
    }

    pamt->cbFormat = ((layer==3)? sizeof(MPEGLAYER3WAVEFORMAT) :
                                  sizeof(MPEG1WAVEFORMAT));
    pamt->pbFormat = CoTaskMemAlloc(pamt->cbFormat);
    if (!pamt->pbFormat)
        return E_OUTOFMEMORY;
    ZeroMemory(pamt->pbFormat, pamt->cbFormat);
    format = (WAVEFORMATEX*)pamt->pbFormat;

    format->wFormatTag      = ((layer == 3) ? WAVE_FORMAT_MPEGLAYER3 :
                                              WAVE_FORMAT_MPEG);
    format->nChannels       = ((mode == 3) ? 1 : 2);
    format->nSamplesPerSec  = freqs[freq_index];
    format->nAvgBytesPerSec = tabsel_123[lsf][layer-1][bitrate_index] * 1000 / 8;

    if (layer == 3)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              (format->nSamplesPerSec<<lsf) + padding;
    else if (layer == 2)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              format->nSamplesPerSec + padding;
    else
        format->nBlockAlign = 4 * (format->nAvgBytesPerSec * 8 * 12 / format->nSamplesPerSec + padding);

    format->wBitsPerSample = 0;

    if (layer == 3)
    {
        MPEGLAYER3WAVEFORMAT *mp3format = (MPEGLAYER3WAVEFORMAT*)format;

        format->cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;

        mp3format->wID = MPEGLAYER3_ID_MPEG;
        mp3format->fdwFlags = MPEGLAYER3_FLAG_PADDING_ON;
        mp3format->nBlockSize = format->nBlockAlign;
        mp3format->nFramesPerBlock = 1;

        /* Beware the evil magic numbers. This struct is apparently horribly
         * under-documented, and the only references I could find had it being
         * set to this with no real explanation. It works fine though, so I'm
         * not complaining (yet).
         */
        mp3format->nCodecDelay = 1393;
    }
    else
    {
        MPEG1WAVEFORMAT *mpgformat = (MPEG1WAVEFORMAT*)format;

        format->cbSize = 22;

        mpgformat->fwHeadLayer   = ((layer == 1) ? ACM_MPEG_LAYER1 :
                                    ((layer == 2) ? ACM_MPEG_LAYER2 :
                                     ACM_MPEG_LAYER3));
        mpgformat->dwHeadBitrate = format->nAvgBytesPerSec * 8;
        mpgformat->fwHeadMode    = ((mode == 3) ? ACM_MPEG_SINGLECHANNEL :
                                    ((mode == 2) ? ACM_MPEG_DUALCHANNEL :
                                     ((mode == 1) ? ACM_MPEG_JOINTSTEREO :
                                      ACM_MPEG_STEREO)));
        mpgformat->fwHeadModeExt = ((mode == 1) ? 0x0F : (1<<mode_ext));
        mpgformat->wHeadEmphasis = emphasis + 1;
        mpgformat->fwHeadFlags   = ACM_MPEG_ID_MPEG1;
    }
    pamt->subtype.Data1 = format->wFormatTag;

    TRACE("MPEG audio stream detected:\n"
          "\tLayer %d (%#x)\n"
          "\tFrequency: %d\n"
          "\tChannels: %d (%d)\n"
          "\tBytesPerSec: %d\n",
          layer, format->wFormatTag, format->nSamplesPerSec,
          format->nChannels, mode, format->nAvgBytesPerSec);

    dump_AM_MEDIA_TYPE(pamt);

    return S_OK;
}


static HRESULT MPEGSplitter_pre_connect(IPin *iface, IPin *pConnectPin, ALLOCATOR_PROPERTIES *props)
{
    PullPin *pPin = impl_PullPin_from_IPin(iface);
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)pPin->pin.pinInfo.pFilter;
    HRESULT hr;
    LONGLONG pos = 0; /* in bytes */
    BYTE header[10];
    int streamtype;
    LONGLONG total, avail;
    AM_MEDIA_TYPE amt;
    PIN_INFO piOutput;

    IAsyncReader_Length(pPin->pReader, &total, &avail);
    This->EndOfFile = total;

    hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
    if (SUCCEEDED(hr))
        pos += 4;

    /* Skip ID3 v2 tag, if any */
    if (SUCCEEDED(hr) && !memcmp("ID3", header, 3))
    do {
        UINT length = 0;
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 6, header + 4);
        if (FAILED(hr))
            break;
        pos += 6;
        TRACE("Found ID3 v2.%d.%d\n", header[3], header[4]);
        if(header[3] <= 4 && header[4] != 0xff &&
           (header[5]&0x0f) == 0 && (header[6]&0x80) == 0 &&
           (header[7]&0x80) == 0 && (header[8]&0x80) == 0 &&
           (header[9]&0x80) == 0)
        {
            length = (header[6]<<21) | (header[7]<<14) |
                     (header[8]<< 7) | (header[9]    );
            if((header[5]&0x10))
                length += 10;
            TRACE("Length: %u\n", length);
        }
        pos += length;

        /* Read the real header for the mpeg splitter */
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
        if (SUCCEEDED(hr))
            pos += 4;
    } while (0);

    while(SUCCEEDED(hr))
    {
        TRACE("Testing header %x:%x:%x:%x\n", header[0], header[1], header[2], header[3]);

        streamtype = MPEGSplitter_head_check(header);
        if (streamtype == MPEG_AUDIO_HEADER)
        {
            LONGLONG length;
            if (parse_header(header, &length, NULL) == S_OK)
            {
                BYTE next_header[4];
                /* Ensure we have a valid header by seeking for the next frame, some bad
                 * encoded ID3v2 may have an incorrect length and we end up finding bytes
                 * like FF FE 00 28 which are nothing more than a Unicode BOM followed by
                 * ')' character from inside a ID3v2 tag. Unfortunately that sequence
                 * matches like a valid mpeg audio header.
                 */
                hr = IAsyncReader_SyncRead(pPin->pReader, pos + length - 4, 4, next_header);
                if (FAILED(hr))
                    break;
                if (parse_header(next_header, &length, NULL) == S_OK)
                    break;
                TRACE("%x:%x:%x:%x is a fake audio header, looking for next...\n",
                      header[0], header[1], header[2], header[3]);
            }
        }
        else if (streamtype) /* Video or System stream */
            break;

        /* No valid header yet; shift by a byte and check again */
        memmove(header, header+1, 3);
        hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, header + 3);
    }
    if (FAILED(hr))
        return hr;
    pos -= 4;
    This->begin_offset = pos;
    memcpy(This->header, header, 4);

    switch(streamtype)
    {
        case MPEG_AUDIO_HEADER:
        {
            LONGLONG duration = 0;
            WAVEFORMATEX *format;

            hr = MPEGSplitter_init_audio(This, header, &piOutput, &amt);
            if (SUCCEEDED(hr))
            {
                format = (WAVEFORMATEX*)amt.pbFormat;

                props->cbAlign = 1;
                props->cbPrefix = 0;
                /* Make the output buffer a multiple of the frame size */
                props->cbBuffer = 0x4000 / format->nBlockAlign *
                                 format->nBlockAlign;
                props->cBuffers = 3;
                hr = Parser_AddPin(&(This->Parser), &piOutput, props, &amt);
            }

            if (FAILED(hr))
            {
                CoTaskMemFree(amt.pbFormat);
                ERR("Could not create pin for MPEG audio stream (%x)\n", hr);
                break;
            }

            /* Check for idv1 tag, and remove it from stream if found */
            hr = IAsyncReader_SyncRead(pPin->pReader, This->EndOfFile-128, 3, header);
            if (FAILED(hr))
                break;
            if (!strncmp((char*)header, "TAG", 3))
                This->EndOfFile -= 128;
            This->Parser.pInputPin->rtStop = MEDIATIME_FROM_BYTES(This->EndOfFile);
            This->Parser.pInputPin->rtStart = This->Parser.pInputPin->rtCurrent = MEDIATIME_FROM_BYTES(This->begin_offset);

            duration = (This->EndOfFile-This->begin_offset) * 10000000 / format->nAvgBytesPerSec;
            TRACE("Duration: %d seconds\n", (DWORD)(duration / 10000000));

            This->Parser.sourceSeeking.llCurrent = 0;
            This->Parser.sourceSeeking.llDuration = duration;
            This->Parser.sourceSeeking.llStop = duration;
            break;
        }
        case MPEG_VIDEO_HEADER:
            FIXME("MPEG video processing not yet supported!\n");
            hr = E_FAIL;
            break;
        case MPEG_SYSTEM_HEADER:
            FIXME("MPEG system streams not yet supported!\n");
            hr = E_FAIL;
            break;

        default:
            break;
    }
    This->position = 0;

    return hr;
}

static HRESULT MPEGSplitter_cleanup(LPVOID iface)
{
    MPEGSplitterImpl *This = iface;

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT WINAPI MPEGSplitter_seek(IMediaSeeking *iface)
{
    MPEGSplitterImpl *This = impl_from_IMediaSeeking(iface);
    PullPin *pPin = This->Parser.pInputPin;
    LONGLONG newpos, timepos, bytepos;
    HRESULT hr = E_INVALIDARG;
    BYTE header[4];

    newpos = This->Parser.sourceSeeking.llCurrent;
    if (This->position/1000000 == newpos/1000000)
    {
        TRACE("Requesting position %x%08x same as current position %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(This->position>>32), (DWORD)This->position);
        return S_OK;
    }

    bytepos = This->begin_offset;
    timepos = 0;
    /* http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm has a whole read up on audio headers */
    while (bytepos + 3 < This->EndOfFile)
    {
        LONGLONG duration = timepos;
        LONGLONG length = 0;
        hr = IAsyncReader_SyncRead(pPin->pReader, bytepos, 4, header);
        if (hr != S_OK)
            break;
        while ((hr=parse_header(header, &length, &duration)) != S_OK &&
               bytepos + 4 < This->EndOfFile)
        {
            /* No valid header yet; shift by a byte and check again */
            memmove(header, header+1, 3);
            hr = IAsyncReader_SyncRead(pPin->pReader, ++bytepos + 3, 1, header + 3);
            if (hr != S_OK)
                break;
        }
        if (hr != S_OK || duration > newpos)
            break;
        bytepos += length;
        timepos = duration;
    }

    if (SUCCEEDED(hr))
    {
        PullPin *pin = This->Parser.pInputPin;

        TRACE("Moving sound to %08u bytes!\n", (DWORD)bytepos);

        EnterCriticalSection(&pin->thread_lock);
        IPin_BeginFlush(&pin->pin.IPin_iface);

        /* Make sure this is done while stopped, BeginFlush takes care of this */
        EnterCriticalSection(&This->Parser.filter.csFilter);
        memcpy(This->header, header, 4);

        pin->rtStart = pin->rtCurrent = MEDIATIME_FROM_BYTES(bytepos);
        pin->rtStop = MEDIATIME_FROM_BYTES((REFERENCE_TIME)This->EndOfFile);
        This->seek = TRUE;
        This->position = newpos;
        LeaveCriticalSection(&This->Parser.filter.csFilter);

        TRACE("Done flushing\n");
        IPin_EndFlush(&pin->pin.IPin_iface);
        LeaveCriticalSection(&pin->thread_lock);
    }
    return hr;
}

static HRESULT MPEGSplitter_disconnect(LPVOID iface)
{
    /* TODO: Find memory leaks etc */
    return S_OK;
}

static HRESULT MPEGSplitter_first_request(LPVOID iface)
{
    MPEGSplitterImpl *This = iface;
    PullPin *pin = This->Parser.pInputPin;
    HRESULT hr;
    LONGLONG length;
    IMediaSample *sample;

    TRACE("Seeking? %d\n", This->seek);

    hr = parse_header(This->header, &length, NULL);
    assert(hr == S_OK);

    if (pin->rtCurrent >= pin->rtStop)
    {
        /* Last sample has already been queued, request nothing more */
        FIXME("Done!\n");
        return S_OK;
    }

    hr = IMemAllocator_GetBuffer(pin->pAlloc, &sample, NULL, NULL, 0);

    pin->rtNext = pin->rtCurrent;
    if (SUCCEEDED(hr))
    {
        LONGLONG rtSampleStart = pin->rtNext;
        /* Add 4 for the next header, which should hopefully work */
        LONGLONG rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(length + 4);

        if (rtSampleStop > pin->rtStop)
            rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(pin->rtStop), pin->cbAlign));

        IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);
        IMediaSample_SetPreroll(sample, FALSE);
        IMediaSample_SetDiscontinuity(sample, TRUE);
        IMediaSample_SetSyncPoint(sample, 1);
        This->seek = FALSE;

        hr = IAsyncReader_Request(pin->pReader, sample, 0);
        if (SUCCEEDED(hr))
        {
            pin->rtCurrent = pin->rtNext;
            pin->rtNext = rtSampleStop;
        }
        else
            IMediaSample_Release(sample);
    }
    if (FAILED(hr))
        ERR("Horsemen of the apocalypse came to bring error 0x%08x\n", hr);

    return hr;
}

static HRESULT WINAPI MPEGSplitter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    MPEGSplitterImpl *This = impl_from_IBaseFilter(iface);
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if ( IsEqualIID(riid, &IID_IUnknown)
      || IsEqualIID(riid, &IID_IPersist)
      || IsEqualIID(riid, &IID_IMediaFilter)
      || IsEqualIID(riid, &IID_IBaseFilter) )
        *ppv = iface;
    else if ( IsEqualIID(riid, &IID_IAMStreamSelect) )
        *ppv = &This->IAMStreamSelect_iface;

    if (*ppv)
    {
        IBaseFilter_AddRef(iface);
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static const IBaseFilterVtbl MPEGSplitter_Vtbl =
{
    MPEGSplitter_QueryInterface,
    Parser_AddRef,
    Parser_Release,
    Parser_GetClassID,
    Parser_Stop,
    Parser_Pause,
    Parser_Run,
    Parser_GetState,
    Parser_SetSyncSource,
    Parser_GetSyncSource,
    Parser_EnumPins,
    Parser_FindPin,
    Parser_QueryFilterInfo,
    Parser_JoinFilterGraph,
    Parser_QueryVendorInfo
};

static HRESULT WINAPI AMStreamSelect_QueryInterface(IAMStreamSelect *iface, REFIID riid, void **ppv)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    return IBaseFilter_QueryInterface(&This->Parser.filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI AMStreamSelect_AddRef(IAMStreamSelect *iface)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    return IBaseFilter_AddRef(&This->Parser.filter.IBaseFilter_iface);
}

static ULONG WINAPI AMStreamSelect_Release(IAMStreamSelect *iface)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    return IBaseFilter_Release(&This->Parser.filter.IBaseFilter_iface);
}

static HRESULT WINAPI AMStreamSelect_Count(IAMStreamSelect *iface, DWORD *streams)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, streams);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMStreamSelect_Info(IAMStreamSelect *iface, LONG index, AM_MEDIA_TYPE **media_type, DWORD *flags, LCID *lcid, DWORD *group, WCHAR **name, IUnknown **object, IUnknown **unknown)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    FIXME("(%p/%p)->(%d,%p,%p,%p,%p,%p,%p,%p) stub!\n", This, iface, index, media_type, flags, lcid, group, name, object, unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMStreamSelect_Enable(IAMStreamSelect *iface, LONG index, DWORD flags)
{
    MPEGSplitterImpl *This = impl_from_IAMStreamSelect(iface);

    FIXME("(%p/%p)->(%d,%x) stub!\n", This, iface, index, flags);

    return E_NOTIMPL;
}

static const IAMStreamSelectVtbl AMStreamSelectVtbl =
{
    AMStreamSelect_QueryInterface,
    AMStreamSelect_AddRef,
    AMStreamSelect_Release,
    AMStreamSelect_Count,
    AMStreamSelect_Info,
    AMStreamSelect_Enable
};

HRESULT MPEGSplitter_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    MPEGSplitterImpl *This;
    HRESULT hr = E_FAIL;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = CoTaskMemAlloc(sizeof(MPEGSplitterImpl));
    if (!This)
        return E_OUTOFMEMORY;

    ZeroMemory(This, sizeof(MPEGSplitterImpl));
    hr = Parser_Create(&(This->Parser), &MPEGSplitter_Vtbl, &CLSID_MPEG1Splitter, MPEGSplitter_process_sample, MPEGSplitter_query_accept, MPEGSplitter_pre_connect, MPEGSplitter_cleanup, MPEGSplitter_disconnect, MPEGSplitter_first_request, NULL, NULL, MPEGSplitter_seek, NULL);
    if (FAILED(hr))
    {
        CoTaskMemFree(This);
        return hr;
    }
    This->IAMStreamSelect_iface.lpVtbl = &AMStreamSelectVtbl;
    This->seek = TRUE;

    /* Note: This memory is managed by the parser filter once created */
    *ppv = &This->Parser.filter.IBaseFilter_iface;

    return hr;
}
