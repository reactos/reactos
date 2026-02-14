/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main class
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

enum FOLDERCOLUMN
{
    COL_NAME = 0,
    COL_TYPE,
    COL_COMPRSIZE,
    COL_PASSWORD,
    COL_SIZE,
    COL_RATIO,
    COL_DATE_MOD,
};

struct FolderViewColumns
{
    int iResource;
    DWORD dwDefaultState;
    int cxChar;
    int fmt;
};

class CZipFolder :
    public CComCoClass<CZipFolder, &CLSID_ZipFolderStorageHandler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    //public IStorage,
    public IContextMenu,
    public IShellExtInit,
    public IPersistFile,
    public IPersistFolder2,
    public IDropTarget, // FIXME: This IDropTarget is a HACK. Must be CZipFolderDropHandler. See com_apitest:zipfldr.
    public IZip
{
    CStringW m_ZipFile;
    CStringW m_ZipDir;
    CComHeapPtr<ITEMIDLIST> m_CurDir;
    unzFile m_UnzipFile = nullptr;
    CComPtr<IDataObject> m_pDataObj;
    HWND m_hwnd = nullptr;

    HRESULT DeleteItems(CComPtr<IDataObject> pDataObj);
    HRESULT DoDeleteItems(CComPtr<IDataObject> pDataObj);
    HRESULT CopyZipEntry(unzFile uf, zipFile zf, unz_file_info64* info, LPCSTR nameA);
    static DWORD WINAPI s_ExtractProc(LPVOID arg);
    BOOL _GetFileTimeString(LPFILETIME lpFileTime, PWSTR pwszResult, UINT cchResult);
    static HRESULT CALLBACK ZipFolderMenuCallback(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
                                                  UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CZipFolder();
    ~CZipFolder();

    HRESULT Initialize(PCWSTR zipFile, PCWSTR zipDir, PCUIDLIST_ABSOLUTE curDir, PCUIDLIST_RELATIVE pidl);
    void Close();

    // *** IZip methods ***
    STDMETHODIMP_(unzFile) getZip();

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(GUID *pguid) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override
    {
        if (pSort)
            *pSort = COL_NAME;
        if (pDisplay)
            *pDisplay = COL_NAME;
        return S_OK;
    }
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags) override;
    STDMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
    STDMETHODIMP MapColumnToSCID(UINT column, SHCOLUMNID *pscid) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override
    {
        return _CEnumZipContents_CreateInstance(this, dwFlags, m_ZipDir, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
    }
    STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
    STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
    STDMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
    STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override;
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override;
    STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override;
    STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override
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
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override;
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;

    // *** IShellExtInit methods ***
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, LPDATAOBJECT pDataObj, HKEY hkeyProgID) override;

    // *** IPersistFile methods ***
    STDMETHODIMP IsDirty() override;
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) override;
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) override;
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName) override;
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName) override;

    //// *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE * pidl) override
    {
        *pidl = ILClone(m_CurDir);
        return S_OK;
    }

    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl) override;

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *lpClassId) override
    {
        *lpClassId = CLSID_ZipFolderStorageHandler;
        return S_OK;
    }

    // *** IDropTarget methods *** // FIXME: This is a HACK. Must be CZipFolderDropHandler. See com_apitest:zipfldr.
    STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

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
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget) // FIXME: This is a HACK. Must be CZipFolderDropHandler. See com_apitest:zipfldr.
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistFolder)
    END_COM_MAP()
};
