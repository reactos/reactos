/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides default context menu implementation
 * PROGRAMMERS: Johannes Anderwald (janderwald@reactos.org)
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

#include "shlwapi.h"
#include "shell32_main.h"
#include "shellfolder.h"
#include "debughlp.h"
#include "shresdef.h"
#include "shlguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmenu);

typedef struct _DynamicShellEntry_
{
    UINT iIdCmdFirst;
    UINT NumIds;
    IContextMenu * CMenu;
    struct _DynamicShellEntry_ * Next;
}DynamicShellEntry, *PDynamicShellEntry;

typedef struct _StaticShellEntry_
{
   LPWSTR szVerb;
   LPWSTR szClass;
   struct _StaticShellEntry_ * Next;
}StaticShellEntry, *PStaticShellEntry;


typedef struct
{
	const IContextMenu2Vtbl *lpVtbl;
	LONG		ref;
    DEFCONTEXTMENU dcm;
    IDataObject * pDataObj;
    DWORD bGroupPolicyActive;
    PDynamicShellEntry dhead; /* first dynamic shell extension entry */
    UINT           iIdSHEFirst; /* first used id */
    UINT           iIdSHELast; /* last used id */
    PStaticShellEntry shead; /* first static shell extension entry */
    UINT           iIdSCMFirst; /* first static used id */
    UINT           iIdSCMLast; /* last static used id */
}IDefaultContextMenuImpl;

static inline IDefaultContextMenuImpl *impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (IDefaultContextMenuImpl *)((char*)iface - FIELD_OFFSET(IDefaultContextMenuImpl, lpVtbl));
}

static
HRESULT
WINAPI
IDefaultContextMenu_fnQueryInterface(
	IContextMenu2 *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
	IDefaultContextMenuImpl *This = (IDefaultContextMenuImpl *)iface;

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IContextMenu) ||
       IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = This;
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


static
ULONG
WINAPI
IDefaultContextMenu_fnAddRef(
	IContextMenu2 *iface)
{
	IDefaultContextMenuImpl *This = (IDefaultContextMenuImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static
ULONG
WINAPI
IDefaultContextMenu_fnRelease(
	IContextMenu2 *iface)
{
	IDefaultContextMenuImpl *This = (IDefaultContextMenuImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount + 1);
    if (!refCount)
    {
	  TRACE(" destroying IContextMenu(%p)\n",This);

	  HeapFree(GetProcessHeap(),0,This);
    }

	return refCount;
}

static
void
SH_AddStaticEntry(IDefaultContextMenuImpl * This, HKEY hKey, WCHAR *szVerb, WCHAR * szClass)
{
  PStaticShellEntry curEntry;
  PStaticShellEntry lastEntry = NULL;

  curEntry = This->shead;

  while(curEntry)
  {
    if (!wcsicmp(curEntry->szVerb, szVerb))
    {
       /* entry already exists */
       return;
    }
    lastEntry = curEntry;
    curEntry = curEntry->Next;
  }
  
  TRACE("adding verb %s szClass %s\n", debugstr_w(szVerb), debugstr_w(szClass));

  curEntry = malloc(sizeof(StaticShellEntry));
  if (curEntry)
  {
      curEntry->Next = NULL;
      curEntry->szVerb = wcsdup(szVerb);
      curEntry->szClass = wcsdup(szClass);
  }

  if (lastEntry)
  {
    lastEntry->Next = curEntry;
  }
  else
  {
    This->shead = curEntry;
  }
}

static
void
SH_AddStaticEntryForKey(IDefaultContextMenuImpl * This, HKEY hKey, WCHAR * szClass)
{
    LONG result;
    DWORD dwIndex;
    WCHAR szName[40];
    DWORD dwName;

    dwIndex = 0;
    do
    {
        szName[0] = 0;
        dwName = sizeof(szName) / sizeof(WCHAR);
        result = RegEnumKeyExW(hKey, dwIndex, szName, &dwName, NULL, NULL, NULL, NULL);
        szName[39] = 0;   
        if (result == ERROR_SUCCESS)
        {
            SH_AddStaticEntry(This, hKey, szName, szClass);
        }
        dwIndex++;
    }while(result == ERROR_SUCCESS);
}

static
void
SH_AddStaticEntryForFileClass(IDefaultContextMenuImpl * This, WCHAR * szExt)
{
    WCHAR szBuffer[100];
    HKEY hKey;
    LONG result;
    DWORD dwBuffer;

    TRACE("SH_AddStaticEntryForFileClass entered with %s\n", debugstr_w(szExt));

    wcscpy(szBuffer, szExt);
    wcscat(szBuffer, L"\\shell");
    result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        SH_AddStaticEntryForKey(This, hKey, szExt);
        RegCloseKey(hKey);
    }

    dwBuffer = sizeof(szBuffer);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, NULL, RRF_RT_REG_SZ, NULL, (LPBYTE)szBuffer, &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        UINT length = strlenW(szBuffer);
        wcscat(szBuffer, L"\\shell");
        TRACE("szBuffer %s\n", debugstr_w(szBuffer));

        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
           szBuffer[length] = 0;
           SH_AddStaticEntryForKey(This, hKey, szBuffer);
           RegCloseKey(hKey);
        }
    }

    strcpyW(szBuffer, "SystemFileAssociations\\");
    dwBuffer = sizeof(szBuffer) - strlenW(szBuffer) * sizeof(WCHAR);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, L"PerceivedType", RRF_RT_REG_SZ, NULL, (LPBYTE)&szBuffer[23], &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        UINT length = strlenW(szBuffer);
        wcscat(szBuffer, L"\\shell");
        TRACE("szBuffer %s\n", debugstr_w(szBuffer));

        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
           szBuffer[length] = 0;
           SH_AddStaticEntryForKey(This, hKey, szBuffer);
           RegCloseKey(hKey);
        }
    }
    RegCloseKey(hKey);
}

static
BOOL 
HasClipboardData()
{
    BOOL ret = FALSE;
    IDataObject * pda;

    if(SUCCEEDED(OleGetClipboard(&pda)))
    {
      STGMEDIUM medium;
      FORMATETC formatetc;

      TRACE("pda=%p\n", pda);

      /* Set the FORMATETC structure*/
      InitFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
      if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
      {
          ret = TRUE;
      }

      IDataObject_Release(pda);
      ReleaseStgMedium(&medium);
    }

    return ret;
}

VOID
DisablePasteOptions(HMENU hMenu)
{
    MENUITEMINFOW mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_DISABLED;

    TRACE("result %d\n", SetMenuItemInfoW(hMenu, FCIDM_SHVIEW_INSERT, FALSE, &mii));
    TRACE("result %d\n", SetMenuItemInfoW(hMenu, FCIDM_SHVIEW_INSERTLINK, FALSE, &mii));
}

static
HRESULT
SH_LoadDynamicContextMenuHandler(IDefaultContextMenuImpl * This, HKEY hKey, const CLSID * szClass, BOOL bExternalInit)
{
  HRESULT hr;
  IContextMenu * cmobj;
  IShellExtInit *shext;
  PDynamicShellEntry curEntry;
  TRACE("SH_LoadDynamicContextMenuHandler entered with This %p hKey %p szClass %s bExternalInit %u\n",This, hKey, wine_dbgstr_guid(szClass), bExternalInit);

  hr = SHCoCreateInstance(NULL, szClass, NULL, &IID_IContextMenu, (void**)&cmobj);
  if (hr != S_OK)
  {
      TRACE("SHCoCreateInstance failed %x\n", GetLastError());
      return hr;
  }

  if (bExternalInit)
  {
      hr = cmobj->lpVtbl->QueryInterface(cmobj, &IID_IShellExtInit, (void**)&shext);
      if (hr != S_OK)
      {
        TRACE("Failed to query for interface IID_IShellExtInit\n");
        cmobj->lpVtbl->Release(cmobj);
        return FALSE;
      }
      hr = shext->lpVtbl->Initialize(shext, NULL, This->pDataObj, hKey);
      shext->lpVtbl->Release(shext);
      if (hr != S_OK)
      {
        TRACE("Failed to initialize shell extension error %x\n", hr);
        cmobj->lpVtbl->Release(cmobj);
        return hr;
      }
  }

  curEntry = malloc(sizeof(DynamicShellEntry));
  if(!curEntry)
  {
      return E_OUTOFMEMORY;
  }

  curEntry->iIdCmdFirst = 0;
  curEntry->Next = NULL;
  curEntry->NumIds = 0;
  curEntry->CMenu = cmobj;

  if (This->dhead)
  {
      PDynamicShellEntry pEntry = This->dhead;

      while(pEntry->Next)
      {
         pEntry = pEntry->Next;
      }

      pEntry->Next = curEntry;
  }
  else
  {
      This->dhead = curEntry;
  }

  return hr;
}

static
UINT
EnumerateDynamicContextHandlerForKey(IDefaultContextMenuImpl *This, HKEY hRootKey)
{
   WCHAR szKey[MAX_PATH] = {0};
   WCHAR szName[MAX_PATH] = {0};
   DWORD dwIndex, dwName;
   LONG res;
   HRESULT hResult;
   UINT index;
   CLSID clsid;
   HKEY hKey;

   static const WCHAR szShellEx[] = { 's','h','e','l','l','e','x','\\','C','o','n','t','e','x','t','M','e','n','u','H','a','n','d','l','e','r','s',0 };

   if (RegOpenKeyExW(hRootKey, szShellEx, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
   {
      TRACE("RegOpenKeyExW failed for key %s\n", debugstr_w(szKey));
      return 0;
   }

   dwIndex = 0;
   index = 0;
   do
   {
      dwName = MAX_PATH;
      res = RegEnumKeyExW(hKey, dwIndex, szName, &dwName, NULL, NULL, NULL, NULL);
      if (res == ERROR_SUCCESS)
      {
         hResult = CLSIDFromString(szName, &clsid);
         if (hResult != S_OK)
         {
             dwName = MAX_PATH;
             if (RegGetValueW(hKey, szName, NULL, RRF_RT_REG_SZ, NULL, szKey, &dwName) == ERROR_SUCCESS)
             {
                if (CLSIDFromString(szKey, &clsid) == S_OK)
                {
                    if (This->bGroupPolicyActive)
                    {
                        if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                                         szKey,
                                         RRF_RT_REG_SZ,
                                         NULL,
                                         NULL,
                                         &dwName) == ERROR_SUCCESS)
                        {
                            SH_LoadDynamicContextMenuHandler(This, hKey, &clsid, TRUE);
                        }

                    }
                    else
                    {
                        SH_LoadDynamicContextMenuHandler(This, hKey, &clsid, TRUE);
                    }
                }
             }
         }
         if (hResult == S_OK)
         {
            if (This->bGroupPolicyActive)
            {
                if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                                 szKey,
                                 RRF_RT_REG_SZ,
                                 NULL,
                                NULL,
                                &dwName) == ERROR_SUCCESS)
                {
                    SH_LoadDynamicContextMenuHandler(This, hKey, &clsid, TRUE);
                }
            }
            else
            {
                SH_LoadDynamicContextMenuHandler(This, hKey, &clsid, TRUE);
            }
         }
      }
      dwIndex++;
   }while(res == ERROR_SUCCESS);

   RegCloseKey(hKey);
   return index;
}


static 
UINT
InsertMenuItemsOfDynamicContextMenuExtension(IDefaultContextMenuImpl * This, HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    PDynamicShellEntry curEntry;
    HRESULT hResult;
    MENUITEMINFOW mii;
    if (!This->dhead)
    {
        This->iIdSHEFirst = 0;
        This->iIdSHELast = 0;
        return indexMenu;
    }

    curEntry = This->dhead;
    This->iIdSHEFirst = idCmdFirst;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_STATE;
    mii.fType = MFT_SEPARATOR;
    mii.fState = MFS_ENABLED;
    InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);

    do
    {
        hResult = IContextMenu_QueryContextMenu(curEntry->CMenu, hMenu, indexMenu, idCmdFirst, idCmdLast, CMF_NORMAL);
        if (SUCCEEDED(hResult))
        {
            curEntry->iIdCmdFirst = idCmdFirst;
            curEntry->NumIds = LOWORD(hResult);
            indexMenu += curEntry->NumIds;
            idCmdFirst += curEntry->NumIds + 0x10;
        }
        TRACE("curEntry %p hresult %x contextmenu %p cmdfirst %x num ids %x\n", curEntry, hResult, curEntry->CMenu, curEntry->iIdCmdFirst, curEntry->NumIds);
        curEntry = curEntry->Next;
    }while(curEntry);

    This->iIdSHELast = idCmdFirst;
    TRACE("SH_LoadContextMenuHandlers first %x last %x\n", This->iIdSHEFirst, This->iIdSHELast);
    return indexMenu;
}

UINT
BuildBackgroundContextMenu(
    IDefaultContextMenuImpl * This,
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast,
    UINT uFlags)
{
    MENUITEMINFOW mii;
    WCHAR szBuffer[MAX_PATH];
    UINT indexMenu = 0;
    HMENU hSubMenu;
    HKEY hKey;

    ZeroMemory(&mii, sizeof(mii));

    TRACE("BuildBackgroundContextMenu entered\n");

    if (!_ILIsDesktop(This->dcm.pidlFolder))
    {
        /* view option is only available in browsing mode */
        hSubMenu = LoadMenuA(shell32_hInstance, "MENU_001");
        if (hSubMenu)
        {
            szBuffer[0] = 0;
            LoadStringW(shell32_hInstance, FCIDM_SHVIEW_VIEW, szBuffer, MAX_PATH);
            szBuffer[MAX_PATH-1] = 0;
            
            TRACE("szBuffer %s\n", debugstr_w(szBuffer));

            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.wID = iIdCmdFirst++;
            mii.dwTypeData = szBuffer;
            mii.cch = strlenW( mii.dwTypeData );
            mii.fState = MFS_ENABLED;
            mii.hSubMenu = hSubMenu;
            InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);
            DestroyMenu(hSubMenu);
        }
    }
    hSubMenu = LoadMenuA(shell32_hInstance, "MENU_002");
    if (hSubMenu)
    {
        /* merge general background context menu in */
        iIdCmdFirst = Shell_MergeMenus(hMenu, hSubMenu, indexMenu, 0, iIdCmdLast, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS) + 1;
        DestroyMenu(hSubMenu);
    }

    if (!HasClipboardData())
    {
        TRACE("disabling paste options\n");
        DisablePasteOptions(hMenu);
    }
    /* load extensions from HKCR\* key */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                     L"*",
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        EnumerateDynamicContextHandlerForKey(This, hKey);
        RegCloseKey(hKey);
    }

    /* load create new shell extension */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                     L"CLSID\\{D969A300-E7FF-11d0-A93B-00A0C90F2719}",
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        static const GUID dummy1 = {0xD969A300, 0xE7FF, 0x11d0, {0xA9, 0x3B, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19} };
        SH_LoadDynamicContextMenuHandler(This, hKey, &dummy1, FALSE);
        RegCloseKey(hKey);
    }
    
    InsertMenuItemsOfDynamicContextMenuExtension(This, hMenu, GetMenuItemCount(hMenu), iIdCmdFirst, iIdCmdLast);
    return iIdCmdLast;
}

static
UINT
AddStaticContextMenusToMenu(
    HMENU hMenu,
    UINT indexMenu,
    IDefaultContextMenuImpl * This)
{
    MENUITEMINFOW mii;
    PStaticShellEntry curEntry;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED | MFS_DEFAULT;
    mii.wID = 0x4000;
    This->iIdSCMFirst = mii.wID;

    curEntry = This->shead;

    while(curEntry)
    {
       /* FIXME
        * load localized verbs if its an open edit find print printto openas properties verb
        */

       mii.dwTypeData = curEntry->szVerb;
       mii.cch = strlenW(mii.dwTypeData);
       InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);
       mii.fState = MFS_ENABLED;
       mii.wID++;
       curEntry = curEntry->Next;
    }
    This->iIdSCMLast = mii.wID - 1;
    return indexMenu;
}

static
const char * 
GetLocalizedString(
    HMENU hMenu,
    UINT wID,
    char * sResult)
{
   MENUITEMINFOA mii;

   sResult[0] = 0;
   if (!hMenu)
   {
       return sResult;
   }

   mii.cbSize = sizeof(mii);
   mii.fMask = MIIM_TYPE;
   mii.fType = MFT_STRING;
   mii.dwTypeData = sResult;
   mii.cch = 100;

   GetMenuItemInfoA(hMenu, wID, FALSE, &mii);
   return sResult;
}
#if 0
UINT
BuildControlPanelContextMenu(
    IDefaultContextMenuImpl * This,
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast)
{
    GUID * guid;
    SFGAOF attributes;
    DWORD dwSize;
    UINT indexMenu = 0;
    LPOLESTR pwszCLSID;
    WCHAR szClass[80];
    UINT length;
    MENUITEMINFOW mii;


    ZeroMemory(&mii, sizeof(MENUITEMINFOW));

    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;

    guid = _ILGetGUIDPointer(This->dcm.apidl[0]);
    if (guid) 
    {
        /* the cpl is registered as a namespace extension */
        dwSize = sizeof(SFGAOF);
        wcscpy(szClass, L"CLSID\\");
        if (SUCCEEDED(StringFromCLSID(guid, &pwszCLSID)))
        {
            wcscpy(&szClass[6], pwszCLSID);
            length = strlenW(szClass);

            CoTaskMemFree(pwszCLSID);

            /* check if the CLSID is a folder */
            wcscat(szClass, L"\\ShellFolder");

            if (RegGetValueW(HKEY_CLASSES_ROOT, szClass, L"Attributes", RRF_RT_REG_BINARY, NULL, &attributes, &dwSize) == ERROR_SUCCESS)
            {
                if (attributes & SFGAO_FOLDER)
                {
                    WCHAR szBuffer[100];
                    /* insert explore string */
                    LoadStringW(shell32_hInstance, FCIDM_SHVIEW_EXPLORE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
                    szBuffer[99] = 0;
                    mii.wID = FCIDM_SHVIEW_EXPLORE;
                    mii.dwTypeData = szBuffer;
                    mii.fState |= MFS_DEFAULT;
                    InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);
                    mii.fState = MFS_ENABLED;
                }
            }

            /* add static handlers*/
            szClass[length] = 0;
            SH_AddStaticEntryForFileClass(This, szClass);         
            indexMenu = AddStaticContextMenusToMenu(hMenu, indexMenu, This);
        }
    }
    else
    {
        /* cpl is registered with full path */
        SH_AddStaticEntryForFileClass(This, L".cpl");
    }

    if (AppendMenu(hMenu, MF_SEPARATOR, -1, NULL))
    {
        LoadStringW(shell32_hInstance, FCIDM_SHVIEW_CREATELINK, szClass, sizeof(szClass) / sizeof(WCHAR));
        szClass[79] = 0;
        InsertMenuItem(hMenu, indexMenu++, TRUE, &mii);
    }
}
#endif

UINT
BuildShellItemContextMenu(
    IDefaultContextMenuImpl * This,
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast,
    UINT uFlags)
{
    WCHAR szExt[10];
    char sBuffer[100];
    HKEY hKey;
    UINT indexMenu;
    SFGAOF rfg;
    HRESULT hr;
    BOOL bAddSep;
    HMENU hLocalMenu;
    GUID * guid;

    TRACE("BuildShellItemContextMenu entered\n");

    if (_ILGetExtension(This->dcm.apidl[0], &sBuffer[1], sizeof(sBuffer)))
    {
        sBuffer[9] = 0;
        sBuffer[0] = '.';

        if (MultiByteToWideChar( CP_ACP, 0, sBuffer, -1, (LPWSTR)szExt, 10))
        {
            TRACE("szExt %d\n", debugstr_w(szExt));
            SH_AddStaticEntryForFileClass(This, szExt);
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szExt, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(This, hKey);
                RegCloseKey(hKey);
            }
        }
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"*", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* load default extensions */
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }

    if (!_ILIsPidlSimple(This->dcm.apidl[0])|| _ILIsFolder(This->dcm.apidl[0]))
    {
        /* add the default verbs open / explore */
        SH_AddStaticEntryForFileClass(This, L"Folder");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Folder", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }

    guid = _ILGetGUIDPointer(This->dcm.apidl[0]);
    if (guid)
    {
        LPOLESTR pwszCLSID;
        WCHAR buffer[60];

        wcscpy(buffer, L"CLSID\\");
        hr = StringFromCLSID(guid, &pwszCLSID);
        if (hr == S_OK)
        {
            wcscpy(&buffer[6], pwszCLSID);
            TRACE("buffer %s\n", debugstr_w(buffer));
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, buffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(This, hKey);
                SH_AddStaticEntryForFileClass(This, buffer);
                RegCloseKey(hKey);
            }
            CoTaskMemFree(pwszCLSID);
        }
    }


    if (_ILIsDrive(This->dcm.apidl[0]))
    {
        SH_AddStaticEntryForFileClass(This, L"Drive");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Drive", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }
    

    /* add static actions */
    rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_FILESYSTEM;
	hr = IShellFolder_GetAttributesOf(This->dcm.psf, This->dcm.cidl, This->dcm.apidl, &rfg);
    if (!SUCCEEDED(hr))
        rfg = 0;

    if (rfg & SFGAO_FILESYSTEM)
    {
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"AllFilesystemObjects", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* sendto service is registered there */
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }

    /* add static context menu handlers */
    indexMenu = AddStaticContextMenusToMenu(hMenu, 0, This);
    /* now process dynamic context menu handlers */
    indexMenu = InsertMenuItemsOfDynamicContextMenuExtension(This, hMenu, indexMenu, iIdCmdFirst, iIdCmdLast);
    TRACE("indexMenu %d\n", indexMenu);

    hLocalMenu = LoadMenuW(shell32_hInstance, L"MENU_SHV_FILE");
    if (!hLocalMenu)
    {
        return iIdCmdLast;
    }

	if (rfg & (SFGAO_CANCOPY | SFGAO_CANMOVE))
	{
	      _InsertMenuItem(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
          if (rfg & SFGAO_CANMOVE)
	          _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_CUT, sBuffer), MFS_ENABLED);
		      if (rfg & SFGAO_CANCOPY)
	          _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_COPY, sBuffer), MFS_ENABLED);
	}

    bAddSep = TRUE;
    if (rfg & SFGAO_CANLINK)
    {
        bAddSep = FALSE;
        _InsertMenuItem(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_CREATELINK, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_CREATELINK, sBuffer), MFS_ENABLED);
    }


    if (rfg & SFGAO_CANDELETE)
	{
        if (bAddSep)
        {
            bAddSep = FALSE;
            _InsertMenuItem(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_DELETE, sBuffer), MFS_ENABLED);
	}

    if (rfg & SFGAO_CANRENAME)
    {
        if (bAddSep)
        {
            bAddSep = FALSE;
            _InsertMenuItem(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_RENAME, sBuffer), MFS_ENABLED);
    }

    _InsertMenuItem(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
    _InsertMenuItem(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_PROPERTIES, MFT_STRING, GetLocalizedString(hLocalMenu, FCIDM_SHVIEW_PROPERTIES, sBuffer), MFS_ENABLED);

    DestroyMenu(hLocalMenu);
    return iIdCmdLast;
}

static
HRESULT
WINAPI
IDefaultContextMenu_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    IDefaultContextMenuImpl * This = impl_from_IContextMenu(iface);

    if (This->dcm.cidl)
    {
        idCmdFirst = BuildShellItemContextMenu(This, hmenu, idCmdFirst, idCmdLast, uFlags);
    }
    else
    {
        idCmdFirst = BuildBackgroundContextMenu(This, hmenu, idCmdFirst, idCmdLast, uFlags);
    }

    return S_OK;
}

static
HRESULT
NotifyShellViewWindow(LPCMINVOKECOMMANDINFO lpcmi)
{
	LPSHELLBROWSER	lpSB;
	LPSHELLVIEW lpSV = NULL;
    HWND hwndSV = NULL;

	if((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
	{
        if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
        {
            IShellView_GetWindow(lpSV, &hwndSV);
        }
	}

    if (LOWORD(lpcmi->lpVerb) == FCIDM_SHVIEW_REFRESH)
    {
        if (lpSV)
            IShellView_Refresh(lpSV);

        return S_OK;
    }

    SendMessageW(hwndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);

    return S_OK;
}

static
HRESULT
DoPaste(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}

static
HRESULT
DoOpenOrExplore(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}

static
HRESULT
DoCopyOrCut(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}

static
HRESULT
DoCreateLink(
	IDefaultContextMenuImpl *iface)
{


    return E_FAIL;
}

static
HRESULT
DoDelete(
	IDefaultContextMenuImpl *iface)
{


    return E_FAIL;
}

static
HRESULT
DoRename(
	IDefaultContextMenuImpl *iface)
{


    return E_FAIL;
}

static
HRESULT
DoProperties(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}

static
HRESULT
DoDynamicShellExtensions(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}


static
HRESULT
DoStaticShellExtensions(
	IDefaultContextMenuImpl *This,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    SHELLEXECUTEINFOW sei;
    PStaticShellEntry pCurrent = This->shead;
    int verb = LOWORD(lpcmi->lpVerb) - This->iIdSCMFirst;

    
    while(pCurrent && verb-- > 0)
       pCurrent = pCurrent->Next;

    if (verb > 0)
        return E_FAIL;


	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_CLASSNAME;
	sei.lpClass = pCurrent->szClass;
	sei.hwnd = lpcmi->hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = pCurrent->szVerb;
	ShellExecuteExW(&sei);
    return S_OK;    


}

static
HRESULT
WINAPI
IDefaultContextMenu_fnInvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    IDefaultContextMenuImpl * This = impl_from_IContextMenu(iface);

    switch(LOWORD(lpcmi->lpVerb))
    {
        case FCIDM_SHVIEW_BIGICON:
        case FCIDM_SHVIEW_SMALLICON:
        case FCIDM_SHVIEW_LISTVIEW:
        case FCIDM_SHVIEW_REPORTVIEW:
        case 0x30: /* FIX IDS in resource files */
        case 0x31:
        case 0x32:
        case 0x33:
        case FCIDM_SHVIEW_AUTOARRANGE:
        case FCIDM_SHVIEW_SNAPTOGRID:
        case FCIDM_SHVIEW_REFRESH:
            return NotifyShellViewWindow(lpcmi);
        case FCIDM_SHVIEW_INSERT:
        case FCIDM_SHVIEW_INSERTLINK:
            return DoPaste(This, lpcmi);
        case FCIDM_SHVIEW_OPEN:
        case FCIDM_SHVIEW_EXPLORE:
            return DoOpenOrExplore(This, lpcmi);
        case FCIDM_SHVIEW_COPY:
        case FCIDM_SHVIEW_CUT:
            return DoCopyOrCut(This, lpcmi);
        case FCIDM_SHVIEW_CREATELINK:
            return DoCreateLink(This);
        case FCIDM_SHVIEW_DELETE:
            return DoDelete(This);
        case FCIDM_SHVIEW_RENAME:
            return DoRename(This);
        case FCIDM_SHVIEW_PROPERTIES:
            return DoProperties(This, lpcmi);
    }

    if (This->iIdSHEFirst && This->iIdSHELast)
    {
        if (LOWORD(lpcmi->lpVerb) >= This->iIdSHEFirst && LOWORD(lpcmi->lpVerb) <= This->iIdSHELast)
        {
            return DoDynamicShellExtensions(This, lpcmi);
        }
    }

    if (This->iIdSCMFirst && This->iIdSCMLast)
    {
        if (LOWORD(lpcmi->lpVerb) >= This->iIdSCMFirst && LOWORD(lpcmi->lpVerb) <= This->iIdSCMLast)
        {
            return DoStaticShellExtensions(This, lpcmi);
        }
    }

    FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
    return E_UNEXPECTED;
}

static
HRESULT
WINAPI
IDefaultContextMenu_fnGetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{

    return S_OK;
}

static
HRESULT
WINAPI
IDefaultContextMenu_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{

    return S_OK;
}

static const IContextMenu2Vtbl cmvt =
{
	IDefaultContextMenu_fnQueryInterface,
	IDefaultContextMenu_fnAddRef,
	IDefaultContextMenu_fnRelease,
	IDefaultContextMenu_fnQueryContextMenu,
	IDefaultContextMenu_fnInvokeCommand,
	IDefaultContextMenu_fnGetCommandString,
	IDefaultContextMenu_fnHandleMenuMsg
};

static 
HRESULT
IDefaultContextMenu_Constructor( 
	const DEFCONTEXTMENU *pdcm,
	REFIID riid,
	void **ppv)
{
   IDefaultContextMenuImpl * This;
   HRESULT hr = E_FAIL;
   IDataObject * pDataObj;

   This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDefaultContextMenuImpl));
   if (This)
   {
       This->lpVtbl = &cmvt;
       This->ref = 1;
       TRACE("cidl %u\n", This->dcm.cidl);
       if (SUCCEEDED(SHCreateDataObject(pdcm->pidlFolder, pdcm->cidl, pdcm->apidl, NULL, &IID_IDataObject, (void**)&pDataObj)))
       {
            This->pDataObj = pDataObj;
       }
       CopyMemory(&This->dcm, pdcm, sizeof(DEFCONTEXTMENU));
       hr = IDefaultContextMenu_fnQueryInterface((IContextMenu2*)This, riid, ppv);
   }

   TRACE("This(%p)(%x) cidl %u\n",This, hr, This->dcm.cidl);
   return hr;
}


/*************************************************************************
 * SHCreateDefaultContextMenu			[SHELL32.325] Vista API
 *
 */

HRESULT
WINAPI
SHCreateDefaultContextMenu(
	const DEFCONTEXTMENU *pdcm,
	REFIID riid,
	void **ppv)
{
#if 1
   HRESULT hr = E_FAIL;

   *ppv = NULL;
   hr = IDefaultContextMenu_Constructor( pdcm, riid, ppv );

   TRACE("pcm %p hr %x\n", pdcm, hr);
   return hr;
#else
   HRESULT hr;
   IContextMenu2 * pcm;

   if (pdcm->cidl > 0)
      pcm = ISvItemCm_Constructor( pdcm->psf, pdcm->pidlFolder, pdcm->apidl, pdcm->cidl );
   else
      pcm = ISvBgCm_Constructor( pdcm->psf, TRUE );

   hr = S_OK;
   *ppv = pcm;

   return hr;
#endif
}

/*************************************************************************
 * CDefFolderMenu_Create2			[SHELL32.701]
 *
 */

INT
WINAPI
CDefFolderMenu_Create2(
	LPCITEMIDLIST pidlFolder,
	HWND hwnd,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	IShellFolder *psf,
	LPFNDFMCALLBACK lpfn,
	UINT nKeys,
	HKEY *ahkeyClsKeys,
	IContextMenu **ppcm)
{
   DEFCONTEXTMENU pdcm;
   HRESULT hr;

   pdcm.hwnd = hwnd;
   pdcm.pcmcb = NULL;
   pdcm.pidlFolder = pidlFolder;
   pdcm.psf = psf;
   pdcm.cidl = cidl;
   pdcm.apidl = apidl;
   pdcm.punkAssociationInfo = NULL;
   pdcm.cKeys = nKeys;
   pdcm.aKeys = ahkeyClsKeys;

   hr = SHCreateDefaultContextMenu(&pdcm, &IID_IContextMenu, (void**)ppcm);
   return hr;
}

