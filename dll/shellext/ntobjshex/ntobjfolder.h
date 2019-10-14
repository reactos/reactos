/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/ntobjfolder.h
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
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

    virtual HRESULT STDMETHODCALLTYPE EnumObjects(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList);

protected:
    virtual HRESULT STDMETHODCALLTYPE InternalBindToObject(
        PWSTR path,
        const NtPidlEntry * info,
        LPITEMIDLIST first,
        LPCITEMIDLIST rest,
        LPITEMIDLIST fullPidl,
        LPBC pbcReserved,
        IShellFolder** ppsfChild);

    virtual HRESULT STDMETHODCALLTYPE ResolveSymLink(
        const NtPidlEntry * info,
        LPITEMIDLIST * fullPidl);

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
    HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath);

protected:
    virtual HRESULT STDMETHODCALLTYPE CompareIDs(LPARAM lParam, const NtPidlEntry * first, const NtPidlEntry * second);
    virtual ULONG STDMETHODCALLTYPE ConvertAttributes(const NtPidlEntry * entry, PULONG inMask);
    virtual BOOL STDMETHODCALLTYPE IsFolder(const NtPidlEntry * info);
    virtual BOOL STDMETHODCALLTYPE IsSymLink(const NtPidlEntry * info);

    virtual HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const NtPidlEntry ** pentry);

    HRESULT FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents);

    HRESULT FormatContentsForDisplay(const NtPidlEntry * info, HKEY rootKey, LPCWSTR ntPath, PCWSTR * strContents);
};