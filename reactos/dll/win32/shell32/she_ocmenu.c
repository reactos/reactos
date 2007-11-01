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
//#define YDEBUG
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

WINE_DEFAULT_DEBUG_CHANNEL (shell);

const GUID CLSID_OpenWith = { 0x09799AFB, 0xAD67, 0x11d1, {0xAB,0xCD,0x00,0xC0,0x4F,0xC3,0x09,0x36} };

typedef struct
{	
    const IContextMenu2Vtbl *lpVtblContextMenu;
	const IShellExtInitVtbl *lpvtblShellExtInit;
    LONG  wId;
    volatile LONG ref;
    WCHAR ** szArray;
    UINT size;
    UINT count;
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
    WCHAR szBuffer[MAX_PATH];
    UINT index;
    static const WCHAR szChoose[] = { 'C','h','o','o','s','e',' ','P','r','o','g','r','a','m','.','.','.',0 };

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    
    for (index = 0; index < This->count; index++)
    {
        mii.wID = idCmdFirst;
        mii.dwTypeData = (LPWSTR)This->szArray[index];
        if (InsertMenuItemW(hMenu, -1, TRUE, &mii))
        {
            idCmdFirst++;
            count++;
        }
    }
    
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, -1, TRUE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
       ERR("Failed to load string\n");
       wcscpy(szBuffer, szChoose);
    }

    mii.wID = idCmdFirst;
    mii.dwTypeData = (LPWSTR)szBuffer;
    if (InsertMenuItemW(hMenu, -1, TRUE, &mii))
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
    BOOL bDefault = FALSE;

    HMENU hSubMenu = NULL;
    SHEOWImpl *This = impl_from_IContextMenu(iface);
   
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, 100) < 0)
    {
       TRACE("failed to load string\n");
       return E_FAIL;
    }

    if (This->count)
    {
        hSubMenu = CreatePopupMenu();
        if (hSubMenu == NULL)
        {
            ERR("failed to create submenu");
            return E_FAIL;
        }
        items = AddItems(This, hSubMenu, idCmdFirst + 1);
    }
    else
    {
        /* no file association found */
        UINT pos = GetMenuDefaultItem(hmenu, TRUE, 0);
        if (pos != -1)
        {
            /* replace default item with "Open With" action */
            bDefault = DeleteMenu(hmenu, pos, MF_BYPOSITION);
        }
    }

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
    if (bDefault)
    {
        mii.fState |= MFS_DEFAULT;
    }

	mii.wID = idCmdFirst;
    This->wId = idCmdFirst;

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
    TRACE("This %p wId %x count %u verb %x\n", This, This->wId, This->count, LOWORD(lpici->lpVerb));    

    if (This->wId > LOWORD(lpici->lpVerb) || This->count + This->wId < LOWORD(lpici->lpVerb))
       return E_FAIL;

    if (This->wId == LOWORD(lpici->lpVerb))
    {
        /* show Open As dialog */
        return S_OK;
    }
    else 
    {
        /* show program select dialog */
        return S_OK;
    }
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

BOOL
SHEOW_ResizeArray(SHEOWImpl *This)
{
  WCHAR ** new_array;
  UINT ncount;

  if (This->count == 0)
      ncount = 10;
  else
      ncount = This->count * 2;

  new_array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ncount * sizeof(WCHAR*));

  if (!new_array)
      return FALSE;

  if (This->szArray)
  {
    memcpy(new_array, This->szArray, This->count * sizeof(WCHAR*));
    HeapFree(GetProcessHeap(), 0, This->szArray);
  }

  This->szArray = new_array;
  This->size = ncount;
  return TRUE;
}


void
SHEOW_AddOWItem(SHEOWImpl *This, WCHAR * szAppName)
{
   UINT index;
   WCHAR * szPtr;

   if (This->count + 1 >= This->size || !This->szArray)
   {
        if (!SHEOW_ResizeArray(This))
            return;
   }

   szPtr = wcsrchr(szAppName, '.');
   if (szPtr)
   {
        szPtr[0] = 0;
   }

   for(index = 0; index < This->count; index++)
   {
        if (!wcsicmp(This->szArray[index], szAppName))
            return;
   }
   This->szArray[This->count] = wcsdup(szAppName);

    if (This->szArray[This->count])
        This->count++;
}

UINT
SHEW_AddOpenWithProgId(SHEOWImpl *This, HKEY hKey)
{
   FIXME("implement me :)))\n");
   return 0;
}


UINT
SHEW_AddOpenWithItem(SHEOWImpl *This, HKEY hKey)
{
  
    UINT NumItems = 0;
    DWORD dwIndex = 0;
    DWORD dwName, dwValue;
    LONG result = ERROR_SUCCESS;
    WCHAR szName[10];
    WCHAR szValue[MAX_PATH];
    WCHAR szMRUList[MAX_PATH] = {0};

    static const WCHAR szMRU[] = {'M','R','U','L','i','s','t', 0 };

    while(result == ERROR_SUCCESS)
    {
        dwName = sizeof(szName);
        dwValue = sizeof(szValue);
        

        result = RegEnumValueW(hKey, 
                               dwIndex, 
                               szName, 
                               &dwName, 
                               NULL,
                               NULL,
                               (LPBYTE)szValue,
                               &dwValue);
        szName[9] = 0;
        szValue[MAX_PATH-1] = 0;

        if (result == ERROR_SUCCESS)
        {
            if (wcsicmp(szValue, szMRU))
            {
                SHEOW_AddOWItem(This, szValue);    
                NumItems++;
            }
            else
            {
                wcscpy(szMRUList, szValue);
            }
        }
        dwIndex++;
    }

    if (szMRUList[0])
    {
        FIXME("handle MRUList\n");
    }
    return NumItems;
}



UINT 
SHEOW_LoadItemFromHKCR(SHEOWImpl *This, WCHAR * szExt)
{
    HKEY hKey;
    HKEY hSubKey;
    LONG result;
    UINT NumKeys = 0;
    WCHAR szBuffer[30];
    WCHAR szResult[70];
    DWORD dwSize;

    static const WCHAR szCROW[] = { 'O','p','e','n','W','i','t','h','L','i','s','t', 0 };
    static const WCHAR szCROP[] = { 'O','p','e','n','W','i','t','h','P','r','o','g','I','D','s',0 };
    static const WCHAR szPT[] = { 'P','e','r','c','e','i','v','e','d','T','y','p','e', 0 };
    static const WCHAR szSys[] = { 'S','y','s','t','e','m','F','i','l','e','A','s','s','o','c','i','a','t','i','o','n','s','\\','%','s','\\','O','p','e','n','W','i','t','h','L','i','s','t', 0 };


    TRACE("SHEOW_LoadItemFromHKCR entered with This %p szExt %s\n",This, debugstr_w(szExt));

    result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                          szExt,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);
    if (result != ERROR_SUCCESS)
        return NumKeys;

    result = RegOpenKeyExW(hKey,
                          szCROW,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys = SHEW_AddOpenWithItem(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    result = RegOpenKeyExW(hKey,
                          szCROP,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys += SHEW_AddOpenWithProgId(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    dwSize = sizeof(szBuffer);

    result = RegGetValueW(hKey,
                          NULL,
                          szPT,
                          RRF_RT_REG_SZ,
                          NULL,
                          szBuffer,
                          &dwSize);
    szBuffer[29] = 0;

    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return NumKeys;
    }

    sprintfW(szResult, szSys, szExt);
    result = RegOpenKeyExW(hKey,
                          szCROW,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys += SHEW_AddOpenWithProgId(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);
    return NumKeys;
}




UINT
SHEOW_LoadItemFromHKCU(SHEOWImpl *This, WCHAR * szExt)
{
    WCHAR szBuffer[MAX_PATH];
    HKEY hKey;
    UINT KeyCount = 0;
    LONG result;
    
    static const WCHAR szOWPL[] = { 'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','F','i','l','e','E','x','t','s',
        '\\','%','s','\\','O','p','e','n','W','i','t','h','P','r','o','g','I','D','s',0 };

    static const WCHAR szOpenWith[] = { 'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','F','i','l','e','E','x','t','s',
        '\\','%','s','\\','O','p','e','n','W','i','t','h','L','i','s','t', 0 };

    TRACE("SHEOW_LoadItemFromHKCU entered with This %p szExt %s\n",This, debugstr_w(szExt));

   /* process HKCU settings */
   sprintfW(szBuffer, szOWPL, szExt);
   TRACE("szBuffer %s\n", debugstr_w(szBuffer));
   result = RegOpenKeyExW(HKEY_CURRENT_USER,
                          szBuffer,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);

   if (result == ERROR_SUCCESS)
   {
       KeyCount = SHEW_AddOpenWithProgId(This, hKey);
       RegCloseKey(hKey);
   }

   sprintfW(szBuffer, szOpenWith, szExt);
   TRACE("szBuffer %s\n", debugstr_w(szBuffer));
   result = RegOpenKeyExW(HKEY_CURRENT_USER,
                          szBuffer,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);

   if (result == ERROR_SUCCESS)
   {
       KeyCount += SHEW_AddOpenWithItem(This, hKey);
       RegCloseKey(hKey);
   }
   return KeyCount;
}

HRESULT
SHEOW_LoadOpenWithItems(SHEOWImpl *This, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    FORMATETC fmt;
    HRESULT hr;
    LPIDA pida;
    LPCITEMIDLIST pidl_folder;
    LPCITEMIDLIST pidl_child; 
    LPCITEMIDLIST pidl; 
    WCHAR szPath[MAX_PATH];
    LPWSTR szPtr;
    static const WCHAR szShortCut[] = { '.','l','n','k', 0 };

    fmt.cfFormat = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = IDataObject_GetData(pdtobj, &fmt, &medium);

    if (FAILED(hr))
    {
        ERR("IDataObject_GetData failed with 0x%x\n", hr);
        return hr;
    }

        /*assert(pida->cidl==1);*/
    pida = (LPIDA)GlobalLock(medium.u.hGlobal);

    pidl_folder = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
    pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

    pidl = ILCombine(pidl_folder, pidl_child);

    GlobalUnlock(medium.u.hGlobal);
    GlobalFree(medium.u.hGlobal);

    if (!pidl)
    {
        ERR("no mem\n");
        return E_OUTOFMEMORY;
    }
    if (_ILIsFolder(pidl_child))
    {
        TRACE("pidl is a folder\n");
        SHFree(pidl);
        return E_FAIL;
    }

    if (!SHGetPathFromIDListW(pidl, szPath))
    {
        SHFree(pidl);
        ERR("SHGetPathFromIDListW failed\n");
        return E_FAIL;
    }
    
    SHFree(pidl);    
    TRACE("szPath %s\n", debugstr_w(szPath));

    szPtr = wcschr(szPath, '.');
    if (szPtr)
    {
        if (!_wcsicmp(szPtr, szShortCut))
        {
            TRACE("pidl is a shortcut\n");
            return E_FAIL;
        }

        SHEOW_LoadItemFromHKCU(This, szPtr);
        SHEOW_LoadItemFromHKCR(This, szPtr);
    }
    TRACE("count %u\n", This->count);
    return S_OK;
}




static HRESULT WINAPI
SHEOW_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);

    TRACE("This %p\n", This);

    return SHEOW_LoadOpenWithItems(This, pdtobj);
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
