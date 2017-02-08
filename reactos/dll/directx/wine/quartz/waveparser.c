/*
 * WAVE Parser Filter
 *
 * Copyright 2005 Christian Costa
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

static const WCHAR wcsOutputPinName[] = {'o','u','t','p','u','t',' ','p','i','n',0};

typedef struct WAVEParserImpl
{
    ParserImpl Parser;
    LONGLONG StartOfFile; /* in media time */
    LONGLONG EndOfFile;
    DWORD dwSampleSize;
    DWORD nSamplesPerSec;
    DWORD dwLength;
} WAVEParserImpl;

static inline WAVEParserImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return CONTAINING_RECORD(iface, WAVEParserImpl, Parser.sourceSeeking.IMediaSeeking_iface);
}

static inline WAVEParserImpl *impl_from_IBaseFilter( IBaseFilter *iface )
{
    return CONTAINING_RECORD(iface, WAVEParserImpl, Parser.filter.IBaseFilter_iface);
}

static LONGLONG bytepos_to_duration(WAVEParserImpl *This, LONGLONG bytepos)
{
    LONGLONG duration = BYTES_FROM_MEDIATIME(bytepos - This->StartOfFile);
    duration *= 10000000;
    duration /= (This->dwSampleSize * This->nSamplesPerSec);

    return duration;
}

static LONGLONG duration_to_bytepos(WAVEParserImpl *This, LONGLONG duration)
{
    LONGLONG bytepos;

    bytepos = (This->dwSampleSize * This->nSamplesPerSec);
    bytepos *= duration;
    bytepos /= 10000000;
    bytepos += BYTES_FROM_MEDIATIME(This->StartOfFile);
    bytepos -= bytepos % This->dwSampleSize;

    return MEDIATIME_FROM_BYTES(bytepos);
}

static HRESULT WAVEParser_Sample(LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie)
{
    WAVEParserImpl *This = iface;
    LPBYTE pbSrcStream = NULL;
    ULONG cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    IMediaSample *newsample = NULL;
    Parser_OutputPin *pOutputPin;
    PullPin *pin = This->Parser.pInputPin;

    IMediaSample_GetPointer(pSample, &pbSrcStream);
    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* Flush occurring */
    if (cbSrcStream == 0)
    {
        TRACE(".. Why do I need you?\n");
        return S_OK;
    }

    pOutputPin = unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1]);

    if (SUCCEEDED(hr))
        hr = IMemAllocator_GetBuffer(pin->pAlloc, &newsample, NULL, NULL, 0);

    if (SUCCEEDED(hr))
    {
        LONGLONG rtSampleStart = pin->rtNext;
        /* Add 4 for the next header, which should hopefully work */
        LONGLONG rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(IMediaSample_GetSize(newsample));

        if (rtSampleStop > pin->rtStop)
            rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(pin->rtStop), pin->cbAlign));

        IMediaSample_SetTime(newsample, &rtSampleStart, &rtSampleStop);

        pin->rtCurrent = pin->rtNext;
        pin->rtNext = rtSampleStop;

        IMediaSample_SetPreroll(newsample, FALSE);
        IMediaSample_SetDiscontinuity(newsample, FALSE);
        IMediaSample_SetSyncPoint(newsample, TRUE);

        hr = IAsyncReader_Request(pin->pReader, newsample, 0);
    }

    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME tAviStart, tAviStop;

        IMediaSample_SetSyncPoint(pSample, TRUE);
        pOutputPin->dwSamplesProcessed++;

        tAviStart = bytepos_to_duration(This, tStart);
        tAviStop = bytepos_to_duration(This, tStart + MEDIATIME_FROM_BYTES(IMediaSample_GetActualDataLength(pSample)));

        IMediaSample_SetTime(pSample, &tAviStart, &tAviStop);

        hr = BaseOutputPinImpl_Deliver(&pOutputPin->pin, pSample);
        if (hr != S_OK && hr != S_FALSE && hr != VFW_E_WRONG_STATE)
            ERR("Error sending sample (%x)\n", hr);
        else if (hr != S_OK)
            /* Unset progression if denied! */
            This->Parser.pInputPin->rtCurrent = tStart;
    }

    if (tStop >= This->EndOfFile || (bytepos_to_duration(This, tStop) >= This->Parser.sourceSeeking.llStop) || hr == VFW_E_NOT_CONNECTED)
    {
        unsigned int i;

        TRACE("End of file reached\n");

        for (i = 0; i < This->Parser.cStreams; i++)
        {
            IPin* ppin;
            HRESULT hr;

            TRACE("Send End Of Stream to output pin %u\n", i);

            hr = IPin_ConnectedTo(This->Parser.ppPins[i+1], &ppin);
            if (SUCCEEDED(hr))
            {
                hr = IPin_EndOfStream(ppin);
                IPin_Release(ppin);
            }
            if (FAILED(hr))
            {
                ERR("%x\n", hr);
                break;
            }
        }

        /* Force the pullpin thread to stop */
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WAVEParser_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream))
	return S_FALSE;
    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_WAVE))
        return S_OK;
    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_AU) || IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_AIFF))
	FIXME("AU and AIFF files not supported yet!\n");
    return S_FALSE;
}

static HRESULT WINAPI WAVEParserImpl_seek(IMediaSeeking *iface)
{
    WAVEParserImpl *This = impl_from_IMediaSeeking(iface);
    PullPin *pPin = This->Parser.pInputPin;
    IPin *victim = NULL;
    LONGLONG newpos, curpos, endpos, bytepos;

    newpos = This->Parser.sourceSeeking.llCurrent;
    curpos = bytepos_to_duration(This, pPin->rtCurrent);
    endpos = bytepos_to_duration(This, This->EndOfFile);
    bytepos = duration_to_bytepos(This, newpos);

    if (newpos > endpos)
    {
        WARN("Requesting position %x%08x beyond end of stream %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(endpos>>32), (DWORD)endpos);
        return E_INVALIDARG;
    }

    if (curpos/1000000 == newpos/1000000)
    {
        TRACE("Requesting position %x%08x same as current position %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(curpos>>32), (DWORD)curpos);
        return S_OK;
    }

    TRACE("Moving sound to %08u bytes!\n", (DWORD)BYTES_FROM_MEDIATIME(bytepos));

    EnterCriticalSection(&pPin->thread_lock);
    IPin_BeginFlush(&pPin->pin.IPin_iface);

    /* Make sure this is done while stopped, BeginFlush takes care of this */
    EnterCriticalSection(&This->Parser.filter.csFilter);
    IPin_ConnectedTo(This->Parser.ppPins[1], &victim);
    if (victim)
    {
        IPin_NewSegment(victim, newpos, endpos, pPin->dRate);
        IPin_Release(victim);
    }

    pPin->rtStart = pPin->rtCurrent = bytepos;
    unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1])->dwSamplesProcessed = 0;
    LeaveCriticalSection(&This->Parser.filter.csFilter);

    TRACE("Done flushing\n");
    IPin_EndFlush(&pPin->pin.IPin_iface);
    LeaveCriticalSection(&pPin->thread_lock);

    return S_OK;
}

static HRESULT WAVEParser_InputPin_PreConnect(IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *props)
{
    PullPin *This = impl_PullPin_from_IPin(iface);
    HRESULT hr;
    RIFFLIST list;
    RIFFCHUNK chunk;
    LONGLONG pos = 0; /* in bytes */
    PIN_INFO piOutput;
    AM_MEDIA_TYPE amt;
    WAVEParserImpl * pWAVEParser = impl_from_IBaseFilter(This->pin.pinInfo.pFilter);
    LONGLONG length, avail;

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = &pWAVEParser->Parser.filter.IBaseFilter_iface;
    lstrcpynW(piOutput.achName, wcsOutputPinName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
    
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

   if (list.fcc != FOURCC_RIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.cb > 1 * 1024 * 1024 * 1024) /* cannot be more than 1Gb in size */
    {
        ERR("Input stream violates RIFF spec\n");
        return E_FAIL;
    }
    if (list.fccListType != mmioFOURCC('W','A','V','E'))
    {
        ERR("Input stream not an WAVE RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    pos += sizeof(chunk);
    if (chunk.fcc != mmioFOURCC('f','m','t',' '))
    {
        ERR("Expected 'fmt ' chunk, but got %.04s\n", (LPSTR)&chunk.fcc);
        return E_FAIL;
    }

    amt.majortype = MEDIATYPE_Audio;
    amt.formattype = FORMAT_WaveFormatEx;
    amt.cbFormat = chunk.cb;
    amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
    amt.pUnk = NULL;
    IAsyncReader_SyncRead(This->pReader, pos, amt.cbFormat, amt.pbFormat);
    amt.subtype = MEDIATYPE_Audio;
    amt.subtype.Data1 = ((WAVEFORMATEX*)amt.pbFormat)->wFormatTag;

    pos += chunk.cb;
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    if (chunk.fcc == mmioFOURCC('f','a','c','t'))
    {
        FIXME("'fact' chunk not supported yet\n");
	pos += sizeof(chunk) + chunk.cb;
	hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    }
    if (chunk.fcc != mmioFOURCC('d','a','t','a'))
    {
        ERR("Expected 'data' chunk, but got %.04s\n", (LPSTR)&chunk.fcc);
        return E_FAIL;
    }

    if (hr == S_OK)
    {
        pWAVEParser->StartOfFile = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFCHUNK));
        pWAVEParser->EndOfFile = MEDIATIME_FROM_BYTES(pos + chunk.cb + sizeof(RIFFCHUNK));
    }

    if (hr != S_OK)
        return E_FAIL;

    props->cbAlign = ((WAVEFORMATEX*)amt.pbFormat)->nBlockAlign;
    props->cbPrefix = 0;
    props->cbBuffer = 4096;
    props->cBuffers = 3;
    pWAVEParser->dwSampleSize = ((WAVEFORMATEX*)amt.pbFormat)->nBlockAlign;
    IAsyncReader_Length(This->pReader, &length, &avail);
    pWAVEParser->dwLength = length / (ULONGLONG)pWAVEParser->dwSampleSize;
    pWAVEParser->nSamplesPerSec = ((WAVEFORMATEX*)amt.pbFormat)->nSamplesPerSec;
    hr = Parser_AddPin(&(pWAVEParser->Parser), &piOutput, props, &amt);
    CoTaskMemFree(amt.pbFormat);

    pWAVEParser->Parser.sourceSeeking.llCurrent = 0;
    pWAVEParser->Parser.sourceSeeking.llStop = pWAVEParser->Parser.sourceSeeking.llDuration = bytepos_to_duration(pWAVEParser, pWAVEParser->EndOfFile);
    TRACE("Duration: %u seconds\n", (DWORD)(pWAVEParser->Parser.sourceSeeking.llDuration / (LONGLONG)10000000));

    This->rtStop = pWAVEParser->EndOfFile;
    This->rtStart = pWAVEParser->StartOfFile;

    TRACE("WAVE File ok\n");

    return hr;
}

static HRESULT WAVEParser_Cleanup(LPVOID iface)
{
    WAVEParserImpl *This = iface;

    TRACE("(%p)->()\n", This);

    return S_OK;
}

static HRESULT WAVEParser_first_request(LPVOID iface)
{
    WAVEParserImpl *This = iface;
    PullPin *pin = This->Parser.pInputPin;
    HRESULT hr;
    IMediaSample *sample;

    if (pin->rtCurrent >= pin->rtStop)
    {
        /* Last sample has already been queued, request nothing more */
        TRACE("Done!\n");
        return S_OK;
    }

    hr = IMemAllocator_GetBuffer(pin->pAlloc, &sample, NULL, NULL, 0);

    pin->rtNext = pin->rtCurrent;
    if (SUCCEEDED(hr))
    {
        LONGLONG rtSampleStart = pin->rtNext;
        /* Add 4 for the next header, which should hopefully work */
        LONGLONG rtSampleStop = rtSampleStart + MEDIATIME_FROM_BYTES(IMediaSample_GetSize(sample));
        Parser_OutputPin *outpin = unsafe_impl_Parser_OutputPin_from_IPin(This->Parser.ppPins[1]);

        if (rtSampleStop > pin->rtStop)
            rtSampleStop = MEDIATIME_FROM_BYTES(ALIGNUP(BYTES_FROM_MEDIATIME(pin->rtStop), pin->cbAlign));

        IMediaSample_SetTime(sample, &rtSampleStart, &rtSampleStop);

        pin->rtCurrent = pin->rtNext;
        pin->rtNext = rtSampleStop;

        IMediaSample_SetPreroll(sample, FALSE);
        if (!outpin->dwSamplesProcessed++)
            IMediaSample_SetDiscontinuity(sample, TRUE);
        else
            IMediaSample_SetDiscontinuity(sample, FALSE);

        hr = IAsyncReader_Request(pin->pReader, sample, 0);
    }
    if (FAILED(hr))
        ERR("Horsemen of the apocalypse came to bring error 0x%08x %p\n", hr, sample);

    return hr;
}

static HRESULT WAVEParser_disconnect(LPVOID iface)
{
    /* TODO: Find and plug memory leaks */
    return S_OK;
}

static const IBaseFilterVtbl WAVEParser_Vtbl =
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

HRESULT WAVEParser_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    WAVEParserImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(WAVEParserImpl));

    hr = Parser_Create(&(This->Parser), &WAVEParser_Vtbl, &CLSID_WAVEParser, WAVEParser_Sample, WAVEParser_QueryAccept, WAVEParser_InputPin_PreConnect, WAVEParser_Cleanup, WAVEParser_disconnect, WAVEParser_first_request, NULL, NULL, WAVEParserImpl_seek, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = &This->Parser.filter.IBaseFilter_iface;

    return hr;
}
