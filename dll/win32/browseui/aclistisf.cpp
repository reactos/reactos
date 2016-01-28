/*
 *  Shell AutoComplete list
 *
 *  Copyright 2015  Thomas Faber
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

CACListISF::CACListISF() :
    m_dwOptions(0)
{
}

CACListISF::~CACListISF()
{
}

// *** IEnumString methods ***
HRESULT STDMETHODCALLTYPE CACListISF::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TRACE("(%p, %d, %p, %p)\n", this, celt, rgelt, pceltFetched);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CACListISF::Reset()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CACListISF::Skip(ULONG celt)
{
    TRACE("(%p, %d)\n", this, celt);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CACListISF::Clone(IEnumString **ppOut)
{
    TRACE("(%p, %p)\n", this, ppOut);
    *ppOut = NULL;
    return E_NOTIMPL;
}

// *** IACList methods ***
HRESULT STDMETHODCALLTYPE CACListISF::Expand(LPCOLESTR pszExpand)
{
    TRACE("(%p, %ls)\n", this, pszExpand);
    return E_NOTIMPL;
}

// *** IACList2 methods ***
HRESULT STDMETHODCALLTYPE CACListISF::SetOptions(DWORD dwFlag)
{
    TRACE("(%p, %lu)\n", this, dwFlag);
    m_dwOptions = dwFlag;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CACListISF::GetOptions(DWORD* pdwFlag)
{
    TRACE("(%p, %p)\n", this, pdwFlag);
    *pdwFlag = m_dwOptions;
    return S_OK;
}

// *** IShellService methods ***
HRESULT STDMETHODCALLTYPE CACListISF::SetOwner(IUnknown *punkOwner)
{
    TRACE("(%p, %p)\n", this, punkOwner);
    return E_NOTIMPL;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CACListISF::GetClassID(CLSID *pClassID)
{
    TRACE("(%p, %p)\n", this, pClassID);
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_ACListISF;
    return S_OK;
}

// *** IPersistFolder methods ***
HRESULT STDMETHODCALLTYPE CACListISF::Initialize(LPCITEMIDLIST pidl)
{
    TRACE("(%p, %p)\n", this, pidl);
    return S_OK;
}
