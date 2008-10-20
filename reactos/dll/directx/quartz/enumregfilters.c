/*
 * Implementation of IEnumRegFilters Interface
 *
 * Copyright 2004 Christian Costa
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

#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct IEnumRegFiltersImpl
{
    const IEnumRegFiltersVtbl * lpVtbl;
    LONG refCount;
    ULONG size;
    REGFILTER* RegFilters;
    ULONG uIndex;
} IEnumRegFiltersImpl;

static const struct IEnumRegFiltersVtbl IEnumRegFiltersImpl_Vtbl;

HRESULT IEnumRegFiltersImpl_Construct(REGFILTER* pInRegFilters, const ULONG size, IEnumRegFilters ** ppEnum)
{
    IEnumRegFiltersImpl* pEnumRegFilters;
    REGFILTER* pRegFilters = NULL;
    unsigned int i;

    TRACE("(%p, %d, %p)\n", pInRegFilters, size, ppEnum);

    pEnumRegFilters = CoTaskMemAlloc(sizeof(IEnumRegFiltersImpl));
    if (!pEnumRegFilters)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }

    /* Accept size of 0 */
    if (size)
    {
        pRegFilters = CoTaskMemAlloc(sizeof(REGFILTER)*size);
        if (!pRegFilters)
	{
            CoTaskMemFree(pEnumRegFilters);
            *ppEnum = NULL;
           return E_OUTOFMEMORY;
        }
    }

    for(i = 0; i < size; i++)
    {
        pRegFilters[i].Clsid = pInRegFilters[i].Clsid;
        pRegFilters[i].Name = CoTaskMemAlloc((strlenW(pInRegFilters[i].Name)+1)*sizeof(WCHAR));
        if (!pRegFilters[i].Name)
        {
            while(i)
                CoTaskMemFree(pRegFilters[--i].Name);
            CoTaskMemFree(pRegFilters);
            CoTaskMemFree(pEnumRegFilters);
            return E_OUTOFMEMORY;
        }
        CopyMemory(pRegFilters[i].Name, pInRegFilters[i].Name, (strlenW(pInRegFilters[i].Name)+1)*sizeof(WCHAR));
    }

    pEnumRegFilters->lpVtbl = &IEnumRegFiltersImpl_Vtbl;
    pEnumRegFilters->refCount = 1;
    pEnumRegFilters->uIndex = 0;
    pEnumRegFilters->RegFilters = pRegFilters;
    pEnumRegFilters->size = size;

    *ppEnum = (IEnumRegFilters *)(&pEnumRegFilters->lpVtbl);

    return S_OK;
}

static HRESULT WINAPI IEnumRegFiltersImpl_QueryInterface(IEnumRegFilters * iface, REFIID riid, LPVOID * ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)iface;
    else if (IsEqualIID(riid, &IID_IEnumRegFilters))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumRegFiltersImpl_AddRef(IEnumRegFilters * iface)
{
    IEnumRegFiltersImpl *This = (IEnumRegFiltersImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)\n", iface);

    return refCount;
}

static ULONG WINAPI IEnumRegFiltersImpl_Release(IEnumRegFilters * iface)
{
    IEnumRegFiltersImpl *This = (IEnumRegFiltersImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)\n", iface);

    if (!refCount)
    {
        ULONG i;

        for(i = 0; i < This->size; i++)
        {
            CoTaskMemFree(This->RegFilters[i].Name);
        }
        CoTaskMemFree(This->RegFilters);
        CoTaskMemFree(This);
        return 0;
    } else
        return refCount;
}

static HRESULT WINAPI IEnumRegFiltersImpl_Next(IEnumRegFilters * iface, ULONG cFilters, REGFILTER ** ppRegFilter, ULONG * pcFetched)
{
    ULONG cFetched; 
    IEnumRegFiltersImpl *This = (IEnumRegFiltersImpl *)iface;
    unsigned int i;

    cFetched = min(This->size, This->uIndex + cFilters) - This->uIndex;

    TRACE("(%p)->(%u, %p, %p)\n", iface, cFilters, ppRegFilter, pcFetched);

    if (cFetched > 0)
    {
        for(i = 0; i < cFetched; i++)
        {
            /* The string in the REGFILTER structure must be allocated in the same block as the REGFILTER structure itself */
            ppRegFilter[i] = CoTaskMemAlloc(sizeof(REGFILTER)+(strlenW(This->RegFilters[This->uIndex + i].Name)+1)*sizeof(WCHAR));
            if (!ppRegFilter[i])
            {
                while(i)
                {
                    CoTaskMemFree(ppRegFilter[--i]);
                    ppRegFilter[i] = NULL;
                }
                return E_OUTOFMEMORY;
        }
            ppRegFilter[i]->Clsid = This->RegFilters[This->uIndex + i].Clsid;
            ppRegFilter[i]->Name = (WCHAR*)((char*)ppRegFilter[i]+sizeof(REGFILTER));
            CopyMemory(ppRegFilter[i]->Name, This->RegFilters[This->uIndex + i].Name,
                            (strlenW(This->RegFilters[This->uIndex + i].Name)+1)*sizeof(WCHAR));
        }

        This->uIndex += cFetched;
        if (pcFetched)
            *pcFetched = cFetched;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT WINAPI IEnumRegFiltersImpl_Skip(IEnumRegFilters * iface, ULONG n)
{
    TRACE("(%p)->(%u)\n", iface, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI IEnumRegFiltersImpl_Reset(IEnumRegFilters * iface)
{
    IEnumRegFiltersImpl *This = (IEnumRegFiltersImpl *)iface;

    TRACE("(%p)\n", iface);

    This->uIndex = 0;
    return S_OK;
}

static HRESULT WINAPI IEnumRegFiltersImpl_Clone(IEnumRegFilters * iface, IEnumRegFilters ** ppEnum)
{
    TRACE("(%p)->(%p)\n", iface, ppEnum);

    return E_NOTIMPL;
}

static const IEnumRegFiltersVtbl IEnumRegFiltersImpl_Vtbl =
{
    IEnumRegFiltersImpl_QueryInterface,
    IEnumRegFiltersImpl_AddRef,
    IEnumRegFiltersImpl_Release,
    IEnumRegFiltersImpl_Next,
    IEnumRegFiltersImpl_Skip,
    IEnumRegFiltersImpl_Reset,
    IEnumRegFiltersImpl_Clone
};
