/*
 *  Multisource AutoComplete list
 *
 *  Copyright 2007  Mikolaj Zalewski
 *  Copyright 2009  Andrew Hill
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

#include "precomp.h"

void CACLMulti::release_obj(struct ACLMultiSublist *obj)
{
    obj->punk->Release();
    if (obj->pEnum)
        obj->pEnum->Release();
    if (obj->pACL)
        obj->pACL->Release();
}

CACLMulti::CACLMulti()
{
    fObjectCount = 0;
    fCurrentObject = 0;
    fObjects = NULL;
}

CACLMulti::~CACLMulti()
{
    int                                     i;

    TRACE("destroying %p\n", this);
    for (i = 0; i < fObjectCount; i++)
        release_obj(&fObjects[i]);
    CoTaskMemFree(fObjects);
}

HRESULT STDMETHODCALLTYPE CACLMulti::Append(IUnknown *punk)
{
    TRACE("(%p, %p)\n", this, punk);
    if (punk == NULL)
        return E_FAIL;

    fObjects = static_cast<ACLMultiSublist *>(
        CoTaskMemRealloc(fObjects, sizeof(fObjects[0]) * (fObjectCount + 1)));
    fObjects[fObjectCount].punk = punk;
    punk->AddRef();
    if (FAILED_UNEXPECTEDLY(punk->QueryInterface(IID_PPV_ARG(IEnumString, &fObjects[fObjectCount].pEnum))))
        fObjects[fObjectCount].pEnum = NULL;
    if (FAILED_UNEXPECTEDLY(punk->QueryInterface(IID_PPV_ARG(IACList, &fObjects[fObjectCount].pACL))))
        fObjects[fObjectCount].pACL = NULL;
    fObjectCount++;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Remove(IUnknown *punk)
{
    int                                     i;

    TRACE("(%p, %p)\n", this, punk);
    for (i = 0; i < fObjectCount; i++)
        if (fObjects[i].punk == punk)
        {
            release_obj(&fObjects[i]);
            MoveMemory(&fObjects[i], &fObjects[i + 1], (fObjectCount - i - 1) * sizeof(ACLMultiSublist));
            fObjectCount--;
            fObjects = static_cast<ACLMultiSublist *>(
                CoTaskMemRealloc(fObjects, sizeof(fObjects[0]) * fObjectCount));
            return S_OK;
        }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TRACE("(%p, %d, %p, %p)\n", this, celt, rgelt, pceltFetched);
    while (fCurrentObject < fObjectCount)
    {
        if (fObjects[fCurrentObject].pEnum)
        {
            /* native browseui 6.0 also returns only one element */
            HRESULT ret = fObjects[fCurrentObject].pEnum->Next(1, rgelt, pceltFetched);
            if (ret != S_FALSE)
                return ret;
        }
        fCurrentObject++;
    }

    if (pceltFetched)
        *pceltFetched = 0;
    *rgelt = NULL;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Reset()
{
    int                                     i;

    fCurrentObject = 0;
    for (i = 0; i < fObjectCount; i++)
    {
        if (fObjects[i].pEnum)
            fObjects[i].pEnum->Reset();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Skip(ULONG celt)
{
    /* native browseui 6.0 returns this: */
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Clone(IEnumString **ppOut)
{
    *ppOut = NULL;
    /* native browseui 6.0 returns this: */
    return E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE CACLMulti::Expand(LPCWSTR wstr)
{
    HRESULT                                 res = S_OK;
    int                                     i;

    for (i = 0; i < fObjectCount; i++)
    {
        if (!fObjects[i].pACL)
            continue;
        res = fObjects[i].pACL->Expand(wstr);
        if (res == S_OK)
            break;
    }
    return res;
}
