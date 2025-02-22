/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search results folder
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#pragma once

#include "shellfind.h"

class CFindFolder :
        public CWindowImpl<CFindFolder>,
        public CComCoClass<CFindFolder, &CLSID_FindFolder>,
        public CComObjectRootEx<CComMultiThreadModelNoCS>,
        public IShellFolder2,
        public IPersistFolder2,
        public IShellFolderViewCB,
        public IConnectionPointContainerImpl<CFindFolder>,
        public IConnectionPointImpl<CFindFolder, &DIID_DSearchCommandEvents>
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

    //// *** IShellFolderViewCB methods ***
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    LPITEMIDLIST m_pidl;
    CComPtr<IShellFolder2> m_pisfInner;
    CComPtr<IShellFolderView> m_shellFolderView;
    CComPtr<IShellBrowser> m_shellBrowser;
    HANDLE m_hStopEvent;

    void NotifyConnections(DISPID id);
    static DWORD WINAPI SearchThreadProc(LPVOID lpParameter);

    //// *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE *pidl);


    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl);


    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *pClassId);

    // *** Message handlers ***
    LRESULT StartSearch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    LRESULT StopSearch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    LRESULT AddResult(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    LRESULT UpdateStatus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

public:
    CFindFolder();

    DECLARE_REGISTRY_RESOURCEID(IDR_FINDFOLDER)

    DECLARE_NOT_AGGREGATABLE(CFindFolder)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_CONNECTION_POINT_MAP(CFindFolder)
        CONNECTION_POINT_ENTRY(DIID_DSearchCommandEvents)
    END_CONNECTION_POINT_MAP()

    BEGIN_MSG_MAP(CFindFolder)
        MESSAGE_HANDLER(WM_SEARCH_START, StartSearch)
        MESSAGE_HANDLER(WM_SEARCH_STOP, StopSearch)
        MESSAGE_HANDLER(WM_SEARCH_ADD_RESULT, AddResult)
        MESSAGE_HANDLER(WM_SEARCH_UPDATE_STATUS, UpdateStatus)
    END_MSG_MAP()

    BEGIN_COM_MAP(CFindFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IConnectionPointContainer, IConnectionPointContainer)
    END_COM_MAP()
};

