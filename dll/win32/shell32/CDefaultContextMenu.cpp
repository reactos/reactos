/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/shv_item_new.c
 * PURPOSE:     provides default context menu implementation
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmenu);

typedef struct _DynamicShellEntry_
{
    UINT iIdCmdFirst;
    UINT NumIds;
    CLSID ClassID;
    CComPtr<IContextMenu> pCM;
} DynamicShellEntry, *PDynamicShellEntry;

typedef struct _StaticShellEntry_
{
    CStringW Verb;
    HKEY hkClass;
} StaticShellEntry, *PStaticShellEntry;


//
// verbs for InvokeCommandInfo
//
struct _StaticInvokeCommandMap_
{
    LPCSTR szStringVerb;
    UINT IntVerb;
} g_StaticInvokeCmdMap[] =
{
    { "RunAs", 0 },  // Unimplemented
    { "Print", 0 },  // Unimplemented
    { "Preview", 0 }, // Unimplemented
    { "Open", FCIDM_SHVIEW_OPEN },
    { CMDSTR_NEWFOLDERA, FCIDM_SHVIEW_NEWFOLDER },
    { "cut", FCIDM_SHVIEW_CUT},
    { "copy", FCIDM_SHVIEW_COPY},
    { "paste", FCIDM_SHVIEW_INSERT},
    { "link", FCIDM_SHVIEW_CREATELINK},
    { "delete", FCIDM_SHVIEW_DELETE},
    { "properties", FCIDM_SHVIEW_PROPERTIES},
    { "rename", FCIDM_SHVIEW_RENAME},
    { "copyto", FCIDM_SHVIEW_COPYTO },
    { "moveto", FCIDM_SHVIEW_MOVETO },
};

class CDefaultContextMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3,
    public IObjectWithSite
{
    private:
        CComPtr<IUnknown> m_site;
        CComPtr<IShellFolder> m_psf;
        CComPtr<IContextMenuCB> m_pmcb;
        LPFNDFMCALLBACK m_pfnmcb;
        UINT m_cidl;
        PCUITEMID_CHILD_ARRAY m_apidl;
        CComPtr<IDataObject> m_pDataObj;
        HKEY* m_aKeys;
        UINT m_cKeys;
        PIDLIST_ABSOLUTE m_pidlFolder;
        DWORD m_bGroupPolicyActive;
        CAtlList<DynamicShellEntry> m_DynamicEntries;
        UINT m_iIdSHEFirst; /* first used id */
        UINT m_iIdSHELast; /* last used id */
        CAtlList<StaticShellEntry> m_StaticEntries;
        UINT m_iIdSCMFirst; /* first static used id */
        UINT m_iIdSCMLast; /* last static used id */
        UINT m_iIdCBFirst; /* first callback used id */
        UINT m_iIdCBLast;  /* last callback used id */
        UINT m_iIdDfltFirst; /* first default part id */
        UINT m_iIdDfltLast; /* last default part id */

        HRESULT _DoCallback(UINT uMsg, WPARAM wParam, LPVOID lParam);
        void AddStaticEntry(const HKEY hkeyClass, const WCHAR *szVerb);
        void AddStaticEntriesForKey(HKEY hKey);
        BOOL IsShellExtensionAlreadyLoaded(REFCLSID clsid);
        HRESULT LoadDynamicContextMenuHandler(HKEY hKey, REFCLSID clsid);
        BOOL EnumerateDynamicContextHandlerForKey(HKEY hRootKey);
        UINT AddShellExtensionsToMenu(HMENU hMenu, UINT* pIndexMenu, UINT idCmdFirst, UINT idCmdLast);
        UINT AddStaticContextMenusToMenu(HMENU hMenu, UINT* IndexMenu, UINT iIdCmdFirst, UINT iIdCmdLast);
        HRESULT DoPaste(LPCMINVOKECOMMANDINFO lpcmi, BOOL bLink);
        HRESULT DoOpenOrExplore(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoCreateLink(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoDelete(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoCopyOrCut(LPCMINVOKECOMMANDINFO lpcmi, BOOL bCopy);
        HRESULT DoRename(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoProperties(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoUndo(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT DoCreateNewFolder(LPCMINVOKECOMMANDINFO lpici);
        HRESULT DoCopyToMoveToFolder(LPCMINVOKECOMMANDINFO lpici, BOOL bCopy);
        HRESULT InvokeShellExt(LPCMINVOKECOMMANDINFO lpcmi);
        HRESULT InvokeRegVerb(LPCMINVOKECOMMANDINFO lpcmi);
        DWORD BrowserFlagsFromVerb(LPCMINVOKECOMMANDINFO lpcmi, PStaticShellEntry pEntry);
        HRESULT TryToBrowse(LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, DWORD wFlags);
        HRESULT InvokePidl(LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, PStaticShellEntry pEntry);
        PDynamicShellEntry GetDynamicEntry(UINT idCmd);
        BOOL MapVerbToCmdId(PVOID Verb, PUINT idCmd, BOOL IsUnicode);

    public:
        CDefaultContextMenu();
        ~CDefaultContextMenu();
        HRESULT WINAPI Initialize(const DEFCONTEXTMENU *pdcm, LPFNDFMCALLBACK lpfn);

        // IContextMenu
        virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

        // IContextMenu2
        virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

        // IContextMenu3
        virtual HRESULT WINAPI HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        // IObjectWithSite
        virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
        virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

        BEGIN_COM_MAP(CDefaultContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        END_COM_MAP()
};

CDefaultContextMenu::CDefaultContextMenu() :
    m_psf(NULL),
    m_pmcb(NULL),
    m_pfnmcb(NULL),
    m_cidl(0),
    m_apidl(NULL),
    m_pDataObj(NULL),
    m_aKeys(NULL),
    m_cKeys(NULL),
    m_pidlFolder(NULL),
    m_bGroupPolicyActive(0),
    m_iIdSHEFirst(0),
    m_iIdSHELast(0),
    m_iIdSCMFirst(0),
    m_iIdSCMLast(0),
    m_iIdCBFirst(0),
    m_iIdCBLast(0),
    m_iIdDfltFirst(0),
    m_iIdDfltLast(0)
{
}

CDefaultContextMenu::~CDefaultContextMenu()
{
    m_DynamicEntries.RemoveAll();
    m_StaticEntries.RemoveAll();

    for (UINT i = 0; i < m_cKeys; i++)
        RegCloseKey(m_aKeys[i]);
    HeapFree(GetProcessHeap(), 0, m_aKeys);

    if (m_pidlFolder)
        CoTaskMemFree(m_pidlFolder);
    _ILFreeaPidl(const_cast<PITEMID_CHILD *>(m_apidl), m_cidl);
}

HRESULT WINAPI CDefaultContextMenu::Initialize(const DEFCONTEXTMENU *pdcm, LPFNDFMCALLBACK lpfn)
{
    TRACE("cidl %u\n", pdcm->cidl);

    if (!pdcm->pcmcb && !lpfn)
    {
        ERR("CDefaultContextMenu needs a callback!\n");
        return E_INVALIDARG;
    }

    m_cidl = pdcm->cidl;
    m_apidl = const_cast<PCUITEMID_CHILD_ARRAY>(_ILCopyaPidl(pdcm->apidl, m_cidl));
    if (m_cidl && !m_apidl)
        return E_OUTOFMEMORY;
    m_psf = pdcm->psf;
    m_pmcb = pdcm->pcmcb;
    m_pfnmcb = lpfn;

    m_cKeys = pdcm->cKeys;
    if (pdcm->cKeys)
    {
        m_aKeys = (HKEY*)HeapAlloc(GetProcessHeap(), 0, sizeof(HKEY) * pdcm->cKeys);
        if (!m_aKeys)
            return E_OUTOFMEMORY;
        memcpy(m_aKeys, pdcm->aKeys, sizeof(HKEY) * pdcm->cKeys);
    }

    m_psf->GetUIObjectOf(pdcm->hwnd, m_cidl, m_apidl, IID_NULL_PPV_ARG(IDataObject, &m_pDataObj));

    if (pdcm->pidlFolder)
    {
        m_pidlFolder = ILClone(pdcm->pidlFolder);
    }
    else
    {
        CComPtr<IPersistFolder2> pf = NULL;
        if (SUCCEEDED(m_psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pf))))
        {
            if (FAILED(pf->GetCurFolder(&m_pidlFolder)))
                ERR("GetCurFolder failed\n");
        }
        TRACE("pidlFolder %p\n", m_pidlFolder);
    }

    return S_OK;
}

HRESULT CDefaultContextMenu::_DoCallback(UINT uMsg, WPARAM wParam, LPVOID lParam)
{
    if (m_pmcb)
    {
        return m_pmcb->CallBack(m_psf, NULL, m_pDataObj, uMsg, wParam, (LPARAM)lParam);
    }
    else if(m_pfnmcb)
    {
        return m_pfnmcb(m_psf, NULL, m_pDataObj, uMsg, wParam, (LPARAM)lParam);
    }

    return E_FAIL;
}

void CDefaultContextMenu::AddStaticEntry(const HKEY hkeyClass, const WCHAR *szVerb)
{
    POSITION it = m_StaticEntries.GetHeadPosition();
    while (it != NULL)
    {
        const StaticShellEntry& info = m_StaticEntries.GetNext(it);
        if (info.Verb.CompareNoCase(szVerb) == 0)
        {
            /* entry already exists */
            return;
        }
    }

    TRACE("adding verb %s\n", debugstr_w(szVerb));

    if (!wcsicmp(szVerb, L"open"))
    {
        /* open verb is always inserted in front */
        m_StaticEntries.AddHead({ szVerb, hkeyClass });
    }
    else
    {
        m_StaticEntries.AddTail({ szVerb, hkeyClass });
    }
}

void CDefaultContextMenu::AddStaticEntriesForKey(HKEY hKey)
{
    WCHAR wszName[40];
    DWORD cchName, dwIndex = 0;
    HKEY hShellKey;

    LRESULT lres = RegOpenKeyExW(hKey, L"shell", 0, KEY_READ, &hShellKey);
    if (lres != STATUS_SUCCESS)
        return;

    while(TRUE)
    {
        cchName = _countof(wszName);
        if (RegEnumKeyExW(hShellKey, dwIndex++, wszName, &cchName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        AddStaticEntry(hKey, wszName);
    }

    RegCloseKey(hShellKey);
}

static
BOOL
HasClipboardData()
{
    BOOL bRet = FALSE;
    CComPtr<IDataObject> pDataObj;

    if (SUCCEEDED(OleGetClipboard(&pDataObj)))
    {
        FORMATETC formatetc;

        TRACE("pDataObj=%p\n", pDataObj.p);

        /* Set the FORMATETC structure*/
        InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
        bRet = SUCCEEDED(pDataObj->QueryGetData(&formatetc));
    }

    return bRet;
}

BOOL
CDefaultContextMenu::IsShellExtensionAlreadyLoaded(REFCLSID clsid)
{
    POSITION it = m_DynamicEntries.GetHeadPosition();
    while (it != NULL)
    {
        const DynamicShellEntry& info = m_DynamicEntries.GetNext(it);
        if (info.ClassID == clsid)
            return TRUE;
    }

    return FALSE;
}

HRESULT
CDefaultContextMenu::LoadDynamicContextMenuHandler(HKEY hKey, REFCLSID clsid)
{
    HRESULT hr;
    TRACE("LoadDynamicContextMenuHandler entered with This %p hKey %p pclsid %s\n", this, hKey, wine_dbgstr_guid(&clsid));

    if (IsShellExtensionAlreadyLoaded(clsid))
        return S_OK;

    CComPtr<IContextMenu> pcm;
    hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IContextMenu, &pcm));
    if (FAILED(hr))
    {
        ERR("SHCoCreateInstance(IContextMenu) failed.clsid %s hr 0x%x\n", wine_dbgstr_guid(&clsid), hr);
        return hr;
    }

    CComPtr<IShellExtInit> pExtInit;
    hr = pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &pExtInit));
    if (FAILED(hr))
    {
        ERR("IContextMenu->QueryInterface(IShellExtInit) failed.clsid %s hr 0x%x\n", wine_dbgstr_guid(&clsid), hr);
        return hr;
    }

    hr = pExtInit->Initialize(m_pidlFolder, m_pDataObj, hKey);
    if (FAILED(hr))
    {
        WARN("IShellExtInit::Initialize failed.clsid %s hr 0x%x\n", wine_dbgstr_guid(&clsid), hr);
        return hr;
    }

    if (m_site)
        IUnknown_SetSite(pcm, m_site);

    m_DynamicEntries.AddTail({ 0, 0, clsid, pcm });

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

        if (FAILED(hr))
        {
            ERR("CLSIDFromString failed for clsid %S hr 0x%x\n", pwszClsid, hr);
            continue;
        }

        if (m_bGroupPolicyActive)
        {
            if (RegGetValueW(HKEY_LOCAL_MACHINE,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                             pwszClsid,
                             RRF_RT_REG_SZ,
                             NULL,
                             NULL,
                             NULL) != ERROR_SUCCESS)
            {
                ERR("Shell extension %s not approved!\n", pwszClsid);
                continue;
            }
        }

        hr = LoadDynamicContextMenuHandler(hKey, clsid);
        if (FAILED(hr))
            WARN("Failed to get context menu entires from shell extension! clsid: %S\n", pwszClsid);
    }

    RegCloseKey(hKey);
    return TRUE;
}

UINT
CDefaultContextMenu::AddShellExtensionsToMenu(HMENU hMenu, UINT* pIndexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    UINT cIds = 0;

    if (m_DynamicEntries.IsEmpty())
        return cIds;

    POSITION it = m_DynamicEntries.GetHeadPosition();
    while (it != NULL)
    {
        DynamicShellEntry& info = m_DynamicEntries.GetNext(it);

        HRESULT hr = info.pCM->QueryContextMenu(hMenu, *pIndexMenu, idCmdFirst + cIds, idCmdLast, CMF_NORMAL);
        if (SUCCEEDED(hr))
        {
            info.iIdCmdFirst = cIds;
            info.NumIds = LOWORD(hr);
            (*pIndexMenu) += info.NumIds;

            cIds += info.NumIds;
            if (idCmdFirst + cIds >= idCmdLast)
                break;
        }
        TRACE("pEntry hr %x contextmenu %p cmdfirst %x num ids %x\n", hr, info.pCM.p, info.iIdCmdFirst, info.NumIds);
    }
    return cIds;
}

UINT
CDefaultContextMenu::AddStaticContextMenusToMenu(
    HMENU hMenu,
    UINT* pIndexMenu,
    UINT iIdCmdFirst,
    UINT iIdCmdLast)
{
    MENUITEMINFOW mii;
    UINT idResource;
    WCHAR wszVerb[40];
    UINT fState;
    UINT cIds = 0;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.dwTypeData = NULL;

    POSITION it = m_StaticEntries.GetHeadPosition();
    bool first = true;
    while (it != NULL)
    {
        StaticShellEntry& info = m_StaticEntries.GetNext(it);

        fState = MFS_ENABLED;
        mii.dwTypeData = NULL;

        /* set first entry as default */
        if (first)
        {
            fState |= MFS_DEFAULT;
            first = false;
        }

        if (info.Verb.CompareNoCase(L"open") == 0)
        {
            /* override default when open verb is found */
            fState |= MFS_DEFAULT;
            idResource = IDS_OPEN_VERB;
        }
        else if (info.Verb.CompareNoCase(L"explore") == 0)
            idResource = IDS_EXPLORE_VERB;
        else if (info.Verb.CompareNoCase(L"runas") == 0)
            idResource = IDS_RUNAS_VERB;
        else if (info.Verb.CompareNoCase(L"edit") == 0)
            idResource = IDS_EDIT_VERB;
        else if (info.Verb.CompareNoCase(L"find") == 0)
            idResource = IDS_FIND_VERB;
        else if (info.Verb.CompareNoCase(L"print") == 0)
            idResource = IDS_PRINT_VERB;
        else if (info.Verb.CompareNoCase(L"printto") == 0)
        {
            continue;
        }
        else
            idResource = 0;

        /* By default use verb for menu item name */
        mii.dwTypeData = (LPWSTR)info.Verb.GetString();

        WCHAR wszKey[256];
        HRESULT hr;
        hr = StringCbPrintfW(wszKey, sizeof(wszKey), L"shell\\%s", info.Verb.GetString());
        if (FAILED_UNEXPECTEDLY(hr))
        {
            continue;
        }

        BOOL Extended = FALSE;
        HKEY hkVerb;
        if (idResource > 0)
        {
            if (LoadStringW(shell32_hInstance, idResource, wszVerb, _countof(wszVerb)))
                mii.dwTypeData = wszVerb; /* use translated verb */
            else
                ERR("Failed to load string\n");

            LONG res = RegOpenKeyW(info.hkClass, wszKey, &hkVerb);
            if (res == ERROR_SUCCESS)
            {
                res = RegQueryValueExW(hkVerb, L"Extended", NULL, NULL, NULL, NULL);
                Extended = (res == ERROR_SUCCESS);

                RegCloseKey(hkVerb);
            }
        }
        else
        {
            LONG res = RegOpenKeyW(info.hkClass, wszKey, &hkVerb);
            if (res == ERROR_SUCCESS)
            {
                DWORD cbVerb = sizeof(wszVerb);
                res = RegLoadMUIStringW(hkVerb, NULL, wszVerb, cbVerb, NULL, 0, NULL);
                if (res == ERROR_SUCCESS)
                {
                    /* use description for the menu entry */
                    mii.dwTypeData = wszVerb;
                }

                res = RegQueryValueExW(hkVerb, L"Extended", NULL, NULL, NULL, NULL);
                Extended = (res == ERROR_SUCCESS);

                RegCloseKey(hkVerb);
            }
        }

        if (!Extended || GetAsyncKeyState(VK_SHIFT) < 0)
        {
            mii.cch = wcslen(mii.dwTypeData);
            mii.fState = fState;
            mii.wID = iIdCmdFirst + cIds;
            InsertMenuItemW(hMenu, *pIndexMenu, TRUE, &mii);
            (*pIndexMenu)++;
            cIds++;
        }

        if (mii.wID >= iIdCmdLast)
            break;
    }

    return cIds;
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

HRESULT
WINAPI
CDefaultContextMenu::QueryContextMenu(
    HMENU hMenu,
    UINT IndexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    HRESULT hr;
    UINT idCmdNext = idCmdFirst;
    UINT cIds = 0;

    TRACE("BuildShellItemContextMenu entered\n");

    /* Load static verbs and shell extensions from registry */
    for (UINT i = 0; i < m_cKeys; i++)
    {
        AddStaticEntriesForKey(m_aKeys[i]);
        EnumerateDynamicContextHandlerForKey(m_aKeys[i]);
    }

    /* Add static context menu handlers */
    cIds = AddStaticContextMenusToMenu(hMenu, &IndexMenu, idCmdFirst, idCmdLast);
    m_iIdSCMFirst = 0;
    m_iIdSCMLast = cIds;
    idCmdNext = idCmdFirst + cIds;

    /* Add dynamic context menu handlers */
    cIds += AddShellExtensionsToMenu(hMenu, &IndexMenu, idCmdNext, idCmdLast);
    m_iIdSHEFirst = m_iIdSCMLast;
    m_iIdSHELast = cIds;
    idCmdNext = idCmdFirst + cIds;
    TRACE("SH_LoadContextMenuHandlers first %x last %x\n", m_iIdSHEFirst, m_iIdSHELast);

    /* Now let the callback add its own items */
    QCMINFO qcminfo = {hMenu, IndexMenu, idCmdNext, idCmdLast, NULL};
    if (SUCCEEDED(_DoCallback(DFM_MERGECONTEXTMENU, uFlags, &qcminfo)))
    {
        cIds += qcminfo.idCmdFirst;
        IndexMenu += qcminfo.idCmdFirst;
        m_iIdCBFirst = m_iIdSHELast;
        m_iIdCBLast = cIds;
        idCmdNext = idCmdFirst + cIds;
    }

    if (uFlags & CMF_VERBSONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cIds);

    /* If this is a background context menu we are done */
    if (!m_cidl)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cIds);

    /* Get the attributes of the items */
    SFGAOF rfg = SFGAO_BROWSABLE | SFGAO_CANCOPY | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_FILESYSTEM | SFGAO_FOLDER;
    hr = m_psf->GetAttributesOf(m_cidl, m_apidl, &rfg);
    if (FAILED_UNEXPECTEDLY(hr))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cIds);

    /* Add the default part of the menu */
    HMENU hmenuDefault = LoadMenu(_AtlBaseModule.GetResourceInstance(), L"MENU_SHV_FILE");

    /* Remove uneeded entries */
    if (!(rfg & SFGAO_CANMOVE))
        DeleteMenu(hmenuDefault, IDM_CUT, MF_BYCOMMAND);
    if (!(rfg & SFGAO_CANCOPY))
        DeleteMenu(hmenuDefault, IDM_COPY, MF_BYCOMMAND);
    if (!((rfg & SFGAO_FILESYSTEM) && HasClipboardData()))
        DeleteMenu(hmenuDefault, IDM_INSERT, MF_BYCOMMAND);
    if (!(rfg & SFGAO_CANLINK))
        DeleteMenu(hmenuDefault, IDM_CREATELINK, MF_BYCOMMAND);
    if (!(rfg & SFGAO_CANDELETE))
        DeleteMenu(hmenuDefault, IDM_DELETE, MF_BYCOMMAND);
    if (!(rfg & SFGAO_CANRENAME))
        DeleteMenu(hmenuDefault, IDM_RENAME, MF_BYCOMMAND);
    if (!(rfg & SFGAO_HASPROPSHEET))
        DeleteMenu(hmenuDefault, IDM_PROPERTIES, MF_BYCOMMAND);

    UINT idMax = Shell_MergeMenus(hMenu, GetSubMenu(hmenuDefault, 0), IndexMenu, idCmdNext, idCmdLast, 0);
    m_iIdDfltFirst = cIds;
    cIds += idMax - idCmdNext;
    m_iIdDfltLast = cIds;

    DestroyMenu(hmenuDefault);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cIds);
}

HRESULT CDefaultContextMenu::DoPaste(LPCMINVOKECOMMANDINFO lpcmi, BOOL bLink)
{
    HRESULT hr;

    CComPtr<IDataObject> pda;
    hr = OleGetClipboard(&pda);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

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
        else
        {
            ERR("No drop effect obtained\n");
        }
        GlobalUnlock(medium2.hGlobal);
    }

    if (bLink)
    {
        dwKey = MK_CONTROL|MK_SHIFT;
    }

    CComPtr<IDropTarget> pdrop;
    if (m_cidl)
        hr = m_psf->GetUIObjectOf(NULL, 1, &m_apidl[0], IID_NULL_PPV_ARG(IDropTarget, &pdrop));
    else
        hr = m_psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdrop));

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SHSimulateDrop(pdrop, pda, dwKey, NULL, NULL);

    TRACE("CP result %x\n", hr);
    return S_OK;
}

HRESULT
CDefaultContextMenu::DoOpenOrExplore(LPCMINVOKECOMMANDINFO lpcmi)
{
    UNIMPLEMENTED;
    return E_FAIL;
}

HRESULT CDefaultContextMenu::DoCreateLink(LPCMINVOKECOMMANDINFO lpcmi)
{
    if (!m_cidl || !m_pDataObj)
        return E_FAIL;

    CComPtr<IDropTarget> pDT;
    HRESULT hr = m_psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pDT));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SHSimulateDrop(pDT, m_pDataObj, MK_CONTROL|MK_SHIFT, NULL, NULL);

    return S_OK;
}

HRESULT CDefaultContextMenu::DoDelete(LPCMINVOKECOMMANDINFO lpcmi)
{
    if (!m_cidl || !m_pDataObj)
        return E_FAIL;

    CComPtr<IDropTarget> pDT;
    HRESULT hr = CRecyclerDropTarget_CreateInstance(IID_PPV_ARG(IDropTarget, &pDT));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    DWORD grfKeyState = (lpcmi->fMask & CMIC_MASK_SHIFT_DOWN) ? MK_SHIFT : 0;
    SHSimulateDrop(pDT, m_pDataObj, grfKeyState, NULL, NULL);

    return S_OK;
}

HRESULT CDefaultContextMenu::DoCopyOrCut(LPCMINVOKECOMMANDINFO lpcmi, BOOL bCopy)
{
    if (!m_cidl || !m_pDataObj)
        return E_FAIL;

    FORMATETC formatetc;
    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT), TYMED_HGLOBAL);
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = GlobalAlloc(GHND, sizeof(DWORD));
    DWORD* pdwFlag = (DWORD*)GlobalLock(medium.hGlobal);
    if (pdwFlag)
        *pdwFlag = bCopy ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
    GlobalUnlock(medium.hGlobal);
    m_pDataObj->SetData(&formatetc, &medium, TRUE);

    HRESULT hr = OleSetClipboard(m_pDataObj);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT CDefaultContextMenu::DoRename(LPCMINVOKECOMMANDINFO lpcmi)
{
    CComPtr<IShellBrowser> psb;
    HRESULT hr;

    if (!m_site || !m_cidl)
        return E_FAIL;

    /* Get a pointer to the shell browser */
    hr = IUnknown_QueryService(m_site, SID_IShellBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellView> lpSV;
    hr = psb->QueryActiveShellView(&lpSV);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SVSIF selFlags = SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT;
    hr = lpSV->SelectItem(m_apidl[0], selFlags);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT
CDefaultContextMenu::DoProperties(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = _DoCallback(DFM_INVOKECOMMAND, DFM_CMD_PROPERTIES, NULL);

    // We are asked to run the default property sheet
    if (hr == S_FALSE)
    {
        return Shell_DefaultContextMenuCallBack(m_psf, m_pDataObj);
    }

    return hr;
}

HRESULT
CDefaultContextMenu::DoUndo(LPCMINVOKECOMMANDINFO lpcmi)
{
    ERR("TODO: Undo\n");
    return E_NOTIMPL;
}

HRESULT
CDefaultContextMenu::DoCopyToMoveToFolder(LPCMINVOKECOMMANDINFO lpici, BOOL bCopy)
{
    HRESULT hr = E_FAIL;
    if (!m_pDataObj)
    {
        ERR("m_pDataObj is NULL\n");
        return hr;
    }

    CComPtr<IContextMenu> pContextMenu;
    if (bCopy)
        hr = SHCoCreateInstance(NULL, &CLSID_CopyToMenu, NULL,
                                IID_PPV_ARG(IContextMenu, &pContextMenu));
    else
        hr = SHCoCreateInstance(NULL, &CLSID_MoveToMenu, NULL,
                                IID_PPV_ARG(IContextMenu, &pContextMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellExtInit> pInit;
    hr = pContextMenu->QueryInterface(IID_PPV_ARG(IShellExtInit, &pInit));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pInit->Initialize(m_pidlFolder, m_pDataObj, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (bCopy)
        lpici->lpVerb = "copyto";
    else
        lpici->lpVerb = "moveto";

    return pContextMenu->InvokeCommand(lpici);
}

// This code is taken from CNewMenu and should be shared between the 2 classes
HRESULT
CDefaultContextMenu::DoCreateNewFolder(
    LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    WCHAR wszName[MAX_PATH];
    WCHAR wszNewFolder[25];
    HRESULT hr;

    /* Get folder path */
    hr = SHGetPathFromIDListW(m_pidlFolder, wszPath);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, _countof(wszNewFolder)))
        return E_FAIL;

    /* Create the name of the new directory */
    if (!PathYetAnotherMakeUniqueName(wszName, wszPath, NULL, wszNewFolder))
        return E_FAIL;

    /* Create the new directory and show the appropriate dialog in case of error */
    if (SHCreateDirectory(lpici->hwnd, wszName) != ERROR_SUCCESS)
        return E_FAIL;

    /* Show and select the new item in the def view */
    LPITEMIDLIST pidl;
    PITEMID_CHILD pidlNewItem;
    CComPtr<IShellView> psv;

    /* Notify the view object about the new item */
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, (LPCVOID)wszName, NULL);

    if (!m_site)
        return S_OK;

    /* Get a pointer to the shell view */
    hr = IUnknown_QueryService(m_site, SID_IFolderView, IID_PPV_ARG(IShellView, &psv));
    if (FAILED_UNEXPECTEDLY(hr))
        return S_OK;

    /* Attempt to get the pidl of the new item */
    hr = SHILCreateFromPathW(wszName, &pidl, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    pidlNewItem = ILFindLastID(pidl);

    hr = psv->SelectItem(pidlNewItem, SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE |
                          SVSI_FOCUSED | SVSI_SELECT);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SHFree(pidl);

    return S_OK;
}

PDynamicShellEntry CDefaultContextMenu::GetDynamicEntry(UINT idCmd)
{
    POSITION it = m_DynamicEntries.GetHeadPosition();
    while (it != NULL)
    {
        DynamicShellEntry& info = m_DynamicEntries.GetNext(it);

        if (idCmd >= info.iIdCmdFirst + info.NumIds)
            continue;

        if (idCmd < info.iIdCmdFirst || idCmd > info.iIdCmdFirst + info.NumIds)
            return NULL;

        return &info;
    }

    return NULL;
}

// FIXME: 260 is correct, but should this be part of the SDK or just MAX_PATH?
#define MAX_VERB 260

BOOL
CDefaultContextMenu::MapVerbToCmdId(PVOID Verb, PUINT idCmd, BOOL IsUnicode)
{
    WCHAR UnicodeStr[MAX_VERB];

    /* Loop through all the static verbs looking for a match */
    for (UINT i = 0; i < _countof(g_StaticInvokeCmdMap); i++)
    {
        /* We can match both ANSI and unicode strings */
        if (IsUnicode)
        {
            /* The static verbs are ANSI, get a unicode version before doing the compare */
            SHAnsiToUnicode(g_StaticInvokeCmdMap[i].szStringVerb, UnicodeStr, MAX_VERB);
            if (!wcscmp(UnicodeStr, (LPWSTR)Verb))
            {
                /* Return the Corresponding Id */
                *idCmd = g_StaticInvokeCmdMap[i].IntVerb;
                return TRUE;
            }
        }
        else
        {
            if (!strcmp(g_StaticInvokeCmdMap[i].szStringVerb, (LPSTR)Verb))
            {
                *idCmd = g_StaticInvokeCmdMap[i].IntVerb;
                return TRUE;
            }
        }
    }

    return FALSE;
}

HRESULT
CDefaultContextMenu::InvokeShellExt(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    TRACE("verb %p first %x last %x\n", lpcmi->lpVerb, m_iIdSHEFirst, m_iIdSHELast);

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
    CComPtr<IShellBrowser> psb;
    HWND hwndTree;
    LPCWSTR FlagsName;
    WCHAR wszKey[256];
    HRESULT hr;
    DWORD wFlags;
    DWORD cbVerb;

    if (!m_site)
        return 0;

    /* Get a pointer to the shell browser */
    hr = IUnknown_QueryService(m_site, SID_IShellBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    /* See if we are in Explore or Browse mode. If the browser's tree is present, we are in Explore mode.*/
    if (SUCCEEDED(psb->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree)
        FlagsName = L"ExplorerFlags";
    else
        FlagsName = L"BrowserFlags";

    /* Try to get the flag from the verb */
    hr = StringCbPrintfW(wszKey, sizeof(wszKey), L"shell\\%s", pEntry->Verb.GetString());
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    cbVerb = sizeof(wFlags);
    if (RegGetValueW(pEntry->hkClass, wszKey, FlagsName, RRF_RT_REG_DWORD, NULL, &wFlags, &cbVerb) == ERROR_SUCCESS)
    {
        return wFlags;
    }

    return 0;
}

HRESULT
CDefaultContextMenu::TryToBrowse(
    LPCMINVOKECOMMANDINFO lpcmi, LPCITEMIDLIST pidl, DWORD wFlags)
{
    CComPtr<IShellBrowser> psb;
    HRESULT hr;

    if (!m_site)
        return E_FAIL;

    /* Get a pointer to the shell browser */
    hr = IUnknown_QueryService(m_site, SID_IShellBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    return psb->BrowseObject(ILCombine(m_pidlFolder, pidl), wFlags);
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
    if (bHasPath)
    {
        wcscpy(wszDir, wszPath);
        PathRemoveFileSpec(wszDir);
    }
    else
    {
        SHGetPathFromIDListW(m_pidlFolder, wszDir);
    }

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = lpcmi->hwnd;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpVerb = pEntry->Verb;
    sei.lpDirectory = wszDir;
    sei.lpIDList = pidlFull;
    sei.hkeyClass = pEntry->hkClass;
    sei.fMask = SEE_MASK_CLASSKEY | SEE_MASK_IDLIST;
    if (bHasPath)
    {
        sei.lpFile = wszPath;
    }

    ShellExecuteExW(&sei);

    ILFree(pidlFull);

    return S_OK;
}

HRESULT
CDefaultContextMenu::InvokeRegVerb(
    LPCMINVOKECOMMANDINFO lpcmi)
{
    INT iCmd = LOWORD(lpcmi->lpVerb);
    HRESULT hr;
    UINT i;

    POSITION it = m_StaticEntries.FindIndex(iCmd);

    if (it == NULL)
        return E_INVALIDARG;

    PStaticShellEntry pEntry = &m_StaticEntries.GetAt(it);

    /* Get the browse flags to see if we need to browse */
    DWORD wFlags = BrowserFlagsFromVerb(lpcmi, pEntry);
    BOOL bBrowsed = FALSE;

    for (i=0; i < m_cidl; i++)
    {
        /* Check if we need to browse */
        if (wFlags > 0)
        {
            /* In xp if we have browsed, we don't open any more folders.
             * In win7 we browse to the first folder we find and
             * open new windows for each of the rest of the folders */
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
    CMINVOKECOMMANDINFO LocalInvokeInfo;
    HRESULT Result;
    UINT CmdId;

    /* Take a local copy of the fixed members of the
       struct as we might need to modify the verb */
    LocalInvokeInfo = *lpcmi;

    /* Check if this is a string verb */
    if (HIWORD(LocalInvokeInfo.lpVerb))
    {
        /* Get the ID which corresponds to this verb, and update our local copy */
        if (MapVerbToCmdId((LPVOID)LocalInvokeInfo.lpVerb, &CmdId, FALSE))
            LocalInvokeInfo.lpVerb = MAKEINTRESOURCEA(CmdId);
    }

    CmdId = LOWORD(LocalInvokeInfo.lpVerb);

    if (!m_DynamicEntries.IsEmpty() && CmdId >= m_iIdSHEFirst && CmdId < m_iIdSHELast)
    {
        LocalInvokeInfo.lpVerb -= m_iIdSHEFirst;
        Result = InvokeShellExt(&LocalInvokeInfo);
        return Result;
    }

    if (!m_StaticEntries.IsEmpty() && CmdId >= m_iIdSCMFirst && CmdId < m_iIdSCMLast)
    {
        LocalInvokeInfo.lpVerb -= m_iIdSCMFirst;
        Result = InvokeRegVerb(&LocalInvokeInfo);
        return Result;
    }

    if (m_iIdCBFirst != m_iIdCBLast && CmdId >= m_iIdCBFirst && CmdId < m_iIdCBLast)
    {
        Result = _DoCallback(DFM_INVOKECOMMAND, CmdId - m_iIdCBFirst, NULL);
        return Result;
    }

    if (m_iIdDfltFirst != m_iIdDfltLast && CmdId >= m_iIdDfltFirst && CmdId < m_iIdDfltLast)
    {
        CmdId -= m_iIdDfltFirst;
        /* See the definitions of IDM_CUT and co to see how this works */
        CmdId += 0x7000;
    }

    /* Check if this is a Id */
    switch (CmdId)
    {
    case FCIDM_SHVIEW_INSERT:
        Result = DoPaste(&LocalInvokeInfo, FALSE);
        break;
    case FCIDM_SHVIEW_INSERTLINK:
        Result = DoPaste(&LocalInvokeInfo, TRUE);
        break;
    case FCIDM_SHVIEW_OPEN:
    case FCIDM_SHVIEW_EXPLORE:
        Result = DoOpenOrExplore(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_COPY:
    case FCIDM_SHVIEW_CUT:
        Result = DoCopyOrCut(&LocalInvokeInfo, CmdId == FCIDM_SHVIEW_COPY);
        break;
    case FCIDM_SHVIEW_CREATELINK:
        Result = DoCreateLink(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_DELETE:
        Result = DoDelete(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_RENAME:
        Result = DoRename(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_PROPERTIES:
        Result = DoProperties(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_NEWFOLDER:
        Result = DoCreateNewFolder(&LocalInvokeInfo);
        break;
    case FCIDM_SHVIEW_COPYTO:
        Result = DoCopyToMoveToFolder(&LocalInvokeInfo, TRUE);
        break;
    case FCIDM_SHVIEW_MOVETO:
        Result = DoCopyToMoveToFolder(&LocalInvokeInfo, FALSE);
        break;
    case FCIDM_SHVIEW_UNDO:
        Result = DoUndo(&LocalInvokeInfo);
        break;
    default:
        Result = E_INVALIDARG;
        ERR("Unhandled Verb %xl\n", LOWORD(LocalInvokeInfo.lpVerb));
        break;
    }

    return Result;
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
    /* We don't handle the help text yet */
    if (uFlags == GCS_HELPTEXTA ||
        uFlags == GCS_HELPTEXTW ||
        HIWORD(idCommand) != 0)
    {
        return E_NOTIMPL;
    }

    UINT CmdId = LOWORD(idCommand);

    if (!m_DynamicEntries.IsEmpty() && CmdId >= m_iIdSHEFirst && CmdId < m_iIdSHELast)
    {
        idCommand -= m_iIdSHEFirst;
        PDynamicShellEntry pEntry = GetDynamicEntry(idCommand);
        if (!pEntry)
            return E_FAIL;

        idCommand -= pEntry->iIdCmdFirst;
        return pEntry->pCM->GetCommandString(idCommand,
                                             uFlags,
                                             lpReserved,
                                             lpszName,
                                             uMaxNameLen);
    }

    if (!m_StaticEntries.IsEmpty() && CmdId >= m_iIdSCMFirst && CmdId < m_iIdSCMLast)
    {
        /* Validation just returns S_OK on a match. The id exists. */
        if (uFlags == GCS_VALIDATEA || uFlags == GCS_VALIDATEW)
            return S_OK;

        CmdId -= m_iIdSCMFirst;

        POSITION it = m_StaticEntries.FindIndex(CmdId);

        if (it == NULL)
            return E_INVALIDARG;

        PStaticShellEntry pEntry = &m_StaticEntries.GetAt(it);

        if (uFlags == GCS_VERBW)
            return StringCchCopyW((LPWSTR)lpszName, uMaxNameLen, pEntry->Verb);

        if (uFlags == GCS_VERBA)
        {
            if (SHUnicodeToAnsi(pEntry->Verb, lpszName, uMaxNameLen))
                return S_OK;
        }

        return E_INVALIDARG;
    }

    //FIXME: Should we handle callbacks here?
    if (m_iIdDfltFirst != m_iIdDfltLast && CmdId >= m_iIdDfltFirst && CmdId < m_iIdDfltLast)
    {
        CmdId -= m_iIdDfltFirst;
        /* See the definitions of IDM_CUT and co to see how this works */
        CmdId += 0x7000;
    }

    /* Loop looking for a matching Id */
    for (UINT i = 0; i < _countof(g_StaticInvokeCmdMap); i++)
    {
        if (g_StaticInvokeCmdMap[i].IntVerb == CmdId)
        {
            /* Validation just returns S_OK on a match */
            if (uFlags == GCS_VALIDATEA || uFlags == GCS_VALIDATEW)
                return S_OK;

            /* Return a copy of the ANSI verb */
            if (uFlags == GCS_VERBA)
                return StringCchCopyA(lpszName, uMaxNameLen, g_StaticInvokeCmdMap[i].szStringVerb);

            /* Convert the ANSI verb to unicode and return that */
            if (uFlags == GCS_VERBW)
            {
                if (SHAnsiToUnicode(g_StaticInvokeCmdMap[i].szStringVerb, (LPWSTR)lpszName, uMaxNameLen))
                    return S_OK;
            }
        }
    }

    return E_INVALIDARG;
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

HRESULT SHGetMenuIdFromMenuMsg(UINT uMsg, LPARAM lParam, UINT *CmdId)
{
    if (uMsg == WM_DRAWITEM)
    {
        DRAWITEMSTRUCT* pDrawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        *CmdId = pDrawStruct->itemID;
        return S_OK;
    }
    else if (uMsg == WM_MEASUREITEM)
    {
        MEASUREITEMSTRUCT* pMeasureStruct = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        *CmdId = pMeasureStruct->itemID;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT SHSetMenuIdInMenuMsg(UINT uMsg, LPARAM lParam, UINT CmdId)
{
    if (uMsg == WM_DRAWITEM)
    {
        DRAWITEMSTRUCT* pDrawStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        pDrawStruct->itemID = CmdId;
        return S_OK;
    }
    else if (uMsg == WM_MEASUREITEM)
    {
        MEASUREITEMSTRUCT* pMeasureStruct = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        pMeasureStruct->itemID = CmdId;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT
WINAPI
CDefaultContextMenu::HandleMenuMsg2(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult)
{
    if (uMsg == WM_INITMENUPOPUP)
    {
        POSITION it = m_DynamicEntries.GetHeadPosition();
        while (it != NULL)
        {
            DynamicShellEntry& info = m_DynamicEntries.GetNext(it);
            SHForwardContextMenuMsg(info.pCM, uMsg, wParam, lParam, plResult, TRUE);
        }
        return S_OK;
    }

    UINT CmdId;
    HRESULT hr = SHGetMenuIdFromMenuMsg(uMsg, lParam, &CmdId);
    if (FAILED(hr))
        return S_FALSE;

    if (CmdId < m_iIdSHEFirst || CmdId >= m_iIdSHELast)
        return S_FALSE;

    CmdId -= m_iIdSHEFirst;
    PDynamicShellEntry pEntry = GetDynamicEntry(CmdId);
    if (pEntry)
    {
        SHSetMenuIdInMenuMsg(uMsg, lParam, CmdId - pEntry->iIdCmdFirst);
        SHForwardContextMenuMsg(pEntry->pCM, uMsg, wParam, lParam, plResult, TRUE);
    }

   return S_OK;
}

HRESULT
WINAPI
CDefaultContextMenu::SetSite(IUnknown *pUnkSite)
{
    m_site = pUnkSite;
    return S_OK;
}

HRESULT
WINAPI
CDefaultContextMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_site)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

static
HRESULT
CDefaultContextMenu_CreateInstance(const DEFCONTEXTMENU *pdcm, LPFNDFMCALLBACK lpfn, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CDefaultContextMenu>(pdcm, lpfn, riid, ppv);
}

/*************************************************************************
 * SHCreateDefaultContextMenu            [SHELL32.325] Vista API
 *
 */

HRESULT
WINAPI
SHCreateDefaultContextMenu(const DEFCONTEXTMENU *pdcm, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (!ppv)
        return E_INVALIDARG;

    hr = CDefaultContextMenu_CreateInstance(pdcm, NULL, riid, ppv);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

/*************************************************************************
 * CDefFolderMenu_Create2            [SHELL32.701]
 *
 */

HRESULT
WINAPI
CDefFolderMenu_Create2(
    PCIDLIST_ABSOLUTE pidlFolder,
    HWND hwnd,
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    IShellFolder *psf,
    LPFNDFMCALLBACK lpfn,
    UINT nKeys,
    const HKEY *ahkeyClsKeys,
    IContextMenu **ppcm)
{
    DEFCONTEXTMENU dcm;
    dcm.hwnd = hwnd;
    dcm.pcmcb = NULL;
    dcm.pidlFolder = pidlFolder;
    dcm.psf = psf;
    dcm.cidl = cidl;
    dcm.apidl = apidl;
    dcm.punkAssociationInfo = NULL;
    dcm.cKeys = nKeys;
    dcm.aKeys = ahkeyClsKeys;

    HRESULT hr = CDefaultContextMenu_CreateInstance(&dcm, lpfn, IID_PPV_ARG(IContextMenu, ppcm));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}
