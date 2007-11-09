/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides new shell item service
 * PROGRAMMERS: Johannes Anderwald (janderwald@reactos.org)
 */

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
//#define YDEBUG
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "shlobj.h"
#include "shtypes.h"
#include "shell32_main.h"
#include "shellfolder.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "stdio.h"
#include "winuser.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef enum
{
   SHELLNEW_TYPE_COMMAND = 1,
   SHELLNEW_TYPE_DATA = 2,
   SHELLNEW_TYPE_FILENAME = 4,
   SHELLNEW_TYPE_NULLFILE = 8
}SHELLNEW_TYPE;


typedef struct __SHELLNEW_ITEM__
{
  SHELLNEW_TYPE Type;
  LPWSTR szExt;
  LPWSTR szTarget;
  LPWSTR szDesc;
  LPWSTR szIcon;
  struct __SHELLNEW_ITEM__ * Next;
}SHELLNEW_ITEM, *PSHELLNEW_ITEM;

typedef struct
{
    const IContextMenu2Vtbl *lpVtblContextMenu;
    LPSHELLFOLDER pSFParent;
    PSHELLNEW_ITEM s_SnHead;
}INewMenuImpl;

static const GUID CLSID_NewMenu = {0xD969A300, 0xE7FF, 0x11d0, {0xA9, 0x3B, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19} };
static const IContextMenu2Vtbl cmvt;
static WCHAR szNew[100];


static
BOOL
GetKeyDescription(LPWSTR szKeyName, LPWSTR szResult)
{
  HKEY hKey;
  DWORD dwDesc, dwError;
  WCHAR szDesc[100];

  static const WCHAR szFriendlyTypeName[] = { '\\','F','r','i','e','n','d','l','y','T','y','p','e','N','a','m','e',0 };

  TRACE("GetKeyDescription: keyname %s\n", debugstr_w(szKeyName));

  if (RegOpenKeyExW(HKEY_CLASSES_ROOT,szKeyName,0, KEY_READ | KEY_QUERY_VALUE,&hKey) != ERROR_SUCCESS)
      return FALSE;

  if (RegLoadMUIStringW(hKey,szFriendlyTypeName,szResult,MAX_PATH,&dwDesc,0,NULL) == ERROR_SUCCESS)
  {
      TRACE("result %s\n", debugstr_w(szResult));
      RegCloseKey(hKey);
      return TRUE;
  }
  /* fetch default value */
  dwDesc = sizeof(szDesc);
  dwError = RegGetValueW(hKey,NULL,NULL, RRF_RT_REG_SZ,NULL,szDesc,&dwDesc);
  if(dwError == ERROR_SUCCESS)
  {
     if (wcsncmp(szDesc, szKeyName, dwDesc / sizeof(WCHAR)))
     {
        /* recurse for to a linked key */
        if (!GetKeyDescription(szDesc, szResult))
        {
           /* use description */
           wcscpy(szResult, szDesc);
        }
     }
     else
     {
        /* use default value as description */
        wcscpy(szResult, szDesc);
     }
  }
  else
  {
     /* registry key w/o default key?? */
     TRACE("RegGetValue failed with %x\n", dwError);
     wcscpy(szResult, szKeyName);
  }

  RegCloseKey(hKey);
  return TRUE;
}


PSHELLNEW_ITEM LoadItem(LPWSTR szKeyName)
{
  HKEY hKey;
  DWORD dwIndex;
  WCHAR szName[MAX_PATH];
  WCHAR szCommand[MAX_PATH];
  WCHAR szDesc[MAX_PATH] = {0};
  WCHAR szIcon[MAX_PATH] = {0};
  DWORD dwName, dwCommand;
  LONG result;
  PSHELLNEW_ITEM pNewItem;
  
  static const WCHAR szShellNew[] = { '\\','S','h','e','l','l','N','e','w',0 };
  static const WCHAR szCmd[] = { 'C','o','m','m','a','n','d',0 };
  static const WCHAR szData[] = { 'D','a','t','a',0 };
  static const WCHAR szFileName[] = { 'F','i','l','e','N','a','m','e', 0 };
  static const WCHAR szNullFile[] = { 'N','u','l','l','F','i','l','e', 0 };


  wcscpy(szName, szKeyName);
  GetKeyDescription(szKeyName, szDesc);
  wcscat(szName, szShellNew);
  result = RegOpenKeyExW(HKEY_CLASSES_ROOT,szName,0,KEY_READ,&hKey);

  //TRACE("LoadItem dwName %d keyname %s szName %s szDesc %s szIcon %s\n", dwName, debugstr_w(szKeyName), debugstr_w(szName), debugstr_w(szDesc), debugstr_w(szIcon));

  if (result != ERROR_SUCCESS)
  {
     return NULL;
  }

  dwIndex = 0;
  pNewItem = NULL;

  do
  {
     dwName = MAX_PATH;
     dwCommand = MAX_PATH;
     result = RegEnumValueW(hKey,dwIndex,szName,&dwName,NULL,NULL,(LPBYTE)szCommand, &dwCommand);
     if (result == ERROR_SUCCESS)
     {
         long type = -1;
         LPWSTR szTarget = szCommand;
         //TRACE("szName %s szCommand %s\n", debugstr_w(szName), debugstr_w(szCommand));
         if (!wcsicmp(szName, szCmd))
         {
            type = SHELLNEW_TYPE_COMMAND;
         }else if (!wcsicmp(szName, szData))
         {
             type = SHELLNEW_TYPE_DATA;
         }
         else if (!wcsicmp(szName, szFileName))
         {
            type = SHELLNEW_TYPE_FILENAME;
         }
         else if (!wcsicmp(szName, szNullFile))
         {
            type = SHELLNEW_TYPE_NULLFILE;
            szTarget = NULL;
         }
         if (type != -1)
         {
            pNewItem = HeapAlloc(GetProcessHeap(), 0, sizeof(SHELLNEW_ITEM));
            pNewItem->Type = type;
            if (szTarget)
                pNewItem->szTarget = wcsdup(szTarget);
            else
                pNewItem->szTarget = NULL;

            pNewItem->szDesc = wcsdup(szDesc);
            pNewItem->szIcon = wcsdup(szIcon);
            pNewItem->szExt = wcsdup(szKeyName);
            pNewItem->Next = NULL;
            break;
         }
     }
     dwIndex++;
  }while(result != ERROR_NO_MORE_ITEMS);
  RegCloseKey(hKey);
  return pNewItem;
}


BOOL
LoadShellNewItems(INewMenuImpl * This)
{
  DWORD dwIndex;
  WCHAR szName[MAX_PATH];
  LONG result;
  PSHELLNEW_ITEM pNewItem;
  PSHELLNEW_ITEM pCurItem = NULL;
  static WCHAR szLnk[] = { '.','l','n','k',0 };

  /* insert do new folder action */
  if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, szNew, sizeof(szNew) / sizeof(WCHAR)))
      szNew[0] = 0;
  szNew[199] = 0;

  dwIndex = 0;
  do
  {
     result = RegEnumKeyW(HKEY_CLASSES_ROOT,dwIndex,szName,MAX_PATH);
     if (result == ERROR_SUCCESS)
     {
        pNewItem = LoadItem(szName);
        if (pNewItem)
        {
            if (!wcsicmp(pNewItem->szExt, szLnk))
            {
                if (This->s_SnHead)
                {
                    pNewItem->Next = This->s_SnHead;
                    This->s_SnHead = pNewItem;
                }
                else
                {
                   This->s_SnHead = pCurItem = pNewItem;
                }
            }
            else
            {
                if (pCurItem)
                {
                   pCurItem->Next = pNewItem;
                   pCurItem = pNewItem;
                }
                else
                {
                   pCurItem = This->s_SnHead = pNewItem;
                }
            }
        }
     }
     dwIndex++;
  }while(result != ERROR_NO_MORE_ITEMS);

  if (This->s_SnHead == NULL)
      return FALSE;
  else
      return TRUE;
}

UINT
InsertShellNewItems(HMENU hMenu, UINT idFirst, UINT idMenu, INewMenuImpl * This)
{
  MENUITEMINFOW mii;
  PSHELLNEW_ITEM pCurItem;
  UINT i;
  WCHAR szBuffer[100];

  if (This->s_SnHead == NULL)
  {
    if (!LoadShellNewItems(This))
        return 0;
  }

  ZeroMemory(&mii, sizeof(mii));
  mii.cbSize = sizeof(mii);

  /* insert do new shortcut action */
  if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWFOLDER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
      szBuffer[0] = 0;
  szBuffer[199] = 0;
  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
  mii.fType = MFT_STRING;
  mii.dwTypeData = szNew;
  mii.cch = strlenW(mii.dwTypeData);
  mii.wID = idFirst++;
  InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);

  /* insert do new shortcut action */
  if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWLINK, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
      szBuffer[0] = 0;
  szBuffer[199] = 0;
  mii.dwTypeData = szBuffer;
  mii.cch = strlenW(mii.dwTypeData);
  mii.wID = idFirst++;
  InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);
  
  /* insert seperator for custom new action */
  mii.fMask = MIIM_TYPE | MIIM_ID;
  mii.fType = MFT_SEPARATOR;
  mii.wID = -1;
  InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);

  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
  /*
   * FIXME 
   * implement loading of icons
   * and using MFT_OWNERDRAWN
   */
  mii.fType = MFT_STRING;
  mii.fState = MFS_ENABLED;

  pCurItem = This->s_SnHead;
  i = 0;

  while(pCurItem)
  {
    if (i >= 1)
    {
       TRACE("szDesc %s\n", debugstr_w(pCurItem->szDesc));
       mii.dwTypeData = pCurItem->szDesc;
       mii.cch = strlenW(mii.dwTypeData);
       mii.wID = idFirst++;
       InsertMenuItemW(hMenu, idMenu++, TRUE, &mii);
    }
    pCurItem = pCurItem->Next;
    i++;
  }
  return (i+2);
}

HRESULT
DoShellNewCmd(INewMenuImpl * This, LPCMINVOKECOMMANDINFO lpcmi)
{
  PSHELLNEW_ITEM pCurItem = This->s_SnHead;
  IPersistFolder3 * psf;
  LPITEMIDLIST pidl;
  STRRET strTemp;
  WCHAR szTemp[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  WCHAR szPath[MAX_PATH];
  STARTUPINFOW sInfo;
  PROCESS_INFORMATION pi;
  UINT i, target;
  HANDLE hFile;
  DWORD dwWritten, dwError;

  static const WCHAR szP1[] = { '%', '1', 0 };
  static const WCHAR szFormat[] = {'%','s',' ','(','%','d',')','%','s',0 };

  i = 1;
  target = LOWORD(lpcmi->lpVerb);

  while(pCurItem)
  {
    if (i == target)
        break;

    pCurItem = pCurItem->Next;
    i++;
  }

  if (!pCurItem)
      return E_UNEXPECTED;

  if (IShellFolder2_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&psf) != S_OK)
  {
      ERR("Failed to get interface IID_IPersistFolder2\n");
      return E_FAIL;
  }
  if (IPersistFolder2_GetCurFolder(psf, &pidl) != S_OK)
  {
      ERR("IPersistFolder2_GetCurFolder failed\n");
      return E_FAIL;
  }

  if (IShellFolder2_GetDisplayNameOf(This->pSFParent, pidl, SHGDN_FORPARSING, &strTemp) != S_OK)
  {
      ERR("IShellFolder_GetDisplayNameOf failed\n");
      return E_FAIL;
  }
  StrRetToBufW(&strTemp, pidl, szPath, MAX_PATH);

  switch(pCurItem->Type)
  {
     case SHELLNEW_TYPE_COMMAND:
     {
         LPWSTR ptr;
         LPWSTR szCmd;

         if (!ExpandEnvironmentStringsW(pCurItem->szTarget, szBuffer, MAX_PATH))
         {
             TRACE("ExpandEnvironmentStrings failed\n");
             break;
         }

         ptr = wcsstr(szBuffer, szP1);
         if (ptr)
         {
            ptr[1] = 's';
            sprintfW(szTemp, szBuffer, szPath);
            ptr = szTemp;
         }
         else
         {
            ptr = szBuffer;
         }

         ZeroMemory(&sInfo, sizeof(sInfo));
         sInfo.cb = sizeof(sizeof(sInfo));
         szCmd = wcsdup(ptr);
         if (!szCmd)
             break;
         if (CreateProcessW(NULL, szCmd, NULL, NULL,FALSE,0,NULL,NULL,&sInfo, &pi))
         {
           CloseHandle( pi.hProcess );
           CloseHandle( pi.hThread );
         }
         free(szCmd);
        break;
     }
     case SHELLNEW_TYPE_DATA:
     case SHELLNEW_TYPE_FILENAME:
     case SHELLNEW_TYPE_NULLFILE:
     {
        i = 2;

        PathAddBackslashW(szPath);
        wcscat(szPath, szNew);
        wcscat(szPath, L" ");
        wcscat(szPath, pCurItem->szDesc);
        wcscpy(szBuffer, szPath);
        wcscat(szBuffer, pCurItem->szExt);
        do
        {
            hFile = CreateFileW(szBuffer, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
                break;
            dwError = GetLastError();

            TRACE("FileName %s szBuffer %s i %u error %x\n", debugstr_w(szBuffer), debugstr_w(szPath), i, dwError);
            sprintfW(szBuffer, szFormat, szPath, i, pCurItem->szExt);
            i++;
        }while(hFile == INVALID_HANDLE_VALUE && dwError == ERROR_FILE_EXISTS);

        if (hFile == INVALID_HANDLE_VALUE)
            return E_FAIL;

        if (pCurItem->Type == SHELLNEW_TYPE_DATA)
        {
            i = WideCharToMultiByte(CP_ACP, 0, pCurItem->szTarget, -1, (LPSTR)szTemp, MAX_PATH*2, NULL, NULL);
            if (i)
            {
                WriteFile(hFile, (LPCVOID)szTemp, i, &dwWritten, NULL);
            }
        }
        CloseHandle(hFile);
        if (pCurItem->Type == SHELLNEW_TYPE_FILENAME)
        {
            if (!CopyFileW(pCurItem->szTarget, szBuffer, FALSE))
                break;
        }
        TRACE("Notifying fs %s\n", debugstr_w(szBuffer));
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, (LPCVOID)szBuffer, NULL);
        break;
     }
  }
  return S_OK;
}
/**************************************************************************
* DoMeasureItem
*/
HRESULT
DoMeasureItem(INewMenuImpl *This, HWND hWnd, MEASUREITEMSTRUCT * lpmis)
{
   PSHELLNEW_ITEM pCurItem;
   PSHELLNEW_ITEM pItem;
   UINT i;
   HDC hDC;
   SIZE size;
   
   TRACE("DoMeasureItem entered with id %x\n", lpmis->itemID);
   
   pCurItem = This->s_SnHead;

   i = 1;
   pItem = NULL;
   while(pCurItem)
   {
      if (i == lpmis->itemID)
      {
         pItem = pCurItem;
         break;
      }
      pCurItem = pCurItem->Next;
      i++;
   }

   if (!pItem)
   {
       TRACE("DoMeasureItem no item found\n");    
       return E_FAIL;
   }
   hDC = GetDC(hWnd);
   GetTextExtentPoint32W(hDC, pCurItem->szDesc, strlenW(pCurItem->szDesc), &size);
   lpmis->itemWidth = size.cx + 32;
   lpmis->itemHeight = max(size.cy, 20);
   ReleaseDC (hWnd, hDC);
   return S_OK;
}
/**************************************************************************
* DoDrawItem
*/
HRESULT
DoDrawItem(INewMenuImpl *This, HWND hWnd, DRAWITEMSTRUCT * drawItem)
{
   PSHELLNEW_ITEM pCurItem;
   PSHELLNEW_ITEM pItem;
   UINT i;
   pCurItem = This->s_SnHead;

   TRACE("DoDrawItem entered with id %x\n", drawItem->itemID);

   i = 1;
   pItem = NULL;
   while(pCurItem)
   {
      if (i == drawItem->itemID)
      {
         pItem = pCurItem;
         break;
      }
      pCurItem = pCurItem->Next;
      i++;
   }

   if (!pItem)
      return E_FAIL;
   
   drawItem->rcItem.left += 20;
   
   DrawTextW(drawItem->hDC, pCurItem->szDesc, wcslen(pCurItem->szDesc), &drawItem->rcItem, 0);
   return S_OK;
}

/**************************************************************************
* DoNewFolder
*/
static void DoNewFolder(
	INewMenuImpl *This,
	IShellView *psv)
{
	ISFHelper * psfhlp;
	WCHAR wszName[MAX_PATH];

	IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlp);
	if (psfhlp)
	{
	  LPITEMIDLIST pidl;
	  ISFHelper_GetUniqueName(psfhlp, wszName, MAX_PATH);
	  ISFHelper_AddFolder(psfhlp, 0, wszName, &pidl);

	  if(psv)
	  {
	    /* if we are in a shellview do labeledit */
	    IShellView_SelectItem(psv,
                    pidl,(SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                    |SVSI_FOCUSED|SVSI_SELECT));
	  }
	  SHFree(pidl);

	  ISFHelper_Release(psfhlp);
	}
}


#if 0
static inline INewMenuImpl *impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (INewMenuImpl *)((char*)iface - FIELD_OFFSET(INewMenuImpl, lpvtblShellExtInit));
}
#endif

static inline INewMenuImpl *impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (INewMenuImpl *)((char*)iface - FIELD_OFFSET(INewMenuImpl, lpVtblContextMenu));
}

static HRESULT WINAPI INewItem_fnQueryInterface(INewMenuImpl * This, REFIID riid, LPVOID *ppvObj)
{
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

     if(IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IContextMenu) ||
        IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = &This->lpVtblContextMenu;
	}
#if 0
	else if(IsEqualIID(riid, &IID_IShellExtInit))
	{
	  *ppvObj = &This->lpvtblShellExtInit;
	}
#endif

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI INewItem_fnAddRef(INewMenuImpl *iface)
{
    /* INewItem service is singleton */
    return 2;
}

static ULONG WINAPI INewItem_fnRelease(INewMenuImpl *This)
{
    /* INewItem service is singleton */
    return 1;
}

static
HRESULT
WINAPI
INewItem_IContextMenu_fnQueryInterface( IContextMenu2* iface, REFIID riid, void** ppvObject )
{
    INewMenuImpl *This = impl_from_IContextMenu(iface);
    return INewItem_fnQueryInterface(This, riid, ppvObject);
}

static
ULONG
WINAPI
INewItem_IContextMenu_fnAddRef(IContextMenu2 *iface)
{
    INewMenuImpl *This = impl_from_IContextMenu(iface);
    return INewItem_fnAddRef(This);
}

static
ULONG
WINAPI
INewItem_IContextMenu_fnRelease(IContextMenu2 *iface)
{
    INewMenuImpl *This = impl_from_IContextMenu(iface);
    return INewItem_fnRelease(This);
}

static
HRESULT
WINAPI
INewItem_IContextMenu_fnQueryContextMenu(IContextMenu2 *iface,
	                                            HMENU hmenu,
	                                            UINT indexMenu,
	                                            UINT idCmdFirst,
	                                            UINT idCmdLast,
	                                            UINT uFlags)
{
    WCHAR szBuffer[200];
    MENUITEMINFOW mii;
    HMENU hSubMenu;
    int id = 1;
    INewMenuImpl *This = impl_from_IContextMenu(iface);

    TRACE("%p %p %u %u %u %u\n", This,
          hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, szBuffer, 200))
    {
        szBuffer[0] = 0;
    }
    szBuffer[199] = 0;

    hSubMenu = CreateMenu();
    memset( &mii, 0, sizeof(mii) );
    mii.cbSize = sizeof (mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.wID = idCmdFirst + id++;
    mii.dwTypeData = szBuffer;
    mii.cch = strlenW( mii.dwTypeData );
    mii.fState = MFS_ENABLED;

    if (hSubMenu)
    {
        id += InsertShellNewItems( hSubMenu, idCmdFirst, 0, This);
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = hSubMenu;
    }


    if (!InsertMenuItemW( hmenu, indexMenu, TRUE, &mii ))
        return E_FAIL;

    return MAKE_HRESULT( SEVERITY_SUCCESS, 0, id );
}

static
HRESULT
WINAPI
INewItem_IContextMenu_fnInvokeCommand( IContextMenu2* iface, 
                                              LPCMINVOKECOMMANDINFO lpici )
{
	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW lpSV = NULL;
    HRESULT hr;
    INewMenuImpl *This = impl_from_IContextMenu(iface);

	if((lpSB = (LPSHELLBROWSER)SendMessageA(lpici->hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  IShellBrowser_QueryActiveShellView(lpSB, &lpSV);
    }

    if (LOWORD(lpici->lpVerb) == 0)
    {
        DoNewFolder(This, lpSV);
    }

    hr = DoShellNewCmd(This, lpici);
    if (SUCCEEDED(hr) && lpSV)
    {
        IShellView_Refresh(lpSV);
    }

    TRACE("INewItem_IContextMenu_fnInvokeCommand %x\n", hr);
    return hr;
}

static
HRESULT
WINAPI
INewItem_IContextMenu_fnGetCommandString( IContextMenu2* iface,
                                                 UINT_PTR idCmd,
                                                 UINT uType,
                                                 UINT* pwReserved,
                                                 LPSTR pszName,
                                                 UINT cchMax )
{
    INewMenuImpl *This = impl_from_IContextMenu(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static 
HRESULT 
WINAPI 
INewItem_IContextMenu_fnHandleMenuMsg(IContextMenu2 *iface,
	                     UINT uMsg,
	                     WPARAM wParam,
	                     LPARAM lParam)
{
    INewMenuImpl *This = impl_from_IContextMenu(iface);
    DRAWITEMSTRUCT * lpids = (DRAWITEMSTRUCT*) lParam;
    MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*) lParam;

	TRACE("INewItem_IContextMenu_fnHandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

#if 0
    switch(uMsg)
    {
       case WM_MEASUREITEM:
             return DoMeasureItem(This, (HWND)wParam, lpmis);
          break;
       case WM_DRAWITEM:
             return DoDrawItem(This, (HWND)wParam, lpids);
          break;
    }
#else
    return S_OK;
#endif

	return E_UNEXPECTED;
}

static const IContextMenu2Vtbl cmvt =
{
	INewItem_IContextMenu_fnQueryInterface,
	INewItem_IContextMenu_fnAddRef,
	INewItem_IContextMenu_fnRelease,
	INewItem_IContextMenu_fnQueryContextMenu,
	INewItem_IContextMenu_fnInvokeCommand,
	INewItem_IContextMenu_fnGetCommandString,
	INewItem_IContextMenu_fnHandleMenuMsg
};

HRESULT WINAPI INewItem_Constructor(LPSHELLFOLDER pSFParent, REFIID riid, LPVOID *ppv)
{
    INewMenuImpl * ow;
    HRESULT res;
    static INewMenuImpl *cached_ow;

    if (!cached_ow)
    {
	    ow = LocalAlloc(LMEM_ZEROINIT, sizeof(INewMenuImpl));
	    if (!ow)
        {
            return E_OUTOFMEMORY;
        }

        ow->lpVtblContextMenu = &cmvt;
        ow->s_SnHead = NULL;

        if (InterlockedCompareExchangePointer((void *)&cached_ow, ow, NULL) != NULL)
        {
            /* some other thread already been here */
            LocalFree( ow );
        }
    }

    TRACE("(%p)->()\n",cached_ow);
    if (cached_ow->pSFParent)
    {
        IShellFolder_Release(cached_ow->pSFParent);
    }
    cached_ow->pSFParent = pSFParent;
    IShellFolder_AddRef(pSFParent);

    res = INewItem_fnQueryInterface( cached_ow, riid, ppv );
    return res;
}
