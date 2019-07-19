#pragma once

#include "shellfind.h"

#define SWM_ADD_ITEM (WM_USER + 0)
#define SWM_UPDATE_STATUS (WM_USER + 1)

class CFindFolder :
        public CComCoClass<CFindFolder, &CLSID_FindFolder>,
        public CComObjectRootEx<CComMultiThreadModelNoCS>,
        public IShellFolder2,
        public IPersistFolder2
{
    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(GUID *pguid);

    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);

    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);

    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags);

    STDMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);

    STDMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *pDetails);

    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);


    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten,
                                  PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes);

    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList);

    STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);

    STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);

    STDMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);

    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut);

    STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut);

    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *prgfInOut,
                               LPVOID *ppvOut);

    STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName);

    STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags,
                           PITEMID_CHILD *pPidlOut);

private:
    LPITEMIDLIST m_pidl;

    //// *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *pidl);


    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);


    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *pClassId);

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_FINDFOLDER)

    DECLARE_NOT_AGGREGATABLE(CFindFolder)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFindFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    END_COM_MAP()
};

