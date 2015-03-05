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
    TRACE("CBandSiteMenu::SetOwner(%p, %p)\n", this, pOwner);
    m_Owner = pOwner;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::QueryContextMenu(
    HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    BOOL ret;

    TRACE("CBandSiteMenu::QueryContextMenu(%p, %p, %u, %u, %u, 0x%x)\n", this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    HMENU hm = LoadMenuW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDM_TASKBAR_TOOLBARS));
    if (!hm)
        return HRESULT_FROM_WIN32(GetLastError());

    MENUITEMINFOW mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
    ret = GetMenuItemInfoW(hm, 0, TRUE, &mii);
    if (!hm)
        return HRESULT_FROM_WIN32(GetLastError());

    mii.dwTypeData = new WCHAR[mii.cch + 1];
    mii.cch = mii.cch + 1;

    ret = GetMenuItemInfoW(hm, 0, TRUE, &mii);
    if (!hm)
        return HRESULT_FROM_WIN32(GetLastError());

    ret = InsertMenuItemW(hmenu, 0, TRUE, &mii);

    delete mii.dwTypeData;

    RemoveMenu(hm, 0, MF_BYPOSITION);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    FIXME("CBandSiteMenu::InvokeCommand is UNIMPLEMENTED (%p, %p)\n", this, lpici);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::GetCommandString(UINT_PTR idCmd, UINT uType,
    UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    FIXME("CBandSiteMenu::GetCommandString is UNIMPLEMENTED (%p, %p, %u, %p, %p, %u)\n", this, idCmd, uType, pwReserved, pszName, cchMax);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("CBandSiteMenu::HandleMenuMsg is UNIMPLEMENTED (%p, %u, %p, %p)\n", this, uMsg, wParam, lParam);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    FIXME("CBandSiteMenu::HandleMenuMsg2 is UNIMPLEMENTED(%p, %u, %p, %p, %p)\n", this, uMsg, wParam, lParam, plResult);
    return E_NOTIMPL;
}
