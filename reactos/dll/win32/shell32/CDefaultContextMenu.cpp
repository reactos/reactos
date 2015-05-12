/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides default context menu implementation
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
 */

/*
TODO:
    The code in NotifyShellViewWindow to deliver commands to the view is broken. It is an excellent
    example of the wrong way to do it.
*/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmenu);

typedef struct _DynamicShellEntry_
{
    UINT iIdCmdFirst;
    UINT NumIds;
    CLSID ClassID;
    IContextMenu *pCM;
    struct _DynamicShellEntry_ *pNext;
} DynamicShellEntry, *PDynamicShellEntry;

typedef struct _StaticShellEntry_
{
    LPWSTR szVerb;
    LPWSTR szClass;
    struct _StaticShellEntry_ *pNext;
} StaticShellEntry, *PStaticShellEntry;

class CDefaultContextMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3
{
    private:
        CComPtr<IShellFolder> m_psf;
        UINT m_cidl;
        PCUITEMID_CHILD_ARRAY m_apidl;
        CComPtr<IDataObject> m_pDataObj;
        PIDLIST_ABSOLUTE m_pidlFolder;
        DWORD m_bGroupPolicyActive;
        PDynamicShellEntry m_pDynamicEntries; /* first dynamic shell extension entry */
        UINT m_iIdSHEFirst; /* first used id */
        UINT m_iIdSHELast; /* last used id */
        PStaticShellEntry m_pStaticEntries; /* first static shell extension entry */
        UINT m_iIdSCMFirst; /* first static used id */
        UINT m_iIdSCMLast; /* last static used id */

        void AddStaticEntry(LPCWSTR pwszVerb, LPCWSTR pwszClass);
        void AddStaticEntryForKey(HKEY hKey, LPCWSTR pwszClass);
        void AddStaticEntryForFileClass(LPCWSTR pwszExt);
        BOOL IsShellExtensionAlreadyLoaded(const CLSID *pclsid);
        HRESULT LoadDynamicContextMenuHandler(HKEY hKey, const CLSID *pclsid);
        BOOL EnumerateDynamicContextHandlerForKey(HKEY hRootKey);
        UINT InsertMenuItemsOfDynamicContextMenuExtension(HMENU hMenu, UINT IndexMenu, UINT idCmdFirst, UINT idCmdLast);
        UINT BuildBackgroundContextMenu(HMENU hMenu, UINT iIdCmdFirst, UINT iIdCmdLast, UINT uFlags);
        UINT AddStaticContextMenusToMenu(HMENU hMenu, UINT IndexMenu);
        UINT BuildShellItemContextMenu(HMENU hMenu, UINT iIdCmdFirst, UINT iIdCmdLast, UINT uFlags);
        HRESULT DoPaste(LPCMINVOKECOMMANDINFO lpcmi, BOOL bLink);
        HRESULT DoOpenOrExplore(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoCreateLink(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoRefresh(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoDelete(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoCopyOrCut(LPCMINVOKECOMMANDINFO lpcmi, BOOL bCopy);
        HRESULT DoRename(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoProperties(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoFormat(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoDynamicShellExtensions(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoStaticShellExtensions(LPCMINVOKECOMMANDINFO lpcmi);
        DWORD BrowserFlagsFromVerb(LPCMINVOKECOMMANDINFO lpcmi, PStaticShellEntry pEntry);
        HRESULT TryToBrowse(LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, DWORD wFlags);
        HRESULT InvokePidl(LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, PStaticShellEntry pEntry);
        PDynamicShellEntry GetDynamicEntry(UINT idCmd);

    public:
        CDefaultContextMenu();
        ~CDefaultContextMenu();
        HRESULT WINAPI Initialize(const DEFCONTEXTMENU *pdcm);

        // IContextMenu
        virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

        // IContextMenu2
        virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

        // IContextMenu3
        virtual HRESULT WINAPI HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        BEGIN_COM_MAP(CDefaultContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
        END_COM_MAP()
};

CDefaultContextMenu::CDefaultContextMenu() :
    m_psf(NULL),
    m_cidl(0),
    m_apidl(NULL),
    m_pDataObj(NULL),
    m_pidlFolder(NULL),
    m_bGroupPolicyActive(0),
    m_pDynamicEntries(NULL),
    m_iIdSHEFirst(0),
    m_iIdSHELast(0),
    m_pStaticEntries(NULL),
    m_iIdSCMFirst(0),
    m_iIdSCMLast(0)
{
}

CDefaultContextMenu::~CDefaultContextMenu()
{
    /* Free dynamic shell extension entries */
    PDynamicShellEntry pDynamicEntry = m_pDynamicEntries, pNextDynamic;
    while (pDynamicEntry)
    {
        pNextDynamic = pDynamicEntry->pNext;
        pDynamicEntry->pCM->Release();
        HeapFree(GetProcessHeap(), 0, pDynamicEntry);
        pDynamicEntry = pNextDynamic;
    }

    /* Free static shell extension entries */
    PStaticShellEntry pStaticEntry = m_pStaticEntries, pNextStatic;
    while (pStaticEntry)
    {
        pNextStatic = pStaticEntry->pNext;
        HeapFree(GetProcessHeap(), 0, pStaticEntry->szClass);
        HeapFree(GetProcessHeap(), 0, pStaticEntry->szVerb);
        HeapFree(GetProcessHeap(), 0, pStaticEntry);
        pStaticEntry = pNextStatic;
    }

    if (m_pidlFolder)
        CoTaskMemFree(m_pidlFolder);
    _ILFreeaPidl(const_cast<PITEMID_CHILD *>(m_apidl), m_cidl);
}

HRESULT WINAPI CDefaultContextMenu::Initialize(const DEFCONTEXTMENU *pdcm)
{
    CComPtr<IDataObject> pDataObj;

    TRACE("cidl %u\n", pdcm->cidl);

    m_cidl = pdcm->cidl;
    m_apidl = const_cast<PCUITEMID_CHILD_ARRAY>(_ILCopyaPidl(pdcm->apidl, m_cidl));
    if (m_cidl && !m_apidl)
        return E_OUTOFMEMORY;
    m_psf = pdcm->psf;

    if (SUCCEEDED(SHCreateDataObject(pdcm->pidlFolder, pdcm->cidl, pdcm->apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj))))
        m_pDataObj = pDataObj;

    if (pdcm->pidlFolder)
    {
        m_pidlFolder = ILClone(pdcm->pidlFolder);
    }
    else
    {
        CComPtr<IPersistFolder2> pf = NULL;
        if (SUCCEEDED(m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pf))))
        {
            if (FAILED(pf->GetCurFolder(reinterpret_cast<LPITEMIDLIST*>(&m_pidlFolder))))
                ERR("GetCurFolder failed\n");
        }
        TRACE("pidlFolder %p\n", m_pidlFolder);
    }

    return S_OK;
}

void
CDefaultContextMenu::AddStaticEntry(const WCHAR *szVerb, const WCHAR *szClass)
{
    PStaticShellEntry pEntry = m_pStaticEntries, pLastEntry = NULL;
    while(pEntry)
    {
        if (!wcsicmp(pEntry->szVerb, szVerb))
        {
            /* entry already exists */
            return;
        }
        pLastEntry = pEntry;
        pEntry = pEntry->pNext;
    }

    TRACE("adding verb %s szClass %s\n", debugstr_w(szVerb), debugstr_w(szClass));

    pEntry = (StaticShellEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(StaticShellEntry));
    if (pEntry)
    {
        pEntry->pNext = NULL;
        pEntry->szVerb = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(szVerb) + 1) * sizeof(WCHAR));
        if (pEntry->szVerb)
            wcscpy(pEntry->szVerb, szVerb);
        pEntry->szClass = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(szClass) + 1) * sizeof(WCHAR));
        if (pEntry->szClass)
            wcscpy(pEntry->szClass, szClass);
    }

    if (!wcsicmp(szVerb, L"open"))
    {
        /* open verb is always inserted in front */
        pEntry->pNext = m_pStaticEntries;
        m_pStaticEntries = pEntry;
    }
    else if (pLastEntry)
        pLastEntry->pNext = pEntry;
    else
        m_pStaticEntries = pEntry;
}

void
CDefaultContextMenu::AddStaticEntryForKey(HKEY hKey, const WCHAR *pwszClass)
{
    WCHAR wszName[40];
    DWORD cchName, dwIndex = 0;

    TRACE("AddStaticEntryForKey %x %ls\n", hKey, pwszClass);

    while(TRUE)
    {
        cchName = _countof(wszName);
        if (RegEnumKeyExW(hKey, dwIndex++, wszName, &cchName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        AddStaticEntry(wszName, pwszClass);
    }
}

void
CDefaultContextMenu::AddStaticEntryForFileClass(const WCHAR * szExt)
{
    WCHAR szBuffer[100];
    HKEY hKey;
    LONG result;
    DWORD dwBuffer;
    UINT Length;
    static WCHAR szShell[] = L"\\shell";
    static WCHAR szShellAssoc[] = L"SystemFileAssociations\\";

    TRACE("AddStaticEntryForFileClass entered with %s\n", debugstr_w(szExt));

    Length = wcslen(szExt);
    if (Length + (sizeof(szShell) / sizeof(WCHAR)) + 1 < sizeof(szBuffer) / sizeof(WCHAR))
    {
        wcscpy(szBuffer, szExt);
        wcscpy(&szBuffer[Length], szShell);
        result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            szBuffer[Length] = 0;
            AddStaticEntryForKey(hKey, szExt);
            RegCloseKey(hKey);
        }
    }

    dwBuffer = sizeof(szBuffer);
    result = RegGetValueW(HKEY_CLASSES_ROOT, szExt, NULL, RRF_RT_REG_SZ, NULL, (LPBYTE)szBuffer, &dwBuffer);
    if (result == ERROR_SUCCESS)
    {
        Length = wcslen(szBuffer);
        if (Length + (sizeof(szShell) / sizeof(WCHAR)) + 1 < sizeof(szBuffer) / sizeof(WCHAR))
        {
            wcscpy(&szBuffer[Length], szShell);
            TRACE("szBuffer %s\n", debugstr_w(szBuffer));

            result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);
            if (result == ERROR_SUCCESS)
            {
                szBuffer[Length] = 0;
                AddStaticEntryForKey(hKey, szBuffer);
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
            AddStaticEntryForKey(hKey, szBuffer);
            RegCloseKey(hKey);
        }
    }
}

static
BOOL
HasClipboardData()
{
    BOOL bRet = FALSE;
    CComPtr<IDataObject> pDataObj;

    if(SUCCEEDED(OleGetClipboard(&pDataObj)))
    {
        STGMEDIUM medium;
        FORMATETC formatetc;

        TRACE("pDataObj=%p\n", pDataObj.p);

        /* Set the FORMATETC structure*/
        InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
        if(SUCCEEDED(pDataObj->GetData(&formatetc, &medium)))
        {
            bRet = TRUE;
            ReleaseStgMedium(&medium);
        }
    }

    return bRet;
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
CDefaultContextMenu::IsShellExtensionAlreadyLoaded(const CLSID *pclsid)
{
    PDynamicShellEntry pEntry = m_pDynamicEntries;

    while (pEntry)
    {
        if (!memcmp(&pEntry->ClassID, pclsid, sizeof(CLSID)))
            return TRUE;
        pEntry = pEntry->pNext;
    }

    return FALSE;
}

HRESULT
CDefaultContextMenu::LoadDynamicContextMenuHandler(HKEY hKey, const CLSID *pclsid)
{
    HRESULT hr;

    TRACE("LoadDynamicContextMenuHandler entered with This %p hKey %p pclsid %s\n", this, hKey, wine_dbgstr_guid(pclsid));

    if (IsShellExtensionAlreadyLoaded(pclsid))
        return S_OK;

    CComPtr<IContextMenu> pcm;
    hr = SHCoCreateInstance(NULL, pclsid, NULL, IID_PPV_ARG(IContextMenu, &pcm));
    if (hr != S_OK)
    {
        ERR("SHCoCreateInstance failed %x\n", GetLastError());
        return hr;
    }

    CComPtr<IShellExtInit> pExtInit;
    hr = pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &pExtInit));
    if (hr != S_OK)
    {
        ERR("Failed to query for interface IID_IShellExtInit hr %x pclsid %s\n", hr, wine_dbgstr_guid(pclsid));
        return hr;
    }

    hr = pExtInit->Initialize(m_pidlFolder, m_pDataObj, hKey);
    if (hr != S_OK)
    {
        TRACE("Failed to initialize shell extension error %x pclsid %s\n", hr, wine_dbgstr_guid(pclsid));
        return hr;
    }

    PDynamicShellEntry pEntry = (DynamicShellEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(DynamicShellEntry));
    if (!pEntry)
    {
        return E_OUTOFMEMORY;
    }

    pEntry->iIdCmdFirst = 0;
    pEntry->pNext = NULL;
    pEntry->NumIds = 0;
    pEntry->pCM = pcm.Detach();
    memcpy(&pEntry->ClassID, pclsid, sizeof(CLSID));

    if (m_pDynamicEntries)
    {
        PDynamicShellEntry pLastEntry = m_pDynamicEntries;

        while (pLastEntry->pNext)
            pLastEntry = pLastEntry->pNext;

        pLastEntry->pNext = pEntry;
    }
    else
        m_pDynamicEntries = pEntry;

    return S_OK;
}

BOOL
CDefaultContextMenu::EnumerateDynamicContextHandlerForKey(HKEY hRootKey)
{

    WCHAR wszName[MAX_PATH], wszBuf[MAX_PATH], *pwszClsid;
    DWORD cchName;
    HRESULT hr;
    HKEY hKey;

    if (RegOpenKeyExW(hRootKey, L"shellex\\ContextMenuHandlers", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyExW failed\n");
        return FALSE;
    }

    DWORD dwIndex = 0;
    while (TRUE)
    {
        cchName = _countof(wszName);
        if (RegEnumKeyExW(hKey, dwIndex++, wszName, &cchName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        /* Key name or key value is CLSID */
        CLSID clsid;
        hr = CLSIDFromString(wszName, &clsid);
        if (hr == S_OK)
            pwszClsid = wszName;
        else
        {
            DWORD cchBuf = _countof(wszBuf);
            if (RegGetValueW(hKey, wszName, NULL, RRF_RT_REG_SZ, NULL, wszBuf, &cchBuf) == ERROR_SUCCESS)
                hr = CLSIDFromString(wszBuf, &clsid);
            pwszClsid = wszBuf;
        }
        if (SUCCEEDED(hr))
        {
            if (m_bGroupPolicyActive)
            {
                if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                                 pwszClsid,
                                 RRF_RT_REG_SZ,
                                 NULL,
                                 NULL,
                                 NULL) == ERROR_SUCCESS)
                {
                    LoadDynamicContextMenuHandler(hKey, &clsid);
                }
            }
            else
                LoadDynamicContextMenuHandler(hKey, &clsid);
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}

UINT
CDefaultContextMenu::InsertMenuItemsOfDynamicContextMenuExtension(HMENU hMenu, UINT IndexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    if (!m_pDynamicEntries)
    {
        m_iIdSHEFirst = 0;
        m_iIdSHELast = 0;
        return IndexMenu;
    }

    PDynamicShellEntry pEntry = m_pDynamicEntries;
    idCmdFirst = 0x5000;
    idCmdLast =  0x6000;
    m_iIdSHEFirst = idCmdFirst;
    do
    {
        HRESULT hr = pEntry->pCM->QueryContextMenu(hMenu, IndexMenu++, idCmdFirst, idCmdLast, CMF_NORMAL);
        if (SUCCEEDED(hr))
        {
            pEntry->iIdCmdFirst = idCmdFirst;
            pEntry->NumIds = LOWORD(hr);
            IndexMenu += pEntry->NumIds;
            idCmdFirst += pEntry->NumIds + 0x10;
        }
        TRACE("pEntry %p hr %x contextmenu %p cmdfirst %x num ids %x\n", pEntry, hr, pEntry->pCM, pEntry->iIdCmdFirst, pEntry->NumIds);
        pEntry = pEntry->pNext;
    } while (pEntry);

    m_iIdSHELast = idCmdFirst;
    TRACE("SH_LoadContextMenuHandlers first %x last %x\n", m_iIdSHEFirst, m_iIdSHELast);
    return IndexMenu;
}

UINT
CDefaultContextMenu::BuildBackgroundContextMenu(
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast,
    UINT uFlags)
{
    UINT IndexMenu = 0;
    HMENU hSubMenu;

    TRACE("BuildBackgroundContextMenu entered\n");

    SFGAOF rfg = SFGAO_FILESYSTEM | SFGAO_FOLDER;
    HRESULT hr = m_psf->GetAttributesOf(0, NULL, &rfg);
    if (FAILED(hr))
    {
        ERR("GetAttributesOf failed: %x\n", hr);
        rfg = 0;
    }

    if (!_ILIsDesktop(m_pidlFolder))
    {
        WCHAR wszBuf[MAX_PATH];

        /* view option is only available in browsing mode */
        hSubMenu = LoadMenuW(shell32_hInstance, L"MENU_001");
        if (hSubMenu && LoadStringW(shell32_hInstance, FCIDM_SHVIEW_VIEW, wszBuf, _countof(wszBuf)))
        {
            TRACE("wszBuf %s\n", debugstr_w(wszBuf));

            MENUITEMINFOW mii;
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.wID = iIdCmdFirst++;
            mii.dwTypeData = wszBuf;
            mii.cch = wcslen(mii.dwTypeData);
            mii.fState = MFS_ENABLED;
            mii.hSubMenu = hSubMenu;
            InsertMenuItemW(hMenu, IndexMenu++, TRUE, &mii);
            DestroyMenu(hSubMenu);
        }
    }

    hSubMenu = LoadMenuW(shell32_hInstance, L"MENU_002");
    if (hSubMenu)
    {
        /* merge general background context menu in */
        iIdCmdFirst = Shell_MergeMenus(hMenu, GetSubMenu(hSubMenu, 0), IndexMenu, 0, 0xFFFF, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS) + 1;
        DestroyMenu(hSubMenu);
    }

    if (!HasClipboardData())
    {
        TRACE("disabling paste options\n");
        DisablePasteOptions(hMenu);
    }

    /* Directory is progid of filesystem folders only */
    if ((rfg & (SFGAO_FILESYSTEM|SFGAO_FOLDER)) == (SFGAO_FILESYSTEM|SFGAO_FOLDER))
    {
        /* Load context menu handlers */
        TRACE("Add background handlers: %p\n", m_pidlFolder);
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Directory\\Background", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }

        if (InsertMenuItemsOfDynamicContextMenuExtension(hMenu, GetMenuItemCount(hMenu) - 1, iIdCmdFirst, iIdCmdLast))
        {
            /* seperate dynamic context menu items */
            _InsertMenuItemW(hMenu, GetMenuItemCount(hMenu) - 1, TRUE, -1, MFT_SEPARATOR, NULL, MFS_ENABLED);
        }
    }

    return iIdCmdLast;
}

UINT
CDefaultContextMenu::AddStaticContextMenusToMenu(
    HMENU hMenu,
    UINT IndexMenu)
{
    MENUITEMINFOW mii;
    UINT idResource;
    WCHAR wszVerb[40];
    UINT fState;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.wID = 0x4000;
    mii.dwTypeData = NULL;
    m_iIdSCMFirst = mii.wID;

    PStaticShellEntry pEntry = m_pStaticEntries;

    while (pEntry)
    {
        fState = MFS_ENABLED;
        mii.dwTypeData = NULL;

        /* set first entry as default */
        if (pEntry == m_pStaticEntries)
            fState |= MFS_DEFAULT;

        if (!wcsicmp(pEntry->szVerb, L"open"))
        {
            /* override default when open verb is found */
            fState |= MFS_DEFAULT;
            idResource = IDS_OPEN_VERB;
        }
        else if (!wcsicmp(pEntry->szVerb, L"explore"))
            idResource = IDS_EXPLORE_VERB;
        else if (!wcsicmp(pEntry->szVerb, L"runas"))
            idResource = IDS_RUNAS_VERB;
        else if (!wcsicmp(pEntry->szVerb, L"edit"))
            idResource = IDS_EDIT_VERB;
        else if (!wcsicmp(pEntry->szVerb, L"find"))
            idResource = IDS_FIND_VERB;
        else if (!wcsicmp(pEntry->szVerb, L"print"))
            idResource = IDS_PRINT_VERB;
        else if (!wcsicmp(pEntry->szVerb, L"printto"))
        {
            pEntry = pEntry->pNext;
            continue;
        }
        else
            idResource = 0;

        /* By default use verb for menu item name */
        mii.dwTypeData = pEntry->szVerb;

        if (idResource > 0)
        {
            if (LoadStringW(shell32_hInstance, idResource, wszVerb, _countof(wszVerb)))
                mii.dwTypeData = wszVerb; /* use translated verb */
            else
                ERR("Failed to load string\n");
        }
        else
        {
            WCHAR wszKey[256];
            HRESULT hr = StringCbPrintfW(wszKey, sizeof(wszKey), L"%s\\shell\\%s", pEntry->szClass, pEntry->szVerb);

            if (SUCCEEDED(hr))
            {
                HKEY hkVerb;
                DWORD cbVerb = sizeof(wszVerb);
                LONG res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszKey, &hkVerb);
                if (res == ERROR_SUCCESS)
                {
                    res = RegLoadMUIStringW(hkVerb, 
                                            NULL,
                                            wszVerb,
                                            cbVerb,
                                            NULL,
                                            0,
                                            NULL);
                    if (res == ERROR_SUCCESS)
                    {
                        /* use description for the menu entry */
                        mii.dwTypeData = wszVerb; 
                    }

                    RegCloseKey(hkVerb);
                }
            }
        }

        mii.cch = wcslen(mii.dwTypeData);
        mii.fState = fState;
        InsertMenuItemW(hMenu, IndexMenu++, TRUE, &mii);

        mii.wID++;
        pEntry = pEntry->pNext;
    }

    m_iIdSCMLast = mii.wID - 1;
    return IndexMenu;
}

void WINAPI _InsertMenuItemW(
    HMENU hMenu,
    UINT indexMenu,
    BOOL fByPosition,
    UINT wID,
    UINT fType,
    LPCWSTR dwTypeData,
    UINT fState)
{
    MENUITEMINFOW mii;
    WCHAR wszText[100];

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    if (fType == MFT_SEPARATOR)
        mii.fMask = MIIM_ID | MIIM_TYPE;
    else if (fType == MFT_STRING)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
        if ((ULONG_PTR)HIWORD((ULONG_PTR)dwTypeData) == 0)
        {
            if (LoadStringW(shell32_hInstance, LOWORD((ULONG_PTR)dwTypeData), wszText, _countof(wszText)))
                mii.dwTypeData = wszText;
            else
            {
                ERR("failed to load string %p\n", dwTypeData);
                return;
            }
        }
        else
            mii.dwTypeData = (LPWSTR)dwTypeData;
        mii.fState = fState;
    }

    mii.wID = wID;
    mii.fType = fType;
    InsertMenuItemW(hMenu, indexMenu, fByPosition, &mii);
}

UINT
CDefaultContextMenu::BuildShellItemContextMenu(
    HMENU hMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast,
    UINT uFlags)
{
    HKEY hKey;
    HRESULT hr;

    TRACE("BuildShellItemContextMenu entered\n");
    ASSERT(m_cidl >= 1);

    STRRET strFile;
    hr = m_psf->GetDisplayNameOf(m_apidl[0], SHGDN_FORPARSING, &strFile);
    if (hr == S_OK)
    {
        WCHAR wszPath[MAX_PATH];
        hr = StrRetToBufW(&strFile, m_apidl[0], wszPath, _countof(wszPath));
        if (hr == S_OK)
        {
            LPCWSTR pwszExt = PathFindExtensionW(wszPath);
            if (pwszExt[0])
            {
                /* enumerate dynamic/static for a given file class */
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    /* add static verbs */
                    AddStaticEntryForFileClass(pwszExt);

                    /* load dynamic extensions from file extension key */
                    EnumerateDynamicContextHandlerForKey(hKey);
                    RegCloseKey(hKey);
                }

                WCHAR wszTemp[40];
                DWORD dwSize = sizeof(wszTemp);
                if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, NULL, RRF_RT_REG_SZ, NULL, wszTemp, &dwSize) == ERROR_SUCCESS)
                {
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszTemp, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                        /* add static verbs from progid key */
                        AddStaticEntryForFileClass(wszTemp);

                        /* load dynamic extensions from progid key */
                        EnumerateDynamicContextHandlerForKey(hKey);
                        RegCloseKey(hKey);
                    }
                }
            }
        }
    }
    else
        ERR("GetDisplayNameOf failed: %x\n", hr);

    GUID *pGuid = _ILGetGUIDPointer(m_apidl[0]);
    if (pGuid)
    {
        LPOLESTR pwszCLSID;
        WCHAR buffer[60];

        wcscpy(buffer, L"CLSID\\");
        hr = StringFromCLSID(*pGuid, &pwszCLSID);
        if (hr == S_OK)
        {
            wcscpy(&buffer[6], pwszCLSID);
            TRACE("buffer %s\n", debugstr_w(buffer));
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, buffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(hKey);
                AddStaticEntryForFileClass(buffer);
                RegCloseKey(hKey);
            }
            CoTaskMemFree(pwszCLSID);
        }
    }

    if (_ILIsDrive(m_apidl[0]))
    {
        AddStaticEntryForFileClass(L"Drive");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Drive", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }

    }

    /* add static actions */
    SFGAOF rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_FILESYSTEM | SFGAO_FOLDER;
    hr = m_psf->GetAttributesOf(m_cidl, m_apidl, &rfg);
    if (FAILED(hr))
    {
        ERR("GetAttributesOf failed: %x\n", hr);
        rfg = 0;
    }

    if (rfg & SFGAO_FOLDER)
    {
        /* add the default verbs open / explore */
        AddStaticEntryForFileClass(L"Folder");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Folder", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }

        /* Directory is only loaded for real filesystem directories */
        if (rfg & SFGAO_FILESYSTEM)
        {
            AddStaticEntryForFileClass(L"Directory");
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Directory", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(hKey);
                RegCloseKey(hKey);
            }
        }
    }

    /* AllFilesystemObjects class is loaded only for files and directories */
    if (rfg & SFGAO_FILESYSTEM)
    {
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"AllFilesystemObjects", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* sendto service is registered here */
            EnumerateDynamicContextHandlerForKey(hKey);
            RegCloseKey(hKey);
        }

        if (!(rfg & SFGAO_FOLDER))
        {
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"*", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                EnumerateDynamicContextHandlerForKey(hKey);
                RegCloseKey(hKey);
            }
        }
    }

    /* add static context menu handlers */
    UINT IndexMenu = AddStaticContextMenusToMenu(hMenu, 0);

    /* now process dynamic context menu handlers */
    BOOL bAddSep = FALSE;
    IndexMenu = InsertMenuItemsOfDynamicContextMenuExtension(hMenu, IndexMenu, iIdCmdFirst, iIdCmdLast);
    TRACE("IndexMenu %d\n", IndexMenu);

    if (_ILIsDrive(m_apidl[0]))
    {
        char szDrive[8] = {0};
        DWORD dwFlags;

        _ILGetDrive(m_apidl[0], szDrive, sizeof(szDrive));
        if (GetVolumeInformationA(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0))
        {
            /* Disable format if read only */
            if (!(dwFlags & FILE_READ_ONLY_VOLUME))
            {
                _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
                _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0x7ABC, MFT_STRING, MAKEINTRESOURCEW(IDS_FORMATDRIVE), MFS_ENABLED);
                bAddSep = TRUE;
            }
        }
    }

    BOOL bClipboardData = (HasClipboardData() && (rfg & SFGAO_FILESYSTEM));
    if (rfg & (SFGAO_CANCOPY | SFGAO_CANMOVE) || bClipboardData)
    {
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        if (rfg & SFGAO_CANMOVE)
            _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_CUT, MFT_STRING, MAKEINTRESOURCEW(IDS_CUT), MFS_ENABLED);
        if (rfg & SFGAO_CANCOPY)
            _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_COPY, MFT_STRING, MAKEINTRESOURCEW(IDS_COPY), MFS_ENABLED);
        if (bClipboardData)
            _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_INSERT, MFT_STRING, MAKEINTRESOURCEW(IDS_PASTE), MFS_ENABLED);

        bAddSep = TRUE;
    }

    if (rfg & SFGAO_CANLINK)
    {
        bAddSep = FALSE;
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_CREATELINK, MFT_STRING, MAKEINTRESOURCEW(IDS_CREATELINK), MFS_ENABLED);
    }

    if (rfg & SFGAO_CANDELETE)
    {
        if (bAddSep)
        {
            bAddSep = FALSE;
            _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_DELETE, MFT_STRING, MAKEINTRESOURCEW(IDS_DELETE), MFS_ENABLED);
    }

    if (rfg & SFGAO_CANRENAME)
    {
        if (bAddSep)
        {
            _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        }
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_RENAME, MFT_STRING, MAKEINTRESOURCEW(IDS_RENAME), MFS_ENABLED);
        bAddSep = TRUE;
    }

    if (rfg & SFGAO_HASPROPSHEET)
    {
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
        _InsertMenuItemW(hMenu, IndexMenu++, TRUE, FCIDM_SHVIEW_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
    }

    return iIdCmdLast;
}

HRESULT
WINAPI
CDefaultContextMenu::QueryContextMenu(
    HMENU hMenu,
    UINT IndexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    if (m_cidl)
        idCmdFirst = BuildShellItemContextMenu(hMenu, idCmdFirst, idCmdLast, uFlags);
    else
        idCmdFirst = BuildBackgroundContextMenu(hMenu, idCmdFirst, idCmdLast, uFlags);

    return S_OK;
}

static
HRESULT
NotifyShellViewWindow(LPCMINVOKECOMMANDINFO lpcmi, BOOL bRefresh)
{
    /* Note: CWM_GETISHELLBROWSER returns not referenced object */
    LPSHELLBROWSER lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    if (!lpSB)
        return E_FAIL;

    CComPtr<IShellView> lpSV;
    if (FAILED(lpSB->QueryActiveShellView(&lpSV)))
        return E_FAIL;

    HWND hwndSV = NULL;
    if (SUCCEEDED(lpSV->GetWindow(&hwndSV)))
        SendMessageW(hwndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);
    return S_OK;
}

HRESULT
CDefaultContextMenu::DoRefresh(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    CComPtr<IPersistFolder2> ppf2 = NULL;
    LPITEMIDLIST pidl;
    HRESULT hr = m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    if (SUCCEEDED(hr))
    {
        hr = ppf2->GetCurFolder(&pidl);
        if (SUCCEEDED(hr))
        {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, NULL);
            ILFree(pidl);
        }
    }
    return hr;
}

HRESULT
CDefaultContextMenu::DoPaste(
    LPCMINVOKECOMMANDINFO lpcmi, BOOL bLink)
{
    HRESULT hr;

    CComPtr<IDataObject> pda;
    hr = OleGetClipboard(&pda);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IShellFolder> psfTarget = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    /* Find target folder */
    if (m_cidl)
    {
        hr = m_psf->BindToObject(m_apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
    }
    else
    {
        CComPtr<IPersistFolder2> ppf2 = NULL;
        LPITEMIDLIST pidl;

        /* cidl is zero due to explorer view */
        hr = m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
        if (SUCCEEDED(hr))
        {
            hr = ppf2->GetCurFolder(&pidl);
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
                    hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
                }
                TRACE("psfTarget %x %p, Desktop %u\n", hr, psfTarget.p, _ILIsDesktop(pidl));
                ILFree(pidl);
            }
        }
    }

    if (FAILED(hr))
    {
        ERR("no IShellFolder\n");
        return hr;
    }

    FORMATETC formatetc2;
    STGMEDIUM medium2;
    InitFormatEtc(formatetc2, RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT), TYMED_HGLOBAL);

    DWORD dwKey= 0;

    if (SUCCEEDED(pda->GetData(&formatetc2, &medium2)))
    {
        DWORD * pdwFlag = (DWORD*)GlobalLock(medium2.hGlobal);
        if (pdwFlag)
        {
            if (*pdwFlag == DROPEFFECT_COPY)
                dwKey = MK_CONTROL;
            else
                dwKey = MK_SHIFT;
        }
        else {
            ERR("No drop effect obtained");
        }
        GlobalUnlock(medium2.hGlobal);
    }

    if (bLink)
    {
        dwKey = MK_CONTROL|MK_SHIFT;
    }

    CComPtr<IDropTarget> pdrop;
    hr = psfTarget->QueryInterface(IID_PPV_ARG(IDropTarget, &pdrop));
    if (FAILED(hr))
    {
        ERR("Error getting IDropTarget interface\n");
        return hr;
    }

    SHSimulateDrop(pdrop, pda, dwKey, NULL, NULL);

    TRACE("CP result %x\n", hr);
    return S_OK;
}

HRESULT
CDefaultContextMenu::DoOpenOrExplore(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    UNIMPLEMENTED;
    return E_FAIL;
}

HRESULT
CDefaultContextMenu::DoCreateLink(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    CComPtr<IDataObject> pDataObj;
    CComPtr<IDropTarget> pDT;
    HRESULT hr;
    CComPtr<IPersistFolder2> ppf2 = NULL;
    LPITEMIDLIST pidl;
    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IShellFolder> psfTarget = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr = SHCreateDataObject(m_pidlFolder, m_cidl, m_apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj))))
    {
        hr = m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
        if (SUCCEEDED(hr))
        {
            hr = ppf2->GetCurFolder(&pidl);
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
                    hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
                }
                TRACE("psfTarget %x %p, Desktop %u\n", hr, psfTarget.p, _ILIsDesktop(pidl));
                ILFree(pidl);
            }
        }

    }

    if (FAILED(hr))
    {
        ERR("no IShellFolder\n");
        return hr;
    }

    hr = psfTarget->QueryInterface(IID_PPV_ARG(IDropTarget, &pDT));
    if (FAILED(hr))
    {
        ERR("no IDropTarget Interface\n");
        return hr;
    }
    SHSimulateDrop(pDT, pDataObj, MK_CONTROL|MK_SHIFT, NULL, NULL);

    return S_OK;
}

HRESULT CDefaultContextMenu::DoDelete(LPCMINVOKECOMMANDINFO lpcmi) {
    TRACE("(%p) Deleting\n", this);

    CComPtr<IDataObject> pDataObject;

    if (SUCCEEDED(SHCreateDataObject(m_pidlFolder, m_cidl, m_apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObject))))
    {
        IStream *s;
        CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObject, &s);
        SHCreateThread(DoDeleteThreadProc, s, NULL, NULL);
    }
    else
        return E_FAIL;
    return S_OK;
}

HRESULT
CDefaultContextMenu::DoCopyOrCut(
    LPCMINVOKECOMMANDINFO lpcmi,
    BOOL bCopy)
{
    CComPtr<IDataObject> pDataObj;
    HRESULT hr;

    if (SUCCEEDED(SHCreateDataObject(m_pidlFolder, m_cidl, m_apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj))))
    {
        if (!bCopy)
        {
            FORMATETC formatetc;
            STGMEDIUM medium;
            InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT), TYMED_HGLOBAL);
            pDataObj->GetData(&formatetc, &medium);
            DWORD * pdwFlag = (DWORD*)GlobalLock(medium.hGlobal);
            if (pdwFlag)
                *pdwFlag = DROPEFFECT_MOVE;
            GlobalUnlock(medium.hGlobal);
            pDataObj->SetData(&formatetc, &medium, TRUE);
        }

        hr = OleSetClipboard(pDataObj);
        return hr;
    }

    /* Note: CWM_GETISHELLBROWSER returns not referenced object */
    LPSHELLBROWSER lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    if (!lpSB)
    {
        ERR("failed to get shellbrowser\n");
        return E_FAIL;
    }

    CComPtr<IShellView> lpSV;
    hr = lpSB->QueryActiveShellView(&lpSV);
    if (FAILED(hr))
    {
        ERR("failed to query the active shellview\n");
        return hr;
    }

    hr = lpSV->GetItemObject(SVGIO_SELECTION, IID_PPV_ARG(IDataObject, &pDataObj));
    if (SUCCEEDED(hr))
    {
        hr = OleSetClipboard(pDataObj);
        if (FAILED(hr))
            ERR("OleSetClipboard failed");
        pDataObj->Release();
    } else
        ERR("failed to get item object\n");

    return hr;
}

HRESULT
CDefaultContextMenu::DoRename(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    /* get the active IShellView. Note: CWM_GETISHELLBROWSER returns not referenced object */
    LPSHELLBROWSER lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    if (!lpSB)
    {
        ERR("CWM_GETISHELLBROWSER failed\n");
        return E_FAIL;
    }

    /* is the treeview focused */
    HWND hwnd;
    if (SUCCEEDED(lpSB->GetControlWindow(FCW_TREE, &hwnd)))
    {
        HTREEITEM hItem = TreeView_GetSelection(hwnd);
        if (hItem)
            (void)TreeView_EditLabel(hwnd, hItem);
    }

    CComPtr<IShellView> lpSV;
    HRESULT hr = lpSB->QueryActiveShellView(&lpSV);
    if (FAILED(hr))
    {
        ERR("CWM_GETISHELLBROWSER failed\n");
        return hr;
    }

    SVSIF selFlags = SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT;
    lpSV->SelectItem(m_apidl[0], selFlags);
    return S_OK;
}

HRESULT
CDefaultContextMenu::DoProperties(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = S_OK;
    const ITEMIDLIST *pidlParent = m_pidlFolder, *pidlChild;

    if (!pidlParent)
    {
        CComPtr<IPersistFolder2> pf;

        /* pidlFolder is optional */
        if (SUCCEEDED(m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pf))))
        {
            pf->GetCurFolder((_ITEMIDLIST**)&pidlParent);
        }
    }

    if (m_cidl > 0)
        pidlChild = m_apidl[0];
    else
    {
        /* Set pidlChild to last pidl of current folder */
        if (pidlParent == m_pidlFolder)
            pidlParent = (ITEMIDLIST*)ILClone(pidlParent);

        pidlChild = (ITEMIDLIST*)ILClone(ILFindLastID(pidlParent));
        ILRemoveLastID((ITEMIDLIST*)pidlParent);
    }

    if (_ILIsMyComputer(pidlChild))
    {
        if (32 >= (UINT)ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL sysdm.cpl", NULL, NULL, SW_SHOWNORMAL))
            hr = E_FAIL;
    }
    else if (_ILIsDesktop(pidlChild))
    {
        if (32 >= (UINT)ShellExecuteW(lpcmi->hwnd, L"open", L"rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL))
            hr = E_FAIL;
    }
    else if (_ILIsDrive(pidlChild))
    {
        WCHAR wszBuf[MAX_PATH];
        ILGetDisplayName(pidlChild, wszBuf);
        if (!SH_ShowDriveProperties(wszBuf, pidlParent, &pidlChild))
            hr = E_FAIL;
    }
    else if (_ILIsNetHood(pidlChild))
    {
        // FIXME path!
        if (32 >= (UINT)ShellExecuteW(NULL, L"open", L"explorer.exe",
                                      L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}",
                                      NULL, SW_SHOWDEFAULT))
            hr = E_FAIL;
    }
    else if (_ILIsBitBucket(pidlChild))
    {
        /* FIXME: detect the drive path of bitbucket if appropiate */
        if(!SH_ShowRecycleBinProperties(L'C'))
            hr = E_FAIL;
    }
    else
    {
        if (m_cidl > 1)
            WARN("SHMultiFileProperties is not yet implemented\n");

        STRRET strFile;
        hr = m_psf->GetDisplayNameOf(pidlChild, SHGDN_FORPARSING, &strFile);
        if (SUCCEEDED(hr))
        {
            WCHAR wszBuf[MAX_PATH];
            hr = StrRetToBufW(&strFile, pidlChild, wszBuf, _countof(wszBuf));
            if (SUCCEEDED(hr))
                hr = SH_ShowPropertiesDialog(wszBuf, pidlParent, &pidlChild);
            else
                ERR("StrRetToBufW failed\n");
        }
        else
            ERR("IShellFolder_GetDisplayNameOf failed for apidl\n");
    }

    /* Free allocated PIDLs */
    if (pidlParent != m_pidlFolder)
        ILFree((ITEMIDLIST*)pidlParent);
    if (m_cidl < 1 || pidlChild != m_apidl[0])
        ILFree((ITEMIDLIST*)pidlChild);

    return hr;
}

HRESULT
CDefaultContextMenu::DoFormat(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    char szDrive[8] = {0};

    if (!_ILGetDrive(m_apidl[0], szDrive, sizeof(szDrive)))
    {
        ERR("pidl is not a drive\n");
        return E_FAIL;
    }

    SHFormatDrive(lpcmi->hwnd, szDrive[0] - 'A', SHFMT_ID_DEFAULT, 0);
    return S_OK;
}

PDynamicShellEntry CDefaultContextMenu::GetDynamicEntry(UINT idCmd)
{
    PDynamicShellEntry pEntry = m_pDynamicEntries;

    while(pEntry && idCmd > pEntry->iIdCmdFirst + pEntry->NumIds)
        pEntry = pEntry->pNext;

    if (!pEntry)
        return NULL;

    if (idCmd < pEntry->iIdCmdFirst || idCmd > pEntry->iIdCmdFirst + pEntry->NumIds)
        return NULL;

    return pEntry;
}

HRESULT
CDefaultContextMenu::DoDynamicShellExtensions(
    LPCMINVOKECOMMANDINFO lpcmi)
{    
    TRACE("verb %p first %x last %x", lpcmi->lpVerb, m_iIdSHEFirst, m_iIdSHELast);

    UINT idCmd = LOWORD(lpcmi->lpVerb);
    PDynamicShellEntry pEntry = GetDynamicEntry(idCmd);
    if (!pEntry)
        return E_FAIL;

    /* invoke the dynamic context menu */
    lpcmi->lpVerb = MAKEINTRESOURCEA(idCmd - pEntry->iIdCmdFirst);
    return pEntry->pCM->InvokeCommand(lpcmi);
}

DWORD
CDefaultContextMenu::BrowserFlagsFromVerb(LPCMINVOKECOMMANDINFO lpcmi, PStaticShellEntry pEntry)
{
    LPSHELLBROWSER lpSB;
    HWND hwndTree;
    LPCWSTR FlagsName;
    WCHAR wszKey[256];
    HRESULT hr;
    DWORD wFlags;
    DWORD cbVerb;

    /* Get a pointer to the shell browser */
    lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    if (lpSB == NULL)
        return 0;

    /* See if we are in Explore or Browse mode. If the browser's tree is present, we are in Explore mode.*/
    if (SUCCEEDED(lpSB->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree)
        FlagsName = L"ExplorerFlags";
    else
        FlagsName = L"BrowserFlags";

    /* Try to get the flag from the verb */
    hr = StringCbPrintfW(wszKey, sizeof(wszKey), L"%s\\shell\\%s", pEntry->szClass, pEntry->szVerb);
    if (!SUCCEEDED(hr))
        return 0;

    cbVerb = sizeof(wFlags);
    if (RegGetValueW(HKEY_CLASSES_ROOT, wszKey, FlagsName, RRF_RT_REG_DWORD, NULL, &wFlags, &cbVerb) == ERROR_SUCCESS)
    {
        return wFlags;
    }

    return 0;
}

HRESULT
CDefaultContextMenu::TryToBrowse(
    LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, DWORD wFlags)
{
    LPSHELLBROWSER lpSB = (LPSHELLBROWSER)SendMessageW(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    HRESULT hr;

    if (lpSB == NULL)
        return E_FAIL;

    hr = lpSB->BrowseObject(ILCombine(m_pidlFolder, pidl), wFlags);

    return hr;
}

HRESULT
CDefaultContextMenu::InvokePidl(LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, PStaticShellEntry pEntry)
{
    LPITEMIDLIST pidlFull = ILCombine(m_pidlFolder, pidl);
    if (pidlFull == NULL)
    {
        return E_FAIL;
    }

    WCHAR wszPath[MAX_PATH];
    BOOL bHasPath = SHGetPathFromIDListW(pidlFull, wszPath);

    WCHAR wszDir[MAX_PATH];
    if(bHasPath)
    {
        wcscpy(wszDir, wszPath);
        PathRemoveFileSpec(wszDir);
    }
    else
    {
        SHGetPathFromIDListW(m_pidlFolder, wszDir);
    }

    HKEY hkeyClass;
    RegOpenKeyExW(HKEY_CLASSES_ROOT, pEntry->szClass, 0, KEY_READ, &hkeyClass);

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = lpcmi->hwnd;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = pEntry->szVerb;
    sei.lpDirectory = wszDir;
    sei.lpIDList = pidlFull;
    sei.hkeyClass = hkeyClass;
    sei.fMask = SEE_MASK_CLASSKEY | SEE_MASK_IDLIST;
    if (bHasPath)
    {
        sei.lpFile = wszPath;
    }

    ShellExecuteExW(&sei);

    RegCloseKey(hkeyClass);

    ILFree(pidlFull);

    return S_OK;
}

HRESULT
CDefaultContextMenu::DoStaticShellExtensions(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    PStaticShellEntry pEntry = m_pStaticEntries;
    INT iCmd = LOWORD(lpcmi->lpVerb) - m_iIdSCMFirst;
    HRESULT hr;
    UINT i;

    while (pEntry && (iCmd--) > 0)
        pEntry = pEntry->pNext;

    if (iCmd > 0)
        return E_FAIL;

    /* Get the browse flags to see if we need to browse */
    DWORD wFlags = BrowserFlagsFromVerb(lpcmi, pEntry);
    BOOL bBrowsed = FALSE;

    for (i=0; i < m_cidl; i++)
    {
        /* Check if we need to browse */
        if (wFlags > 0)
        {
            /* In xp if we have browsed, we don't open any more folders .
             * In win7 we browse to the first folder we find and
             * open new windows fo for each of the rest of the folders */
            if (bBrowsed)
                continue;

            hr = TryToBrowse(lpcmi, m_apidl[i], wFlags);
            if (SUCCEEDED(hr))
            {
                bBrowsed = TRUE;
                continue;
            }
        }

        InvokePidl(lpcmi, m_apidl[i], pEntry);
    }

    return S_OK;
}

HRESULT
WINAPI
CDefaultContextMenu::InvokeCommand(
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
            return NotifyShellViewWindow(lpcmi, FALSE);
        case FCIDM_SHVIEW_REFRESH:
            return DoRefresh(lpcmi);
        case FCIDM_SHVIEW_INSERT:
            return DoPaste(lpcmi, FALSE);
        case FCIDM_SHVIEW_INSERTLINK:
            return DoPaste(lpcmi, TRUE);
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

    if (m_iIdSHEFirst && m_iIdSHELast)
    {
        if (LOWORD(lpcmi->lpVerb) >= m_iIdSHEFirst && LOWORD(lpcmi->lpVerb) <= m_iIdSHELast)
            return DoDynamicShellExtensions(lpcmi);
    }

    if (m_iIdSCMFirst && m_iIdSCMLast)
    {
        if (LOWORD(lpcmi->lpVerb) >= m_iIdSCMFirst && LOWORD(lpcmi->lpVerb) <= m_iIdSCMLast)
            return DoStaticShellExtensions(lpcmi);
    }

    FIXME("Unhandled Verb %xl\n", LOWORD(lpcmi->lpVerb));
    return E_UNEXPECTED;
}

HRESULT
WINAPI
CDefaultContextMenu::GetCommandString(
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
CDefaultContextMenu::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    /* FIXME: Should we implement this as well? */
    return S_OK;
}

HRESULT 
WINAPI 
CDefaultContextMenu::HandleMenuMsg2(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *plResult)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
    {
        PDynamicShellEntry pEntry = m_pDynamicEntries;
        while (pEntry)
        {
            SHForwardContextMenuMsg(pEntry->pCM, uMsg, wParam, lParam, plResult, TRUE);
            pEntry = pEntry->pNext;
        }
        break;
    }
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* pDrawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        PDynamicShellEntry pEntry = GetDynamicEntry(pDrawStruct->itemID);
        if(pEntry)
            SHForwardContextMenuMsg(pEntry->pCM, uMsg, wParam, lParam, plResult, TRUE);
        break;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT* pMeasureStruct = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        PDynamicShellEntry pEntry = GetDynamicEntry(pMeasureStruct->itemID);
        if(pEntry)
            SHForwardContextMenuMsg(pEntry->pCM, uMsg, wParam, lParam, plResult, TRUE);
        break;
    }
    case WM_MENUCHAR :
        /* FIXME */
        break;
    default:
        ERR("Got unknown message:%d\n", uMsg);
    }
   return S_OK;
}

static
HRESULT
IDefaultContextMenu_Constructor(
    const DEFCONTEXTMENU *pdcm,
    REFIID riid,
    void **ppv)
{
    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;

    CComObject<CDefaultContextMenu> *pCM;
    HRESULT hr = CComObject<CDefaultContextMenu>::CreateInstance(&pCM);
    if (FAILED(hr))
        return hr;
    pCM->AddRef(); // CreateInstance returns object with 0 ref count */

    CComPtr<IUnknown> pResult;
    hr = pCM->QueryInterface(riid, (void **)&pResult);
    if (FAILED(hr))
    {
        pCM->Release();
        return hr;
    }

    hr = pCM->Initialize(pdcm);
    if (FAILED(hr))
    {
        pCM->Release();
        return hr;
    }

    *ppv = pResult.Detach();
    pCM->Release();
    TRACE("This(%p) cidl %u\n", *ppv, pdcm->cidl);
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
    *ppv = NULL;
    HRESULT hr = IDefaultContextMenu_Constructor(pdcm, riid, ppv);
    if (FAILED(hr))
        ERR("IDefaultContextMenu_Constructor failed: %x\n", hr);
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
    pdcm.hwnd = hwnd;
    pdcm.pcmcb = NULL;
    pdcm.pidlFolder = pidlFolder;
    pdcm.psf = psf;
    pdcm.cidl = cidl;
    pdcm.apidl = apidl;
    pdcm.punkAssociationInfo = NULL;
    pdcm.cKeys = nKeys;
    pdcm.aKeys = ahkeyClsKeys;

    HRESULT hr = SHCreateDefaultContextMenu(&pdcm, IID_PPV_ARG(IContextMenu, ppcm));
    return hr;
}

