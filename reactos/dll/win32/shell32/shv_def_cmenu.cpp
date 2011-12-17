/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides default context menu implementation
 * PROGRAMMERS: Johannes Anderwald (janderwald@reactos.org)
 */

/*
TODO:
1. In DoStaticShellExtensions, check for "Explore" and "Open" verbs, and for BrowserFlags or
    ExplorerFlags under those entries. These flags indicate if we should browse to the new item
    instead of attempting to open it.
2. The code in NotifyShellViewWindow to deliver commands to the view is broken. It is an excellent
    example of the wrong way to do it.
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

WCHAR *build_paths_list(LPCWSTR wszBasePath, int cidl, LPCITEMIDLIST *pidls);

class IDefaultContextMenuImpl :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2
{
private:
    DEFCONTEXTMENU dcm;
    IDataObject * pDataObj;
    DWORD bGroupPolicyActive;
    PDynamicShellEntry dhead; /* first dynamic shell extension entry */
    UINT           iIdSHEFirst; /* first used id */
    UINT           iIdSHELast; /* last used id */
    PStaticShellEntry shead; /* first static shell extension entry */
    UINT           iIdSCMFirst; /* first static used id */
    UINT           iIdSCMLast; /* last static used id */
public:
    IDefaultContextMenuImpl();
    ~IDefaultContextMenuImpl();
    HRESULT WINAPI Initialize(const DEFCONTEXTMENU *pdcm);
    void SH_AddStaticEntry(const WCHAR *szVerb, const WCHAR *szClass);
    void SH_AddStaticEntryForKey(HKEY hKey, const WCHAR *szClass);
    void SH_AddStaticEntryForFileClass(const WCHAR *szExt);
    BOOL IsShellExtensionAlreadyLoaded(const CLSID *szClass);
    HRESULT SH_LoadDynamicContextMenuHandler(HKEY hKey, const CLSID *szClass, BOOL bExternalInit);
    UINT EnumerateDynamicContextHandlerForKey(HKEY hRootKey);
    UINT InsertMenuItemsOfDynamicContextMenuExtension(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);
    UINT BuildBackgroundContextMenu(HMENU hMenu, UINT iIdCmdFirst, UINT iIdCmdLast, UINT uFlags);
    UINT AddStaticContextMenusToMenu(HMENU hMenu, UINT indexMenu);
    UINT BuildShellItemContextMenu(HMENU hMenu, UINT iIdCmdFirst, UINT iIdCmdLast, UINT uFlags);
    HRESULT DoPaste(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoOpenOrExplore(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoCreateLink(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoDelete(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoCopyOrCut(LPCMINVOKECOMMANDINFO lpcmi, BOOL bCopy);
    HRESULT DoRename(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoProperties(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoFormat(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoDynamicShellExtensions(LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT DoStaticShellExtensions(LPCMINVOKECOMMANDINFO lpcmi);

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand,UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

    // IContextMenu2
    virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

BEGIN_COM_MAP(IDefaultContextMenuImpl)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
END_COM_MAP()
};

IDefaultContextMenuImpl::IDefaultContextMenuImpl()
{
    memset (&dcm, 0, sizeof(dcm));
    pDataObj = NULL;
    bGroupPolicyActive = 0;
    dhead = NULL;
    iIdSHEFirst = 0;
    iIdSHELast = 0;
    shead = NULL;
    iIdSCMFirst = 0;
    iIdSCMLast = 0;
}

IDefaultContextMenuImpl::~IDefaultContextMenuImpl()
{
    PDynamicShellEntry dEntry, dNext;
    PStaticShellEntry sEntry, sNext;

    /* free dynamic shell extension entries */
    dEntry = dhead;
    while (dEntry)
    {
        dNext = dEntry->Next;
        dEntry->CMenu->Release();
        HeapFree(GetProcessHeap(), 0, dEntry);
        dEntry = dNext;
    }
    /* free static shell extension entries */
    sEntry = shead;
    while (sEntry)
    {
        sNext = sEntry->Next;
        HeapFree(GetProcessHeap(), 0, sEntry->szClass);
        HeapFree(GetProcessHeap(), 0, sEntry->szVerb);
        HeapFree(GetProcessHeap(), 0, sEntry);
        sEntry = sNext;
    } 
}

HRESULT WINAPI IDefaultContextMenuImpl::Initialize(const DEFCONTEXTMENU *pdcm)
{
   IDataObject *newDataObj;

   TRACE("cidl %u\n", dcm.cidl);
   if (SUCCEEDED(SHCreateDataObject(pdcm->pidlFolder, pdcm->cidl, pdcm->apidl, NULL, IID_IDataObject, (void**)&newDataObj)))
        pDataObj = newDataObj;
   CopyMemory(&dcm, pdcm, sizeof(DEFCONTEXTMENU));
   return S_OK;
}

void
IDefaultContextMenuImpl::SH_AddStaticEntry(const WCHAR *szVerb, const WCHAR * szClass)
{
    PStaticShellEntry curEntry;
    PStaticShellEntry lastEntry = NULL;

    curEntry = shead;
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

    curEntry = (StaticShellEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(StaticShellEntry));
    if (curEntry)
    {
        curEntry->Next = NULL;
        curEntry->szVerb = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(szVerb)+1) * sizeof(WCHAR));
        if (curEntry->szVerb)
            wcscpy(curEntry->szVerb, szVerb);
        curEntry->szClass = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(szClass)+1) * sizeof(WCHAR));
        if (curEntry->szClass)
            wcscpy(curEntry->szClass, szClass);
    }

    if (!wcsicmp(szVerb, L"open"))
    {
        /* open verb is always inserted in front */
        curEntry->Next = shead;
        shead = curEntry;
        return;
    }



    if (lastEntry)
    {
        lastEntry->Next = curEntry;
    }
    else
    {
        shead = curEntry;
    }
}

void
IDefaultContextMenuImpl::SH_AddStaticEntryForKey(HKEY hKey, const WCHAR * szClass)
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
            SH_AddStaticEntry(szName, szClass);
        }
        dwIndex++;
    }while(result == ERROR_SUCCESS);
}

void
IDefaultContextMenuImpl::SH_AddStaticEntryForFileClass(const WCHAR * szExt)
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
            SH_AddStaticEntryForKey(hKey, szExt);
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
                SH_AddStaticEntryForKey(hKey, szBuffer);
                RegCloseKey(hKey);
            }
        }
    }

    wcscpy(szBuffer, szShellAssoc);
    dwBuffer = sizeof(szBuffer) - sizeof(szShellAssoc) - sizeof(WCHAR);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, L"PerceivedType", RRF_RT_REG_SZ, NULL, (LPBYTE)&szBuffer[_countof(szShellAssoc) - 1], &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        Length = wcslen(&szBuffer[_countof(szShellAssoc)]) + _countof(szShellAssoc);
        wcscat(szBuffer, L"\\shell");
        TRACE("szBuffer %s\n", debugstr_w(szBuffer));

        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
           szBuffer[Length] = 0;
           SH_AddStaticEntryForKey(hKey, szBuffer);
           RegCloseKey(hKey);
        }
    }
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
      InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
      if(SUCCEEDED(pda->GetData(&formatetc,&medium)))
      {
          ret = TRUE;
          ReleaseStgMedium(&medium);          
      }

      pda->Release();
    }

    return ret;
}

static
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
IDefaultContextMenuImpl::IsShellExtensionAlreadyLoaded(const CLSID * szClass)
{
    PDynamicShellEntry curEntry = dhead;

    while(curEntry)
    {
        if (!memcmp(&curEntry->ClassID, szClass, sizeof(CLSID)))
            return TRUE;
        curEntry = curEntry->Next;
    }
    return FALSE;
}


HRESULT
IDefaultContextMenuImpl::SH_LoadDynamicContextMenuHandler(HKEY hKey, const CLSID * pClass, BOOL bExternalInit)
{
  HRESULT hr;
  IContextMenu * cmobj;
  IShellExtInit *shext;
  PDynamicShellEntry curEntry;
  LPOLESTR pstr;

  StringFromCLSID(*pClass, &pstr);

  TRACE("SH_LoadDynamicContextMenuHandler entered with This %p hKey %p pClass %s bExternalInit %u\n", this, hKey, wine_dbgstr_guid(pClass), bExternalInit);

  if (IsShellExtensionAlreadyLoaded(pClass))
      return S_OK;

  hr = SHCoCreateInstance(NULL, pClass, NULL, IID_IContextMenu, (void**)&cmobj);
  if (hr != S_OK)
  {
      ERR("SHCoCreateInstance failed %x\n", GetLastError());
      return hr;
  }

  if (bExternalInit)
  {
      hr = cmobj->QueryInterface(IID_IShellExtInit, (void**)&shext);
      if (hr != S_OK)
      {
        ERR("Failed to query for interface IID_IShellExtInit hr %x pClass %s\n", hr, wine_dbgstr_guid(pClass));
        cmobj->Release();
        return FALSE;
      }
      hr = shext->Initialize(NULL, pDataObj, hKey);
      shext->Release();
      if (hr != S_OK)
      {
        TRACE("Failed to initialize shell extension error %x pClass %s\n", hr, wine_dbgstr_guid(pClass));
        cmobj->Release();
        return hr;
      }
  }

  curEntry = (DynamicShellEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(DynamicShellEntry));
  if(!curEntry)
  {
      cmobj->Release();
      return E_OUTOFMEMORY;
  }

  curEntry->iIdCmdFirst = 0;
  curEntry->Next = NULL;
  curEntry->NumIds = 0;
  curEntry->CMenu = cmobj;
  memcpy(&curEntry->ClassID, pClass, sizeof(CLSID));

  if (dhead)
  {
      PDynamicShellEntry pEntry = dhead;

      while(pEntry->Next)
      {
         pEntry = pEntry->Next;
      }

      pEntry->Next = curEntry;
  }
  else
  {
      dhead = curEntry;
  }

  return hr;
}

UINT
IDefaultContextMenuImpl::EnumerateDynamicContextHandlerForKey(HKEY hRootKey)
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
             if (bGroupPolicyActive)
             {
                 if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                                  szKey,
                                  RRF_RT_REG_SZ,
                                  NULL,
                                  NULL,
                                 &dwName) == ERROR_SUCCESS)
                 {
                     SH_LoadDynamicContextMenuHandler(hKey, &clsid, TRUE);
                 }
             }
             else
             {
                 SH_LoadDynamicContextMenuHandler(hKey, &clsid, TRUE);
             }
         }
      }
      dwIndex++;
   }while(res == ERROR_SUCCESS);

   RegCloseKey(hKey);
   return index;
}

UINT
IDefaultContextMenuImpl::InsertMenuItemsOfDynamicContextMenuExtension(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    PDynamicShellEntry curEntry;
    HRESULT hResult;

    if (!dhead)
    {
        iIdSHEFirst = 0;
        iIdSHELast = 0;
        return indexMenu;
    }

    curEntry = dhead;
    idCmdFirst = 0x5000;
    idCmdLast =  0x6000;
    iIdSHEFirst = idCmdFirst;
    do
    {
        hResult = curEntry->CMenu->QueryContextMenu(hMenu, indexMenu++, idCmdFirst, idCmdLast, CMF_NORMAL);
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

    iIdSHELast = idCmdFirst;
    TRACE("SH_LoadContextMenuHandlers first %x last %x\n", iIdSHEFirst, iIdSHELast);
    return indexMenu;
}

UINT
IDefaultContextMenuImpl::BuildBackgroundContextMenu(
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

    if (!_ILIsDesktop(dcm.pidlFolder))
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
        EnumerateDynamicContextHandlerForKey(hKey);
        RegCloseKey(hKey);
    }

    /* load create new shell extension */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                     L"CLSID\\{D969A300-E7FF-11d0-A93B-00A0C90F2719}",
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        SH_LoadDynamicContextMenuHandler(hKey, &CLSID_NewMenu, TRUE);
        RegCloseKey(hKey);
    }
    
    if (InsertMenuItemsOfDynamicContextMenuExtension(hMenu, GetMenuItemCount(hMenu)-1, iIdCmdFirst, iIdCmdLast))
    {
        /* seperate dynamic context menu items */
        _InsertMenuItemW(hMenu, GetMenuItemCount(hMenu)-1, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    }

    return iIdCmdLast;
}

UINT
IDefaultContextMenuImpl::AddStaticContextMenusToMenu(
    HMENU hMenu,
    UINT indexMenu)
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
    mii.fState = MFS_ENABLED;
    mii.wID = 0x4000;
    mii.dwTypeData = NULL;
    iIdSCMFirst = mii.wID;

    curEntry = shead;

    while(curEntry)
    {
        fState = MFS_ENABLED;
        if (!wcsicmp(curEntry->szVerb, L"open"))
        {
           fState |= MFS_DEFAULT;
           idResource = IDS_OPEN_VERB;
        }
        else if (!wcsicmp(curEntry->szVerb, L"explore"))
           idResource = IDS_EXPLORE_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"runas"))
           idResource = IDS_RUNAS_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"edit"))
           idResource = IDS_EDIT_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"find"))
           idResource = IDS_FIND_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"print"))
           idResource = IDS_PRINT_VERB;
        else if (!wcsicmp(curEntry->szVerb, L"printto"))
        {
            curEntry = curEntry->Next;
            continue;
        }
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
            else
            {
                ERR("Failed to load string, defaulting to NULL value for mii.dwTypeData\n");
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
        mii.fState = fState;
        InsertMenuItemW(hMenu, indexMenu++, TRUE, &mii);

        mii.wID++;
        curEntry = curEntry->Next;
     }
     iIdSCMLast = mii.wID - 1;
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
                ERR("failed to load string %p\n", dwTypeData);
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
IDefaultContextMenuImpl::BuildShellItemContextMenu(
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
    BOOL bAddSep = FALSE;
    GUID * guid;
    BOOL bClipboardData;
    STRRET strFile;
    LPWSTR pOffset;
    DWORD dwSize;

    TRACE("BuildShellItemContextMenu entered\n");

    hr = dcm.psf->GetDisplayNameOf(dcm.apidl[0], SHGDN_FORPARSING, &strFile);
    if (hr == S_OK)
    {
        hr = StrRetToBufW(&strFile, dcm.apidl[0], szPath, MAX_PATH);
        if (hr == S_OK)
        {
            pOffset = wcsrchr(szPath, L'.');
            if (pOffset)
            {
                /* enumerate dynamic/static for a given file class */
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pOffset, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    /* add static verbs */
                    SH_AddStaticEntryForFileClass(pOffset);
                    /* load dynamic extensions from file extension key */
                    EnumerateDynamicContextHandlerForKey(hKey);
                    RegCloseKey(hKey);
                }
                dwSize = sizeof(szTemp);
                if (RegGetValueW(HKEY_CLASSES_ROOT, pOffset, NULL, RRF_RT_REG_SZ, NULL, szTemp, &dwSize) == ERROR_SUCCESS)
                {
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                        /* add static verbs from progid key */
                        SH_AddStaticEntryForFileClass(szTemp);
                        /* load dynamic extensions from progid key */
                        EnumerateDynamicContextHandlerForKey(hKey);
                        RegCloseKey(hKey);
                   }
                }
            }
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"*", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                /* load default extensions */
                EnumerateDynamicContextHandlerForKey(hKey);
                RegCloseKey(hKey);
            }
        }
    }
    else
        ERR("GetDisplayNameOf failed: %x\n", hr);

    guid = _ILGetGUIDPointer(dcm.apidl[0]);
    if (guid)
    {
        LPOLESTR pwszCLSID;
        WCHAR buffer[60];

        wcscpy(buffer, L"CLSID\\");
        hr = StringFromCLSID(*guid, &pwszCLSID);
        if (hr == S_OK)
        {
            wcscpy(&buffer[6], pwszCLSID);
            TRACE("buffer %s\n", debugstr_w(buffer));
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, buffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(hKey);
                SH_AddStaticEntryForFileClass(buffer);
                RegCloseKey(hKey);
            }
            CoTaskMemFree(pwszCLSID);
        }
    }


    if (_ILIsDrive(dcm.apidl[0]))
    {
        SH_AddStaticEntryForFileClass(L"Drive");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Drive", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }

    }

    /* add static actions */
    rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_FILESYSTEM | SFGAO_FOLDER;
    hr = dcm.psf->GetAttributesOf(dcm.cidl, dcm.apidl, &rfg);
    if (FAILED(hr))
    {
        WARN("GetAttributesOf failed: %x\n", hr);
        rfg = 0;
    }

    if ((rfg & SFGAO_FOLDER) || _ILIsControlPanel(dcm.apidl[dcm.cidl]))
    {
        /* add the default verbs open / explore */
        SH_AddStaticEntryForFileClass(L"Folder");
        SH_AddStaticEntryForFileClass(L"Directory");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Folder", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Directory", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }
    }


    if (rfg & SFGAO_FILESYSTEM)
    {
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"AllFilesystemObjects", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* sendto service is registered here */
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }
    }

    /* add static context menu handlers */
    indexMenu = AddStaticContextMenusToMenu(hMenu, 0);
    /* now process dynamic context menu handlers */
    indexMenu = InsertMenuItemsOfDynamicContextMenuExtension(hMenu, indexMenu, iIdCmdFirst, iIdCmdLast);
    TRACE("indexMenu %d\n", indexMenu);

    if (_ILIsDrive(dcm.apidl[0]))
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

HRESULT
WINAPI
IDefaultContextMenuImpl::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    if (dcm.cidl)
    {
        idCmdFirst = BuildShellItemContextMenu(hmenu, idCmdFirst, idCmdLast, uFlags);
    }
    else
    {
        idCmdFirst = BuildBackgroundContextMenu(hmenu, idCmdFirst, idCmdLast, uFlags);
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
        if(SUCCEEDED(lpSB->QueryActiveShellView(&lpSV)))
        {
            lpSV->GetWindow(&hwndSV);
        }
    }

    if (LOWORD(lpcmi->lpVerb) == FCIDM_SHVIEW_REFRESH || bRefresh)
    {
        if (lpSV)
            lpSV->Refresh();

        return S_OK;
    }

    SendMessageW(hwndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);

    return S_OK;
}

HRESULT
IDefaultContextMenuImpl::DoPaste(
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

    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    hr = pda->GetData(&formatetc,&medium);

    if (FAILED(hr))
    {
        pda->Release();
        return E_FAIL;
    }

    /* lock the handle */
    lpcida = (LPIDA)GlobalLock(medium.hGlobal);
    if (!lpcida)
    {
        ReleaseStgMedium(&medium);
        pda->Release();
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
        pda->Release();
        return E_FAIL;
    }

    if (_ILIsDesktop(pidl))
    {
        /* use desktop shellfolder */
        psfFrom = psfDesktop;
    }
    else if (FAILED(psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&psfFrom)))
    {
        ERR("no IShellFolder\n");

        psfDesktop->Release();
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        pda->Release();

        return E_FAIL;
    }

    if (dcm.cidl)
    {
        psfDesktop->Release();
        hr = dcm.psf->BindToObject(dcm.apidl[0], NULL, IID_IShellFolder, (LPVOID*)&psfTarget);
    }
    else
    {
        IPersistFolder2 *ppf2 = NULL;
        LPITEMIDLIST pidl;

        /* cidl is zero due to explorer view */
        hr = dcm.psf->QueryInterface (IID_IPersistFolder2, (LPVOID *) &ppf2);
        if (SUCCEEDED(hr))
        {
            hr = ppf2->GetCurFolder (&pidl);
            ppf2->Release();
            if (SUCCEEDED(hr))
            {
                if (_ILIsDesktop(pidl))
                {
                    /* use desktop shellfolder */
                    psfTarget = psfDesktop;
                }
                else
                {
                    /* retrieve target desktop folder */
                    hr = psfDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&psfTarget);
                }
                TRACE("psfTarget %x %p, Desktop %u\n", hr, psfTarget, _ILIsDesktop(pidl));
                ILFree(pidl);
            }
        }
    }

    if (FAILED(hr))
    {
        ERR("no IShellFolder\n");

        psfFrom->Release();
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        pda->Release();

        return E_FAIL;
    }


    /* get source and destination shellfolder */
    if (FAILED(psfTarget->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlpdst)))
    {
        ERR("no IID_ISFHelper for destination\n");

        psfFrom->Release();
        psfTarget->Release();
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        pda->Release();

        return E_FAIL;
    }

    if (FAILED(psfFrom->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlpsrc)))
    {
        ERR("no IID_ISFHelper for source\n");

        psfhlpdst->Release();
        psfFrom->Release();
        psfTarget->Release();
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        pda->Release();
        return E_FAIL;
    }

    /* FIXXME
     * do we want to perform a copy or move ???
     */
    hr = psfhlpdst->CopyItems(psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl);

    psfhlpdst->Release();
    psfhlpsrc->Release();
    psfFrom->Release();
    psfTarget->Release();
     SHFree(pidl);
    _ILFreeaPidl(apidl, lpcida->cidl);
    ReleaseStgMedium(&medium);
    pda->Release();
    TRACE("CP result %x\n",hr);
    return S_OK;
}

HRESULT
IDefaultContextMenuImpl::DoOpenOrExplore(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    UNIMPLEMENTED;
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

HRESULT
IDefaultContextMenuImpl::DoCreateLink(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szTarget[MAX_PATH] = {0};
    WCHAR szDirPath[MAX_PATH];
    LPWSTR pszFile;
    STRRET strFile;
    LPWSTR pszExt;
    HRESULT hr;
    IShellLinkW * nLink;
    IPersistFile * ipf;
    static WCHAR szLnk[] = L"lnk";

    if (dcm.psf->GetDisplayNameOf(dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, dcm.apidl[0], szPath, MAX_PATH) != S_OK)
        return E_FAIL;

    pszExt = wcsrchr(szPath, L'.');

    if (pszExt && !wcsicmp(pszExt + 1, szLnk))
    {
        if (!GetUniqueFileName(szPath, pszExt + 1, szTarget, TRUE))
            return E_FAIL;

        hr = IShellLink_ConstructFromFile(NULL, IID_IPersistFile, dcm.apidl[0], (LPVOID*)&ipf);
        if (hr != S_OK)
        {
            return hr;
        }
        hr = ipf->Save(szTarget, FALSE);
        ipf->Release();
        NotifyShellViewWindow(lpcmi, TRUE);
        return hr;
    }
    else
    {
        if (!GetUniqueFileName(szPath, szLnk, szTarget, TRUE))
            return E_FAIL;

        hr = ShellLink::_CreatorClass::CreateInstance(NULL, IID_IShellLinkW, (void**)&nLink);
        if (hr != S_OK)
        {
            return E_FAIL;
        }

        GetFullPathName(szPath, MAX_PATH, szDirPath, &pszFile);
        if (pszFile) pszFile[0] = 0;

        if (SUCCEEDED(nLink->SetPath(szPath)) &&
            SUCCEEDED(nLink->SetWorkingDirectory(szDirPath)))
        {
            if (SUCCEEDED(nLink->QueryInterface(IID_IPersistFile, (LPVOID*)&ipf)))
            {
                hr = ipf->Save(szTarget, TRUE);
                ipf->Release();
            }
        }
        nLink->Release();
        NotifyShellViewWindow(lpcmi, TRUE);
        return hr;
    }
}

HRESULT
IDefaultContextMenuImpl::DoDelete(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr;
    STRRET strTemp;
    WCHAR szPath[MAX_PATH];
    LPWSTR wszPath, wszPos;
    SHFILEOPSTRUCTW op;
    int ret;
    LPSHELLBROWSER lpSB;
    HWND hwnd;


    hr = dcm.psf->GetDisplayNameOf(dcm.apidl[0], SHGDN_FORPARSING, &strTemp);
    if(hr != S_OK)
    {
       ERR("IShellFolder_GetDisplayNameOf failed with %x\n", hr);
       return hr;
    }
    ZeroMemory(szPath, sizeof(szPath));
    hr = StrRetToBufW(&strTemp, dcm.apidl[0], szPath, MAX_PATH);
    if (hr != S_OK)
    {
        ERR("StrRetToBufW failed with %x\n", hr);
        return hr;
    }

    /* Only keep the base path */
    wszPos = strrchrW(szPath, '\\');
    if (wszPos != NULL)
    {
        *(wszPos + 1) = '\0';
    }

    wszPath = build_paths_list(szPath, dcm.cidl, dcm.apidl);

    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = wszPath;
    op.fFlags = FOF_ALLOWUNDO;
    ret = SHFileOperationW(&op);

    if (ret)
    {
        ERR("SHFileOperation failed with 0x%x for %s\n", GetLastError(), debugstr_w(wszPath));
        return S_OK;
    }

    /* get the active IShellView */
    if ((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
    {
        /* is the treeview focused */
        if (SUCCEEDED(lpSB->GetControlWindow(FCW_TREE, &hwnd)))
        {
            HTREEITEM hItem = TreeView_GetSelection(hwnd);
            if (hItem)
            {
                (void)TreeView_DeleteItem(hwnd, hItem);
            }
        }
    }
    NotifyShellViewWindow(lpcmi, TRUE);

    HeapFree(GetProcessHeap(), 0, wszPath);
    return S_OK;

}

HRESULT
IDefaultContextMenuImpl::DoCopyOrCut(
    LPCMINVOKECOMMANDINFO lpcmi,
    BOOL bCopy)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV;
    LPDATAOBJECT pDataObj;
    HRESULT hr;

    if (SUCCEEDED(SHCreateDataObject(dcm.pidlFolder, dcm.cidl, dcm.apidl, NULL, IID_IDataObject, (void**)&pDataObj)))
    {
        hr = OleSetClipboard(pDataObj);
        pDataObj->Release();
        return hr;
    }

    lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0);
    if (!lpSB)
    {
        TRACE("failed to get shellbrowser\n");
        return E_FAIL;
    }

    hr = lpSB->QueryActiveShellView(&lpSV);
    if (FAILED(hr))
    {
        TRACE("failed to query the active shellview\n");
        return hr;
    }

    hr = lpSV->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (LPVOID*)&pDataObj);
    if (FAILED(hr))
    {
        TRACE("failed to get item object\n");
        return hr;
    }

    hr = OleSetClipboard(pDataObj);
    if (FAILED(hr))
    {
        WARN("OleSetClipboard failed");
    }
    pDataObj->Release();
    lpSV->Release();
    return S_OK;
}

HRESULT
IDefaultContextMenuImpl::DoRename(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV;
    HWND hwnd;

    /* get the active IShellView */
    if ((lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER,0,0)))
    {
        /* is the treeview focused */
        if (SUCCEEDED(lpSB->GetControlWindow(FCW_TREE, &hwnd)))
        {
            HTREEITEM hItem = TreeView_GetSelection(hwnd);
            if (hItem)
            {
                (void)TreeView_EditLabel(hwnd, hItem);
            }
        }

        if(SUCCEEDED(lpSB->QueryActiveShellView(&lpSV)))
        {
            lpSV->SelectItem(dcm.apidl[0],
              SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
            lpSV->Release();
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT
IDefaultContextMenuImpl::DoProperties(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    WCHAR szDrive[MAX_PATH];
    STRRET strFile;

    if (dcm.cidl &&_ILIsMyComputer(dcm.apidl[0]))
    {
         ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL sysdm.cpl", NULL, NULL, SW_SHOWNORMAL);
         return S_OK;
    }
    else if (dcm.cidl == 0 && dcm.pidlFolder != NULL && _ILIsDesktop(dcm.pidlFolder))
    {
        ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
        return S_OK;
    }
    else if (_ILIsDrive(dcm.apidl[0]))
    {
        ILGetDisplayName(dcm.apidl[0], szDrive);
        SH_ShowDriveProperties(szDrive, dcm.pidlFolder, dcm.apidl);
        return S_OK;
    }
    else if (_ILIsNetHood(dcm.apidl[0]))
    {
        //FIXME path!
        ShellExecuteW(NULL, L"open", L"explorer.exe",
                      L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", 
                      NULL, SW_SHOWDEFAULT);
        return S_OK;
    }
    else if (_ILIsBitBucket(dcm.apidl[0]))
    {
        /* FIXME
         * detect the drive path of bitbucket if appropiate
         */

         SH_ShowRecycleBinProperties(L'C');
         return S_OK;
    }

    if (dcm.cidl > 1)
        WARN("SHMultiFileProperties is not yet implemented\n");

    if (dcm.psf->GetDisplayNameOf(dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, dcm.apidl[0], szDrive, MAX_PATH) != S_OK)
        return E_FAIL;

    return SH_ShowPropertiesDialog(szDrive, dcm.pidlFolder, dcm.apidl);
}

HRESULT
IDefaultContextMenuImpl::DoFormat(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    char sDrive[5] = {0};

    if (!_ILGetDrive(dcm.apidl[0], sDrive, sizeof(sDrive)))
    {
        ERR("pidl is not a drive\n");
        return E_FAIL;
    }

    SHFormatDrive(lpcmi->hwnd, sDrive[0] - 'A', SHFMT_ID_DEFAULT, 0);
    return S_OK;
}

HRESULT
IDefaultContextMenuImpl::DoDynamicShellExtensions(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    UINT verb = LOWORD(lpcmi->lpVerb);
    PDynamicShellEntry pCurrent = dhead;

    TRACE("verb %p first %x last %x", lpcmi->lpVerb, iIdSHEFirst, iIdSHELast);

    while(pCurrent && verb > pCurrent->iIdCmdFirst + pCurrent->NumIds)
        pCurrent = pCurrent->Next;

    if (!pCurrent)
        return E_FAIL;

    if (verb >= pCurrent->iIdCmdFirst && verb <= pCurrent->iIdCmdFirst + pCurrent->NumIds)
    {
        /* invoke the dynamic context menu */
        lpcmi->lpVerb = MAKEINTRESOURCEA(verb - pCurrent->iIdCmdFirst);
        return pCurrent->CMenu->InvokeCommand(lpcmi);
    }

    return E_FAIL;
}


HRESULT
IDefaultContextMenuImpl::DoStaticShellExtensions(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    STRRET strFile;
    WCHAR szPath[MAX_PATH];
    WCHAR szDir[MAX_PATH];
    SHELLEXECUTEINFOW sei;
    PStaticShellEntry pCurrent = shead;
    int verb = LOWORD(lpcmi->lpVerb) - iIdSCMFirst;

    
    while(pCurrent && verb-- > 0)
       pCurrent = pCurrent->Next;

    if (verb > 0)
        return E_FAIL;


    if (dcm.psf->GetDisplayNameOf(dcm.apidl[0], SHGDN_FORPARSING, &strFile) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
        return E_FAIL;
    }

    if (StrRetToBufW(&strFile, dcm.apidl[0], szPath, MAX_PATH) != S_OK)
        return E_FAIL;

    wcscpy(szDir, szPath);
    PathRemoveFileSpec(szDir);

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_CLASSNAME;
    sei.lpClass = pCurrent->szClass;
    sei.hwnd = lpcmi->hwnd;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = pCurrent->szVerb;
    sei.lpFile = szPath;
    sei.lpDirectory = szDir;
    ShellExecuteExW(&sei);
    return S_OK;

}

HRESULT
WINAPI
IDefaultContextMenuImpl::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpcmi)
{
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
            return DoPaste(lpcmi);
        case FCIDM_SHVIEW_OPEN:
        case FCIDM_SHVIEW_EXPLORE:
            return DoOpenOrExplore(lpcmi);
        case FCIDM_SHVIEW_COPY:
        case FCIDM_SHVIEW_CUT:
            return DoCopyOrCut(lpcmi, LOWORD(lpcmi->lpVerb) == FCIDM_SHVIEW_COPY);
        case FCIDM_SHVIEW_CREATELINK:
            return DoCreateLink(lpcmi);
        case FCIDM_SHVIEW_DELETE:
            return DoDelete(lpcmi);
        case FCIDM_SHVIEW_RENAME:
            return DoRename(lpcmi);
        case FCIDM_SHVIEW_PROPERTIES:
            return DoProperties(lpcmi);
        case 0x7ABC:
            return DoFormat(lpcmi);
    }

    if (iIdSHEFirst && iIdSHELast)
    {
        if (LOWORD(lpcmi->lpVerb) >= iIdSHEFirst && LOWORD(lpcmi->lpVerb) <= iIdSHELast)
        {
            return DoDynamicShellExtensions(lpcmi);
        }
    }

    if (iIdSCMFirst && iIdSCMLast)
    {
        if (LOWORD(lpcmi->lpVerb) >= iIdSCMFirst && LOWORD(lpcmi->lpVerb) <= iIdSCMLast)
        {
            return DoStaticShellExtensions(lpcmi);
        }
    }

    FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
    return E_UNEXPECTED;
}

HRESULT
WINAPI
IDefaultContextMenuImpl::GetCommandString(
    UINT_PTR idCommand,
    UINT uFlags,
    UINT* lpReserved,
    LPSTR lpszName,
    UINT uMaxNameLen)
{

    return S_OK;
}

HRESULT
WINAPI
IDefaultContextMenuImpl::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{

    return S_OK;
}

static 
HRESULT
IDefaultContextMenu_Constructor( 
    const DEFCONTEXTMENU *pdcm,
    REFIID riid,
    void **ppv)
{
    CComObject<IDefaultContextMenuImpl>        *theContextMenu;
    CComPtr<IUnknown>                        result;
    HRESULT                                    hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY (theContextMenu = new CComObject<IDefaultContextMenuImpl>);
    if (theContextMenu == NULL)
        return E_OUTOFMEMORY;
    hResult = theContextMenu->QueryInterface (riid, (void **)&result);
    if (FAILED (hResult))
    {
        delete theContextMenu;
        return hResult;
    }
    hResult = theContextMenu->Initialize (pdcm);
    if (FAILED (hResult))
        return hResult;
    *ppv = result.Detach ();
    TRACE("This(%p)(%x) cidl %u\n", *ppv, hResult, pdcm->cidl);
    return S_OK;
}

/*************************************************************************
 * SHCreateDefaultContextMenu            [SHELL32.325] Vista API
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
   if (FAILED(hr)) WARN("IDefaultContextMenu_Constructor failed: %x\n", hr);
   TRACE("pcm %p hr %x\n", pdcm, hr);
   return hr;
}

/*************************************************************************
 * CDefFolderMenu_Create2            [SHELL32.701]
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

   hr = SHCreateDefaultContextMenu(&pdcm, IID_IContextMenu, (void**)ppcm);
   return hr;
}

