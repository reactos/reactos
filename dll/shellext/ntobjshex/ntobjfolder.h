/*
 * PROJECT:     NT Object Namespace shell extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NT Object Namespace folder class header
 * COPYRIGHT:   Copyright 2015-2017 David Quintana <gigaherz@gmail.com>
 */

#pragma once

extern const GUID CLSID_NtObjectFolder;

class CNtObjectFolderExtractIcon :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW
{
    PCWSTR m_NtPath;
    PCITEMID_CHILD    m_pcidlChild;

public:
    CNtObjectFolderExtractIcon();

    virtual ~CNtObjectFolderExtractIcon();

    HRESULT Initialize(LPCWSTR ntPath, PCIDLIST_ABSOLUTE parent, UINT cidl, PCUITEMID_CHILD_ARRAY apidl);

    STDMETHOD(GetIconLocation)(
        UINT uFlags,
        LPWSTR szIconFile,
        UINT cchMax,
        INT *piIndex,
        UINT *pwFlags) override;

    STDMETHOD(Extract)(
        LPCWSTR pszFile,
        UINT nIconIndex,
        HICON *phiconLarge,
        HICON *phiconSmall,
        UINT nIconSize) override;

    DECLARE_NOT_AGGREGATABLE(CNtObjectFolderExtractIcon)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CNtObjectFolderExtractIcon)
        COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    END_COM_MAP()

};

class CNtObjectFolder :
    public CComCoClass<CNtObjectFolder, &CLSID_NtObjectFolder>,
    public CCommonFolder<CNtObjectFolder, NtPidlEntry, CNtObjectFolderExtractIcon>
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_NTOBJECTFOLDER)

    CNtObjectFolder();
    virtual ~CNtObjectFolder();

    // IShellFolder

    STDMETHOD(EnumObjects)(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList) override;

protected:
    STDMETHOD(InternalBindToObject)(
        PWSTR path,
        const NtPidlEntry * info,
        LPITEMIDLIST first,
        LPCITEMIDLIST rest,
        LPITEMIDLIST fullPidl,
        LPBC pbcReserved,
        IShellFolder** ppsfChild) override;

    STDMETHOD(ResolveSymLink)(
        const NtPidlEntry * info,
        LPITEMIDLIST * fullPidl) override;

public:
    STDMETHOD(GetDefaultColumnState)(
        UINT iColumn,
        SHCOLSTATEF *pcsFlags) override;

    STDMETHOD(GetDetailsEx)(
        LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid,
        VARIANT *pv) override;

    STDMETHOD(GetDetailsOf)(
        LPCITEMIDLIST pidl,
        UINT iColumn,
        SHELLDETAILS *psd) override;

    STDMETHOD(MapColumnToSCID)(
        UINT iColumn,
        SHCOLUMNID *pscid) override;

    // IPersistFolder
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl) override;

    // Internal
    HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath);

protected:
    STDMETHOD(CompareIDs)(LPARAM lParam, const NtPidlEntry * first, const NtPidlEntry * second) override;
    STDMETHOD_(ULONG, ConvertAttributes)(const NtPidlEntry * entry, PULONG inMask) override;
    STDMETHOD_(BOOL, IsFolder)(const NtPidlEntry * info) override;
    STDMETHOD_(BOOL, IsSymLink)(const NtPidlEntry * info) override;

    virtual HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const NtPidlEntry ** pentry);

    HRESULT FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents);

    HRESULT FormatContentsForDisplay(const NtPidlEntry * info, HKEY rootKey, LPCWSTR ntPath, PCWSTR * strContents);
};
