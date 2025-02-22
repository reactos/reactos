/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "precomp.h"

CRegTreeOptions::CRegTreeOptions()
{
}

CRegTreeOptions::~CRegTreeOptions()
{
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::InitTree(HWND paramC, HKEY param10, char const *param14, char const *param18)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::WalkTree(WALK_TREE_CMD paramC)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::ToggleItem(HTREEITEM paramC)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::ShowHelp(HTREEITEM paramC, unsigned long param10)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::SetSite(IUnknown *pUnkSite)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::GetSite(REFIID riid, void **ppvSite)
{
    return E_NOTIMPL;
}
