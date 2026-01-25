/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main class
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

static FolderViewColumns g_ColumnDefs[] =
{
    { IDS_COL_NAME,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   25, LVCFMT_LEFT },
    { IDS_COL_TYPE,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   20, LVCFMT_LEFT },
    { IDS_COL_COMPRSIZE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   10, LVCFMT_RIGHT },
    { IDS_COL_PASSWORD,  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   10, LVCFMT_LEFT },
    { IDS_COL_SIZE,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   10, LVCFMT_RIGHT },
    { IDS_COL_RATIO,     SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   10, LVCFMT_LEFT },
    { IDS_COL_DATE_MOD,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT,  15, LVCFMT_LEFT },
};

CZipFolder::CZipFolder() : m_UnzipFile(NULL)
{
}

CZipFolder::~CZipFolder()
{
    Close();
}

void CZipFolder::Close()
{
    if (m_UnzipFile)
        unzClose(m_UnzipFile);
    m_UnzipFile = NULL;
}

STDMETHODIMP_(unzFile) CZipFolder::getZip()
{
    if (!m_UnzipFile)
        m_UnzipFile = unzOpen2_64(m_ZipFile, &g_FFunc);
    return m_UnzipFile;
}

// Adapted from CFileDefExt::GetFileTimeString
BOOL CZipFolder::_GetFileTimeString(LPFILETIME lpFileTime, PWSTR pwszResult, UINT cchResult)
{
    SYSTEMTIME st;

    if (!FileTimeToSystemTime(lpFileTime, &st))
        return FALSE;

    size_t cchRemaining = cchResult;
    PWSTR pwszEnd = pwszResult;
    int cchWritten = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pwszEnd, cchRemaining);
    if (cchWritten)
        --cchWritten; // GetDateFormatW returns count with terminating zero
    else
        return FALSE;
    cchRemaining -= cchWritten;
    pwszEnd += cchWritten;

    StringCchCopyExW(pwszEnd, cchRemaining, L" ", &pwszEnd, &cchRemaining, 0);

    cchWritten = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, pwszEnd, cchRemaining);
    if (cchWritten)
        --cchWritten; // GetTimeFormatW returns count with terminating zero
    else
        return FALSE;

    return TRUE;
}

STDMETHODIMP CZipFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (!pcsFlags || iColumn >= _countof(g_ColumnDefs))
        return E_INVALIDARG;
    *pcsFlags = g_ColumnDefs[iColumn].dwDefaultState;
    return S_OK;
}

STDMETHODIMP CZipFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (iColumn >= _countof(g_ColumnDefs))
        return E_FAIL;

    psd->cxChar = g_ColumnDefs[iColumn].cxChar;
    psd->fmt = g_ColumnDefs[iColumn].fmt;

    if (pidl == NULL)
    {
        return SHSetStrRet(&psd->str, _AtlBaseModule.GetResourceInstance(), g_ColumnDefs[iColumn].iResource);
    }

    PCUIDLIST_RELATIVE curpidl = ILGetNext(pidl);
    if (curpidl->mkid.cb != 0)
    {
        DPRINT1("ERROR, unhandled PIDL!\n");
        return E_FAIL;
    }

    const ZipPidlEntry* zipEntry = _ZipFromIL(pidl);
    if (!zipEntry)
        return E_INVALIDARG;

    WCHAR Buffer[100];
    bool isDir = zipEntry->ZipType == ZIP_PIDL_DIRECTORY;
    switch (iColumn)
    {
        case COL_NAME:
            return GetDisplayNameOf(pidl, 0, &psd->str);
        case COL_TYPE:
        {
            SHFILEINFOW shfi;
            DWORD dwAttributes = isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            ULONG_PTR firet = SHGetFileInfoW(zipEntry->Name, dwAttributes, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
            if (!firet)
                return E_FAIL;
            return SHSetStrRet(&psd->str, shfi.szTypeName);
        }
        case COL_COMPRSIZE:
        case COL_SIZE:
        {
            if (isDir)
                return SHSetStrRet(&psd->str, L"");

            ULONG64 Size = iColumn == COL_COMPRSIZE ? zipEntry->CompressedSize : zipEntry->UncompressedSize;
            if (!StrFormatByteSizeW(Size, Buffer, _countof(Buffer)))
                return E_FAIL;
            return SHSetStrRet(&psd->str, Buffer);
        }
        case COL_PASSWORD:
            if (isDir)
                return SHSetStrRet(&psd->str, L"");
            return SHSetStrRet(&psd->str, _AtlBaseModule.GetResourceInstance(), zipEntry->Password ? IDS_YES : IDS_NO);
        case COL_RATIO:
        {
            if (isDir)
                return SHSetStrRet(&psd->str, L"");

            int ratio = 0;
            if (zipEntry->UncompressedSize)
                ratio = 100 - (int)((zipEntry->CompressedSize*100)/zipEntry->UncompressedSize);
            StringCchPrintfW(Buffer, _countof(Buffer), L"%d%%", ratio);
            return SHSetStrRet(&psd->str, Buffer);
        }
        case COL_DATE_MOD:
        {
            if (isDir)
                return SHSetStrRet(&psd->str, L"");
            FILETIME ftLocal;
            DosDateTimeToFileTime((WORD)(zipEntry->DosDate>>16), (WORD)zipEntry->DosDate, &ftLocal);
            if (!_GetFileTimeString(&ftLocal, Buffer, _countof(Buffer)))
                return E_FAIL;
            return SHSetStrRet(&psd->str, Buffer);
        }
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CZipFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    if (riid == IID_IShellFolder)
    {
        CStringW newZipDir = m_ZipDir;
        PCUIDLIST_RELATIVE curpidl = pidl;
        while (curpidl->mkid.cb)
        {
            const ZipPidlEntry* zipEntry = _ZipFromIL(curpidl);
            if (!zipEntry)
            {
                return E_FAIL;
            }
            newZipDir += zipEntry->Name;
            newZipDir += L'/';

            curpidl = ILGetNext(curpidl);
        }
        return ShellObjectCreatorInit<CZipFolder>(m_ZipFile, newZipDir, m_CurDir, pidl, riid, ppvOut);
    }
    DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__, guid2string(riid));
    return E_NOTIMPL;
}

STDMETHODIMP CZipFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    const ZipPidlEntry* zipEntry1 = _ZipFromIL(pidl1);
    const ZipPidlEntry* zipEntry2 = _ZipFromIL(pidl2);

    if (!zipEntry1 || !zipEntry2)
        return E_INVALIDARG;

    int result = 0;
    if (zipEntry1->ZipType != zipEntry2->ZipType)
        result = zipEntry1->ZipType - zipEntry2->ZipType;
    else
        result = StrCmpIW(zipEntry1->Name, zipEntry2->Name);

    if (!result && zipEntry1->ZipType == ZIP_PIDL_DIRECTORY)
    {
        PCUIDLIST_RELATIVE child1 = ILGetNext(pidl1);
        PCUIDLIST_RELATIVE child2 = ILGetNext(pidl2);

        if (child1->mkid.cb && child2->mkid.cb)
            return CompareIDs(lParam, child1, child2);
        else if (child1->mkid.cb)
            result = 1;
        else if (child2->mkid.cb)
            result = -1;
    }

    return MAKE_COMPARE_HRESULT(result);
}

STDMETHODIMP CZipFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    static const GUID UnknownIID = // {93F81976-6A0D-42C3-94DD-AA258A155470}
    {0x93F81976, 0x6A0D, 0x42C3, {0x94, 0xDD, 0xAA, 0x25, 0x8A, 0x15, 0x54, 0x70}};
    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvparams = {sizeof(SFV_CREATE), this};
        CComPtr<IShellFolderViewCB> pcb;

        HRESULT hr = _CFolderViewCB_CreateInstance(IID_PPV_ARG(IShellFolderViewCB, &pcb));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        sfvparams.psfvcb = pcb;
        hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppvOut);

        return hr;
    }
    else if (riid == IID_IExplorerCommandProvider)
    {
        return _CExplorerCommandProvider_CreateInstance(this, riid, ppvOut);
    }
    else if (riid == IID_IContextMenu)
    {
        // Folder context menu
        return QueryInterface(riid, ppvOut);
    }
    else if (riid == IID_IDropTarget)
    {
        *ppvOut = static_cast<IDropTarget*>(this);
        AddRef();
        return S_OK;
    }
    if (UnknownIID != riid)
        DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__, guid2string(riid));
    return E_NOTIMPL;
}

STDMETHODIMP CZipFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    if (!rgfInOut || !cidl || !apidl)
        return E_INVALIDARG;

    *rgfInOut = 0;

    //static DWORD dwFileAttrs = SFGAO_STREAM | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_CANCOPY | SFGAO_CANMOVE;
    //static DWORD dwFolderAttrs = SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_STORAGE | SFGAO_CANCOPY | SFGAO_CANMOVE;
    static DWORD dwFileAttrs = SFGAO_STREAM;
    static DWORD dwFolderAttrs = SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE | SFGAO_DROPTARGET;

    while (cidl > 0 && *apidl)
    {
        const ZipPidlEntry* zipEntry = _ZipFromIL(*apidl);

        if (zipEntry)
        {
            if (zipEntry->ZipType == ZIP_PIDL_FILE)
                *rgfInOut |= dwFileAttrs;
            else
                *rgfInOut |= dwFolderAttrs;
        }
        else
        {
            *rgfInOut = 0;
        }

        apidl++;
        cidl--;
    }

    *rgfInOut &= ~SFGAO_VALIDATE;
    return S_OK;
}

HRESULT CALLBACK CZipFolder::ZipFolderMenuCallback(
    IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
    UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case DFM_MERGECONTEXTMENU:
        {
            CComQIIDPtr<I_ID(IContextMenu)> spContextMenu(psf);
            if (!spContextMenu)
                return E_NOINTERFACE;

            QCMINFO *pqcminfo = (QCMINFO *)lParam;
            HRESULT hr = spContextMenu->QueryContextMenu(pqcminfo->hmenu,
                                                 pqcminfo->indexMenu,
                                                 pqcminfo->idCmdFirst,
                                                 pqcminfo->idCmdLast,
                                                 CMF_NORMAL);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            pqcminfo->idCmdFirst += HRESULT_CODE(hr);
            return S_OK;
        }
        case DFM_INVOKECOMMAND:
        {
            CComQIIDPtr<I_ID(IContextMenu)> spContextMenu(psf);
            if (!spContextMenu)
                return E_NOINTERFACE;

            CMINVOKECOMMANDINFO ici = { sizeof(ici) };
            ici.lpVerb = MAKEINTRESOURCEA(wParam);
            return spContextMenu->InvokeCommand(&ici);
        }
        case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
            return S_FALSE;
    }
    return E_NOTIMPL;
}

STDMETHODIMP CZipFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    if ((riid == IID_IExtractIconA || riid == IID_IExtractIconW) && cidl == 1)
    {
        const ZipPidlEntry* zipEntry = _ZipFromIL(*apidl);
        if (zipEntry)
        {
            DWORD dwAttributes = (zipEntry->ZipType == ZIP_PIDL_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            return SHCreateFileExtractIconW(zipEntry->Name, dwAttributes, riid, ppvOut);
        }
    }
    else if (riid == IID_IContextMenu && cidl >= 0)
    {
        // Context menu of an object inside the zip
        const ZipPidlEntry* zipEntry = _ZipFromIL(*apidl);
        if (zipEntry)
        {
            HKEY keys[1] = {0};
            int nkeys = 0;
            if (zipEntry->ZipType == ZIP_PIDL_DIRECTORY)
            {
                LSTATUS res = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Folder", 0, KEY_READ | KEY_QUERY_VALUE, keys);
                if (res != ERROR_SUCCESS)
                    return E_FAIL;
                nkeys++;
            }
            return CDefFolderMenu_Create2(NULL, hwndOwner, cidl, apidl, this, ZipFolderMenuCallback, nkeys, keys, (IContextMenu**)ppvOut);
        }
    }
    else if (riid == IID_IDataObject && cidl >= 1)
    {
        return CIDLData_CreateFromIDArray(m_CurDir, cidl, apidl, (IDataObject**)ppvOut);
    }
    else if (riid == IID_IDropTarget)
    {
        AddRef();
        *ppvOut = static_cast<IDropTarget*>(this);
        return S_OK;
    }

    DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__ , guid2string(riid));
    return E_NOINTERFACE;
}

STDMETHODIMP CZipFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!pidl)
        return S_FALSE;

    PCUIDLIST_RELATIVE curpidl = ILGetNext(pidl);
    if (curpidl->mkid.cb != 0)
    {
        DPRINT1("ERROR, unhandled PIDL!\n");
        return E_FAIL;
    }

    const ZipPidlEntry* zipEntry = _ZipFromIL(pidl);
    if (!zipEntry)
        return E_FAIL;

    return SHSetStrRet(strRet, zipEntry->Name);
}

STDMETHODIMP CZipFolder::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    if (idCmd != 0)
        return E_INVALIDARG;

    switch (uFlags)
    {
        case GCS_VERBA:
            return StringCchCopyA(pszName, cchMax, EXTRACT_VERBA);
        case GCS_VERBW:
            return StringCchCopyW((PWSTR)pszName, cchMax, EXTRACT_VERBW);
        case GCS_HELPTEXTA:
        {
            CStringA helpText(MAKEINTRESOURCEA(IDS_HELPTEXT));
            return StringCchCopyA(pszName, cchMax, helpText);
        }
        case GCS_HELPTEXTW:
        {
            CStringW helpText(MAKEINTRESOURCEA(IDS_HELPTEXT));
            return StringCchCopyW((PWSTR)pszName, cchMax, helpText);
        }
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return S_OK;
    }

    return E_INVALIDARG;
}

STDMETHODIMP CZipFolder::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (!pici || (pici->cbSize != sizeof(CMINVOKECOMMANDINFO) && pici->cbSize != sizeof(CMINVOKECOMMANDINFOEX)))
        return E_INVALIDARG;

    if (pici->lpVerb == MAKEINTRESOURCEA(0) || (HIWORD(pici->lpVerb) && !strcmp(pici->lpVerb, EXTRACT_VERBA)))
    {
        BSTR ZipFile = m_ZipFile.AllocSysString();
        InterlockedIncrement(&g_ModuleRefCnt);

        DWORD tid;
        HANDLE hThread = CreateThread(NULL, 0, s_ExtractProc, ZipFile, NULL, &tid);
        if (hThread)
        {
            CloseHandle(hThread);
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

STDMETHODIMP CZipFolder::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT idCmd = idCmdFirst;

    if (!(uFlags & CMF_DEFAULTONLY))
    {
        CStringW menuText(MAKEINTRESOURCEW(IDS_MENUITEM));

        if (indexMenu)
        {
            InsertMenuW(hmenu, indexMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        }
        InsertMenuW(hmenu, indexMenu++, MF_BYPOSITION | MF_STRING, idCmd++, menuText);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idCmd - idCmdFirst);
}

STDMETHODIMP CZipFolder::Initialize(PCIDLIST_ABSOLUTE pidlFolder, LPDATAOBJECT pDataObj, HKEY hkeyProgID)
{
    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg;

    HRESULT hr = pDataObj->GetData(&etc, &stg);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = E_FAIL;
    HDROP hdrop = (HDROP)GlobalLock(stg.hGlobal);
    if (hdrop)
    {
        UINT uNumFiles = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);
        if (uNumFiles == 1)
        {
            WCHAR szFile[MAX_PATH * 2];
            if (DragQueryFileW(hdrop, 0, szFile, _countof(szFile)))
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHParseDisplayName(szFile, NULL, &pidl, 0, NULL);
                if (!FAILED_UNEXPECTEDLY(hr))
                {
                    hr = Initialize(pidl);
                }
            }
            else
            {
                DbgPrint("Failed to query the file.\r\n");
            }
        }
        else
        {
            DbgPrint("Invalid number of files: %d\r\n", uNumFiles);
        }
        GlobalUnlock(stg.hGlobal);
    }
    else
    {
        DbgPrint("Could not lock stg.hGlobal\r\n");
    }
    ReleaseStgMedium(&stg);
    return hr;
}

STDMETHODIMP CZipFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    WCHAR tmpPath[MAX_PATH];

    if (SHGetPathFromIDListW(pidl, tmpPath))
    {
        m_ZipFile = tmpPath;
        m_CurDir.Attach(ILClone(pidl));
        return S_OK;
    }
    DbgPrint("%s() => Unable to parse pidl\n", __FUNCTION__);
    return E_INVALIDARG;
}

STDMETHODIMP CZipFolder::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CZipFolder::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CZipFolder::DragLeave()
{
    return S_OK;
}

STDMETHODIMP CZipFolder::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM sm;

    HRESULT hr = pDataObj->GetData(&fe, &sm);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    HDROP hDrop = (HDROP)GlobalLock(sm.hGlobal);
    if (hDrop)
    {
        Close();

        CZipCreator* pCreator = CZipCreator::DoCreate(m_ZipFile);

        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < fileCount; i++)
        {
            WCHAR szFilePath[MAX_PATH];
            DragQueryFileW(hDrop, i, szFilePath, MAX_PATH);
            pCreator->DoAddItem(szFilePath);
        }

        CZipCreator::runThread(pCreator);

        GlobalUnlock(sm.hGlobal);
        *pdwEffect = DROPEFFECT_COPY;
    }
    ReleaseStgMedium(&sm);

    return S_OK;
}

STDMETHODIMP CZipFolder::Initialize(PCWSTR zipFile, PCWSTR zipDir, PCUIDLIST_ABSOLUTE curDir, PCUIDLIST_RELATIVE pidl)
{
    m_ZipFile = zipFile;
    m_ZipDir = zipDir;

    m_CurDir.Attach(ILCombine(curDir, pidl));
    return S_OK;
}

DWORD WINAPI CZipFolder::s_ExtractProc(LPVOID arg)
{
    CComBSTR ZipFile;
    ZipFile.Attach((BSTR)arg);

    _CZipExtract_runWizard(ZipFile);

    InterlockedDecrement(&g_ModuleRefCnt);
    return 0;
}
