/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search results folder
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#include "CFindFolder.h"
#include <exdispid.h>

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

static HRESULT SHELL32_CoCreateInitSF(LPCITEMIDLIST pidlRoot, PERSIST_FOLDER_TARGET_INFO* ppfti,
                                LPCITEMIDLIST pidlChild, const GUID* clsid, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr;
    CComPtr<IShellFolder> pShellFolder;

    hr = SHCoCreateInstance(NULL, clsid, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
    if (FAILED(hr))
        return hr;

    LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
    CComPtr<IPersistFolder> ppf;
    CComPtr<IPersistFolder3> ppf3;

    if (ppfti && SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3))))
    {
        ppf3->InitializeEx(NULL, pidlAbsolute, ppfti);
    }
    else if (SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf))))
    {
        ppf->Initialize(pidlAbsolute);
    }
    ILFree (pidlAbsolute);

    return pShellFolder->QueryInterface(riid, ppvOut);
}

static void WINAPI _InsertMenuItemW(
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
        if (IS_INTRESOURCE(dwTypeData))
        {
            if (LoadStringW(_AtlBaseModule.GetResourceInstance(), LOWORD((ULONG_PTR)dwTypeData), wszText, _countof(wszText)))
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

struct FolderViewColumns
{
    int iResource;
    DWORD dwDefaultState;
    int fmt;
    int cxChar;
};

static FolderViewColumns g_ColumnDefs[] =
{
    {IDS_COL_NAME,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {IDS_COL_LOCATION,  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {IDS_COL_RELEVANCE, SHCOLSTATE_TYPE_STR,                          LVCFMT_LEFT, 0}
};

CFindFolder::CFindFolder() :
    m_hStopEvent(NULL)
{
}

static LPITEMIDLIST _ILCreate(LPCWSTR lpszPath)
{
    CComHeapPtr<ITEMIDLIST> lpFSPidl(ILCreateFromPathW(lpszPath));
    if (!lpFSPidl)
    {
        ERR("Failed to create pidl from path\n");
        return NULL;
    }
    LPITEMIDLIST lpLastFSPidl = ILFindLastID(lpFSPidl);

    int pathLen = (PathFindFileNameW(lpszPath) - lpszPath) * sizeof(WCHAR);
    int cbData = sizeof(WORD) + pathLen + lpLastFSPidl->mkid.cb;
    LPITEMIDLIST pidl = (LPITEMIDLIST) SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    LPBYTE p = (LPBYTE) pidl;
    *((WORD *) p) = cbData;
    p += sizeof(WORD);

    memcpy(p, lpszPath, pathLen);
    p += pathLen - sizeof(WCHAR);
    *((WCHAR *) p) = '\0';
    p += sizeof(WCHAR);

    memcpy(p, lpLastFSPidl, lpLastFSPidl->mkid.cb);
    p += lpLastFSPidl->mkid.cb;

    *((WORD *) p) = 0;

    return pidl;
}

static LPCWSTR _ILGetPath(LPCITEMIDLIST pidl)
{
    if (!pidl || !pidl->mkid.cb)
        return NULL;
    return (LPCWSTR) pidl->mkid.abID;
}

static LPCITEMIDLIST _ILGetFSPidl(LPCITEMIDLIST pidl)
{
    if (!pidl || !pidl->mkid.cb)
        return pidl;
    return (LPCITEMIDLIST) ((LPBYTE) pidl->mkid.abID
                            + ((wcslen((LPCWSTR) pidl->mkid.abID) + 1) * sizeof(WCHAR)));
}

struct _SearchData
{
    HWND hwnd;
    HANDLE hStopEvent;
    CStringW szPath;
    CStringW szFileName;
    CStringA szQueryA;
    CStringW szQueryW;
    BOOL SearchHidden;
    CComPtr<CFindFolder> pFindFolder;
};

template<typename TChar, typename TString, int (&StrNCmp)(const TChar *, const TChar *, size_t)>
static const TChar* StrStrN(const TChar *lpFirst, const TString &lpSrch, UINT cchMax)
{
    if (!lpFirst || lpSrch.IsEmpty() || !cchMax)
        return NULL;

    for (UINT i = cchMax; i > 0 && *lpFirst; i--, lpFirst++)
    {
        if (!StrNCmp(lpFirst, lpSrch, lpSrch.GetLength()))
            return (const TChar*)lpFirst;
    }

    return NULL;
}

template<typename TChar, typename TString, int (&StrNCmp)(const TChar *, const TChar *, size_t)>
static UINT StrStrNCount(const TChar *lpFirst, const TString &lpSrch, UINT cchMax)
{
    const TChar *lpSearchEnd = lpFirst + cchMax;
    UINT uCount = 0;
    while (lpFirst < lpSearchEnd && (lpFirst = StrStrN<TChar, TString, StrNCmp>(lpFirst, lpSrch, cchMax)))
    {
        uCount++;
        lpFirst += lpSrch.GetLength();
        cchMax = lpSearchEnd - lpFirst;
    }
    return uCount;
}

static UINT StrStrCountNIA(const CHAR *lpFirst, const CStringA &lpSrch, UINT cchMax)
{
    return StrStrNCount<CHAR, CStringA, _strnicmp>(lpFirst, lpSrch, cchMax);
}

static UINT StrStrCountNIW(const WCHAR *lpFirst, const CStringW &lpSrch, UINT cchMax)
{
    return StrStrNCount<WCHAR, CStringW, _wcsnicmp>(lpFirst, lpSrch, cchMax);
}

static UINT SearchFile(LPCWSTR lpFilePath, _SearchData *pSearchData)
{
    HANDLE hFile = CreateFileW(lpFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD size = GetFileSize(hFile, NULL);
    HANDLE hFileMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (hFileMap == INVALID_HANDLE_VALUE)
        return 0;

    LPBYTE lpFileContent = (LPBYTE) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hFileMap);
    if (!lpFileContent)
        return 0;

    UINT uMatches = 0;
    // Check for UTF-16 BOM
    if (size >= 2 && lpFileContent[0] == 0xFF && lpFileContent[1] == 0xFE)
    {
        uMatches = StrStrCountNIW((LPCWSTR) lpFileContent, pSearchData->szQueryW, size / sizeof(WCHAR));
    }
    else
    {
        uMatches = StrStrCountNIA((LPCSTR) lpFileContent, pSearchData->szQueryA, size / sizeof(CHAR));
    }

    UnmapViewOfFile(lpFileContent);

    return uMatches;
}

static BOOL FileNameMatch(LPCWSTR FindDataFileName, _SearchData *pSearchData)
{
    if (pSearchData->szFileName.IsEmpty() || PathMatchSpecW(FindDataFileName, pSearchData->szFileName))
    {
        return TRUE;
    }
    return FALSE;
}

static BOOL ContentsMatch(LPCWSTR szPath, _SearchData *pSearchData)
{
    if (pSearchData->szQueryA.IsEmpty() || SearchFile(szPath, pSearchData))
    {
        return TRUE;
    }
    return FALSE;
}

static BOOL AttribHiddenMatch(DWORD FileAttributes, _SearchData *pSearchData)
{
    if (!(FileAttributes & FILE_ATTRIBUTE_HIDDEN) || (pSearchData->SearchHidden))
    {
        return TRUE;
    }
    return FALSE;
}

static UINT RecursiveFind(LPCWSTR lpPath, _SearchData *pSearchData)
{
    if (WaitForSingleObject(pSearchData->hStopEvent, 0) != WAIT_TIMEOUT)
        return 0;

    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;
    HANDLE hFindFile;
    BOOL bMoreFiles = TRUE;
    UINT uTotalFound = 0;

    PathCombineW(szPath, lpPath, L"*.*");

    for (hFindFile = FindFirstFileW(szPath, &FindData);
        bMoreFiles && hFindFile != INVALID_HANDLE_VALUE;
        bMoreFiles = FindNextFileW(hFindFile, &FindData))
    {
        if (!wcscmp(FindData.cFileName, L".") || !wcscmp(FindData.cFileName, L".."))
            continue;

        PathCombineW(szPath, lpPath, FindData.cFileName);

        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            CStringW status;
            if (FileNameMatch(FindData.cFileName, pSearchData)
                && AttribHiddenMatch(FindData.dwFileAttributes, pSearchData))
            {
                PostMessageW(pSearchData->hwnd, WM_SEARCH_ADD_RESULT, 0, (LPARAM) StrDupW(szPath));
                uTotalFound++;
            }
            status.Format(IDS_SEARCH_FOLDER, FindData.cFileName);
            PostMessageW(pSearchData->hwnd, WM_SEARCH_UPDATE_STATUS, 0, (LPARAM) StrDupW(status.GetBuffer()));

            uTotalFound += RecursiveFind(szPath, pSearchData);
        }
        else if (FileNameMatch(FindData.cFileName, pSearchData)
                && AttribHiddenMatch(FindData.dwFileAttributes, pSearchData)
                && ContentsMatch(szPath, pSearchData))
        {
            uTotalFound++;
            PostMessageW(pSearchData->hwnd, WM_SEARCH_ADD_RESULT, 0, (LPARAM) StrDupW(szPath));
        }
    }

    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);

    return uTotalFound;
}

DWORD WINAPI CFindFolder::SearchThreadProc(LPVOID lpParameter)
{
    _SearchData *data = static_cast<_SearchData*>(lpParameter);

    data->pFindFolder->NotifyConnections(DISPID_SEARCHSTART);

    UINT uTotalFound = RecursiveFind(data->szPath, data);

    data->pFindFolder->NotifyConnections(DISPID_SEARCHCOMPLETE);

    CStringW status;
    status.Format(IDS_SEARCH_FILES_FOUND, uTotalFound);
    ::PostMessageW(data->hwnd, WM_SEARCH_UPDATE_STATUS, 0, (LPARAM) StrDupW(status.GetBuffer()));
    ::SendMessageW(data->hwnd, WM_SEARCH_STOP, 0, 0);

    CloseHandle(data->hStopEvent);
    delete data;

    return 0;
}

void CFindFolder::NotifyConnections(DISPID id)
{
    DISPPARAMS dispatchParams = {0};
    CComDynamicUnkArray &subscribers =
        IConnectionPointImpl<CFindFolder, &DIID_DSearchCommandEvents>::m_vec;
    for (IUnknown** pSubscriber = subscribers.begin(); pSubscriber < subscribers.end(); pSubscriber++)
    {
        if (!*pSubscriber)
            continue;

        CComPtr<IDispatch> pDispatch;
        HRESULT hResult = (*pSubscriber)->QueryInterface(IID_PPV_ARG(IDispatch, &pDispatch));
        if (!FAILED_UNEXPECTEDLY(hResult))
            pDispatch->Invoke(id, GUID_NULL, 0, DISPATCH_METHOD, &dispatchParams, NULL, NULL, NULL);
    }
}

LRESULT CFindFolder::StartSearch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HKEY hkey;
    DWORD size = sizeof(DWORD);
    DWORD result;
    DWORD SearchHiddenValue = 0;

    if (!lParam)
        return 0;

    // Clear all previous search results
    UINT uItemIndex;
    m_shellFolderView->RemoveObject(NULL, &uItemIndex);

    _SearchData* pSearchData = new _SearchData();
    pSearchData->pFindFolder = this;
    pSearchData->hwnd = m_hWnd;

    SearchStart *pSearchParams = (SearchStart *) lParam;
    pSearchData->szPath = pSearchParams->szPath;
    pSearchData->szFileName = pSearchParams->szFileName;
    pSearchData->szQueryA = pSearchParams->szQuery;
    pSearchData->szQueryW = pSearchParams->szQuery;
    pSearchData->SearchHidden = pSearchParams->SearchHidden;
    SHFree(pSearchParams);

    TRACE("pSearchParams->SearchHidden is '%d'.\n", pSearchData->SearchHidden);

    if (pSearchData->SearchHidden)
        SearchHiddenValue = 1;
    else
        SearchHiddenValue = 0;

    /* Placing the code to save the changed settings to the registry here has the effect of not saving any changes */
    /* to the registry unless the user clicks on the "Search" button. This is the same as what we see in Windows.  */
    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", 0, KEY_SET_VALUE, &hkey);
    if (result == ERROR_SUCCESS)
    {
        if (RegSetValueExW(hkey, L"SearchHidden", NULL, REG_DWORD, (const BYTE*)&SearchHiddenValue, size) == ERROR_SUCCESS)
        {
            TRACE("SearchHidden is '%d'.\n", SearchHiddenValue);
        }
        else
        {
            ERR("RegSetValueEx for \"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SearchHidden\" Failed.\n");
        }
        RegCloseKey(hkey);
    }
    else
    {
        ERR("RegOpenKey for \"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\" Failed.\n");
    }

    if (m_hStopEvent)
        SetEvent(m_hStopEvent);
    pSearchData->hStopEvent = m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!SHCreateThread(SearchThreadProc, pSearchData, NULL, NULL))
    {
        SHFree(pSearchData);
        return 0;
    }

    return 0;
}

LRESULT CFindFolder::StopSearch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (m_hStopEvent)
    {
        SetEvent(m_hStopEvent);
        m_hStopEvent = NULL;
    }
    return 0;
}

LRESULT CFindFolder::AddResult(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!lParam)
        return 0;

    CComHeapPtr<WCHAR> lpPath((LPWSTR) lParam);

    CComHeapPtr<ITEMIDLIST> lpSearchPidl(_ILCreate(lpPath));
    if (lpSearchPidl)
    {
        UINT uItemIndex;
        m_shellFolderView->AddObject(lpSearchPidl, &uItemIndex);
    }

    return 0;
}

LRESULT CFindFolder::UpdateStatus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComHeapPtr<WCHAR> status((LPWSTR) lParam);
    if (m_shellBrowser)
    {
        m_shellBrowser->SetStatusTextSB(status);
    }

    return 0;
}

// *** IShellFolder2 methods ***
STDMETHODIMP CFindFolder::GetDefaultSearchGUID(GUID *pguid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::GetDefaultColumn(DWORD, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

STDMETHODIMP CFindFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (!pcsFlags)
        return E_INVALIDARG;
    if (iColumn >= _countof(g_ColumnDefs))
        return m_pisfInner->GetDefaultColumnState(iColumn - _countof(g_ColumnDefs) + 1, pcsFlags);
    *pcsFlags = g_ColumnDefs[iColumn].dwDefaultState;
    return S_OK;
}

STDMETHODIMP CFindFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisfInner->GetDetailsEx(pidl, pscid, pv);
}

STDMETHODIMP CFindFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    if (iColumn >= _countof(g_ColumnDefs))
        return m_pisfInner->GetDetailsOf(_ILGetFSPidl(pidl), iColumn - _countof(g_ColumnDefs) + 1, pDetails);

    pDetails->cxChar = g_ColumnDefs[iColumn].cxChar;
    pDetails->fmt = g_ColumnDefs[iColumn].fmt;

    if (!pidl)
        return SHSetStrRet(&pDetails->str, _AtlBaseModule.GetResourceInstance(), g_ColumnDefs[iColumn].iResource);

    if (iColumn == 1)
    {
        return SHSetStrRet(&pDetails->str, _ILGetPath(pidl));
    }

    return GetDisplayNameOf(pidl, SHGDN_NORMAL, &pDetails->str);
}

STDMETHODIMP CFindFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IShellFolder methods ***
STDMETHODIMP CFindFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten,
                                           PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    *ppEnumIDList = NULL;
    return S_FALSE;
}

STDMETHODIMP CFindFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return m_pisfInner->CompareIDs(lParam, _ILGetFSPidl(pidl1), _ILGetFSPidl(pidl2));
}

STDMETHODIMP CFindFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvparams = {};
        sfvparams.cbSize = sizeof(SFV_CREATE);
        sfvparams.pshf = this;
        sfvparams.psfvcb = this;
        HRESULT hr = SHCreateShellFolderView(&sfvparams, (IShellView **) ppvOut);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            return hr;
        }

        return ((IShellView * ) * ppvOut)->QueryInterface(IID_PPV_ARG(IShellFolderView, &m_shellFolderView));
    }
    return E_NOINTERFACE;
}

STDMETHODIMP CFindFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    CComHeapPtr<PCITEMID_CHILD> aFSPidl;
    aFSPidl.Allocate(cidl);
    for (UINT i = 0; i < cidl; i++)
    {
        aFSPidl[i] = _ILGetFSPidl(apidl[i]);
    }

    return m_pisfInner->GetAttributesOf(cidl, aFSPidl, rgfInOut);
}

class CFindFolderContextMenu :
        public IContextMenu,
        public CComObjectRootEx<CComMultiThreadModelNoCS>
{
    CComPtr<IContextMenu> m_pInner;
    CComPtr<IShellFolderView> m_shellFolderView;
    UINT m_firstCmdId;
    static const UINT ADDITIONAL_MENU_ITEMS = 2;

    //// *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        m_firstCmdId = indexMenu;
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst++, MFT_STRING, MAKEINTRESOURCEW(IDS_SEARCH_OPEN_FOLDER), MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst++, MFT_SEPARATOR, NULL, 0);
        return m_pInner->QueryContextMenu(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
    {
        if (!IS_INTRESOURCE(lpcmi->lpVerb))
        {
            return m_pInner->InvokeCommand(lpcmi);
        }

        if (LOWORD(lpcmi->lpVerb) < m_firstCmdId + ADDITIONAL_MENU_ITEMS)
        {
            PCUITEMID_CHILD *apidl;
            UINT cidl;
            HRESULT hResult = m_shellFolderView->GetSelectedObjects(&apidl, &cidl);
            if (FAILED_UNEXPECTEDLY(hResult))
                return hResult;

            for (UINT i = 0; i < cidl; i++)
            {
                CComHeapPtr<ITEMIDLIST> folderPidl(ILCreateFromPathW(_ILGetPath(apidl[i])));
                if (!folderPidl)
                    return E_OUTOFMEMORY;
                CComHeapPtr<ITEMIDLIST> filePidl(ILCombine(folderPidl, _ILGetFSPidl(apidl[i])));
                if (!filePidl)
                    return E_OUTOFMEMORY;
                SHOpenFolderAndSelectItems(folderPidl, 1, &filePidl, 0);
            }
            return S_OK;
        }

        CMINVOKECOMMANDINFOEX actualCmdInfo;
        memcpy(&actualCmdInfo, lpcmi, lpcmi->cbSize);
        actualCmdInfo.lpVerb -= ADDITIONAL_MENU_ITEMS;
        return m_pInner->InvokeCommand((CMINVOKECOMMANDINFO *)&actualCmdInfo);
    }

    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
    {
        return m_pInner->GetCommandString(idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);
    }

public:
    static HRESULT Create(IShellFolderView *pShellFolderView, IContextMenu *pInnerContextMenu, IContextMenu **pContextMenu)
    {
        CComObject<CFindFolderContextMenu> *pObj;
        HRESULT hResult = CComObject<CFindFolderContextMenu>::CreateInstance(&pObj);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        pObj->m_shellFolderView = pShellFolderView;
        pObj->m_pInner = pInnerContextMenu;
        return pObj->QueryInterface(IID_PPV_ARG(IContextMenu, pContextMenu));
    }

    BEGIN_COM_MAP(CFindFolderContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

STDMETHODIMP CFindFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                                        UINT *prgfInOut, LPVOID *ppvOut)
{
    if (cidl <= 0)
    {
        return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    CComHeapPtr<PCITEMID_CHILD> aFSPidl;
    aFSPidl.Allocate(cidl);
    for (UINT i = 0; i < cidl; i++)
    {
        aFSPidl[i] = _ILGetFSPidl(apidl[i]);
    }

    if (riid == IID_IContextMenu)
    {
        CComHeapPtr<ITEMIDLIST> folderPidl(ILCreateFromPathW(_ILGetPath(apidl[0])));
        if (!folderPidl)
            return E_OUTOFMEMORY;
        CComPtr<IShellFolder> pDesktopFolder;
        HRESULT hResult = SHGetDesktopFolder(&pDesktopFolder);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        CComPtr<IShellFolder> pShellFolder;
        hResult = pDesktopFolder->BindToObject(folderPidl, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        CComPtr<IContextMenu> pContextMenu;
        hResult = pShellFolder->GetUIObjectOf(hwndOwner, cidl, aFSPidl, riid, prgfInOut, (LPVOID *)&pContextMenu);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        return CFindFolderContextMenu::Create(m_shellFolderView, pContextMenu, (IContextMenu **)ppvOut);
    }

    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, aFSPidl, riid, prgfInOut, ppvOut);
}

STDMETHODIMP CFindFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName)
{
    return m_pisfInner->GetDisplayNameOf(_ILGetFSPidl(pidl), dwFlags, pName);
}

STDMETHODIMP CFindFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags,
                                    PITEMID_CHILD *pPidlOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

//// *** IShellFolderViewCB method ***
STDMETHODIMP CFindFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_DEFVIEWMODE:
        {
            FOLDERVIEWMODE *pViewMode = (FOLDERVIEWMODE *) lParam;
            *pViewMode = FVM_DETAILS;
            return S_OK;
        }
        case SFVM_WINDOWCREATED:
        {
            // Subclass window to receive window messages
            SubclassWindow((HWND) wParam);

            // Get shell browser for updating status bar text
            CComPtr<IServiceProvider> pServiceProvider;
            HRESULT hr = m_shellFolderView->QueryInterface(IID_PPV_ARG(IServiceProvider, &pServiceProvider));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            hr = pServiceProvider->QueryService(SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &m_shellBrowser));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            // Open search bar
            CComPtr<IWebBrowser2> pWebBrowser2;
            hr = m_shellBrowser->QueryInterface(IID_PPV_ARG(IWebBrowser2, &pWebBrowser2));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            WCHAR pwszGuid[MAX_PATH];
            StringFromGUID2(CLSID_FileSearchBand, pwszGuid, _countof(pwszGuid));
            CComVariant searchBar(pwszGuid);
            return pWebBrowser2->ShowBrowserBar(&searchBar, NULL, NULL);
        }
    }
    return E_NOTIMPL;
}

//// *** IPersistFolder2 methods ***
STDMETHODIMP CFindFolder::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    *pidl = ILClone(m_pidl);
    return S_OK;
}

// *** IPersistFolder methods ***
STDMETHODIMP CFindFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    m_pidl = ILClone(pidl);
    if (!m_pidl)
        return E_OUTOFMEMORY;

    return SHELL32_CoCreateInitSF(m_pidl,
                                  NULL,
                                  NULL,
                                  &CLSID_ShellFSFolder,
                                  IID_PPV_ARG(IShellFolder2, &m_pisfInner));
}

// *** IPersist methods ***
STDMETHODIMP CFindFolder::GetClassID(CLSID *pClassId)
{
    if (pClassId == NULL)
        return E_INVALIDARG;
    *pClassId = CLSID_FindFolder;
    return S_OK;
}
