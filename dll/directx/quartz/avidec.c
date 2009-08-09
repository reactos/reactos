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

#include "config.h"

#include "quartz_private.h"
#include "pin.h"

#include "uuids.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "vfw.h"
#include "dvdmedia.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "transform.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct AVIDecImpl
{
    TransformFilterImpl tf;
    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
} AVIDecImpl;

static HRESULT AVIDec_ProcessBegin(TransformFilterImpl* pTransformFilter)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
    DWORD result;

    TRACE("(%p)->()\n", This);

    result = ICDecompressBegin(This->hvid, This->pBihIn, This->pBihOut);
    if (result != ICERR_OK)
    {
        ERR("Cannot start processing (%d)\n", result);
	return E_FAIL;
    }
    return S_OK;
}

static HRESULT AVIDec_ProcessSampleData(InputPin *pin, IMediaSample *pSample)
{
    AVIDecImpl* This = (AVIDecImpl *)pin->pin.pinInfo.pFilter;
    AM_MEDIA_TYPE amt;
    HRESULT hr;
    DWORD res;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;
    LONGLONG tStart, tStop;

    EnterCriticalSection(&This->tf.csFilter);
    if (This->tf.state == State_Stopped)
    {
        LeaveCriticalSection(&This->tf.csFilter);
        return VFW_E_WRONG_STATE;
    }

    if (pin->end_of_stream || pin->flushing)
    {
        LeaveCriticalSection(&This->tf.csFilter);
        return S_FALSE;
    }

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        goto error;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %ld\n", pbSrcStream, (long)cbSrcStream);

    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr)) {
        ERR("Unable to retrieve media type\n");
        goto error;
    }

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = cbSrcStream;

    hr = OutputPin_GetDeliveryBuffer((OutputPin*)This->tf.ppPins[1], &pOutSample, NULL, NULL, 0);
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

    res = ICDecompress(This->hvid, 0, This->pBihIn, pbSrcStream, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%x)\n", res);

    IMediaSample_SetActualDataLength(pOutSample, This->pBihOut->biSizeImage);

    IMediaSample_SetPreroll(pOutSample, (IMediaSample_IsPreroll(pSample) == S_OK));
    IMediaSample_SetDiscontinuity(pOutSample, (IMediaSample_IsDiscontinuity(pSample) == S_OK));
    IMediaSample_SetSyncPoint(pOutSample, (IMediaSample_IsSyncPoint(pSample) == S_OK));

    if (IMediaSample_GetTime(pSample, &tStart, &tStop) == S_OK)
        IMediaSample_SetTime(pOutSample, &tStart, &tStop);
    else
        IMediaSample_SetTime(pOutSample, NULL, NULL);

    LeaveCriticalSection(&This->tf.csFilter);
    hr = OutputPin_SendSample((OutputPin*)This->tf.ppPins[1], pOutSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
        ERR("Error sending sample (%x)\n", hr);
    IMediaSample_Release(pOutSample);
    return hr;

error:
    if (pOutSample)
        IMediaSample_Release(pOutSample);

    LeaveCriticalSection(&This->tf.csFilter);
    return hr;
}

static HRESULT AVIDec_ProcessEnd(TransformFilterImpl* pTransformFilter)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
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

static HRESULT AVIDec_ConnectInput(InputPin *pin, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* This = (AVIDecImpl*)pin->pin.pinInfo.pFilter;
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

    TRACE("(%p)->(%p)\n", This, pmt);

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
        TRACE("Fourcc: %s\n", debugstr_an((char *)&pmt->subtype.Data1, 4));

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

            /* Update buffer size of media samples in output */
            ((OutputPin*)This->tf.ppPins[1])->allocProps.cbBuffer = This->pBihOut->biSizeImage;

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

failed:

    TRACE("Connection refused\n");
    return hr;
}

static HRESULT AVIDec_Cleanup(InputPin *pin)
{
    AVIDecImpl *This = (AVIDecImpl *)pin->pin.pinInfo.pFilter;

    TRACE("(%p)->()\n", This);
    
    if (This->hvid)
        ICClose(This->hvid);
    if (This->pBihIn)
        CoTaskMemFree(This->pBihIn);
    if (This->pBihOut)
        CoTaskMemFree(This->pBihOut);

    This->hvid = NULL;
    This->pBihIn = NULL;
    This->pBihOut = NULL;

    return S_OK;
}

static const TransformFuncsTable AVIDec_FuncsTable = {
    AVIDec_ProcessBegin,
    AVIDec_ProcessSampleData,
    AVIDec_ProcessEnd,
    NULL,
    AVIDec_ConnectInput,
    AVIDec_Cleanup
};

HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVIDecImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(AVIDecImpl));

    This->hvid = NULL;
    This->pBihIn = NULL;
    This->pBihOut = NULL;

    hr = TransformFilter_Create(&(This->tf), &CLSID_AVIDec, &AVIDec_FuncsTable, NULL, NULL, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
