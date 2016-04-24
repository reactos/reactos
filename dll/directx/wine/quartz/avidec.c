/*
 * AVI Decompressor (VFW decompressors wrapper)
 *
 * Copyright 2004-2005 Christian Costa
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

typedef struct AVIDecImpl
{
    TransformFilter tf;

    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
    REFERENCE_TIME late;
} AVIDecImpl;

static const IBaseFilterVtbl AVIDec_Vtbl;

static inline AVIDecImpl *impl_from_TransformFilter( TransformFilter *iface )
{
    return CONTAINING_RECORD(iface, AVIDecImpl, tf.filter);
}

static HRESULT WINAPI AVIDec_StartStreaming(TransformFilter* pTransformFilter)
{
    AVIDecImpl* This = impl_from_TransformFilter(pTransformFilter);
    DWORD result;

    TRACE("(%p)->()\n", This);
    This->late = -1;

    result = ICDecompressBegin(This->hvid, This->pBihIn, This->pBihOut);
    if (result != ICERR_OK)
    {
        ERR("Cannot start processing (%d)\n", result);
	return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI AVIDec_EndFlush(TransformFilter *pTransformFilter) {
    AVIDecImpl* This = impl_from_TransformFilter(pTransformFilter);
    This->late = -1;
    return S_OK;
}

static HRESULT WINAPI AVIDec_NotifyDrop(TransformFilter *pTransformFilter, IBaseFilter *sender, Quality qm) {
    AVIDecImpl *This = impl_from_TransformFilter(pTransformFilter);

    EnterCriticalSection(&This->tf.filter.csFilter);
    if (qm.Late > 0)
        This->late = qm.Late + qm.TimeStamp;
    else
        This->late = -1;
    LeaveCriticalSection(&This->tf.filter.csFilter);
    return S_OK;
}

static int AVIDec_DropSample(AVIDecImpl *This, REFERENCE_TIME tStart) {
    if (This->late < 0)
        return 0;

    if (tStart < This->late) {
        TRACE("Dropping sample\n");
        return 1;
    }
    This->late = -1;
    return 0;
}

static HRESULT WINAPI AVIDec_Receive(TransformFilter *tf, IMediaSample *pSample)
{
    AVIDecImpl* This = impl_from_TransformFilter(tf);
    AM_MEDIA_TYPE amt;
    HRESULT hr;
    DWORD res;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;
    LONGLONG tStart, tStop;
    DWORD flags = 0;

    EnterCriticalSection(&This->tf.csReceive);
    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        goto error;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr)) {
        ERR("Unable to retrieve media type\n");
        goto error;
    }

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = cbSrcStream;

    hr = BaseOutputPinImpl_GetDeliveryBuffer((BaseOutputPin*)This->tf.ppPins[1], &pOutSample, NULL, NULL, 0);
    if (FAILED(hr)) {
        ERR("Unable to get delivery buffer (%x)\n", hr);
        goto error;
    }

    hr = IMediaSample_SetActualDataLength(pOutSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
    if (FAILED(hr)) {
	ERR("Unable to get pointer to buffer (%x)\n", hr);
	goto error;
    }
    cbDstStream = IMediaSample_GetSize(pOutSample);
    if (cbDstStream < This->pBihOut->biSizeImage) {
        ERR("Sample size is too small %d < %d\n", cbDstStream, This->pBihOut->biSizeImage);
        hr = E_FAIL;
        goto error;
    }

    if (IMediaSample_IsPreroll(pSample) == S_OK)
        flags |= ICDECOMPRESS_PREROLL;
    if (IMediaSample_IsSyncPoint(pSample) != S_OK)
        flags |= ICDECOMPRESS_NOTKEYFRAME;
    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (hr == S_OK && AVIDec_DropSample(This, tStart))
        flags |= ICDECOMPRESS_HURRYUP;

    res = ICDecompress(This->hvid, flags, This->pBihIn, pbSrcStream, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%x)\n", res);

    /* Drop sample if its intended to be dropped */
    if (flags & ICDECOMPRESS_HURRYUP) {
        hr = S_OK;
        goto error;
    }

    IMediaSample_SetActualDataLength(pOutSample, This->pBihOut->biSizeImage);

    IMediaSample_SetPreroll(pOutSample, (IMediaSample_IsPreroll(pSample) == S_OK));
    IMediaSample_SetDiscontinuity(pOutSample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));
    IMediaSample_SetSyncPoint(pOutSample, (IMediaSample_IsSyncPoint(pSample) == S_OK));

    if (hr == S_OK)
        IMediaSample_SetTime(pOutSample, &tStart, &tStop);
    else if (hr == VFW_S_NO_STOP_TIME)
        IMediaSample_SetTime(pOutSample, &tStart, NULL);
    else
        IMediaSample_SetTime(pOutSample, NULL, NULL);

    if (IMediaSample_GetMediaTime(pSample, &tStart, &tStop) == S_OK)
        IMediaSample_SetMediaTime(pOutSample, &tStart, &tStop);
    else
        IMediaSample_SetMediaTime(pOutSample, NULL, NULL);

    LeaveCriticalSection(&This->tf.csReceive);
    hr = BaseOutputPinImpl_Deliver((BaseOutputPin*)This->tf.ppPins[1], pOutSample);
    EnterCriticalSection(&This->tf.csReceive);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Error sending sample (%x)\n", hr);

error:
    if (pOutSample)
        IMediaSample_Release(pOutSample);

    LeaveCriticalSection(&This->tf.csReceive);
    return hr;
}

static HRESULT WINAPI AVIDec_StopStreaming(TransformFilter* pTransformFilter)
{
    AVIDecImpl* This = impl_from_TransformFilter(pTransformFilter);
    DWORD result;

    TRACE("(%p)->()\n", This);

    if (!This->hvid)
        return S_OK;

    result = ICDecompressEnd(This->hvid);
    if (result != ICERR_OK)
    {
        ERR("Cannot stop processing (%d)\n", result);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI AVIDec_SetMediaType(TransformFilter *tf, PIN_DIRECTION dir, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* This = impl_from_TransformFilter(tf);
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

    TRACE("(%p)->(%p)\n", This, pmt);

    if (dir != PINDIR_INPUT)
        return S_OK;

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Video)+4, sizeof(GUID)-4)))
    {
        VIDEOINFOHEADER *format1 = (VIDEOINFOHEADER *)pmt->pbFormat;
        VIDEOINFOHEADER2 *format2 = (VIDEOINFOHEADER2 *)pmt->pbFormat;
        BITMAPINFOHEADER *bmi;

        if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
            bmi = &format1->bmiHeader;
        else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
            bmi = &format2->bmiHeader;
        else
            goto failed;
        TRACE("Fourcc: %s\n", debugstr_an((const char *)&pmt->subtype.Data1, 4));

        This->hvid = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, bmi, NULL, ICMODE_DECOMPRESS);
        if (This->hvid)
        {
            AM_MEDIA_TYPE* outpmt = &This->tf.pmt;
            const CLSID* outsubtype;
            DWORD bih_size;
            DWORD output_depth = bmi->biBitCount;
            DWORD result;
            FreeMediaType(outpmt);

            switch(bmi->biBitCount)
            {
                case 32: outsubtype = &MEDIASUBTYPE_RGB32; break;
                case 24: outsubtype = &MEDIASUBTYPE_RGB24; break;
                case 16: outsubtype = &MEDIASUBTYPE_RGB565; break;
                case 8:  outsubtype = &MEDIASUBTYPE_RGB8; break;
                default:
                    WARN("Non standard input depth %d, forced output depth to 32\n", bmi->biBitCount);
                    outsubtype = &MEDIASUBTYPE_RGB32;
                    output_depth = 32;
                    break;
            }

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            bih_size = bmi->biSize + bmi->biClrUsed * 4;
            This->pBihIn = CoTaskMemAlloc(bih_size);
            if (!This->pBihIn)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            This->pBihOut = CoTaskMemAlloc(bih_size);
            if (!This->pBihOut)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            memcpy(This->pBihIn, bmi, bih_size);
            memcpy(This->pBihOut, bmi, bih_size);

            /* Update output format as non compressed bitmap */
            This->pBihOut->biCompression = 0;
            This->pBihOut->biBitCount = output_depth;
            This->pBihOut->biSizeImage = This->pBihOut->biWidth * This->pBihOut->biHeight * This->pBihOut->biBitCount / 8;
            TRACE("Size: %u\n", This->pBihIn->biSize);
            result = ICDecompressQuery(This->hvid, This->pBihIn, This->pBihOut);
            if (result != ICERR_OK)
            {
                ERR("Unable to found a suitable output format (%d)\n", result);
                goto failed;
            }

            /* Update output media type */
            CopyMediaType(outpmt, pmt);
            outpmt->subtype = *outsubtype;

            if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo))
                memcpy(&(((VIDEOINFOHEADER *)outpmt->pbFormat)->bmiHeader), This->pBihOut, This->pBihOut->biSize);
            else if (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo2))
                memcpy(&(((VIDEOINFOHEADER2 *)outpmt->pbFormat)->bmiHeader), This->pBihOut, This->pBihOut->biSize);
            else
                assert(0);

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

failed:

    TRACE("Connection refused\n");
    return hr;
}

static HRESULT WINAPI AVIDec_CompleteConnect(TransformFilter *tf, PIN_DIRECTION dir, IPin *pin)
{
    AVIDecImpl* This = impl_from_TransformFilter(tf);

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT WINAPI AVIDec_BreakConnect(TransformFilter *tf, PIN_DIRECTION dir)
{
    AVIDecImpl *This = impl_from_TransformFilter(tf);

    TRACE("(%p)->()\n", This);

    if (dir == PINDIR_INPUT)
    {
        if (This->hvid)
            ICClose(This->hvid);
        if (This->pBihIn)
            CoTaskMemFree(This->pBihIn);
        if (This->pBihOut)
            CoTaskMemFree(This->pBihOut);

        This->hvid = NULL;
        This->pBihIn = NULL;
        This->pBihOut = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI AVIDec_DecideBufferSize(TransformFilter *tf, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    AVIDecImpl *pAVI = impl_from_TransformFilter(tf);
    ALLOCATOR_PROPERTIES actual;

    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    if (ppropInputRequest->cbBuffer < pAVI->pBihOut->biSizeImage)
            ppropInputRequest->cbBuffer = pAVI->pBihOut->biSizeImage;

    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const TransformFilterFuncTable AVIDec_FuncsTable = {
    AVIDec_DecideBufferSize,
    AVIDec_StartStreaming,
    AVIDec_Receive,
    AVIDec_StopStreaming,
    NULL,
    AVIDec_SetMediaType,
    AVIDec_CompleteConnect,
    AVIDec_BreakConnect,
    NULL,
    NULL,
    AVIDec_EndFlush,
    NULL,
    AVIDec_NotifyDrop
};

HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVIDecImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hr = TransformFilter_Construct(&AVIDec_Vtbl, sizeof(AVIDecImpl), &CLSID_AVIDec, &AVIDec_FuncsTable, (IBaseFilter**)&This);

    if (FAILED(hr))
        return hr;

    This->hvid = NULL;
    This->pBihIn = NULL;
    This->pBihOut = NULL;

    *ppv = &This->tf.filter.IBaseFilter_iface;

    return hr;
}

static const IBaseFilterVtbl AVIDec_Vtbl =
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
