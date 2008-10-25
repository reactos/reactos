/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides default context menu implementation
 * PROGRAMMERS: Johannes Anderwald (janderwald@reactos.org)
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(dmenu);

typedef struct _DynamicShellEntry_
{
    UINT iIdCmdFirst;
    UINT NumIds;
    CLSID ClassID;
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
}IDefaultContextMenuImpl, *LPIDefaultContextMenuImpl;

static LPIDefaultContextMenuImpl __inline impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (LPIDefaultContextMenuImpl)((char*)iface - FIELD_OFFSET(IDefaultContextMenuImpl, lpVtbl));
}

VOID INewItem_SetCurrentShellFolder(IShellFolder * psfParent); // HACK


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
    PDynamicShellEntry dEntry, dNext;
    PStaticShellEntry sEntry, sNext;
    IDefaultContextMenuImpl *This = (IDefaultContextMenuImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(count=%u)\n", This, refCount + 1);
    if (!refCount)
    {
        /* free dynamic shell extension entries */
        dEntry = This->dhead;
        while(dEntry)
        {
            dNext = dEntry->Next;
            IContextMenu_Release(dEntry->CMenu);
            HeapFree(GetProcessHeap(), 0, dEntry);
            dEntry = dNext;
        }
        /* free static shell extension entries */
        sEntry = This->shead;
        while(sEntry)
        {
            sNext = sEntry->Next;
            HeapFree(GetProcessHeap(), 0, sEntry->szClass);
            HeapFree(GetProcessHeap(), 0, sEntry->szVerb);
            HeapFree(GetProcessHeap(), 0, sEntry);
            sEntry = sNext;
        } 
        HeapFree(GetProcessHeap(),0,This);
    }

    return refCount;
}

static
void
SH_AddStaticEntry(IDefaultContextMenuImpl * This, WCHAR *szVerb, WCHAR * szClass)
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

    curEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(StaticShellEntry));
    if (curEntry)
    {
        curEntry->Next = NULL;
        curEntry->szVerb = HeapAlloc(GetProcessHeap(), 0, (wcslen(szVerb)+1) * sizeof(WCHAR));
        if (curEntry->szVerb)
            wcscpy(curEntry->szVerb, szVerb);
        curEntry->szClass = HeapAlloc(GetProcessHeap(), 0, (wcslen(szClass)+1) * sizeof(WCHAR));
        if (curEntry->szClass)
            wcscpy(curEntry->szClass, szClass);
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
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = 0;
        if (result == ERROR_SUCCESS)
        {
            SH_AddStaticEntry(This, szName, szClass);
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
    UINT Length;
    static WCHAR szShell[] = L"\\shell";
    static WCHAR szShellAssoc[] = L"SystemFileAssociations\\";

    TRACE("SH_AddStaticEntryForFileClass entered with %s\n", debugstr_w(szExt));

    Length = wcslen(szExt);
    if (Length + (sizeof(szShell)/sizeof(WCHAR)) + 1 < sizeof(szBuffer)/sizeof(WCHAR))
    {
        wcscpy(szBuffer, szExt);
        wcscpy(&szBuffer[Length], szShell);
        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            szBuffer[Length] = 0;
            SH_AddStaticEntryForKey(This, hKey, szExt);
            RegCloseKey(hKey);
        }
    }

    dwBuffer = sizeof(szBuffer);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, NULL, RRF_RT_REG_SZ, NULL, (LPBYTE)szBuffer, &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        Length = wcslen(szBuffer);
        if (Length + (sizeof(szShell)/sizeof(WCHAR)) + 1 < sizeof(szBuffer)/sizeof(WCHAR))
        {
            wcscpy(&szBuffer[Length], szShell);
            TRACE("szBuffer %s\n", debugstr_w(szBuffer));

            result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
            if (result == ERROR_SUCCESS)
            {
                szBuffer[Length] = 0;
                SH_AddStaticEntryForKey(This, hKey, szBuffer);
                RegCloseKey(hKey);
            }
        }
    }

    wcscpy(szBuffer, szShellAssoc);
    dwBuffer = sizeof(szBuffer) - sizeof(szShellAssoc) - sizeof(WCHAR);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, L"PerceivedType", RRF_RT_REG_SZ, NULL, (LPBYTE)&szBuffer[(sizeof(szShellAssoc)/sizeof(WCHAR))], &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        Length = wcslen(&szBuffer[(sizeof(szShellAssoc)/sizeof(WCHAR))]) + (sizeof(szShellAssoc)/sizeof(WCHAR));
        wcscat(&szBuffer[(sizeof(szShellAssoc)/sizeof(WCHAR))], szShell);
        TRACE("szBuffer %s\n", debugstr_w(szBuffer));

        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
           szBuffer[Length] = 0;
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

BOOL
IsShellExtensionAlreadyLoaded(IDefaultContextMenuImpl * This, const CLSID * szClass)
{
    PDynamicShellEntry curEntry = This->dhead;

    while(curEntry)
    {
        if (!memcmp(&curEntry->ClassID, szClass, sizeof(CLSID)))
            return TRUE;
        curEntry = curEntry->Next;
    }
    return FALSE;
}


static
HRESULT
SH_LoadDynamicContextMenuHandler(IDefaultContextMenuImpl * This, HKEY hKey, const CLSID * szClass, BOOL bExternalInit)
{
  HRESULT hr;
  IContextMenu * cmobj;
  IShellExtInit *shext;
  PDynamicShellEntry curEntry;
  //WCHAR szTemp[100];
  LPOLESTR pstr;

  StringFromCLSID(szClass, &pstr);

  TRACE("SH_LoadDynamicContextMenuHandler entered with This %p hKey %p szClass %s bExternalInit %u\n",This, hKey, wine_dbgstr_guid(szClass), bExternalInit);
  //swprintf(szTemp, L"This %p hKey %p szClass %s bExternalInit %u", This, hKey, pstr, bExternalInit);
  //MessageBoxW(NULL, szTemp, NULL, MB_OK);

  if (IsShellExtensionAlreadyLoaded(This, szClass))
      return S_OK;

  hr = SHCoCreateInstance(NULL, szClass, NULL, &IID_IContextMenu, (void**)&cmobj);
  if (hr != S_OK)
  {
      TRACE("SHCoCreateInstance failed %x\n", GetLastError());
      return hr;
  }

  if (bExternalInit)
  {
      hr = IContextMenu_QueryInterface(cmobj, &IID_IShellExtInit, (void**)&shext);
      if (hr != S_OK)
      {
        TRACE("Failed to query for interface IID_IShellExtInit\n");
        IContextMenu_Release(cmobj);
        return FALSE;
      }
      hr = IShellExtInit_Initialize(shext, NULL, This->pDataObj, hKey);
      IShellExtInit_Release(shext);
      if (hr != S_OK)
      {
        TRACE("Failed to initialize shell extension error %x\n", hr);
        IContextMenu_Release(cmobj);
        return hr;
      }
  }

  curEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(DynamicShellEntry));
  if(!curEntry)
  {
      IContextMenu_Release(cmobj);
      return E_OUTOFMEMORY;
  }

  curEntry->iIdCmdFirst = 0;
  curEntry->Next = NULL;
  curEntry->NumIds = 0;
  curEntry->CMenu = cmobj;
  memcpy(&curEntry->ClassID, szClass, sizeof(CLSID));

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


  if (!memcmp(szClass, &CLSID_NewMenu, sizeof(CLSID)))
  {
      /* A REAL UGLY HACK */
     INewItem_SetCurrentShellFolder(This->dcm.psf);
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
                 hResult = CLSIDFromString(szKey, &clsid);
             }
         }
         if (SUCCEEDED(hResult))
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

    if (!This->dhead)
    {
        This->iIdSHEFirst = 0;
        This->iIdSHELast = 0;
        return indexMenu;
    }

    curEntry = This->dhead;
    idCmdFirst = 0x5000;
    idCmdLast =  0x6000;
    This->iIdSHEFirst = idCmdFirst;
    do
    {
        hResult = IContextMenu_QueryContextMenu(curEntry->CMenu, hMenu, indexMenu++, idCmdFirst, idCmdLast, CMF_NORMAL);
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
            mii.cch = wcslen( mii.dwTypeData );
            mii.fState = MFS_ENABLED;
            mii.hSubMenu = hSubMenu;
            InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);
            DestroyMenu(hSubMenu);
        }
    }
    hSubMenu = LoadMenuW(shell32_hInstance, L"MENU_002");
    if (hSubMenu)
    {
        /* merge general background context menu in */
        iIdCmdFirst = Shell_MergeMenus(hMenu, GetSubMenu(hSubMenu, 0), indexMenu, 0, 0xFFFF, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS) + 1;
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
        SH_LoadDynamicContextMenuHandler(This, hKey, &CLSID_NewMenu, TRUE);
        RegCloseKey(hKey);
    }
    
    if (InsertMenuItemsOfDynamicContextMenuExtension(This, hMenu, GetMenuItemCount(hMenu)-1, iIdCmdFirst, iIdCmdLast))
    {
        /* seperate dynamic context menu items */
        _InsertMenuItemW(hMenu, GetMenuItemCount(hMenu)-1, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    }

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
    UINT idResource;
    PStaticShellEntry curEntry;
    WCHAR szVerb[40];
    WCHAR szTemp[50];
    DWORD dwSize;
    UINT fState;
    UINT Length;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED | MFS_DEFAULT;
    mii.wID = 0x4000;
    This->iIdSCMFirst = mii.wID;

    curEntry = This->shead;

    while(curEntry)
    {
        fState = MFS_ENABLED;
        if (!wcsicmp(curEntry->szVerb, L"open"))
        {
           fState |= MFS_DEFAULT;
           idResource = IDS_OPEN_VERB;
        }
        else if (!wcsicmp(curEntry->szVerb, L"runas"))
           idResource = IDS_RUNAS_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"edit"))
           idResource = IDS_EDIT_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"find"))
           idResource = IDS_FIND_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"print"))
           idResource = IDS_PRINT_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"play"))
           idResource = IDS_PLAY_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"preview"))
           idResource = IDS_PREVIEW_VERB;
        else
           idResource = 0;

        if (idResource > 0)
        {
            if (LoadStringW(shell32_hInstance, idResource, szVerb, sizeof(szVerb)/sizeof(WCHAR)))
            {
                /* use translated verb */
                szVerb[(sizeof(szVerb)/sizeof(WCHAR))-1] = L'\0';
                mii.dwTypeData = szVerb;
            }
        }
        else
        {
            Length = wcslen(curEntry->szClass) + wcslen(curEntry->szVerb) + 8;
            if (Length < sizeof(szTemp)/sizeof(WCHAR))
            {
                wcscpy(szTemp, curEntry->szClass);
                wcscat(szTemp, L"\\shell\\");
                wcscat(szTemp, curEntry->szVerb);
                dwSize = sizeof(szVerb);

                if (RegGetValueW(HKEY_CLASSES_ROOT, szTemp, NULL, RRF_RT_REG_SZ, NULL, szVerb, &dwSize) == ERROR_SUCCESS)
                {
                    /* use description for the menu entry */
                    mii.dwTypeData = szVerb;
                }
                else
                {
                    /* use verb for the menu entry */
                    mii.dwTypeData = curEntry->szVerb;
                }
            }
           
        }

        mii.cch = wcslen(mii.dwTypeData);
        InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);
        mii.fState = fState;
        mii.wID++;
        curEntry = curEntry->Next;
     }
     This->iIdSCMLast = mii.wID - 1;
     return indexMenu;
}

void WINAPI _InsertMenuItemW (
    HMENU hmenu,
    UINT indexMenu,
    BOOL fByPosition,
    UINT wID,
    UINT fType,
    LPCWSTR dwTypeData,
    UINT fState)
{
    MENUITEMINFOW mii;
    WCHAR szText[100];

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    if (fType == MFT_SEPARATOR)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE;
    }
    else if (fType == MFT_STRING)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        if ((ULONG_PTR)HIWORD((ULONG_PTR)dwTypeData) == 0)
        {
            if (LoadStringW(shell32_hInstance, LOWORD((ULONG_PTR)dwTypeData), szText, sizeof(szText)/sizeof(WCHAR)))
            {
                szText[(sizeof(szText)/sizeof(WCHAR))-1] = 0;
                mii.dwTypeData = szText;
            }
            else
            {
                TRACE("failed to load string %p\n", dwTypeData);
                return;
            }
        }
        else
        {
            mii.dwTypeData = (LPWSTR) dwTypeData;
        }
        mii.fState = fState;
    }

    mii.wID = wID;
    mii.fType = fType;
    InsertMenuItemW( hmenu, indexMenu, fByPosition, &mii);
}

UINT
BuildShellItemContextMenu(
    IDefaultContextMenuImpl * This,
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast,
    UINT uFlags)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szTemp[40];
    HKEY hKey;
    UINT indexMenu;
    SFGAOF rfg;
    HRESULT hr;
    BOOL bAddSep;
    GUID * guid;
    BOOL bClipboardData;
    STRRET strFile;
    LPWSTR pOffset;
    DWORD dwSize;

    TRACE("BuildShellItemContextMenu entered\n");

    if (IShellFolder2_GetDisplayNameOf(This->dcm.psf, This->dcm.apidl[0], SHGDN_FORPARSING, &strFile) == S_OK)
    {
        if (StrRetToBufW(&strFile, This->dcm.apidl[0], szPath, MAX_PATH) == S_OK)
        {
            pOffset = wcsrchr(szPath, L'.');
            if (pOffset)
            {
                /* enumerate dynamic/static for a given file class */
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pOffset, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    /* add static verbs */
                    SH_AddStaticEntryForFileClass(This, pOffset);
                    /* load dynamic extensions from file extension key */
                    EnumerateDynamicContextHandlerForKey(This, hKey);
                    RegCloseKey(hKey);
                }
                dwSize = sizeof(szTemp);
                if (RegGetValueW(HKEY_CLASSES_ROOT, pOffset, NULL, RRF_RT_REG_SZ, NULL, szTemp, &dwSize) == ERROR_SUCCESS)
                {
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                        /* add static verbs from progid key */
                        SH_AddStaticEntryForFileClass(This, szTemp);
                        /* load dynamic extensions from progid key */
                        EnumerateDynamicContextHandlerForKey(This, hKey);
                        RegCloseKey(hKey);
                   }
                }
            }
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"*", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                /* load default extensions */
                EnumerateDynamicContextHandlerForKey(This, hKey);
                RegCloseKey(hKey);
            }
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
    rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_FILESYSTEM | SFGAO_FOLDER;
    hr = IShellFolder_GetAttributesOf(This->dcm.psf, This->dcm.cidl, This->dcm.apidl, &rfg);
    if (!SUCCEEDED(hr))
        rfg = 0;

    if (rfg & SFGAO_FOLDER)
    {
        /* add the default verbs open / explore */
        SH_AddStaticEntryForFileClass(This, L"Folder");
        SH_AddStaticEntryForFileClass(This, L"Directory");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Folder", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Directory", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }


    if (rfg & SFGAO_FILESYSTEM)
    {
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"AllFilesystemObjects", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* sendto service is registered here */
            EnumerateDynamicContextHandlerForKey(This, hKey);
            RegCloseKey(hKey);
        }
    }

    /* add static context menu handlers */
    indexMenu = AddStaticContextMenusToMenu(hMenu, 0, This);
    /* now process dynamic context menu handlers */
    indexMenu = InsertMenuItemsOfDynamicContextMenuExtension(This, hMenu, indexMenu, iIdCmdFirst, iIdCmdLast);
    TRACE("indexMenu %d\n", indexMenu);

    if (_ILIsDrive(This->dcm.apidl[0]))
    {
        /* The 'Format' option must be always available, 
         * thus it is not registered as a static shell extension 
         */
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0x7ABC, MFT_STRING, MAKEINTRESOURCEW(IDS_FORMATDRIVE), MFS_ENABLED);
        bAddSep = TRUE;
    }

    bClipboardData = (HasClipboardData() && (rfg & SFGAO_FILESYSTEM));
    if (rfg & (SFGAO_CANCOPY | SFGAO_CANMOVE) || bClipboardData)
    {
          _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
          if (rfg & SFGAO_CANMOVE)
              _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, MAKEINTRESOURCEW(IDS_CUT), MFS_ENABLED);
          if (rfg & SFGAO_CANCOPY)
             _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, MAKEINTRESOURCEW(IDS_COPY), MFS_ENABLED);
          if (bClipboardData)
             _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_INSERT, MFT_STRING, MAKEINTRESOURCEW(IDS_INSERT), MFS_ENABLED);

          bAddSep = TRUE;
    }


    if (rfg & SFGAO_CANLINK)
    {
        bAddSep = FALSE;
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_CREATELINK, MFT_STRING, MAKEINTRESOURCEW(IDS_CREATELINK), MFS_ENABLED);
    }


    if (rfg & SFGAO_CANDELETE)
    {
        if (bAddSep)
        {
            bAddSep = FALSE;
            _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, MAKEINTRESOURCEW(IDS_DELETE), MFS_ENABLED);
    }

    if (rfg & SFGAO_CANRENAME)
    {
        if (bAddSep)
        {
            _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, MAKEINTRESOURCEW(IDS_RENAME), MFS_ENABLED);
        bAddSep = TRUE;
    }

    if (rfg & SFGAO_HASPROPSHEET)
    {
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, FCIDM_SHVIEW_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
    }

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
NotifyShellViewWindow(LPCMINVOKECOMMANDINFO lpcmi, BOOL bRefresh)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV = NULL;
    HWND hwndSV = NULL;

    if((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
    {
        if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
        {
            IShellView_GetWindow(lpSV, &hwndSV);
        }
    }

    if (LOWORD(lpcmi->lpVerb) == FCIDM_SHVIEW_REFRESH || bRefresh)
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
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    IDataObject * pda;
    STGMEDIUM medium;
    FORMATETC formatetc;
    LPITEMIDLIST * apidl;
    LPITEMIDLIST pidl;
    IShellFolder *psfFrom = NULL, *psfDesktop, *psfTarget = NULL;
    LPIDA lpcida;
    ISFHelper *psfhlpdst, *psfhlpsrc;
    HRESULT hr;

    if (OleGetClipboard(&pda) != S_OK)
        return E_FAIL;

    InitFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    hr = IDataObject_GetData(pda,&formatetc,&medium);

    if (FAILED(hr))
    {
        IDataObject_Release(pda);
        return E_FAIL;
    }

    /* lock the handle */
    lpcida = GlobalLock(medium.u.hGlobal);
    if (!lpcida)
    {
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);
        return E_FAIL;
    }

    /* convert the data into pidl */
    apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

    if (!apidl)
        return E_FAIL;

    if (FAILED(SHGetDesktopFolder(&psfDesktop)))
    {
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);
        return E_FAIL;
    }

    if (FAILED(IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom)))
    {
        ERR("no IShellFolder\n");

        IShellFolder_Release(psfDesktop);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);

        return E_FAIL;
    }

    IShellFolder_Release(psfDesktop);

    if (FAILED(IShellFolder_BindToObject(This->dcm.psf, This->dcm.apidl[0], NULL, &IID_IShellFolder, (LPVOID*)&psfTarget)))
    {
        ERR("no IShellFolder\n");

        IShellFolder_Release(psfFrom);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);

        return E_FAIL;
    }


    /* get source and destination shellfolder */
    if (FAILED(IShellFolder_QueryInterface(psfTarget, &IID_ISFHelper, (LPVOID*)&psfhlpdst)))
    {
        ERR("no IID_ISFHelper for destination\n");

        IShellFolder_Release(psfFrom);
        IShellFolder_Release(psfTarget);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);

        return E_FAIL;
    }

    if (FAILED(IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (LPVOID*)&psfhlpsrc)))
    {
        ERR("no IID_ISFHelper for source\n");

        ISFHelper_Release(psfhlpdst);
        IShellFolder_Release(psfFrom);
        IShellFolder_Release(psfTarget);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        IDataObject_Release(pda);
        return E_FAIL;
    }

    /* FIXXME
     * do we want to perform a copy or move ???
     */
    hr = ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);

    ISFHelper_Release(psfhlpdst);
    ISFHelper_Release(psfhlpsrc);
    IShellFolder_Release(psfFrom);
    IShellFolder_Release(psfTarget);
     SHFree(pidl);
    _ILFreeaPidl(apidl, lpcida->cidl);
    ReleaseStgMedium(&medium);
    IDataObject_Release(pda);
    return S_OK;
}

static
HRESULT
DoOpenOrExplore(
	IDefaultContextMenuImpl *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{


    return E_FAIL;
}

BOOL
GetUniqueFileName(LPWSTR szBasePath, LPWSTR szExt, LPWSTR szTarget, BOOL bShortcut)
{
    UINT RetryCount = 0, Length;
    WCHAR szLnk[40];
    HANDLE hFile;

    if (!bShortcut)
    {
        Length = LoadStringW(shell32_hInstance, IDS_LNK_FILE, szLnk, sizeof(szLnk)/sizeof(WCHAR));
    }

    do
    {
        if (!bShortcut)
        {
            if (RetryCount)
                swprintf(szTarget, L"%s%s(%u).%s", szLnk, szBasePath, RetryCount, szExt);
            else
                swprintf(szTarget, L"%s%s.%s", szLnk, szBasePath, szExt);
        }
        else
        {
            if (RetryCount)
                swprintf(szTarget, L"%s(%u).%s", szBasePath, RetryCount, szExt);
            else
                swprintf(szTarget, L"%s.%s", szBasePath, szExt);
        }

        hFile = CreateFileW(szTarget, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            return TRUE;
        }

    }while(RetryCount++ < 100);

    return FALSE;

}

static
HRESULT
DoCreateLink(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szTarget[MAX_PATH] = {0};
    STRRET strFile;
    LPWSTR pszExt;
    HRESULT hr;
    IShellLinkW * nLink;
    IPersistFile * ipf;
    static WCHAR szLnk[] = L"lnk";

    if (IShellFolder2_GetDisplayNameOf(This->dcm.psf, This->dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, This->dcm.apidl[0], szPath, MAX_PATH) != S_OK)
        return E_FAIL;


    pszExt = wcsrchr(szPath, L'.');
    pszExt[0] = 0;

    if (!wcsicmp(pszExt + 1, szLnk))
    {
        if (!GetUniqueFileName(szPath, pszExt + 1, szTarget, TRUE))
            return E_FAIL;

        hr = IShellLink_ConstructFromFile(NULL, &IID_IPersistFile, This->dcm.apidl[0], (LPVOID*)&ipf);
        if (hr != S_OK)
        {
            return hr;
        }
        hr = IPersistFile_Save(ipf, szTarget, FALSE);
        IPersistFile_Release(ipf);
        NotifyShellViewWindow(lpcmi, TRUE);
        return hr;
    }
    else
    {
        if (!GetUniqueFileName(szPath, szLnk, szTarget, TRUE))
            return E_FAIL;

        hr = IShellLink_Constructor(NULL, &IID_IShellLinkW, (LPVOID*)&nLink);
        if (hr != S_OK)
        {
            return E_FAIL;
        }
        pszExt[0] = '.';
        if (SUCCEEDED(IShellLinkW_SetPath(nLink, szPath)))
        {
            if (SUCCEEDED(IShellLinkW_QueryInterface(nLink, &IID_IPersistFile, (LPVOID*)&ipf)))
            {
                hr = IPersistFile_Save(ipf, szTarget, TRUE);
                IPersistFile_Release(ipf);
            }
        }
        IShellLinkW_Release(nLink);
        NotifyShellViewWindow(lpcmi, TRUE);
        return hr;
    }
}

static
HRESULT
DoDelete(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr;
    STRRET strTemp;
    WCHAR szPath[MAX_PATH];
    SHFILEOPSTRUCTW op;
    int ret;
    LPSHELLBROWSER lpSB;
    HWND hwnd;


    hr = IShellFolder2_GetDisplayNameOf(This->dcm.psf, This->dcm.apidl[0], SHGDN_FORPARSING, &strTemp);
    if(hr != S_OK)
    {
       ERR("IShellFolder_GetDisplayNameOf failed with %x\n", hr);
       return hr;
    }
    ZeroMemory(szPath, sizeof(szPath));
    hr = StrRetToBufW(&strTemp, This->dcm.apidl[0], szPath, MAX_PATH);
    if (hr != S_OK)
    {
        ERR("StrRetToBufW failed with %x\n", hr);
        return hr;
    }
    /* FIXME
     * implement deletion with multiple files
     */

    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = szPath;
    op.fFlags = FOF_ALLOWUNDO;
    ret = SHFileOperationW(&op);

    if (ret)
    {
        TRACE("SHFileOperation failed with %0x%x", GetLastError());
        return S_OK;
    }

    /* get the active IShellView */
    if ((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
    {
        /* is the treeview focused */
        if (SUCCEEDED(IShellBrowser_GetControlWindow(lpSB, FCW_TREE, &hwnd)))
        {
            HTREEITEM hItem = TreeView_GetSelection(hwnd);
            if (hItem)
            {
                (void)TreeView_DeleteItem(hwnd, hItem);
            }
        }
    }
    NotifyShellViewWindow(lpcmi, TRUE);

    return S_OK;

}

static
HRESULT
DoCopyOrCut(
    IDefaultContextMenuImpl *iface,
    LPCMINVOKECOMMANDINFO lpcmi,
    BOOL bCopy)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV;
    LPDATAOBJECT lpDo;
    HRESULT hr;

    lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0);
    if (!lpSB)
    {
        TRACE("failed to get shellbrowser\n");
        return E_FAIL;
    }

    hr = IShellBrowser_QueryActiveShellView(lpSB, &lpSV);
    if (FAILED(hr))
    {
        TRACE("failed to query the active shellview\n");
        return hr;
    }

    hr = IShellView_GetItemObject(lpSV, SVGIO_SELECTION, &IID_IDataObject, (LPVOID*)&lpDo);
    if (FAILED(hr))
    {
        TRACE("failed to get item object\n");
        return hr;
    }

    hr = OleSetClipboard(lpDo);
    if (FAILED(hr))
    {
        WARN("OleSetClipboard failed");
    }
    IDataObject_Release(lpDo);
    IShellView_Release(lpSV);
    return S_OK;
}

static
HRESULT
DoRename(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV;
    HWND hwnd;

    /* get the active IShellView */
    if ((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
    {
        /* is the treeview focused */
        if (SUCCEEDED(IShellBrowser_GetControlWindow(lpSB, FCW_TREE, &hwnd)))
        {
            HTREEITEM hItem = TreeView_GetSelection(hwnd);
            if (hItem)
            {
                (void)TreeView_EditLabel(hwnd, hItem);
            }
        }

        if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
        {
            IShellView_SelectItem(lpSV, This->dcm.apidl[0],
              SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
            IShellView_Release(lpSV);
            return S_OK;
        }
    }
    return E_FAIL;
}

static
HRESULT
DoProperties(
	IDefaultContextMenuImpl *This,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    WCHAR szDrive[MAX_PATH];
    STRRET strFile;

    if (This->dcm.cidl &&_ILIsMyComputer(This->dcm.apidl[0]))
    {
         ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL sysdm.cpl", NULL, NULL, SW_SHOWNORMAL);
         return S_OK;
    }
    else if (This->dcm.cidl == 0 && _ILIsDesktop(This->dcm.pidlFolder))
    {
        ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
        return S_OK;
    }
    else if (_ILIsDrive(This->dcm.apidl[0]))
    {
        ILGetDisplayName(This->dcm.apidl[0], szDrive);
        SH_ShowDriveProperties(szDrive, This->dcm.pidlFolder, This->dcm.apidl);
        return S_OK;
    }
    else if (_ILIsNetHood(This->dcm.apidl[0]))
    {
        /* FIXME
         * implement nethood properties
         */
        FIXME("implement network connection shell folder\n");
        return S_OK;
    }
    else if (_ILIsBitBucket(This->dcm.apidl[0]))
    {
        /* FIXME
         * detect the drive path of bitbucket if appropiate
         */

         SH_ShowRecycleBinProperties(L'C');
         return S_OK;
    }

    if (This->dcm.cidl > 1)
        WARN("SHMultiFileProperties is not yet implemented\n");

    if (IShellFolder2_GetDisplayNameOf(This->dcm.psf, This->dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, This->dcm.apidl[0], szDrive, MAX_PATH) != S_OK)
        return E_FAIL;

    return SH_ShowPropertiesDialog(szDrive, This->dcm.pidlFolder, This->dcm.apidl);
}

static
HRESULT
DoFormat(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    char sDrive[5] = {0};

    if (!_ILGetDrive(This->dcm.apidl[0], sDrive, sizeof(sDrive)))
    {
        ERR("pidl is not a drive\n");
        return E_FAIL;
    }

    SHFormatDrive(lpcmi->hwnd, sDrive[0] - 'A', SHFMT_ID_DEFAULT, 0);
    return S_OK;
}

static
HRESULT
DoDynamicShellExtensions(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    UINT verb = LOWORD(lpcmi->lpVerb);
    PDynamicShellEntry pCurrent = This->dhead;

    TRACE("verb %p first %x last %x", lpcmi->lpVerb, This->iIdSHEFirst, This->iIdSHELast);

    while(pCurrent && verb > pCurrent->iIdCmdFirst + pCurrent->NumIds)
        pCurrent = pCurrent->Next;

    if (!pCurrent)
        return E_FAIL;

    if (verb >= pCurrent->iIdCmdFirst && verb <= pCurrent->iIdCmdFirst + pCurrent->NumIds)
    {
        /* invoke the dynamic context menu */
        lpcmi->lpVerb = MAKEINTRESOURCEA(verb - pCurrent->iIdCmdFirst);
        return IContextMenu_InvokeCommand(pCurrent->CMenu, lpcmi);
    }

    return E_FAIL;
}


static
HRESULT
DoStaticShellExtensions(
    IDefaultContextMenuImpl *This,
    LPCMINVOKECOMMANDINFO lpcmi)
{
    STRRET strFile;
    WCHAR szPath[MAX_PATH];
    SHELLEXECUTEINFOW sei;
    PStaticShellEntry pCurrent = This->shead;
    int verb = LOWORD(lpcmi->lpVerb) - This->iIdSCMFirst;

    
    while(pCurrent && verb-- > 0)
       pCurrent = pCurrent->Next;

    if (verb > 0)
        return E_FAIL;


    if (IShellFolder2_GetDisplayNameOf(This->dcm.psf, This->dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, This->dcm.apidl[0], szPath, MAX_PATH) != S_OK)
        return E_FAIL;


    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_CLASSNAME;
    sei.lpClass = pCurrent->szClass;
    sei.hwnd = lpcmi->hwnd;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = pCurrent->szVerb;
    sei.lpFile = szPath;
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
            return NotifyShellViewWindow(lpcmi, FALSE);
        case FCIDM_SHVIEW_INSERT:
        case FCIDM_SHVIEW_INSERTLINK:
            return DoPaste(This, lpcmi);
        case FCIDM_SHVIEW_OPEN:
        case FCIDM_SHVIEW_EXPLORE:
            return DoOpenOrExplore(This, lpcmi);
        case FCIDM_SHVIEW_COPY:
        case FCIDM_SHVIEW_CUT:
            return DoCopyOrCut(This, lpcmi, LOWORD(lpcmi->lpVerb) == FCIDM_SHVIEW_COPY);
        case FCIDM_SHVIEW_CREATELINK:
            return DoCreateLink(This, lpcmi);
        case FCIDM_SHVIEW_DELETE:
            return DoDelete(This, lpcmi);
        case FCIDM_SHVIEW_RENAME:
            return DoRename(This, lpcmi);
        case FCIDM_SHVIEW_PROPERTIES:
            return DoProperties(This, lpcmi);
        case 0x7ABC:
            return DoFormat(This, lpcmi);
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
       if (SUCCEEDED(hr))
           IContextMenu_Release((IContextMenu2*)This);
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
   HRESULT hr = E_FAIL;

   *ppv = NULL;
   hr = IDefaultContextMenu_Constructor( pdcm, riid, ppv );

   TRACE("pcm %p hr %x\n", pdcm, hr);
   return hr;
}

/*************************************************************************
 * CDefFolderMenu_Create2			[SHELL32.701]
 *
 */

HRESULT
WINAPI
CDefFolderMenu_Create2(
	LPCITEMIDLIST pidlFolder,
	HWND hwnd,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	IShellFolder *psf,
	LPFNDFMCALLBACK lpfn,
	UINT nKeys,
	const HKEY *ahkeyClsKeys,
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

