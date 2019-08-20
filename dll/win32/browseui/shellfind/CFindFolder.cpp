#include "CFindFolder.h"
#include <exdispid.h>

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

// FIXME: Remove this declaration after the function has been fully implemented
EXTERN_C HRESULT
WINAPI
SHOpenFolderAndSelectItems(LPITEMIDLIST pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags);

struct FolderViewColumns
{
    LPCWSTR wzColumnName;
    DWORD dwDefaultState;
    int fmt;
    int cxChar;
};

static FolderViewColumns g_ColumnDefs[] =
{
    {L"Name",      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {L"In Folder", SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {L"Relevance", SHCOLSTATE_TYPE_STR,                          LVCFMT_LEFT, 0}
};

CFindFolder::CFindFolder() :
    m_hStopEvent(NULL)
{
}

static LPITEMIDLIST _ILCreate(LPCWSTR lpszPath)
{
    CComHeapPtr<ITEMIDLIST> lpFSPidl(ILCreateFromPathW(lpszPath));
    if (!(LPITEMIDLIST)lpFSPidl)
    {
        ERR("Failed to create pidl from path\n");
        return 0;
    }
    LPITEMIDLIST lpLastFSPidl = ILFindLastID(lpFSPidl);

    int pathLen = (wcslen(lpszPath) + 1) * sizeof(WCHAR);
    int cbData = sizeof(WORD) + pathLen + lpLastFSPidl->mkid.cb;
    LPITEMIDLIST pidl = (LPITEMIDLIST) SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    LPBYTE p = (LPBYTE) pidl;
    *((WORD *) p) = cbData;
    p += sizeof(WORD);

    memcpy(p, lpszPath, pathLen);
    p += pathLen;

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

static UINT StrStrCountA(const CHAR *lpFirst, const CStringA &lpSrch, UINT cchMax)
{
    return StrStrNCount<CHAR, CStringA, strncmp>(lpFirst, lpSrch, cchMax);
}

static UINT StrStrCountW(const WCHAR *lpFirst, const CStringW &lpSrch, UINT cchMax)
{
    return StrStrNCount<WCHAR, CStringW, wcsncmp>(lpFirst, lpSrch, cchMax);
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
        uMatches = StrStrCountW((LPCWSTR) lpFileContent, pSearchData->szQueryW, size / sizeof(WCHAR));
    }
    else
    {
        uMatches = StrStrCountA((LPCSTR) lpFileContent, pSearchData->szQueryA, size / sizeof(CHAR));
    }

    UnmapViewOfFile(lpFileContent);

    return uMatches;
}

static VOID RecursiveFind(LPCWSTR lpPath, _SearchData *pSearchData)
{
    if (WaitForSingleObject(pSearchData->hStopEvent, 0) != WAIT_TIMEOUT)
        return;

    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW FindData;
    HANDLE hFindFile;
    BOOL bMoreFiles = TRUE;

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
            CStringW* status = new CStringW();
            status->Format(L"Searching '%s'", FindData.cFileName);
            PostMessageW(pSearchData->hwnd, WM_SEARCH_UPDATE_STATUS, 0, (LPARAM) status);

            RecursiveFind(szPath, pSearchData);
        }
        else if ((pSearchData->szFileName.IsEmpty() || PathMatchSpecW(FindData.cFileName, pSearchData->szFileName))
                && (pSearchData->szQueryA.IsEmpty() || SearchFile(szPath, pSearchData)))
        {
            uTotalFound++;
            PostMessageW(pSearchData->hwnd, WM_SEARCH_ADD_RESULT, 0, (LPARAM) StrDupW(szPath));
        }
    }

    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);
}

DWORD WINAPI CFindFolder::SearchThreadProc(LPVOID lpParameter)
{
    _SearchData *data = static_cast<_SearchData*>(lpParameter);

    data->pFindFolder->NotifyConnections(DISPID_SEARCHSTART);

    RecursiveFind(params->szPath, data);

    data->pFindFolder->NotifyConnections(DISPID_SEARCHCOMPLETE);

    SHFree(params);
    SHFree(lpParameter);

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
    SHFree(pSearchParams);

    if (m_hStopEvent)
        SetEvent(m_hStopEvent);
    pSearchData->hStopEvent = m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!SHCreateThread(SearchThreadProc, pSearchData, NULL, NULL))
    {
        SHFree(pSearchData);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
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
    CStringW *status = (CStringW *) lParam;
    if (m_shellBrowser)
    {
        m_shellBrowser->SetStatusTextSB(status->GetBuffer());
    }
    delete status;

    return S_OK;
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
        return SHSetStrRet(&pDetails->str, g_ColumnDefs[iColumn].wzColumnName);

    if (iColumn == 1)
    {
        WCHAR path[MAX_PATH];
        wcscpy(path, _ILGetPath(pidl));
        PathRemoveFileSpecW(path);
        return SHSetStrRet(&pDetails->str, path);
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

STDMETHODIMP CFindFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                                        UINT *prgfInOut, LPVOID *ppvOut)
{
    if (riid == IID_IDataObject && cidl == 1)
    {
        WCHAR path[MAX_PATH];
        wcscpy(path, (LPCWSTR) apidl[0]->mkid.abID);
        PathRemoveFileSpecW(path);
        CComHeapPtr<ITEMIDLIST> rootPidl(ILCreateFromPathW(path));
        if (!rootPidl)
            return E_OUTOFMEMORY;
        PCITEMID_CHILD aFSPidl[1];
        aFSPidl[0] = _ILGetFSPidl(apidl[0]);
        return IDataObject_Constructor(hwndOwner, rootPidl, aFSPidl, cidl, (IDataObject **) ppvOut);
    }

    if (cidl <= 0)
    {
        return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    PCITEMID_CHILD *aFSPidl = new PCITEMID_CHILD[cidl];
    for (UINT i = 0; i < cidl; i++)
    {
        aFSPidl[i] = _ILGetFSPidl(apidl[i]);
    }

    if (riid == IID_IContextMenu)
    {
        HKEY hKeys[16];
        UINT cKeys = 0;
        AddFSClassKeysToArray(aFSPidl[0], hKeys, &cKeys);

        DEFCONTEXTMENU dcm;
        dcm.hwnd = hwndOwner;
        dcm.pcmcb = this;
        dcm.pidlFolder = m_pidl;
        dcm.psf = this;
        dcm.cidl = cidl;
        dcm.apidl = apidl;
        dcm.cKeys = cKeys;
        dcm.aKeys = hKeys;
        dcm.punkAssociationInfo = NULL;
        HRESULT hr = SHCreateDefaultContextMenu(&dcm, riid, ppvOut);
        delete[] aFSPidl;

        return hr;
    }

    HRESULT hr = m_pisfInner->GetUIObjectOf(hwndOwner, cidl, aFSPidl, riid, prgfInOut, ppvOut);
    delete[] aFSPidl;

    return hr;
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
            SubclassWindow((HWND) wParam);

            CComPtr<IServiceProvider> pServiceProvider;
            HRESULT hr = m_shellFolderView->QueryInterface(IID_PPV_ARG(IServiceProvider, &pServiceProvider));
            if (FAILED_UNEXPECTEDLY(hr))
            {
                return hr;
            }
            return pServiceProvider->QueryService(SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &m_shellBrowser));
        }
    }
    return E_NOTIMPL;
}

//// *** IContextMenuCB method ***
STDMETHODIMP CFindFolder::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case DFM_MERGECONTEXTMENU:
        {
            QCMINFO *pqcminfo = (QCMINFO *) lParam;
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_SEPARATOR, NULL, 0);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_STRING, L"Open Containing Folder", MFS_ENABLED);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_SEPARATOR, NULL, 0);
            return S_OK;
        }
        case DFM_INVOKECOMMAND:
        case DFM_INVOKECOMMANDEX:
        {
            if (wParam != 1)
                break;

            PCUITEMID_CHILD *apidl;
            UINT cidl;
            HRESULT hr = m_shellFolderView->GetSelectedObjects(&apidl, &cidl);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            for (UINT i = 0; i < cidl; i++)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                DWORD attrs = 0;
                hr = SHILCreateFromPathW((LPCWSTR) apidl[i]->mkid.abID, &pidl, &attrs);
                if (SUCCEEDED(hr))
                {
                    SHOpenFolderAndSelectItems(NULL, 1, &pidl, 0);
                }
            }

            return S_OK;
        }
        case DFM_GETDEFSTATICID:
            return S_FALSE;
    }
    return Shell_DefaultContextMenuCallBack(m_pisfInner, pdtobj);
}

//// *** IPersistFolder2 methods ***
STDMETHODIMP CFindFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    *pidl = ILClone(m_pidl);
    return S_OK;
}

// *** IPersistFolder methods ***
STDMETHODIMP CFindFolder::Initialize(LPCITEMIDLIST pidl)
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
    memcpy(pClassId, &CLSID_FindFolder, sizeof(CLSID));
    return S_OK;
}
