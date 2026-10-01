/*
 * Copyright (C) 2003 Michael Günnewig
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "mediaobj.h"
#include "dmort.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdmo);

/***********************************************************************
 *        MoCreateMediaType    (MSDMO.@)
 *
 * Allocate a new media type structure
 */
HRESULT WINAPI MoCreateMediaType(DMO_MEDIA_TYPE** ppmedia, DWORD cbFormat)
{
    HRESULT r;

    TRACE("%p %lu\n", ppmedia, cbFormat);

    if (!ppmedia)
        return E_POINTER;

    *ppmedia = CoTaskMemAlloc(sizeof(DMO_MEDIA_TYPE));
    if (!*ppmedia)
        return E_OUTOFMEMORY;

    r = MoInitMediaType(*ppmedia, cbFormat);
    if (FAILED(r))
    {
        CoTaskMemFree(*ppmedia);
        *ppmedia = NULL;
    }

    return r;
}

/***********************************************************************
 *        MoInitMediaType        (MSDMO.@)
 *
 * Initialize media type structure
 */
HRESULT WINAPI MoInitMediaType(DMO_MEDIA_TYPE* pmedia, DWORD cbFormat)
{
    TRACE("%p %lu\n", pmedia, cbFormat);

    if (!pmedia)
        return E_POINTER;

    memset(pmedia, 0, sizeof(DMO_MEDIA_TYPE));

    if (cbFormat > 0)
    {
        pmedia->pbFormat = CoTaskMemAlloc(cbFormat);
        if (!pmedia->pbFormat)
            return E_OUTOFMEMORY;

        pmedia->cbFormat = cbFormat;
    }

    return S_OK;
}

/***********************************************************************
 *        MoDeleteMediaType    (MSDMO.@)
 *
 * Delete a media type structure
 */
HRESULT WINAPI MoDeleteMediaType(DMO_MEDIA_TYPE* pmedia)
{
    TRACE("%p\n", pmedia);

    if (!pmedia)
        return E_POINTER;

    MoFreeMediaType(pmedia);
    CoTaskMemFree(pmedia);

    return S_OK;
}

/***********************************************************************
 *        MoFreeMediaType        (MSDMO.@)
 *
 * Free allocated members of a media type structure
 */
HRESULT WINAPI MoFreeMediaType(DMO_MEDIA_TYPE* pmedia)
{
    TRACE("%p\n", pmedia);

    if (!pmedia)
        return E_POINTER;

    if (pmedia->pUnk)
    {
        IUnknown_Release(pmedia->pUnk);
        pmedia->pUnk = NULL;
    }

    CoTaskMemFree(pmedia->pbFormat);
    pmedia->pbFormat = NULL;

    return S_OK;
}

/***********************************************************************
 *        MoDuplicateMediaType    (MSDMO.@)
 *
 * Duplicates a media type structure
 */
HRESULT WINAPI MoDuplicateMediaType(DMO_MEDIA_TYPE** ppdst,
                                    const DMO_MEDIA_TYPE* psrc)
{
    HRESULT r;

    TRACE("%p %p\n", ppdst, psrc);

    if (!ppdst || !psrc)
        return E_POINTER;

    *ppdst = CoTaskMemAlloc(sizeof(DMO_MEDIA_TYPE));
    if (!*ppdst)
        return E_OUTOFMEMORY;

    r = MoCopyMediaType(*ppdst, psrc);
    if (FAILED(r))
    {
        MoFreeMediaType(*ppdst);
        *ppdst = NULL;
    }

    return r;
}

/***********************************************************************
 *        MoCopyMediaType        (MSDMO.@)
 *
 * Copy a new media type structure
 */
HRESULT WINAPI MoCopyMediaType(DMO_MEDIA_TYPE* pdst,
                               const DMO_MEDIA_TYPE* psrc)
{
    TRACE("%p %p\n", pdst, psrc);

    if (!pdst || !psrc)
        return E_POINTER;

    pdst->majortype = psrc->majortype;
    pdst->subtype = psrc->subtype;
    pdst->formattype = psrc->formattype;

    pdst->bFixedSizeSamples    = psrc->bFixedSizeSamples;
    pdst->bTemporalCompression = psrc->bTemporalCompression;
    pdst->lSampleSize          = psrc->lSampleSize;
    pdst->cbFormat             = psrc->cbFormat;

    if (psrc->pbFormat && psrc->cbFormat > 0)
    {
        pdst->pbFormat = CoTaskMemAlloc(psrc->cbFormat);
        if (!pdst->pbFormat)
            return E_OUTOFMEMORY;

        memcpy(pdst->pbFormat, psrc->pbFormat, psrc->cbFormat);
    }
    else
        pdst->pbFormat = NULL;

    if (psrc->pUnk)
    {
        pdst->pUnk = psrc->pUnk;
        IUnknown_AddRef(pdst->pUnk);
    }
    else
        pdst->pUnk = NULL;

    return S_OK;
}
