/*
 *	Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
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

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define YDEBUG
#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "objbase.h"

#include "shell32_main.h"
#include "shellfolder.h"
#include "shresdef.h"

const GUID CLSID_OpenWith = { 0x09799AFB, 0xAD67, 0x11d1, {0xAB,0xCD,0x00,0xC0,0x4F,0xC3,0x09,0x36} };

typedef struct
{	
    const IContextMenu2Vtbl *lpVtblContextMenu;
	const IShellExtInitVtbl *lpvtblShellExtInit;
    LONG  wId;
    volatile LONG ref;
} SHEOWImpl;


static const IShellExtInitVtbl eivt;
static const IContextMenu2Vtbl cmvt;
static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj);
static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface);

HRESULT WINAPI SHEOW_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv)
{
    SHEOWImpl * ow;
    HRESULT res;

	ow = LocalAlloc(LMEM_ZEROINIT, sizeof(SHEOWImpl));
	if (!ow)
    {
        return E_OUTOFMEMORY;
    }

    ow->ref = 1;
    ow->lpVtblContextMenu = &cmvt;
    ow->lpvtblShellExtInit = &eivt;

    TRACE("(%p)->()\n",ow);

    res = SHEOWCm_fnQueryInterface( (IContextMenu2*)&ow->lpVtblContextMenu, riid, ppv );
    SHEOWCm_fnRelease( (IContextMenu2*)&ow->lpVtblContextMenu );
    return res;
}

static inline SHEOWImpl *impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpvtblShellExtInit));
}

static inline SHEOWImpl *impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpVtblContextMenu));
}

static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

     if(IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IContextMenu) ||
        IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = &This->lpVtblContextMenu;
	}
	else if(IsEqualIID(riid, &IID_IShellExtInit))
	{
	  *ppvObj = &This->lpvtblShellExtInit;
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI SHEOWCm_fnAddRef(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

static UINT
AddItems(SHEOWImpl *This, HMENU hMenu, UINT idCmdFirst)
{
    UINT count = 0;
    MENUITEMINFOW mii;
    WCHAR szBuffer[50];
    static const WCHAR szChoose[] = { 'C','h','o','o','s','e',' ','P','r','o','g','r','a','m','.','.','.',0 };

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = idCmdFirst;

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
       ERR("Failed to load string\n");
       wcscpy(szBuffer, szChoose);
    }

    mii.dwTypeData = (LPWSTR)szBuffer;
    if (InsertMenuItemW(hMenu, 0, TRUE, &mii))
        count++;

    return count;
}


static HRESULT WINAPI SHEOWCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    MENUITEMINFOW	mii;
    USHORT items = 0;
    WCHAR szBuffer[100];

    HMENU hSubMenu = NULL;
    SHEOWImpl *This = impl_from_IContextMenu(iface);
   
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, 100) < 0)
    {
       TRACE("failed to load string\n");
       return E_FAIL;
    }


    hSubMenu = CreatePopupMenu();
    if (hSubMenu == NULL)
    {
       ERR("failed to create submenu");
       return E_FAIL;
    }
    items = AddItems(This, hSubMenu, idCmdFirst);

    ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    if (hSubMenu)
    {
       mii.fMask |= MIIM_SUBMENU;
       mii.hSubMenu = hSubMenu;
    }
    mii.dwTypeData = (LPWSTR) szBuffer;
	mii.fState = MFS_ENABLED;
	mii.wID = idCmdFirst + items;
	mii.fType = MFT_STRING;
	if (InsertMenuItemW( hmenu, 0, TRUE, &mii))
        items++;

    TRACE("items %x\n",items);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, items);
}

static HRESULT WINAPI
SHEOWCm_fnInvokeCommand( IContextMenu2* iface, LPCMINVOKECOMMANDINFO lpici )
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
    
    TRACE("This %p\n", This);

    return E_FAIL;
}

static HRESULT WINAPI
SHEOWCm_fnGetCommandString( IContextMenu2* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static HRESULT WINAPI SHEOWCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    TRACE("This %p uMsg %x\n",This, uMsg);

    return E_NOTIMPL;
}

static const IContextMenu2Vtbl cmvt =
{
	SHEOWCm_fnQueryInterface,
	SHEOWCm_fnAddRef,
	SHEOWCm_fnRelease,
	SHEOWCm_fnQueryContextMenu,
	SHEOWCm_fnInvokeCommand,
	SHEOWCm_fnGetCommandString,
	SHEOWCm_fnHandleMenuMsg
};

static HRESULT WINAPI
SHEOW_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);

    TRACE("This %p\n", This);

    return S_OK;
}

static ULONG WINAPI SHEOW_ExtInit_AddRef(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOW_ExtInit_Release(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

static HRESULT WINAPI
SHEOW_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
    return SHEOWCm_fnQueryInterface((IContextMenu2*)This, riid, ppvObject);
}

static const IShellExtInitVtbl eivt =
{
    SHEOW_ExtInit_QueryInterface,
    SHEOW_ExtInit_AddRef,
    SHEOW_ExtInit_Release,
    SHEOW_ExtInit_Initialize
};
