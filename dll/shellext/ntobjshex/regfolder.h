/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/regfolder.h
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
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

    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(
        UINT uFlags,
        LPWSTR szIconFile,
        UINT cchMax,
        INT *piIndex,
        UINT *pwFlags);

    virtual HRESULT STDMETHODCALLTYPE Extract(
        LPCWSTR pszFile,
        UINT nIconIndex,
        HICON *phiconLarge,
        HICON *phiconSmall,
        UINT nIconSize);

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
    virtual HRESULT STDMETHODCALLTYPE EnumObjects(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList);

protected:
    virtual HRESULT STDMETHODCALLTYPE InternalBindToObject(
        PWSTR path,
        const RegPidlEntry * info,
        LPITEMIDLIST first,
        LPCITEMIDLIST rest,
        LPITEMIDLIST fullPidl,
        LPBC pbcReserved,
        IShellFolder** ppsfChild);

public:
    virtual HRESULT STDMETHODCALLTYPE GetDefaultColumnState(
        UINT iColumn,
        SHCOLSTATEF *pcsFlags);

    virtual HRESULT STDMETHODCALLTYPE GetDetailsEx(
        LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid,
        VARIANT *pv);

    virtual HRESULT STDMETHODCALLTYPE GetDetailsOf(
        LPCITEMIDLIST pidl,
        UINT iColumn,
        SHELLDETAILS *psd);

    virtual HRESULT STDMETHODCALLTYPE MapColumnToSCID(
        UINT iColumn,
        SHCOLUMNID *pscid);

    // IPersistFolder
    virtual HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidl);

    // Internal
    virtual HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath, HKEY hRoot);

protected:
    virtual HRESULT STDMETHODCALLTYPE CompareIDs(LPARAM lParam, const RegPidlEntry * first, const RegPidlEntry * second);
    virtual ULONG STDMETHODCALLTYPE ConvertAttributes(const RegPidlEntry * entry, PULONG inMask);
    virtual BOOL STDMETHODCALLTYPE IsFolder(const RegPidlEntry * info);

    virtual HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const RegPidlEntry ** pentry);

    HRESULT FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents);

    HRESULT FormatContentsForDisplay(const RegPidlEntry * info, HKEY rootKey, LPCWSTR ntPath, PCWSTR * strContents);
};
