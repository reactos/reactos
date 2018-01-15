/*
 * IShellItem implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
 * Copyright 2009 Andrew Hill
 * Copyright 2013 Katayama Hirofumi MZ
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HRESULT WINAPI SHCreateItemFromIDList(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv)
{
	CComPtr<IPersistIDList> persist;
	HRESULT ret;

	if (!pidl)
		return E_INVALIDARG;

	*ppv = NULL;
	ret = CoCreateInstance(CLSID_ShellItem, NULL, CLSCTX_INPROC_SERVER, IID_IPersistIDList, &persist);
	if (FAILED(ret))
		return ret;

	ret = persist->SetIDList(pidl);
	if (FAILED(ret))
	{
		return ret;
	}

	ret = persist->QueryInterface(riid, ppv);
	return ret;
}

HRESULT WINAPI SHCreateItemFromParsingName(PCWSTR pszPath,
	IBindCtx *pbc, REFIID riid, void **ppv)
{
	LPITEMIDLIST pidl;
	HRESULT ret;

	*ppv = NULL;

	ret = SHParseDisplayName(pszPath, pbc, &pidl, 0, NULL);
	if (SUCCEEDED(ret))
	{
		ret = SHCreateItemFromIDList(pidl, riid, ppv);
		ILFree(pidl);
	}
	return ret;
}
