/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * COPYRIGHT:   Copyright Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <shellutils.h>

#include "CQuickLaunchBand.h"

EXTERN_C
HRESULT WINAPI RSHELL_CISFBand_CreateInstance(REFIID riid, void** ppv);

// {260CB95D-4544-44F6-A079-575BAA60B72F}
EXTERN_C const GUID CLSID_QuickLaunchBand = { 0x260cb95d, 0x4544, 0x44f6, { 0xa0, 0x79, 0x57, 0x5b, 0xaa, 0x60, 0xb7, 0x2f } };

// Componenet Category Registration
EXTERN_C
HRESULT RegisterComCat(VOID)
{
    CComPtr<ICatRegister> pcr;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(ICatRegister, &pcr));
    if (SUCCEEDED(hr))
    {
        CATID catid = CATID_DeskBand;
        hr = pcr->RegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);
    }
    return hr;
}

EXTERN_C
HRESULT UnregisterComCat(VOID)
{
    CComPtr<ICatRegister> pcr;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(ICatRegister, &pcr));
    if (SUCCEEDED(hr))
    {
        CATID catid = CATID_DeskBand;
        hr = pcr->UnRegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);
    }
    return hr;
}

// Pidl Browser
/// Opens a folder browser dialog, allowing the user to select a folder for enumeration.
/// 
/// @param hwnd
///      A handle to browser dialog window.
/// @param nCSIDL
///      A CSIDL representing the root from which the browse folder dialog shows the files
///      and folders.
/// @return The PIDL to selected folder.
static LPITEMIDLIST PidlBrowse(HWND hwnd, INT nCSIDL)
{
    CComHeapPtr<ITEMIDLIST> pidlRoot;
    if (nCSIDL)
        SHGetSpecialFolderLocation(hwnd, nCSIDL, &pidlRoot);

    WCHAR path[MAX_PATH];
    BROWSEINFO bi = { hwnd, pidlRoot, path, L"Choose a folder", 0, NULL, 0, 0 };
    LPITEMIDLIST pidlSelected = SHBrowseForFolder(&bi);

    return pidlSelected;
}

// CQuickLaunchBand
CQuickLaunchBand::CQuickLaunchBand()
{
}

CQuickLaunchBand::~CQuickLaunchBand()
{
}

// ATL Construct
/// Creates an instance of CISFBand, and initializes its Shell Folder Band for enumeration.
/// @return The error code.
HRESULT CQuickLaunchBand::FinalConstruct()
{
    HRESULT hr = RSHELL_CISFBand_CreateInstance(IID_PPV_ARG(IUnknown, &m_punkISFB));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolderBand> pISFB;
    hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IShellFolderBand, &pISFB));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder> pISF;
    hr = SHGetDesktopFolder(&pISF);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST> pidl(PidlBrowse(m_hWndBro, CSIDL_DESKTOP));
    if (pidl == NULL)
        return E_FAIL;
    pISFB->InitializeSFB(pISF, pidl);

    return hr;
}

// IObjectWithSite

STDMETHODIMP CQuickLaunchBand::SetSite(IUnknown *pUnkSite)
{
    // Internal CISFBand Calls
    CComPtr<IObjectWithSite> pIOWS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pIOWS));
    if (FAILED(hr))
        return hr;

    return pIOWS->SetSite(pUnkSite);
}

STDMETHODIMP CQuickLaunchBand::GetSite(_In_ REFIID riid, _Out_ VOID **ppvSite)
{
    CComPtr<IObjectWithSite> pIOWS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pIOWS));
    if (FAILED(hr))
        return hr;

    return pIOWS->GetSite(riid, ppvSite);
}

// IDeskBand

STDMETHODIMP CQuickLaunchBand::GetWindow(_Out_ HWND *phwnd)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->GetWindow(phwnd);
}

STDMETHODIMP CQuickLaunchBand::ContextSensitiveHelp(_In_ BOOL fEnterMode)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->ContextSensitiveHelp(fEnterMode);
}

STDMETHODIMP CQuickLaunchBand::ShowDW(_In_ BOOL bShow)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->ShowDW(bShow);
}

STDMETHODIMP CQuickLaunchBand::CloseDW(_In_ DWORD dwReserved)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->CloseDW(dwReserved);
}

STDMETHODIMP
CQuickLaunchBand::ResizeBorderDW(
    _In_ LPCRECT prcBorder,
    _In_ IUnknown *punkToolbarSite,
    _In_ BOOL fReserved)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
}

STDMETHODIMP
CQuickLaunchBand::GetBandInfo(
    _In_ DWORD dwBandID,
    _In_ DWORD dwViewMode,
    _Inout_ DESKBANDINFO *pdbi)
{
    // Internal CISFBand Calls
    CComPtr<IDeskBand> pIDB;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
    if (FAILED(hr))
        return hr;

    return pIDB->GetBandInfo(dwBandID, dwViewMode, pdbi);
}

// IPersistStream

STDMETHODIMP CQuickLaunchBand::GetClassID(_Out_ CLSID *pClassID)
{
    // Internal CISFBand Calls
    CComPtr<IPersistStream> pIPS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
    if (FAILED(hr))
        return hr;

    return pIPS->GetClassID(pClassID);
}

STDMETHODIMP CQuickLaunchBand::IsDirty()
{
    // Internal CISFBand Calls
    CComPtr<IPersistStream> pIPS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
    if (FAILED(hr))
        return hr;

    return pIPS->IsDirty();
}

STDMETHODIMP CQuickLaunchBand::Load(_In_ IStream *pStm)
{
    CComPtr<IPersistStream> pIPS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
    if (FAILED(hr))
        return hr;

    return pIPS->Load(pStm);
}

STDMETHODIMP CQuickLaunchBand::Save(_In_ IStream *pStm, _In_ BOOL fClearDirty)
{
    // Internal CISFBand Calls
    CComPtr<IPersistStream> pIPS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
    if (FAILED(hr))
        return hr;

    return pIPS->Save(pStm, fClearDirty);
}

STDMETHODIMP CQuickLaunchBand::GetSizeMax(_Out_ ULARGE_INTEGER *pcbSize)
{
    CComPtr<IPersistStream> pIPS;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
    if (FAILED(hr))
        return hr;

    return pIPS->GetSizeMax(pcbSize);
}

// IWinEventHandler

STDMETHODIMP CQuickLaunchBand::ContainsWindow(_In_ HWND hWnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
     // Internal CISFBand Calls
     CComPtr<IWinEventHandler> pWEH;
     HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IWinEventHandler, &pWEH));
     if (FAILED(hr))
         return hr;

     return pWEH->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

STDMETHODIMP CQuickLaunchBand::IsWindowOwner(HWND hWnd)
{
    // Internal CISFBand Calls
    CComPtr<IWinEventHandler> pWEH;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IWinEventHandler, &pWEH));
    if (FAILED(hr))
        return hr;

    return pWEH->IsWindowOwner(hWnd);
}

/*****************************************************************************/
// IOleCommandTarget methods

STDMETHODIMP
CQuickLaunchBand::QueryStatus(
    _In_ const GUID *pguidCmdGroup,
    _In_ ULONG cCmds,
    _Inout_ OLECMD prgCmds[],
    _Inout_ OLECMDTEXT *pCmdText)
{
    // Internal CISFBand Calls
    CComPtr<IOleCommandTarget> pOCT;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
    if (FAILED(hr))
        return hr;

    return pOCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

STDMETHODIMP
CQuickLaunchBand::Exec(
    _In_ const GUID *pguidCmdGroup,
    _In_ DWORD nCmdID,
    _In_ DWORD nCmdexecopt,
    _In_ VARIANT *pvaIn,
    _Inout_ VARIANT *pvaOut)
{
    // Internal CISFBand Calls
    CComPtr<IOleCommandTarget> pOCT;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
    if (FAILED(hr))
        return hr;

    return pOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
}

// IContextMenu

STDMETHODIMP
CQuickLaunchBand::GetCommandString(
    _In_ UINT_PTR idCmd,
    _In_ UINT uFlags,
    _In_ UINT *pwReserved,
    _Out_ LPSTR pszName,
    _In_ UINT cchMax)
{
    // Internal CISFBand Calls
    CComPtr<IContextMenu> pICM;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
    if (FAILED(hr))
        return hr;

    return pICM->GetCommandString(idCmd, uFlags, pwReserved, pszName, cchMax);
}

STDMETHODIMP CQuickLaunchBand::InvokeCommand(_In_ LPCMINVOKECOMMANDINFO pici)
{
    // Internal CISFBand Calls
    CComPtr<IContextMenu> pICM;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
    if (FAILED(hr))
        return hr;

    return pICM->InvokeCommand(pici);
}

STDMETHODIMP
CQuickLaunchBand::QueryContextMenu(
    _Out_ HMENU hmenu,
    _In_ UINT indexMenu,
    _In_ UINT idCmdFirst,
    _In_ UINT idCmdLast,
    _In_ UINT uFlags)
{
    // Internal CISFBand Calls
    CComPtr<IContextMenu> pICM;
    HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
    if (FAILED(hr))
        return hr;

    return pICM->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
}
