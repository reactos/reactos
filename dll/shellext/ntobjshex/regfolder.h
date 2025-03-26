/*
 * PROJECT:     NT Object Namespace shell extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     System Registry folder class header
 * COPYRIGHT:   Copyright 2015-2017 David Quintana <gigaherz@gmail.com>
 */

#pragma once

extern const GUID CLSID_RegistryFolder;

class CRegistryFolderExtractIcon :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW
{
    PCIDLIST_ABSOLUTE m_pcidlFolder;
    PCITEMID_CHILD    m_pcidlChild;

public:
    CRegistryFolderExtractIcon();

    virtual ~CRegistryFolderExtractIcon();

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

    DECLARE_NOT_AGGREGATABLE(CRegistryFolderExtractIcon)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRegistryFolderExtractIcon)
        COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    END_COM_MAP()

};

class CRegistryFolder :
    public CComCoClass<CRegistryFolder, &CLSID_RegistryFolder>,
    public CCommonFolder<CRegistryFolder, RegPidlEntry, CRegistryFolderExtractIcon>
{
    HKEY m_hRoot;

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRYFOLDER)

    CRegistryFolder();
    virtual ~CRegistryFolder();

    // IShellFolder
    STDMETHOD(EnumObjects)(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList) override;

protected:
    STDMETHOD(InternalBindToObject)(
        PWSTR path,
        const RegPidlEntry * info,
        LPITEMIDLIST first,
        LPCITEMIDLIST rest,
        LPITEMIDLIST fullPidl,
        LPBC pbcReserved,
        IShellFolder** ppsfChild) override;

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
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath, HKEY hRoot);

protected:
    STDMETHOD(CompareIDs)(LPARAM lParam, const RegPidlEntry * first, const RegPidlEntry * second);
    STDMETHOD_(ULONG, ConvertAttributes)(const RegPidlEntry * entry, PULONG inMask);
    STDMETHOD_(BOOL, IsFolder)(const RegPidlEntry * info);

    virtual HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const RegPidlEntry ** pentry);

    HRESULT FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents);

    HRESULT FormatContentsForDisplay(const RegPidlEntry * info, HKEY rootKey, LPCWSTR ntPath, PCWSTR * strContents);
};
