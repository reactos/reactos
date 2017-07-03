/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CQuickLaunchBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

// {260CB95D-4544-44F6-A079-575BAA60B72F}
static const GUID CLSID_QuickLaunchBand = { 0x260cb95d, 0x4544, 0x44f6, { 0xa0, 0x79, 0x57, 0x5b, 0xaa, 0x60, 0xb7, 0x2f } };

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

    HRESULT CQuickLaunchBand::FinalConstruct()
    {
        HRESULT hr = CISFBand_CreateInstance(IID_PPV_ARG(IUnknown, &m_punkISFB));
        if (FAILED_UNEXPECTEDLY(hr)) return hr;

        CComPtr<IShellFolderBand> pISFB;
        hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IShellFolderBand, &pISFB));
        if (FAILED_UNEXPECTEDLY(hr)) return hr;

        CComPtr<IShellFolder> pISF;
        hr = SHGetDesktopFolder(&pISF);
        if (FAILED_UNEXPECTEDLY(hr)) return hr;

        CComHeapPtr<ITEMIDLIST> pidl(PidlBrowse(m_hWndBro, CSIDL_DESKTOP));
        if (pidl == NULL) return E_FAIL;
        pISFB->InitializeSFB(pISF, pidl);
        
        return hr;
    }

// IObjectWithSite
    STDMETHODIMP CQuickLaunchBand::SetSite(IUnknown *pUnkSite)
    { 
        TRACE("CQuickLaunchBand::SetSite(0x%p)\n", pUnkSite);

        // Internal CISFBand Calls
        CComPtr<IObjectWithSite> pIOWS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pIOWS));
        if (FAILED(hr)) return hr; 

        return pIOWS->SetSite(pUnkSite);
    }

    STDMETHODIMP CQuickLaunchBand::GetSite(IN REFIID riid, OUT VOID **ppvSite)
    {        
        TRACE("CQuickLaunchBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

        // Internal CISFBand Calls
        CComPtr<IObjectWithSite> pIOWS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pIOWS));
        if (FAILED(hr)) return hr;

        return pIOWS->GetSite(riid, ppvSite);
    }

/*****************************************************************************/
// IDeskBand
    STDMETHODIMP CQuickLaunchBand::GetWindow(OUT HWND *phwnd)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->GetWindow(phwnd);
    }

    STDMETHODIMP CQuickLaunchBand::ContextSensitiveHelp(IN BOOL fEnterMode)
    {
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->ContextSensitiveHelp(fEnterMode);
    }

    STDMETHODIMP CQuickLaunchBand::ShowDW(IN BOOL bShow)
    {        
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->ShowDW(bShow);       
    }

    STDMETHODIMP CQuickLaunchBand::CloseDW(IN DWORD dwReserved)
    {        
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->CloseDW(dwReserved);
    }

    STDMETHODIMP CQuickLaunchBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) 
    {        
        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    }

    STDMETHODIMP CQuickLaunchBand::GetBandInfo(IN DWORD dwBandID, IN DWORD dwViewMode, IN OUT DESKBANDINFO *pdbi)
    {        
        TRACE("CQuickLaunchBand::GetBandInfo(0x%x,0x%x,0x%p)\n", dwBandID, dwViewMode, pdbi);

        // Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBand, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->GetBandInfo(dwBandID, dwViewMode, pdbi);
    }    

 /*****************************************************************************/
 // IDeskBar   
    STDMETHODIMP CQuickLaunchBand::SetClient(IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);

        // Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBar, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->SetClient(punkClient);
    }

    STDMETHODIMP CQuickLaunchBand::GetClient(OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);

        // Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBar, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->GetClient(ppunkClient);
    }

    STDMETHODIMP CQuickLaunchBand::OnPosRectChangeDB(IN RECT *prc)
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);      

        // Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IDeskBar, &pIDB));
        if (FAILED(hr)) return hr;

        return pIDB->OnPosRectChangeDB(prc);
    }

/*****************************************************************************/
// IPersistStream
    STDMETHODIMP CQuickLaunchBand::GetClassID(OUT CLSID *pClassID)
    {        
        TRACE("CQuickLaunchBand::GetClassID(0x%p)\n", pClassID);
        
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr)) return hr;

        return pIPS->GetClassID(pClassID);
    }

    STDMETHODIMP CQuickLaunchBand::IsDirty()
    {        
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr)) return hr;

        return pIPS->IsDirty();
    }

    STDMETHODIMP CQuickLaunchBand::Load(IN IStream *pStm)
    {
        TRACE("CQuickLaunchBand::Load called\n");
        
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr)) return hr;

        return pIPS->Load(pStm);
    }

    STDMETHODIMP CQuickLaunchBand::Save(IN IStream *pStm, IN BOOL fClearDirty)
    {       
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr)) return hr;

        return pIPS->Save(pStm, fClearDirty);
    }

    STDMETHODIMP CQuickLaunchBand::GetSizeMax(OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("CQuickLaunchBand::GetSizeMax called\n");        
       
        // Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IPersistStream, &pIPS));
        if (FAILED(hr)) return hr;

        return pIPS->GetSizeMax(pcbSize);
    }
    

/*****************************************************************************/
// IWinEventHandler
    STDMETHODIMP CQuickLaunchBand::ProcessMessage(IN HWND hWnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam, OUT LRESULT *plrResult)
    {
        TRACE("CQuickLaunchBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
                
        return E_NOTIMPL;
    }

    STDMETHODIMP CQuickLaunchBand::ContainsWindow(IN HWND hWnd)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {         
         // Internal CISFBand Calls
         CComPtr<IWinEventHandler> pWEH;
         HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IWinEventHandler, &pWEH));
         if (FAILED(hr)) return hr;

         return pWEH->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    }

    STDMETHODIMP CQuickLaunchBand::IsWindowOwner(HWND hWnd)
    {        
        // Internal CISFBand Calls
        CComPtr<IWinEventHandler> pWEH;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IWinEventHandler, &pWEH));
        if (FAILED(hr)) return hr;

        return pWEH->IsWindowOwner(hWnd);
    }
    
/*****************************************************************************/
// *** IOleCommandTarget methods ***
    STDMETHODIMP CQuickLaunchBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {        
        // Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
        if (FAILED(hr)) return hr;

        return pOCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    STDMETHODIMP CQuickLaunchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {       
        // Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &pOCT));
        if (FAILED(hr)) return hr;

        return pOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }  

/*****************************************************************************/
// *** IContextMenu ***
    STDMETHODIMP CQuickLaunchBand::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr)) return hr;

        return pICM->GetCommandString(idCmd, uFlags, pwReserved, pszName, cchMax);
    }

    STDMETHODIMP CQuickLaunchBand::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr)) return hr;

        return pICM->InvokeCommand(pici);
    }

    STDMETHODIMP CQuickLaunchBand::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        // Internal CISFBand Calls
        CComPtr<IContextMenu> pICM;
        HRESULT hr = m_punkISFB->QueryInterface(IID_PPV_ARG(IContextMenu, &pICM));
        if (FAILED(hr)) return hr;

        return pICM->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }