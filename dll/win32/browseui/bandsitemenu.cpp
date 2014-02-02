/*
 *  Band site menu
 *
 *  Copyright 2007  Hervé Poussineua
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

CBandSiteMenu::CBandSiteMenu()
{
}

CBandSiteMenu::~CBandSiteMenu()
{
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::SetOwner(IUnknown *pOwner)
{
    FIXME("(%p, %p)\n", this, pOwner);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::QueryContextMenu(
    HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    FIXME("(%p, %p, %u, %u, %u, 0x%x)\n", this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    FIXME("(%p, %p)\n", this, lpici);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::GetCommandString(UINT_PTR idCmd, UINT uType,
    UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    FIXME("(%p, %p, %u, %p, %p, %u)\n", this, idCmd, uType, pwReserved, pszName, cchMax);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("(%p, %u, %p, %p)\n", this, uMsg, wParam, lParam);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    FIXME("(%p, %u, %p, %p, %p)\n", this, uMsg, wParam, lParam, plResult);
    return E_NOTIMPL;
}
