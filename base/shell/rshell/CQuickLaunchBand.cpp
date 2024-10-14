/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CQuickLaunchBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
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

extern "C"
HRESULT WINAPI RSHELL_CISFBand_CreateInstance(REFIID riid, void** ppv);

// {260CB95D-4544-44F6-A079-575BAA60B72F}
const GUID CLSID_QuickLaunchBand = { 0x260cb95d, 0x4544, 0x44f6, { 0xa0, 0x79, 0x57, 0x5b, 0xaa, 0x60, 0xb7, 0x2f } };

// Componenet Category Registration
    HRESULT RegisterComCat()
    {
        CComPtr<ICatRegister> pcr;
        HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICatRegister, &pcr));
        if (SUCCEEDED(hr))
        {
            CATID catid = CATID_DeskBand;
            hr = pcr->RegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);
        }
        return hr;
    }

    HRESULT UnregisterComCat()
    {
        CComPtr<ICatRegister> pcr;
        HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICatRegister, &pcr));
        if (SUCCEEDED(hr))
        {
            CATID catid = CATID_DeskBand;
            hr = pcr->UnRegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);
        }
        return hr;
    }

// Pidl Browser
/*++
* @name PidlBrowse
*
* Opens a folder browser dialog,
* allowing the user to select a folder for enumeration.
*
* @param hwnd
*        A handle to browser dialog window.
* @param nCSIDL
*        A CSIDL representing the root from which the browse folder dialog shows the files and folders.
*
* @return The PIDL to selected folder.
*
*--*/
    LPITEMIDLIST PidlBrowse(HWND hwnd, int nCSIDL)
    {
        CComHeapPtr<ITEMIDLIST> pidlRoot;

        WCHAR path[MAX_PATH];

        if (nCSIDL)
        {
            SHGetSpecialFolderLocation(hwnd, nCSIDL, &pidlRoot);
        }

        BROWSEINFO bi = { hwnd, pidlRoot, path, L"Choose a folder", 0, NULL, 0, 0 };
        LPITEMIDLIST pidlSelected = SHBrowseForFolder(&bi);

        return pidlSelected;
    }

// CQuickLaunchBand

    CQuickLaunchBand::CQuickLaunchBand() {}

    CQuickLaunchBand::~CQuickLaunchBand() {}

/*****************************************************************************/
// ATL Construct
/*++
* @name FinalConstruct
*
* Creates an instance of CISFBand, and initializes its Shell Folder Band for enumeration.
*
* @return The error code.
*
*--*/
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

    STDMETHODIMP CQuickLaunchBand::GetSite(IN REFIID riid, OUT VOID **ppvSite)
    {
        CComPtr<IObjectWithSite> pIOWS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pIOWS));
        if (FAILED(hr))
            return hr;

        return pIOWS->GetSite(riid, ppvSite);
    }

/*****************************************************************************/
// IDeskBand
    STDMETHODIMP CQuickLaunchBand::GetWindow(OUT HWND *phwnd)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->GetWindow(phwnd);
    }

    STDMETHODIMP CQuickLaunchBand::ContextSensitiveHelp(IN BOOL fEnterMode)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->ContextSensitiveHelp(fEnterMode);
    }

    STDMETHODIMP CQuickLaunchBand::ShowDW(IN BOOL bShow)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->ShowDW(bShow);
    }

    STDMETHODIMP CQuickLaunchBand::CloseDW(IN DWORD dwReserved)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->CloseDW(dwReserved);
    }

    STDMETHODIMP CQuickLaunchBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    }

    STDMETHODIMP CQuickLaunchBand::GetBandInfo(IN DWORD dwBandID, IN DWORD dwViewMode, IN OUT DESKBANDINFO *pdbi)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr))
            return hr;

        return pIDB->GetBandInfo(dwBandID, dwViewMode, pdbi);
    }

/*****************************************************************************/
// IPersistStream
    STDMETHODIMP CQuickLaunchBand::GetClassID(OUT CLSID *pClassID)
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

    STDMETHODIMP CQuickLaunchBand::Load(IN IStream *pStm)
    {
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr))
            return hr;

        return pIPS->Load(pStm);
    }

    STDMETHODIMP CQuickLaunchBand::Save(IN IStream *pStm, IN BOOL fClearDirty)
    {
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr))
            return hr;

        return pIPS->Save(pStm, fClearDirty);
    }

    STDMETHODIMP CQuickLaunchBand::GetSizeMax(OUT ULARGE_INTEGER *pcbSize)
    {
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr))
            return hr;

        return pIPS->GetSizeMax(pcbSize);
    }


/*****************************************************************************/
// IWinEventHandler
    STDMETHODIMP CQuickLaunchBand::ContainsWindow(IN HWND hWnd)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
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
// *** IOleCommandTarget methods ***
    STDMETHODIMP CQuickLaunchBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {
        // Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
        if (FAILED(hr))
            return hr;

        return pOCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    STDMETHODIMP CQuickLaunchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        // Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
        if (FAILED(hr))
            return hr;

        return pOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }

/*****************************************************************************/
// *** IContextMenu ***
    STDMETHODIMP CQuickLaunchBand::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr))
            return hr;

        return pICM->GetCommandString(idCmd, uFlags, pwReserved, pszName, cchMax);
    }

    STDMETHODIMP CQuickLaunchBand::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr))
            return hr;

        return pICM->InvokeCommand(pici);
    }

    STDMETHODIMP CQuickLaunchBand::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr))
            return hr;

        return pICM->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }