/*
 * Implementation of IEnumMediaTypes Interface
 *
 * Copyright 2003 Robert Shearman
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

HRESULT CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc)
{
    *pDest = *pSrc;
    if (!pSrc->pbFormat) return S_OK;
    if (!(pDest->pbFormat = CoTaskMemAlloc(pSrc->cbFormat)))
        return E_OUTOFMEMORY;
    memcpy(pDest->pbFormat, pSrc->pbFormat, pSrc->cbFormat);
    if (pDest->pUnk)
        IUnknown_AddRef(pDest->pUnk);
    return S_OK;
}

void FreeMediaType(AM_MEDIA_TYPE * pMediaType)
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

static AM_MEDIA_TYPE * CreateMediaType(AM_MEDIA_TYPE const * pSrc)
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

void DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}

BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards)
{
    TRACE("pmt1: ");
    dump_AM_MEDIA_TYPE(pmt1);
    TRACE("pmt2: ");
    dump_AM_MEDIA_TYPE(pmt2);
    return (((bWildcards && (IsEqualGUID(&pmt1->majortype, &GUID_NULL) || IsEqualGUID(&pmt2->majortype, &GUID_NULL))) || IsEqualGUID(&pmt1->majortype, &pmt2->majortype)) &&
            ((bWildcards && (IsEqualGUID(&pmt1->subtype, &GUID_NULL)   || IsEqualGUID(&pmt2->subtype, &GUID_NULL)))   || IsEqualGUID(&pmt1->subtype, &pmt2->subtype)));
}

void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt)
{
    if (!pmt)
        return;
    TRACE("\t%s\n\t%s\n\t...\n\t%s\n", qzdebugstr_guid(&pmt->majortype), qzdebugstr_guid(&pmt->subtype), qzdebugstr_guid(&pmt->formattype));
}

typedef struct IEnumMediaTypesImpl
{
    const IEnumMediaTypesVtbl * lpVtbl;
    LONG refCount;
    ENUMMEDIADETAILS enumMediaDetails;
    ULONG uIndex;
} IEnumMediaTypesImpl;

static const struct IEnumMediaTypesVtbl IEnumMediaTypesImpl_Vtbl;

HRESULT IEnumMediaTypesImpl_Construct(const ENUMMEDIADETAILS * pDetails, IEnumMediaTypes ** ppEnum)
{
    ULONG i;
    IEnumMediaTypesImpl * pEnumMediaTypes = CoTaskMemAlloc(sizeof(IEnumMediaTypesImpl));

    if (!pEnumMediaTypes)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    pEnumMediaTypes->lpVtbl = &IEnumMediaTypesImpl_Vtbl;
    pEnumMediaTypes->refCount = 1;
    pEnumMediaTypes->uIndex = 0;
    pEnumMediaTypes->enumMediaDetails.cMediaTypes = pDetails->cMediaTypes;
    pEnumMediaTypes->enumMediaDetails.pMediaTypes = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * pDetails->cMediaTypes);
    for (i = 0; i < pDetails->cMediaTypes; i++)
        if (FAILED(CopyMediaType(&pEnumMediaTypes->enumMediaDetails.pMediaTypes[i], &pDetails->pMediaTypes[i])))
        {
           while (i--)
              CoTaskMemFree(pEnumMediaTypes->enumMediaDetails.pMediaTypes[i].pbFormat);
           CoTaskMemFree(pEnumMediaTypes->enumMediaDetails.pMediaTypes);
           return E_OUTOFMEMORY;
        }
    *ppEnum = (IEnumMediaTypes *)(&pEnumMediaTypes->lpVtbl);
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_QueryInterface(IEnumMediaTypes * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IEnumMediaTypes))
        *ppv = iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumMediaTypesImpl_AddRef(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI IEnumMediaTypesImpl_Release(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        ULONG i;
        for (i = 0; i < This->enumMediaDetails.cMediaTypes; i++)
            if (This->enumMediaDetails.pMediaTypes[i].pbFormat)
                CoTaskMemFree(This->enumMediaDetails.pMediaTypes[i].pbFormat);
        CoTaskMemFree(This->enumMediaDetails.pMediaTypes);
        CoTaskMemFree(This);
    }
    return refCount;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Next(IEnumMediaTypes * iface, ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched)
{
    ULONG cFetched; 
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    cFetched = min(This->enumMediaDetails.cMediaTypes, This->uIndex + cMediaTypes) - This->uIndex;

    TRACE("(%u, %p, %p)\n", cMediaTypes, ppMediaTypes, pcFetched);
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
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("(%u)\n", cMediaTypes);

    if (This->uIndex + cMediaTypes < This->enumMediaDetails.cMediaTypes)
    {
        This->uIndex += cMediaTypes;
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Reset(IEnumMediaTypes * iface)
{
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("()\n");

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumMediaTypesImpl_Clone(IEnumMediaTypes * iface, IEnumMediaTypes ** ppEnum)
{
    HRESULT hr;
    IEnumMediaTypesImpl *This = (IEnumMediaTypesImpl *)iface;

    TRACE("(%p)\n", ppEnum);

    hr = IEnumMediaTypesImpl_Construct(&This->enumMediaDetails, ppEnum);
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
