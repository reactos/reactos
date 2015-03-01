/*
 * ACM Wrapper
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

typedef struct ACMWrapperImpl
{
    TransformFilter tf;

    HACMSTREAM has;
    LPWAVEFORMATEX pWfIn;
    LPWAVEFORMATEX pWfOut;

    LONGLONG lasttime_real;
    LONGLONG lasttime_sent;
} ACMWrapperImpl;

static const IBaseFilterVtbl ACMWrapper_Vtbl;

static inline ACMWrapperImpl *impl_from_TransformFilter( TransformFilter *iface )
{
    return CONTAINING_RECORD(iface, ACMWrapperImpl, tf.filter);
}

static HRESULT WINAPI ACMWrapper_Receive(TransformFilter *tf, IMediaSample *pSample)
{
    ACMWrapperImpl* This = impl_from_TransformFilter(tf);
    AM_MEDIA_TYPE amt;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream, cbSrcStream;
    LPBYTE pbDstStream;
    LPBYTE pbSrcStream = NULL;
    ACMSTREAMHEADER ash;
    BOOL unprepare_header = FALSE, preroll;
    MMRESULT res;
    HRESULT hr;
    LONGLONG tStart = -1, tStop = -1, tMed;
    LONGLONG mtStart = -1, mtStop = -1, mtMed;

    EnterCriticalSection(&This->tf.csReceive);
    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        LeaveCriticalSection(&This->tf.csReceive);
        return hr;
    }

    preroll = (IMediaSample_IsPreroll(pSample) == S_OK);

    IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (IMediaSample_GetMediaTime(pSample, &mtStart, &mtStop) != S_OK)
        mtStart = mtStop = -1;
    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* Prevent discontinuities when codecs 'absorb' data but not give anything back in return */
    if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
    {
        This->lasttime_real = tStart;
        This->lasttime_sent = tStart;
    }
    else if (This->lasttime_real == tStart)
        tStart = This->lasttime_sent;
    else
        WARN("Discontinuity\n");

    tMed = tStart;
    mtMed = mtStart;

    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr))
    {
        ERR("Unable to retrieve media type\n");
        LeaveCriticalSection(&This->tf.csReceive);
        return hr;
    }

    ash.pbSrc = pbSrcStream;
    ash.cbSrcLength = cbSrcStream;

    while(hr == S_OK && ash.cbSrcLength)
    {
        hr = BaseOutputPinImpl_GetDeliveryBuffer((BaseOutputPin*)This->tf.ppPins[1], &pOutSample, NULL, NULL, 0);
        if (FAILED(hr))
        {
            ERR("Unable to get delivery buffer (%x)\n", hr);
            LeaveCriticalSection(&This->tf.csReceive);
            return hr;
        }
        IMediaSample_SetPreroll(pOutSample, preroll);

	hr = IMediaSample_SetActualDataLength(pOutSample, 0);
	assert(hr == S_OK);

	hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
	if (FAILED(hr)) {
	    ERR("Unable to get pointer to buffer (%x)\n", hr);
	    goto error;
	}
	cbDstStream = IMediaSample_GetSize(pOutSample);

	ash.cbStruct = sizeof(ash);
	ash.fdwStatus = 0;
	ash.dwUser = 0;
	ash.pbDst = pbDstStream;
	ash.cbDstLength = cbDstStream;

	if ((res = acmStreamPrepareHeader(This->has, &ash, 0))) {
	    ERR("Cannot prepare header %d\n", res);
	    goto error;
	}
	unprepare_header = TRUE;

        if (IMediaSample_IsDiscontinuity(pSample) == S_OK)
        {
            res = acmStreamConvert(This->has, &ash, ACM_STREAMCONVERTF_START);
            IMediaSample_SetDiscontinuity(pOutSample, TRUE);
            /* One sample could be converted to multiple packets */
            IMediaSample_SetDiscontinuity(pSample, FALSE);
        }
        else
        {
            res = acmStreamConvert(This->has, &ash, 0);
            IMediaSample_SetDiscontinuity(pOutSample, FALSE);
        }

        if (res)
        {
            if(res != MMSYSERR_MOREDATA)
                ERR("Cannot convert data header %d\n", res);
            goto error;
        }

        TRACE("used in %u/%u, used out %u/%u\n", ash.cbSrcLengthUsed, ash.cbSrcLength, ash.cbDstLengthUsed, ash.cbDstLength);

        hr = IMediaSample_SetActualDataLength(pOutSample, ash.cbDstLengthUsed);
        assert(hr == S_OK);

        /* Bug in acm codecs? It apparently uses the input, but doesn't necessarily output immediately */
        if (!ash.cbSrcLengthUsed)
        {
            WARN("Sample was skipped? Outputted: %u\n", ash.cbDstLengthUsed);
            ash.cbSrcLength = 0;
            goto error;
        }

        TRACE("Sample start time: %u.%03u\n", (DWORD)(tStart/10000000), (DWORD)((tStart/10000)%1000));
        if (ash.cbSrcLengthUsed == cbSrcStream)
        {
            IMediaSample_SetTime(pOutSample, &tStart, &tStop);
            tStart = tMed = tStop;
        }
        else if (tStop != tStart)
        {
            tMed = tStop - tStart;
            tMed = tStart + tMed * ash.cbSrcLengthUsed / cbSrcStream;
            IMediaSample_SetTime(pOutSample, &tStart, &tMed);
            tStart = tMed;
        }
        else
        {
            ERR("No valid timestamp found\n");
            IMediaSample_SetTime(pOutSample, NULL, NULL);
        }

        if (mtStart < 0) {
            IMediaSample_SetMediaTime(pOutSample, NULL, NULL);
        } else if (ash.cbSrcLengthUsed == cbSrcStream) {
            IMediaSample_SetMediaTime(pOutSample, &mtStart, &mtStop);
            mtStart = mtMed = mtStop;
        } else if (mtStop >= mtStart) {
            mtMed = mtStop - mtStart;
            mtMed = mtStart + mtMed * ash.cbSrcLengthUsed / cbSrcStream;
            IMediaSample_SetMediaTime(pOutSample, &mtStart, &mtMed);
            mtStart = mtMed;
        } else {
            IMediaSample_SetMediaTime(pOutSample, NULL, NULL);
        }

        TRACE("Sample stop time: %u.%03u\n", (DWORD)(tStart/10000000), (DWORD)((tStart/10000)%1000));

        LeaveCriticalSection(&This->tf.csReceive);
        hr = BaseOutputPinImpl_Deliver((BaseOutputPin*)This->tf.ppPins[1], pOutSample);
        EnterCriticalSection(&This->tf.csReceive);

        if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
            if (FAILED(hr))
                ERR("Error sending sample (%x)\n", hr);
            goto error;
        }

error:
        if (unprepare_header && (res = acmStreamUnprepareHeader(This->has, &ash, 0)))
            ERR("Cannot unprepare header %d\n", res);
        unprepare_header = FALSE;
        ash.pbSrc += ash.cbSrcLengthUsed;
        ash.cbSrcLength -= ash.cbSrcLengthUsed;

        IMediaSample_Release(pOutSample);
        pOutSample = NULL;

    }

    This->lasttime_real = tStop;
    This->lasttime_sent = tMed;

    LeaveCriticalSection(&This->tf.csReceive);
    return hr;
}

static HRESULT WINAPI ACMWrapper_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir, const AM_MEDIA_TYPE * pmt)
{
    ACMWrapperImpl* This = impl_from_TransformFilter(tf);
    MMRESULT res;

    TRACE("(%p)->(%i %p)\n", This, dir, pmt);

    if (dir != PINDIR_INPUT)
        return S_OK;

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Audio)+4, sizeof(GUID)-4)) &&
        (IsEqualIID(&pmt->formattype, &FORMAT_WaveFormatEx)))
    {
        HACMSTREAM drv;
        WAVEFORMATEX *wfx = (WAVEFORMATEX*)pmt->pbFormat;
        AM_MEDIA_TYPE* outpmt = &This->tf.pmt;

        if (!wfx || wfx->wFormatTag == WAVE_FORMAT_PCM || wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            return VFW_E_TYPE_NOT_ACCEPTED;
        FreeMediaType(outpmt);

        This->pWfIn = (LPWAVEFORMATEX)pmt->pbFormat;

	/* HACK */
	/* TRACE("ALIGN = %d\n", pACMWrapper->pWfIn->nBlockAlign); */
	/* pACMWrapper->pWfIn->nBlockAlign = 1; */

	/* Set output audio data to PCM */
        CopyMediaType(outpmt, pmt);
        outpmt->subtype.Data1 = WAVE_FORMAT_PCM;
	This->pWfOut = (WAVEFORMATEX*)outpmt->pbFormat;
	This->pWfOut->wFormatTag = WAVE_FORMAT_PCM;
	This->pWfOut->wBitsPerSample = 16;
	This->pWfOut->nBlockAlign = This->pWfOut->wBitsPerSample * This->pWfOut->nChannels / 8;
	This->pWfOut->cbSize = 0;
	This->pWfOut->nAvgBytesPerSec = This->pWfOut->nChannels * This->pWfOut->nSamplesPerSec
						* (This->pWfOut->wBitsPerSample/8);

        if (!(res = acmStreamOpen(&drv, NULL, This->pWfIn, This->pWfOut, NULL, 0, 0, 0)))
        {
            This->has = drv;

            TRACE("Connection accepted\n");
            return S_OK;
        }
	else
	    FIXME("acmStreamOpen returned %d\n", res);
        FreeMediaType(outpmt);
        TRACE("Unable to find a suitable ACM decompressor\n");
    }

    TRACE("Connection refused\n");
    return VFW_E_TYPE_NOT_ACCEPTED;
}

static HRESULT WINAPI ACMWrapper_CompleteConnect(TransformFilter *tf, PIN_DIRECTION dir, IPin *pin)
{
    ACMWrapperImpl* This = impl_from_TransformFilter(tf);
    MMRESULT res;
    HACMSTREAM drv;

    TRACE("(%p)\n", This);

    if (dir != PINDIR_INPUT)
        return S_OK;

    if (!(res = acmStreamOpen(&drv, NULL, This->pWfIn, This->pWfOut, NULL, 0, 0, 0)))
    {
        This->has = drv;

        TRACE("Connection accepted\n");
        return S_OK;
    }

    FIXME("acmStreamOpen returned %d\n", res);
    TRACE("Unable to find a suitable ACM decompressor\n");
    return VFW_E_TYPE_NOT_ACCEPTED;
}

static HRESULT WINAPI ACMWrapper_BreakConnect(TransformFilter *tf, PIN_DIRECTION dir)
{
    ACMWrapperImpl *This = impl_from_TransformFilter(tf);

    TRACE("(%p)->(%i)\n", This,dir);

    if (dir == PINDIR_INPUT)
    {
        if (This->has)
            acmStreamClose(This->has, 0);

        This->has = 0;
        This->lasttime_real = This->lasttime_sent = -1;
    }

    return S_OK;
}

static HRESULT WINAPI ACMWrapper_DecideBufferSize(TransformFilter *tf, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    ACMWrapperImpl *pACM = impl_from_TransformFilter(tf);
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < pACM->pWfOut->nAvgBytesPerSec / 2)
            ppropInputRequest->cbBuffer = pACM->pWfOut->nAvgBytesPerSec / 2;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const TransformFilterFuncTable ACMWrapper_FuncsTable = {
    ACMWrapper_DecideBufferSize,
    NULL,
    ACMWrapper_Receive,
    NULL,
    NULL,
    ACMWrapper_SetMediaType,
    ACMWrapper_CompleteConnect,
    ACMWrapper_BreakConnect,
    NULL,
    NULL,
    NULL,
    NULL
};

HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    ACMWrapperImpl* This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hr = TransformFilter_Construct(&ACMWrapper_Vtbl, sizeof(ACMWrapperImpl), &CLSID_ACMWrapper, &ACMWrapper_FuncsTable, (IBaseFilter**)&This);

    if (FAILED(hr))
        return hr;

    *ppv = This;
    This->lasttime_real = This->lasttime_sent = -1;

    return hr;
}

static const IBaseFilterVtbl ACMWrapper_Vtbl =
{
    TransformFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    TransformFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    TransformFilterImpl_Stop,
    TransformFilterImpl_Pause,
    TransformFilterImpl_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    TransformFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};
