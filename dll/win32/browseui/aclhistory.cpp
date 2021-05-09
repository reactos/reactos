/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement CLSID_ACLHistory for auto-completion
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

CACLHistory::CACLHistory()
{
    TRACE("CACLHistory::CACLHistory(%p)\n", this);
}

CACLHistory::~CACLHistory()
{
    TRACE("CACLHistory::~CACLHistory(%p)\n", this);
}

STDMETHODIMP CACLHistory::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    FIXME("CACLHistory::Next(%p, %lu, %p, %p): stub\n", this, celt, rgelt, pceltFetched);
    if (pceltFetched)
        *pceltFetched = 0;
    if (rgelt)
        *rgelt = NULL;
    if (celt != 1)
        return E_NOTIMPL;
    // FIXME: *rgelt, *pceltFetched, return value
    return S_FALSE;
}

STDMETHODIMP CACLHistory::Reset()
{
    FIXME("CACLHistory::Reset(%p): stub\n", this);
    return S_OK;
}

STDMETHODIMP CACLHistory::Skip(ULONG celt)
{
    TRACE("CACLHistory::Clone(%p, %lu)\n", this, celt);
    return E_NOTIMPL;
}

STDMETHODIMP CACLHistory::Clone(IEnumString **ppenum)
{
    FIXME("CACLHistory::Clone(%p, %p): stub\n", this, ppenum);
    if (ppenum)
        *ppenum = NULL;
    return E_NOTIMPL;
}
