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

#include <assert.h>
#include <math.h>

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "mmreg.h"
#include "mmsystem.h"

#include "winternl.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "parser.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#define SEQUENCE_HEADER_CODE     0xB3
#define PACK_START_CODE          0xBA

#define SYSTEM_START_CODE        0xBB
#define AUDIO_ELEMENTARY_STREAM  0xC0
#define VIDEO_ELEMENTARY_STREAM  0xE0

#define MPEG_SYSTEM_HEADER 3
#define MPEG_VIDEO_HEADER 2
#define MPEG_AUDIO_HEADER 1
#define MPEG_NO_HEADER 0

#define SEEK_INTERVAL (ULONGLONG)(10 * 10000000) /* Add an entry every 10 seconds */

struct seek_entry {
    ULONGLONG bytepos;
    ULONGLONG timepos;
};

typedef struct MPEGSplitterImpl
{
    ParserImpl Parser;
    LONGLONG EndOfFile;
    LONGLONG duration;
    LONGLONG position;
    DWORD begin_offset;
    BYTE header[4];

    /* Whether we just seeked (or started playing) */
    BOOL seek;

    /* Seeking cache */
    ULONG seek_entries;
    struct seek_entry *seektable;
} MPEGSplitterImpl;

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
static const WCHAR wszVideoStream[] = {'V','i','d','e','o',0};

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
    LONGLONG duration;

    int bitrate_index, freq_index, lsf = 1, mpeg1, layer, padding, bitrate, length;

    if (!(header[0] == 0xff && ((header[1]>>5)&0x7) == 0x7 &&
       ((header[1]>>1)&0x3) != 0 && ((header[2]>>4)&0xf) != 0xf &&
       ((header[2]>>2)&0x3) != 0x3))
    {
        FIXME("Not a valid header: %02x:%02x\n", header[0], header[1]);
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
    if (!bitrate || layer != 3)
    {
        FIXME("Not a valid header: %02x:%02x:%02x:%02x\n", header[0], header[1], header[2], header[3]);
        return E_INVALIDARG;
    }


    if (layer == 3 || layer == 2)
        length = 144 * bitrate / freqs[freq_index] + padding;
    else
        length = 4 * (12 * bitrate / freqs[freq_index] + padding);

    duration = (ULONGLONG)10000000 * (ULONGLONG)(length) / (ULONGLONG)(bitrate/8);
    *plen = length;
    if (pduration)
        *pduration += duration;
    return S_OK;
}

static HRESULT FillBuffer(MPEGSplitterImpl *This, IMediaSample *pCurrentSample)
{
    Parser_OutputPin * pOutputPin = (Parser_OutputPin*)This->Parser.ppPins[1];
    LONGLONG length = 0;
    LONGLONG pos = BYTES_FROM_MEDIATIME(This->Parser.pInputPin->rtNext);
    LONGLONG time = This->position;
    HRESULT hr;
    BYTE *fbuf = NULL;
    DWORD len = IMediaSample_GetActualDataLength(pCurrentSample);

    TRACE("Source length: %u\n", len);
    IMediaSample_GetPointer(pCurrentSample, &fbuf);

    /* Find the next valid header.. it <SHOULD> be right here */
    assert(parse_header(fbuf, &length, &This->position) == S_OK);
    assert(length == len || length + 4 == len);
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

            hr = IMemAllocator_GetBuffer(pin->pAlloc, &sample, NULL, NULL, 0);

            if (rtSampleStop > pin->rtStop)
                rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(pin->rtStop), pin->cbAlign));

            IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);
            IMediaSample_SetPreroll(sample, 0);
            IMediaSample_SetDiscontinuity(sample, 0);
            IMediaSample_SetSyncPoint(sample, 1);
            pin->rtCurrent = rtSampleStart;
            pin->rtNext = rtSampleStop;

            if (SUCCEEDED(hr))
                hr = IAsyncReader_Request(pin->pReader, sample, 0);
            if (FAILED(hr))
                FIXME("o_Ox%08x\n", hr);
        }
    }
    /* If not, we're presumably at the end of file */

    TRACE("Media time : %u.%03u\n", (DWORD)(This->position/10000000), (DWORD)((This->position/10000)%1000));

    IMediaSample_SetTime(pCurrentSample, &time, &This->position);

    hr = OutputPin_SendSample(&pOutputPin->pin, pCurrentSample);

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
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    BYTE *pbSrcStream;
    DWORD cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop, tAviStart = This->position;
    Parser_OutputPin * pOutputPin;
    HRESULT hr;

    pOutputPin = (Parser_OutputPin*)This->Parser.ppPins[1];

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

    if (BYTES_FROM_MEDIATIME(tStop) >= This->EndOfFile || This->position >= This->Parser.mediaSeeking.llStop)
    {
        int i;

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
                WARN("Error sending EndOfStream to pin %d (%x)\n", i, hr);
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

    return S_FALSE;
}


static HRESULT MPEGSplitter_init_audio(MPEGSplitterImpl *This, const BYTE *header, PIN_INFO *ppiOutput, AM_MEDIA_TYPE *pamt)
{
    WAVEFORMATEX *format;
    int bitrate_index;
    int freq_index;
    int mode_ext;
    int emphasis;
    int lsf = 1;
    int mpeg1;
    int layer;
    int mode;

    ZeroMemory(pamt, sizeof(*pamt));
    ppiOutput->dir = PINDIR_OUTPUT;
    ppiOutput->pFilter = (IBaseFilter*)This;
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
                              (format->nSamplesPerSec<<lsf) + 1;
    else if (layer == 2)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              format->nSamplesPerSec + 1;
    else
        format->nBlockAlign = 4 * (format->nAvgBytesPerSec * 8 * 12 / format->nSamplesPerSec + 1);

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
    PullPin *pPin = (PullPin *)iface;
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)pPin->pin.pinInfo.pFilter;
    HRESULT hr;
    LONGLONG pos = 0; /* in bytes */
    BYTE header[10];
    int streamtype = 0;
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
        UINT length;
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 6, header + 4);
        if (FAILED(hr))
            break;
        pos += 6;
        TRACE("Found ID3 v2.%d.%d\n", header[3], header[4]);
        length  = (header[6] & 0x7F) << 21;
        length += (header[7] & 0x7F) << 14;
        length += (header[8] & 0x7F) << 7;
        length += (header[9] & 0x7F);
        TRACE("Length: %u\n", length);
        pos += length;

        /* Read the real header for the mpeg splitter */
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
        if (SUCCEEDED(hr))
            pos += 4;
        TRACE("%x:%x:%x:%x\n", header[0], header[1], header[2], header[3]);
    } while (0);

    while(SUCCEEDED(hr) && !(streamtype=MPEGSplitter_head_check(header)))
    {
        TRACE("%x:%x:%x:%x\n", header[0], header[1], header[2], header[3]);
        /* No valid header yet; shift by a byte and check again */
        memmove(header, header+1, 3);
        hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, header + 3);
    }
    if (FAILED(hr))
        return hr;
    pos -= 4;
    This->begin_offset = pos;
    memcpy(This->header, header, 4);

    This->seektable[0].bytepos = pos;
    This->seektable[0].timepos = 0;

    switch(streamtype)
    {
        case MPEG_AUDIO_HEADER:
        {
            LONGLONG duration = 0;
            DWORD last_entry = 0;

            DWORD ticks = GetTickCount();

            hr = MPEGSplitter_init_audio(This, header, &piOutput, &amt);
            if (SUCCEEDED(hr))
            {
                WAVEFORMATEX *format = (WAVEFORMATEX*)amt.pbFormat;

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
                if (amt.pbFormat)
                    CoTaskMemFree(amt.pbFormat);
                ERR("Could not create pin for MPEG audio stream (%x)\n", hr);
                break;
            }

            /* Check for idv1 tag, and remove it from stream if found */
            hr = IAsyncReader_SyncRead(pPin->pReader, This->EndOfFile-128, 3, header+4);
            if (FAILED(hr))
                break;
            if (!strncmp((char*)header+4, "TAG", 3))
                This->EndOfFile -= 128;
            This->Parser.pInputPin->rtStop = MEDIATIME_FROM_BYTES(This->EndOfFile);
            This->Parser.pInputPin->rtStart = This->Parser.pInputPin->rtCurrent = MEDIATIME_FROM_BYTES(This->begin_offset);

            /* http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm has a whole read up on audio headers */
            while (pos + 3 < This->EndOfFile)
            {
                LONGLONG length = 0;
                hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
                if (hr != S_OK)
                    break;
                while (parse_header(header, &length, &duration))
                {
                    /* No valid header yet; shift by a byte and check again */
                    memmove(header, header+1, 3);
                    hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, header + 3);
                    if (hr != S_OK || This->EndOfFile - pos < 4)
                       break;
                }
                pos += length;

                if (This->seektable && (duration / SEEK_INTERVAL) > last_entry)
                {
                    if (last_entry + 1 > duration / SEEK_INTERVAL)
                    {
                        ERR("Somehow skipped %d interval lengths instead of 1\n", (DWORD)(duration/SEEK_INTERVAL) - (last_entry + 1));
                    }
                    ++last_entry;

                    TRACE("Entry: %u\n", last_entry);
                    if (last_entry >= This->seek_entries)
                    {
                        This->seek_entries += 64;
                        This->seektable = CoTaskMemRealloc(This->seektable, (This->seek_entries)*sizeof(struct seek_entry));
                    }
                    This->seektable[last_entry].bytepos = pos;
                    This->seektable[last_entry].timepos = duration;
                }

                TRACE("Pos: %x%08x/%x%08x\n", (DWORD)(pos >> 32), (DWORD)pos, (DWORD)(This->EndOfFile>>32), (DWORD)This->EndOfFile);
            }
            hr = S_OK;
            TRACE("Duration: %d seconds\n", (DWORD)(duration / 10000000));
            TRACE("Parsing took %u ms\n", GetTickCount() - ticks);
            This->duration = duration;

            This->Parser.mediaSeeking.llCurrent = 0;
            This->Parser.mediaSeeking.llDuration = duration;
            This->Parser.mediaSeeking.llStop = duration;
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
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT MPEGSplitter_seek(IBaseFilter *iface)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    PullPin *pPin = This->Parser.pInputPin;
    LONGLONG newpos, timepos, bytepos;
    HRESULT hr = S_OK;
    BYTE header[4];

    newpos = This->Parser.mediaSeeking.llCurrent;

    if (newpos > This->duration)
    {
        WARN("Requesting position %x%08x beyond end of stream %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(This->duration>>32), (DWORD)This->duration);
        return E_INVALIDARG;
    }

    if (This->position/1000000 == newpos/1000000)
    {
        TRACE("Requesting position %x%08x same as current position %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(This->position>>32), (DWORD)This->position);
        return S_OK;
    }

    /* Position, cached */
    bytepos = This->seektable[newpos / SEEK_INTERVAL].bytepos;
    timepos = This->seektable[newpos / SEEK_INTERVAL].timepos;

    hr = IAsyncReader_SyncRead(pPin->pReader, bytepos, 4, header);
    while (bytepos + 3 < This->EndOfFile)
    {
        LONGLONG length = 0;
        hr = IAsyncReader_SyncRead(pPin->pReader, bytepos, 4, header);
        if (hr != S_OK || timepos >= newpos)
            break;

        while (parse_header(header, &length, &timepos) && bytepos + 3 < This->EndOfFile)
        {
            /* No valid header yet; shift by a byte and check again */
            memmove(header, header+1, 3);
            hr = IAsyncReader_SyncRead(pPin->pReader, ++bytepos, 1, header + 3);
            if (hr != S_OK)
                break;
         }
         bytepos += length;
         TRACE("Pos: %x%08x/%x%08x\n", (DWORD)(bytepos >> 32), (DWORD)bytepos, (DWORD)(This->EndOfFile>>32), (DWORD)This->EndOfFile);
    }

    if (SUCCEEDED(hr))
    {
        PullPin *pin = This->Parser.pInputPin;
        IPin *victim = NULL;

        TRACE("Moving sound to %08u bytes!\n", (DWORD)bytepos);

        EnterCriticalSection(&pin->thread_lock);
        IPin_BeginFlush((IPin *)pin);

        /* Make sure this is done while stopped, BeginFlush takes care of this */
        EnterCriticalSection(&This->Parser.csFilter);
        memcpy(This->header, header, 4);
        IPin_ConnectedTo(This->Parser.ppPins[1], &victim);
        if (victim)
        {
            IPin_NewSegment(victim, newpos, This->duration, pin->dRate);
            IPin_Release(victim);
        }

        pin->rtStart = pin->rtCurrent = MEDIATIME_FROM_BYTES(bytepos);
        pin->rtStop = MEDIATIME_FROM_BYTES((REFERENCE_TIME)This->EndOfFile);
        This->seek = TRUE;
        This->position = newpos;
        LeaveCriticalSection(&This->Parser.csFilter);

        TRACE("Done flushing\n");
        IPin_EndFlush((IPin *)pin);
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
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    PullPin *pin = This->Parser.pInputPin;
    HRESULT hr;
    LONGLONG length;
    IMediaSample *sample;

    TRACE("Seeking? %d\n", This->seek);
    assert(parse_header(This->header, &length, NULL) == S_OK);

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

        hr = IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);

        pin->rtCurrent = pin->rtNext;
        pin->rtNext = rtSampleStop;

        IMediaSample_SetPreroll(sample, FALSE);
        IMediaSample_SetDiscontinuity(sample, This->seek);
        IMediaSample_SetSyncPoint(sample, 1);
        This->seek = 0;

        hr = IAsyncReader_Request(pin->pReader, sample, 0);
    }
    if (FAILED(hr))
        ERR("Horsemen of the apocalypse came to bring error 0x%08x\n", hr);

    return hr;
}

static const IBaseFilterVtbl MPEGSplitter_Vtbl =
{
    Parser_QueryInterface,
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
    This->seektable = CoTaskMemAlloc(sizeof(struct seek_entry) * 64);
    if (!This->seektable)
    {
        CoTaskMemFree(This);
        return E_OUTOFMEMORY;
    }
    This->seek_entries = 64;

    hr = Parser_Create(&(This->Parser), &MPEGSplitter_Vtbl, &CLSID_MPEG1Splitter, MPEGSplitter_process_sample, MPEGSplitter_query_accept, MPEGSplitter_pre_connect, MPEGSplitter_cleanup, MPEGSplitter_disconnect, MPEGSplitter_first_request, NULL, NULL, MPEGSplitter_seek, NULL);
    if (FAILED(hr))
    {
        CoTaskMemFree(This);
        return hr;
    }
    This->seek = 1;

    /* Note: This memory is managed by the parser filter once created */
    *ppv = (LPVOID)This;

    return hr;
}
