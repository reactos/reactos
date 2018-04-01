/*
 * Implementation of MedaType utility functions
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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
#include "dshow.h"

#include "wine/strmbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE *dest, const AM_MEDIA_TYPE *src)
{
    *dest = *src;
    if (src->pbFormat)
    {
        dest->pbFormat = CoTaskMemAlloc(src->cbFormat);
        if (!dest->pbFormat)
            return E_OUTOFMEMORY;
        memcpy(dest->pbFormat, src->pbFormat, src->cbFormat);
    }
    if (dest->pUnk)
        IUnknown_AddRef(dest->pUnk);
    return S_OK;
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType)
{
    if (pMediaType->pbFormat)
    {
        CoTaskMemFree(pMediaType->pbFormat);
        pMediaType->pbFormat = NULL;
    }
    if (pMediaType->pUnk)
    {
        IUnknown_Release(pMediaType->pUnk);
        pMediaType->pUnk = NULL;
    }
}

AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc)
{
    AM_MEDIA_TYPE * pDest;

    pDest = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pDest)
        return NULL;

    if (FAILED(CopyMediaType(pDest, pSrc)))
    {
        CoTaskMemFree(pDest);
        return NULL;
    }

    return pDest;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}

typedef struct tagENUMEDIADETAILS
{
    ULONG cMediaTypes;
    AM_MEDIA_TYPE * pMediaTypes;
} ENUMMEDIADETAILS;

typedef struct IEnumMediaTypesImpl
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refCount;
    BasePin *basePin;
    BasePin_GetMediaType enumMediaFunction;
    BasePin_GetMediaTypeVersion mediaVersionFunction;
    LONG currentVersion;
    ENUMMEDIADETAILS enumMediaDetails;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static inline IEnumMediaTypesImpl *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, IEnumMediaTypesImpl, IEnumMediaTypes_iface);
}

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

HRESULT WINAPI EnumMediaTypes_Construct(BasePin *basePin, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum)
{
    ULONG i;
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));
    AM_MEDIA_TYPE amt;

    *ppEnum = NULL;

    if (!pEnumMediaTypes)
        return E_OUTOFMEMORY;

    pEnumMediaTypes->IEnumMediaTypes_iface.lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    pEnumMediaTypes->enumMediaFunction = enumFunc;
    pEnumMediaTypes->mediaVersionFunction = versionFunc;
    IPin_AddRef(&basePin->IPin_iface);
    pEnumMediaTypes->basePin = basePin;

    i = 0;
    while (enumFunc(basePin, i, &amt) == S_OK)
    {
        FreeMediaType(&amt);
        i++;
    }

    pEnumMediaTypes->enumMediaDetails.cMediaTypes = i;
    pEnumMediaTypes->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * i);
    memset(pEnumMediaTypes->enumMediaDetails.pMediaTypes, 0, sizeof(AM_MEDIA_TYPE) * i);
    for (i = 0; i < pEnumMediaTypes->enumMediaDetails.cMediaTypes; i++)
    {
        HRESULT hr;

        if (FAILED(hr = enumFunc(basePin, i, &pEnumMediaTypes->enumMediaDetails.pMediaTypes[i])))
        {
            IEnumMediaTypes_Release(&pEnumMediaTypes->IEnumMediaTypes_iface);
            return hr;
        }
    }
    *ppEnum = &pEnumMediaTypes->IEnumMediaTypes_iface;
    pEnumMediaTypes->currentVersion = versionFunc(basePin);
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, void ** ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumMediaTypes))
    {
        IEnumMediaTypes_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    WARN("No interface for %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG ref = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);
    ULONG ref = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        ULONG i;
        for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
            FreeMediaType(&This->enumMediaDetails.pMediaTypes[i]);
        CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
        IPin_Release(&This->basePin->IPin_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes * iface, ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched)
{
    ULONG cFetched;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%u, %p, %p)\n", iface, cMediaTypes, ppMediaTypes, pcFetched);

    cFetched = min(This->enumMediaDetails.cMediaTypes, This->uIndex + cMediaTypes) - This->uIndex;

    if (This->currentVersion != This->mediaVersionFunction(This->basePin))
        return VFW_E_ENUM_OUT_OF_SYNC;

    TRACE("Next uIndex: %u, cFetched: %u\n", This->uIndex, cFetched);

    if (cFetched > 0)
    {
        ULONG i;
        for (i = 0; i < cFetched; i++)
            if (!(ppMediaTypes[i] = CreateMediaType(&This->enumMediaDetails.pMediaTypes[This->uIndex + i])))
            {
                while (i--)
                    DeleteMediaType(ppMediaTypes[i]);
                *pcFetched = 0;
                return E_OUTOFMEMORY;
            }
    }

    if ((cMediaTypes != 1) || pcFetched)
        *pcFetched = cFetched;

    This->uIndex += cFetched;

    if (cFetched != cMediaTypes)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Skip(IEnumMediaTypes * iface, ULONG cMediaTypes)
{
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%u)\n", iface, cMediaTypes);

    if (This->currentVersion != This->mediaVersionFunction(This->basePin))
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (This->uIndex + cMediaTypes < This->enumMediaDetails.cMediaTypes)
    {
        This->uIndex += cMediaTypes;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    ULONG i;
    AM_MEDIA_TYPE amt;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->()\n", iface);

    for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
        FreeMediaType(&This->enumMediaDetails.pMediaTypes[i]);
    CoTaskMemFree(This->enumMediaDetails.pMediaTypes);

    i = 0;
    while (This->enumMediaFunction(This->basePin, i, &amt) == S_OK) i++;

    This->enumMediaDetails.cMediaTypes = i;
    This->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * i);
    for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
    {
        This->enumMediaFunction(This->basePin, i,&amt);
        if (FAILED(CopyMediaType(&This->enumMediaDetails.pMediaTypes[i], &amt)))
        {
            while (i--)
                FreeMediaType(&This->enumMediaDetails.pMediaTypes[i]);
            CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
            return E_OUTOFMEMORY;
        }
    }

    This->currentVersion = This->mediaVersionFunction(This->basePin);
    This->uIndex = 0;

    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    IEnumMediaTypesImpl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%p)\n", iface, ppEnum);

    hr = EnumMediaTypes_Construct(This->basePin, This->enumMediaFunction, This->mediaVersionFunction, ppEnum);
    if (FAILED(hr))
        return hr;
    return IEnumMediaTypes_Skip(*ppEnum, This->uIndex);
}

static const IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl =
{
    IEnumMediaTypesImpl_QueryInterface,
    IEnumMediaTypesImpl_AddRef,
    IEnumMediaTypesImpl_Release,
    IEnumMediaTypesImpl_Next,
    IEnumMediaTypesImpl_Skip,
    IEnumMediaTypesImpl_Reset,
    IEnumMediaTypesImpl_Clone
};
