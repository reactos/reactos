/*
 *	IContextMenu
 *	ShellView Background Context Menu (shv_bg_cm)
 *
 *	Copyright 1999	Juergen Schmied <juergen.schmied@metronet.de>
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
*  IContextMenu Implementation
*/
typedef struct
{
	const IContextMenu2Vtbl *lpVtbl;
	IShellFolder*	pSFParent;
	LONG		ref;
	BOOL		bDesktop;
    UINT iIdShellNewFirst;
    UINT iIdShellNewLast;
} BgCmImpl;

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


static const IContextMenu2Vtbl cmvt;

static PSHELLNEW_ITEM s_SnHead = NULL;

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
LoadShellNewItems()
{
  DWORD dwIndex;
  WCHAR szName[MAX_PATH];
  LONG result;
  PSHELLNEW_ITEM pNewItem;
  PSHELLNEW_ITEM pCurItem = NULL;
  static WCHAR szLnk[] = { '.','l','n','k',0 };

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
                if (s_SnHead)
                {
                    pNewItem->Next = s_SnHead;
                    s_SnHead = pNewItem;
                }
                else
                {
                   s_SnHead = pCurItem = pNewItem;
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
                   pCurItem = s_SnHead = pNewItem;
                }
            }
        }
     }
     dwIndex++;
  }while(result != ERROR_NO_MORE_ITEMS);

  if (s_SnHead == NULL)
      return FALSE;
  else
      return TRUE;
}
VOID
InsertShellNewItems(HMENU hMenu, UINT idFirst, UINT idMenu, BgCmImpl * This)
{
  MENUITEMINFOW mii;
  PSHELLNEW_ITEM pCurItem;
  UINT i;
  if (s_SnHead == NULL)
  {
    if (!LoadShellNewItems())
        return;

  }

  ZeroMemory(&mii, sizeof(mii));
  mii.cbSize = sizeof(mii);

  This->iIdShellNewFirst = idFirst;
  
  /*
   * FIXME: small hack for new shortcut
   */

  mii.fMask = MIIM_ID;
  mii.wID = idFirst;
  SetMenuItemInfoW(hMenu, 1, TRUE, &mii);
  idFirst++;

  mii.fMask = MIIM_TYPE | MIIM_ID;
  mii.fType = MFT_SEPARATOR;
  mii.wID = -1;
  InsertMenuItemW(hMenu, -1, TRUE, &mii);


  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
  mii.fType = MFT_OWNERDRAW;
  mii.fState = MFS_ENABLED;

  pCurItem = s_SnHead;
  i = 0;

  while(pCurItem)
  {
    if (i >= 1)
    {
       mii.dwTypeData = pCurItem->szDesc;
       mii.cch = strlenW(mii.dwTypeData);
       mii.wID = idFirst;
       InsertMenuItemW(hMenu, idMenu, TRUE, &mii);
       idMenu++;
       idFirst++;
    }
    pCurItem = pCurItem->Next;
    i++;
  }
  This->iIdShellNewLast = idFirst;
}
VOID
DoShellNewCmd(BgCmImpl * This, LPCMINVOKECOMMANDINFO lpcmi)
{
  PSHELLNEW_ITEM pCurItem = s_SnHead;
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

  static const WCHAR szNew[] = { 'N','e','w',' ',0 }; //FIXME
  static const WCHAR szP1[] = { '%', '1', 0 };
  static const WCHAR szFormat[] = {'%','s',' ','(','%','d',')','%','s',0 };

  i = This->iIdShellNewFirst;
  target = LOWORD(lpcmi->lpVerb);

  while(pCurItem)
  {
    if (i == target)
        break;

    pCurItem = pCurItem->Next;
    i++;
  }

  if (!pCurItem)
      return;

  if (This->bDesktop)
  {
     if (!SHGetSpecialFolderPathW(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
     {
        ERR("Failed to get desktop folder location");
        return;
     }
  }
  else
  {
     if (IShellFolder2_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&psf) != S_OK)
     {
        ERR("Failed to get interface IID_IPersistFolder2\n");
        return;
     }
     if (IPersistFolder2_GetCurFolder(psf, &pidl) != S_OK)
     {
        ERR("IPersistFolder2_GetCurFolder failed\n");
        return;
     }

     if (IShellFolder2_GetDisplayNameOf(This->pSFParent, pidl, SHGDN_FORPARSING, &strTemp) != S_OK)
     {
        ERR("IShellFolder_GetDisplayNameOf failed\n");
        return;
     }
     StrRetToBufW(&strTemp, pidl, szPath, MAX_PATH);
  }
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
            return;

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
}
HRESULT
DoMeasureItem(BgCmImpl *This, HWND hWnd, MEASUREITEMSTRUCT * lpmis)
{
   PSHELLNEW_ITEM pCurItem;
   PSHELLNEW_ITEM pItem;
   UINT i;
   HDC hDC;
   SIZE size;
   
   TRACE("DoMeasureItem entered with id %x\n", lpmis->itemID);
   
   pCurItem = s_SnHead;

   i = This->iIdShellNewFirst;
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
      return E_FAIL;

   hDC = GetDC(hWnd);
   GetTextExtentPoint32W(hDC, pCurItem->szDesc, strlenW(pCurItem->szDesc), &size);
   lpmis->itemWidth = size.cx + 32;
   lpmis->itemHeight = max(size.cy, 20);
   ReleaseDC (hWnd, hDC);
   return S_OK;
}

HRESULT
DoDrawItem(BgCmImpl *This, HWND hWnd, DRAWITEMSTRUCT * drawItem)
{
   PSHELLNEW_ITEM pCurItem;
   PSHELLNEW_ITEM pItem;
   UINT i;
   pCurItem = s_SnHead;

   TRACE("DoDrawItem entered with id %x\n", drawItem->itemID);

   i = This->iIdShellNewFirst;
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
*   ISVBgCm_Constructor()
*/
IContextMenu2 *ISvBgCm_Constructor(IShellFolder* pSFParent, BOOL bDesktop)
{
	BgCmImpl* cm;

	cm = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(BgCmImpl));
	cm->lpVtbl = &cmvt;
	cm->ref = 1;
	cm->pSFParent = pSFParent;
	cm->bDesktop = bDesktop;
	if(pSFParent) IShellFolder_AddRef(pSFParent);

	TRACE("(%p)->()\n",cm);
	return (IContextMenu2*)cm;
}

/**************************************************************************
*  ISVBgCm_fnQueryInterface
*/
static HRESULT WINAPI ISVBgCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

        if(IsEqualIID(riid, &IID_IUnknown) ||
           IsEqualIID(riid, &IID_IContextMenu) ||
           IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IShellExtInit))  /*IShellExtInit*/
	{
	  FIXME("-- LPSHELLEXTINIT pointer requested\n");
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

/**************************************************************************
*  ISVBgCm_fnAddRef
*/
static ULONG WINAPI ISVBgCm_fnAddRef(IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

/**************************************************************************
*  ISVBgCm_fnRelease
*/
static ULONG WINAPI ISVBgCm_fnRelease(IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

/**************************************************************************
* ISVBgCm_fnQueryContextMenu()
*/

static HRESULT WINAPI ISVBgCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    HMENU	hMyMenu;
    UINT	idMax;
    MENUITEMINFOW mii;
    HRESULT hr;

    BgCmImpl *This = (BgCmImpl *)iface;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);


    hMyMenu = LoadMenuA(shell32_hInstance, "MENU_002");
    if (uFlags & CMF_DEFAULTONLY)
    {
        HMENU ourMenu = GetSubMenu(hMyMenu,0);
        UINT oldDef = GetMenuDefaultItem(hMenu,TRUE,GMDI_USEDISABLED);
        UINT newDef = GetMenuDefaultItem(ourMenu,TRUE,GMDI_USEDISABLED);
        if (newDef != oldDef)
            SetMenuDefaultItem(hMenu,newDef,TRUE);
        if (newDef!=0xFFFFFFFF)
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, newDef+1);
        else
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    else
    {
        idMax = Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu,
                                  idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);
        hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst+1);
    }
    DestroyMenu(hMyMenu);

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfoW(hMenu, 10, TRUE, &mii))
    {
      InsertShellNewItems(mii.hSubMenu, 0x6000, 0x6000, This);
    }

    TRACE("(%p)->returning 0x%x\n",This,hr);
    return hr;
}

/**************************************************************************
* DoNewFolder
*/
static void DoNewFolder(
	IContextMenu2 *iface,
	IShellView *psv)
{
	BgCmImpl *This = (BgCmImpl *)iface;
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

/**************************************************************************
* DoPaste
*/
static BOOL DoPaste(
	IContextMenu2 *iface)
{
	BgCmImpl *This = (BgCmImpl *)iface;
	BOOL bSuccess = FALSE;
	IDataObject * pda;

	TRACE("\n");

	if(SUCCEEDED(OleGetClipboard(&pda)))
	{
	  STGMEDIUM medium;
	  FORMATETC formatetc;

	  TRACE("pda=%p\n", pda);

	  /* Set the FORMATETC structure*/
	  InitFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);

	  /* Get the pidls from IDataObject */
	  if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
          {
	    LPITEMIDLIST * apidl;
	    LPITEMIDLIST pidl;
	    IShellFolder *psfFrom = NULL, *psfDesktop;

	    LPIDA lpcida = GlobalLock(medium.u.hGlobal);
	    TRACE("cida=%p\n", lpcida);

	    apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

	    /* bind to the source shellfolder */
	    SHGetDesktopFolder(&psfDesktop);
	    if(psfDesktop)
	    {
	      IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom);
	      IShellFolder_Release(psfDesktop);
	    }

	    if (psfFrom)
	    {
	      /* get source and destination shellfolder */
	      ISFHelper *psfhlpdst, *psfhlpsrc;
	      IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (LPVOID*)&psfhlpdst);
	      IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (LPVOID*)&psfhlpsrc);

	      /* do the copy/move */
	      if (psfhlpdst && psfhlpsrc)
	      {
	        ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);
		/* FIXME handle move
		ISFHelper_DeleteItems(psfhlpsrc, lpcida->cidl, apidl);
		*/
	      }
	      if(psfhlpdst) ISFHelper_Release(psfhlpdst);
	      if(psfhlpsrc) ISFHelper_Release(psfhlpsrc);
	      IShellFolder_Release(psfFrom);
	    }

	    _ILFreeaPidl(apidl, lpcida->cidl);
	    SHFree(pidl);

	    /* release the medium*/
	    ReleaseStgMedium(&medium);
	  }
	  IDataObject_Release(pda);
	}
#if 0
	HGLOBAL  hMem;

	OpenClipboard(NULL);
	hMem = GetClipboardData(CF_HDROP);

	if(hMem)
	{
	  char * pDropFiles = (char *)GlobalLock(hMem);
	  if(pDropFiles)
	  {
	    int len, offset = sizeof(DROPFILESTRUCT);

	    while( pDropFiles[offset] != 0)
	    {
	      len = strlen(pDropFiles + offset);
	      TRACE("%s\n", pDropFiles + offset);
	      offset += len+1;
	    }
	  }
	  GlobalUnlock(hMem);
	}
	CloseClipboard();
#endif
	return bSuccess;
}

/**************************************************************************
* ISVBgCm_fnInvokeCommand()
*/
static HRESULT WINAPI ISVBgCm_fnInvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW lpSV = NULL;
	HWND hWndSV = 0;

	TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

	/* get the active IShellView */
	if((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
	  if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
	  {
	    IShellView_GetWindow(lpSV, &hWndSV);
	  }
	}

	  if(HIWORD(lpcmi->lpVerb))
	  {
	    TRACE("%s\n",lpcmi->lpVerb);

	    if (! strcmp(lpcmi->lpVerb,CMDSTR_NEWFOLDERA))
	    {
                DoNewFolder(iface, lpSV);
	    }
	    else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWLISTA))
	    {
	      if(hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW,0),0 );
	    }
	    else if (! strcmp(lpcmi->lpVerb,CMDSTR_VIEWDETAILSA))
	    {
	      if(hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW,0),0 );
	    }
	    else
	    {
	      FIXME("please report: unknown verb %s\n",lpcmi->lpVerb);
	    }
	  }
	  else
	  {
	    switch(LOWORD(lpcmi->lpVerb))
	    {
	      case FCIDM_SHVIEW_REFRESH:
	        if (lpSV) IShellView_Refresh(lpSV);
	        break;

	      case FCIDM_SHVIEW_NEWFOLDER:
	        DoNewFolder(iface, lpSV);
		break;

	      case FCIDM_SHVIEW_INSERT:
	        DoPaste(iface);
	        break;

	      case FCIDM_SHVIEW_PROPERTIES:
		if (This->bDesktop) {
		    ShellExecuteA(lpcmi->hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
		} else {
		    FIXME("launch item properties dialog\n");
		}
		break;

	      default:
            if (LOWORD(lpcmi->lpVerb) >= This->iIdShellNewFirst && LOWORD(lpcmi->lpVerb) <= This->iIdShellNewLast)
            {
                DoShellNewCmd(This, lpcmi);
                break;
            }

	        /* if it's an id just pass it to the parent shv */
	        if (hWndSV) SendMessageA(hWndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0),0 );
		break;
	    }
	  }

        if (lpSV)
	  IShellView_Release(lpSV);	/* QueryActiveShellView does AddRef */

	return NOERROR;
}

/**************************************************************************
 *  ISVBgCm_fnGetCommandString()
 *
 */
static HRESULT WINAPI ISVBgCm_fnGetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
	BgCmImpl *This = (BgCmImpl *)iface;

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existence of the menu items, the file dialog enables
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
	    {
	      return NOERROR;
	    }
	  }
	}

	FIXME("unknown command string\n");
	return E_FAIL;
}

/**************************************************************************
* ISVBgCm_fnHandleMenuMsg()
*/
static HRESULT WINAPI ISVBgCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    BgCmImpl *This = (BgCmImpl *)iface;
    DRAWITEMSTRUCT * lpids = (DRAWITEMSTRUCT*) lParam;
    MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*) lParam;

	TRACE("ISVBgCm_fnHandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    switch(uMsg)
    {
       case WM_MEASUREITEM:
          if (lpmis->itemID >= This->iIdShellNewFirst && lpmis->itemID <= This->iIdShellNewLast)
             return DoMeasureItem(This, (HWND)wParam, lpmis);
          break;
       case WM_DRAWITEM:
          if (lpmis->itemID >= This->iIdShellNewFirst && lpmis->itemID <= This->iIdShellNewLast)
             return DoDrawItem(This, (HWND)wParam, lpids);
          break;
    }

	return E_NOTIMPL;
}

/**************************************************************************
* IContextMenu2 VTable
*
*/
static const IContextMenu2Vtbl cmvt =
{
	ISVBgCm_fnQueryInterface,
	ISVBgCm_fnAddRef,
	ISVBgCm_fnRelease,
	ISVBgCm_fnQueryContextMenu,
	ISVBgCm_fnInvokeCommand,
	ISVBgCm_fnGetCommandString,
	ISVBgCm_fnHandleMenuMsg
};
