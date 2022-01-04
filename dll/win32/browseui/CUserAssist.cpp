/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IUserAssist implementation
 * COPYRIGHT:   Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */
// See http://www.geoffchappell.com/studies/windows/ie/browseui/interfaces/iuserassist.htm

#include "precomp.h"

CUserAssist::CUserAssist()
{
}

CUserAssist::~CUserAssist()
{
}

// *** IUserAssist methods ***
STDMETHODIMP CUserAssist::FireEvent(GUID const *guid, INT param1, ULONG param2, WPARAM wparam, LPARAM lparam)
{
    TRACE("(%u, %d, %d, %p, %p)\n", this, guid, param1, param2, wparam, lparam);
    return E_NOTIMPL;
}

STDMETHODIMP CUserAssist::QueryEvent(GUID const *guid, INT param, WPARAM wparam, LPARAM lparam, PVOID ptr)
{
    TRACE("(%u, %d, %p, %p, %p)\n", this, guid, param, wparam, lparam, ptr);
    return E_NOTIMPL;
}

STDMETHODIMP CUserAssist::SetEvent(GUID const *guid, INT param, WPARAM wparam, LPARAM lparam, PVOID ptr)
{
    TRACE("(%u, %d, %p, %p, %p)\n", this, guid, param, wparam, lparam, ptr);
    return E_NOTIMPL;
}

STDMETHODIMP CUserAssist::Enable(BOOL bEnable)
{
    TRACE("(%d)\n", this, bEnable);
    return E_NOTIMPL;
}
