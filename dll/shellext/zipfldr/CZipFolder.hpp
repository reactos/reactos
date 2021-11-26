/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main class
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */


EXTERN_C HRESULT WINAPI SHCreateFileExtractIconW(LPCWSTR pszPath, DWORD dwFileAttributes, REFIID riid, void **ppv);


struct FolderViewColumns
{
    int iResource;
    DWORD dwDefaultState;
    int cxChar;
    int fmt;
};

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


class CZipFolder :
    public CComCoClass<CZipFolder, &CLSID_ZipFolderStorageHandler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    //public IStorage,
    public IContextMenu,
    public IShellExtInit,
    //public IPersistFile,
    public IPersistFolder2,
    public IZip
{
    CStringW m_ZipFile;
    CStringA m_ZipDir;
    CComHeapPtr<ITEMIDLIST> m_CurDir;
    unzFile m_UnzipFile;

public:
    CZipFolder()
        :m_UnzipFile(NULL)
    {
    }

    ~CZipFolder()
    {
        Close();
    }

    void Close()
    {
        if (m_UnzipFile)
            unzClose(m_UnzipFile);
        m_UnzipFile = NULL;
    }

    // *** IZip methods ***
    STDMETHODIMP_(unzFile) getZip()
    {
        if (!m_UnzipFile)
        {
            m_UnzipFile = unzOpen2_64(m_ZipFile, &g_FFunc);
        }

        return m_UnzipFile;
    }

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(GUID *pguid)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
    {
        if (!pcsFlags || iColumn >= _countof(g_ColumnDefs))
            return E_INVALIDARG;
        *pcsFlags = g_ColumnDefs[iColumn].dwDefaultState;
        return S_OK;
    }
    STDMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    // Adapted from CFileDefExt::GetFileTimeString
    BOOL _GetFileTimeString(LPFILETIME lpFileTime, LPWSTR pwszResult, UINT cchResult)
    {
        SYSTEMTIME st;

        if (!FileTimeToSystemTime(lpFileTime, &st))
            return FALSE;

        size_t cchRemaining = cchResult;
        LPWSTR pwszEnd = pwszResult;
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
    STDMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
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
        case 0: /* Name, ReactOS specific? */
            return GetDisplayNameOf(pidl, 0, &psd->str);
        case 1: /* Type */
        {
            SHFILEINFOA shfi;
            DWORD dwAttributes = isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            ULONG_PTR firet = SHGetFileInfoA(zipEntry->Name, dwAttributes, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
            if (!firet)
                return E_FAIL;
            return SHSetStrRet(&psd->str, shfi.szTypeName);
        }
        case 2: /* Compressed size */
        case 4: /* Size */
        {
            if (isDir)
                return SHSetStrRet(&psd->str, L"");

            ULONG64 Size = iColumn == 2 ? zipEntry->CompressedSize : zipEntry->UncompressedSize;
            if (!StrFormatByteSizeW(Size, Buffer, _countof(Buffer)))
                return E_FAIL;
            return SHSetStrRet(&psd->str, Buffer);
        }
        case 3: /* Password */
            if (isDir)
                return SHSetStrRet(&psd->str, L"");
            return SHSetStrRet(&psd->str, _AtlBaseModule.GetResourceInstance(), zipEntry->Password ? IDS_YES : IDS_NO);
        case 5: /* Ratio */
        {
            if (isDir)
                return SHSetStrRet(&psd->str, L"");

            int ratio = 0;
            if (zipEntry->UncompressedSize)
                ratio = 100 - (int)((zipEntry->CompressedSize*100)/zipEntry->UncompressedSize);
            StringCchPrintfW(Buffer, _countof(Buffer), L"%d%%", ratio);
            return SHSetStrRet(&psd->str, Buffer);
        }
        case 6: /* Date */
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
    STDMETHODIMP MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
    {
        return _CEnumZipContents_CreateInstance(this, dwFlags, m_ZipDir, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
    }
    STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
    {
        if (riid == IID_IShellFolder)
        {
            CStringA newZipDir = m_ZipDir;
            PCUIDLIST_RELATIVE curpidl = pidl;
            while (curpidl->mkid.cb)
            {
                const ZipPidlEntry* zipEntry = _ZipFromIL(curpidl);
                if (!zipEntry)
                {
                    return E_FAIL;
                }
                newZipDir += zipEntry->Name;
                newZipDir += '/';

                curpidl = ILGetNext(curpidl);
            }
            return ShellObjectCreatorInit<CZipFolder>(m_ZipFile, newZipDir, m_CurDir, pidl, riid, ppvOut);
        }
        DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__, guid2string(riid));
        return E_NOTIMPL;
    }
    STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
    {
        const ZipPidlEntry* zipEntry1 = _ZipFromIL(pidl1);
        const ZipPidlEntry* zipEntry2 = _ZipFromIL(pidl2);

        if (!zipEntry1 || !zipEntry2)
            return E_INVALIDARG;

        int result = 0;
        if (zipEntry1->ZipType != zipEntry2->ZipType)
            result = zipEntry1->ZipType - zipEntry2->ZipType;
        else
            result = stricmp(zipEntry1->Name, zipEntry2->Name);

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
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
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
        if (UnknownIID != riid)
            DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__, guid2string(riid));
        return E_NOTIMPL;
    }
    STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
    {
        if (!rgfInOut || !cidl || !apidl)
            return E_INVALIDARG;

        *rgfInOut = 0;

        //static DWORD dwFileAttrs = SFGAO_STREAM | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_CANCOPY | SFGAO_CANMOVE;
        //static DWORD dwFolderAttrs = SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_STORAGE | SFGAO_CANCOPY | SFGAO_CANMOVE;
        static DWORD dwFileAttrs = SFGAO_STREAM;
        static DWORD dwFolderAttrs = SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;


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
    static HRESULT CALLBACK ZipFolderMenuCallback(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
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
        case DFM_INVOKECOMMANDEX:
        case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
            return S_FALSE;
        }
        return E_NOTIMPL;
    }
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
    {
        if ((riid == IID_IExtractIconA || riid == IID_IExtractIconW) && cidl == 1)
        {
            const ZipPidlEntry* zipEntry = _ZipFromIL(*apidl);
            if (zipEntry)
            {
                CComHeapPtr<WCHAR> pathW;

                int len = MultiByteToWideChar(CP_ACP, 0, zipEntry->Name, -1, NULL, 0);
                pathW.Allocate(len);
                MultiByteToWideChar(CP_ACP, 0, zipEntry->Name, -1, pathW, len);

                DWORD dwAttributes = (zipEntry->ZipType == ZIP_PIDL_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
                return SHCreateFileExtractIconW(pathW, dwAttributes, riid, ppvOut);
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

        DbgPrint("%s(%S) UNHANDLED\n", __FUNCTION__ , guid2string(riid));
        return E_NOINTERFACE;
    }
    STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
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

        return SHSetStrRet(strRet, (LPCSTR)zipEntry->Name);
    }
    STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    //// IStorage
    //STDMETHODIMP CreateStream(LPCOLESTR pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm);
    //STDMETHODIMP OpenStream(LPCOLESTR pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
    //STDMETHODIMP CreateStorage(LPCOLESTR pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage **ppstg);
    //STDMETHODIMP OpenStorage(LPCOLESTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
    //STDMETHODIMP CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest);
    //STDMETHODIMP MoveElementTo(LPCOLESTR pwcsName, IStorage *pstgDest, LPCOLESTR pwcsNewName, DWORD grfFlags);
    //STDMETHODIMP Commit(DWORD grfCommitFlags);
    //STDMETHODIMP Revert();
    //STDMETHODIMP EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);
    //STDMETHODIMP DestroyElement(LPCOLESTR pwcsName);
    //STDMETHODIMP RenameElement(LPCOLESTR pwcsOldName, LPCOLESTR pwcsNewName);
    //STDMETHODIMP SetElementTimes(LPCOLESTR pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime);
    //STDMETHODIMP SetClass(REFCLSID clsid);
    //STDMETHODIMP SetStateBits(DWORD grfStateBits, DWORD grfMask);
    //STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);

    // *** IContextMenu methods ***
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
    {
        if (idCmd != 0)
            return E_INVALIDARG;

        switch (uFlags)
        {
        case GCS_VERBA:
            return StringCchCopyA(pszName, cchMax, EXTRACT_VERBA);
        case GCS_VERBW:
            return StringCchCopyW((LPWSTR)pszName, cchMax, EXTRACT_VERBW);
        case GCS_HELPTEXTA:
        {
            CStringA helpText(MAKEINTRESOURCEA(IDS_HELPTEXT));
            return StringCchCopyA(pszName, cchMax, helpText);
        }
        case GCS_HELPTEXTW:
        {
            CStringW helpText(MAKEINTRESOURCEA(IDS_HELPTEXT));
            return StringCchCopyW((LPWSTR)pszName, cchMax, helpText);
        }
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return S_OK;
        }

        return E_INVALIDARG;
    }
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici)
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
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
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

    // *** IShellExtInit methods ***
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, LPDATAOBJECT pDataObj, HKEY hkeyProgID)
    {
        FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg;

        HRESULT hr = pDataObj->GetData(&etc, &stg);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            return hr;
        }
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

    //// IPersistFile
    ////STDMETHODIMP GetClassID(CLSID *pclsid);
    //STDMETHODIMP IsDirty();
    //STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    //STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    //STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
    //STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

    //// *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE * pidl)
    {
        *pidl = ILClone(m_CurDir);
        return S_OK;
    }

    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl)
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

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *lpClassId)
    {
        DbgPrint("%s\n", __FUNCTION__);
        return E_NOTIMPL;
    }


    STDMETHODIMP Initialize(PCWSTR zipFile, PCSTR zipDir, PCUIDLIST_ABSOLUTE curDir, PCUIDLIST_RELATIVE pidl)
    {
        m_ZipFile = zipFile;
        m_ZipDir = zipDir;

        m_CurDir.Attach(ILCombine(curDir, pidl));
        return S_OK;
    }
    static DWORD WINAPI s_ExtractProc(LPVOID arg)
    {
        CComBSTR ZipFile;
        ZipFile.Attach((BSTR)arg);

        _CZipExtract_runWizard(ZipFile);

        InterlockedDecrement(&g_ModuleRefCnt);
        return 0;
    }

public:
    DECLARE_NO_REGISTRY()   // Handled manually because this object is exposed via multiple clsid's
    DECLARE_NOT_AGGREGATABLE(CZipFolder)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CZipFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
//        COM_INTERFACE_ENTRY_IID(IID_IStorage, IStorage)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        //COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    END_COM_MAP()
};

