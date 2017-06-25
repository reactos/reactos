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

//Componenet Category Registration
    HRESULT RegisterComCat()
    {
        CComPtr<ICatRegister> pcr;
        HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
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
        HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
        if (SUCCEEDED(hr))
        {
            CATID catid = CATID_DeskBand;
            hr = pcr->UnRegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);            
        }
        return hr;
    }

//Pidl Browser
    LPITEMIDLIST PidlBrowse(HWND hwnd, int nCSIDL)
    {
        LPITEMIDLIST pidlRoot = NULL;
        LPITEMIDLIST pidlSelected = NULL;        
        WCHAR path[MAX_PATH];

        if (nCSIDL)
        {
            SHGetSpecialFolderLocation(hwnd, nCSIDL, &pidlRoot);
        }        

        BROWSEINFO bi = { hwnd, pidlRoot, path, L"Choose a folder", 0, NULL, 0, 0 };
        pidlSelected = SHBrowseForFolder(&bi);

        if (pidlRoot)
        {
            CoTaskMemFree(pidlRoot);
        }

        return pidlSelected;
    }

//CQuickLaunchBand

    CQuickLaunchBand::CQuickLaunchBand() {}

    CQuickLaunchBand::~CQuickLaunchBand() {}

/*****************************************************************************/
//ATL Construct

    HRESULT CQuickLaunchBand::FinalConstruct()
    {
        HRESULT hr = CISFBand_CreateInstance(IID_IUnknown, (void**) &m_punkISFB);
        if (SUCCEEDED(hr))
        {           
            CComPtr<IShellFolderBand> pISFB;
            hr = m_punkISFB->QueryInterface(IID_IShellFolderBand, (void**) &pISFB);
            if (SUCCEEDED(hr))
            {
                CComPtr<IShellFolder> pISF;
                hr = SHGetDesktopFolder(&pISF);
                if (SUCCEEDED(hr))
                {                    
                    LPITEMIDLIST pidl = PidlBrowse(m_hWndBro, CSIDL_DESKTOP); 
                    if (pidl == NULL) return E_FAIL;
                    pISFB->InitializeSFB(pISF, pidl);                    
                }                                               
            }            
        }       
        return hr;
    }

//IObjectWithSite
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::SetSite(IUnknown *pUnkSite)
    { 
        TRACE("CQuickLaunchBand::SetSite(0x%p)\n", pUnkSite);

        //Internal CISFBand Calls
        CComPtr<IObjectWithSite> pIOWS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IObjectWithSite, (void**)&pIOWS);
        if (FAILED(hr)) return hr; 

        return pIOWS->SetSite(pUnkSite);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {        
        TRACE("CQuickLaunchBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

        //Internal CISFBand Calls
        CComPtr<IObjectWithSite> pIOWS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IObjectWithSite, (void**)&pIOWS);
        if (FAILED(hr)) return hr;

        return pIOWS->GetSite(riid, ppvSite);
    }

/*****************************************************************************/
//IDeskBand
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetWindow(OUT HWND *phwnd)
    {
        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->GetWindow(phwnd);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->ContextSensitiveHelp(fEnterMode);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ShowDW(
        IN BOOL bShow)
    {        
        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->ShowDW(bShow);       
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::CloseDW(
        IN DWORD dwReserved)
    {        
        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->CloseDW(dwReserved);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {        
        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetBandInfo(  //Need a check
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {        
        TRACE("CQuickLaunchBand::GetBandInfo(0x%x,0x%x,0x%p)\n", dwBandID, dwViewMode, pdbi);

        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->GetBandInfo(dwBandID, dwViewMode, pdbi);
    }    

 /*****************************************************************************/
 //IDeskBar   
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::SetClient(
        IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);

        //Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBar, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->SetClient(punkClient);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetClient(
        OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);

        //Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBar, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->GetClient(ppunkClient);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::OnPosRectChangeDB(
        IN RECT *prc)
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);      

        //Internal CISFBand Calls
        CComPtr<IDeskBar> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBar, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->OnPosRectChangeDB(prc);
    }

/*****************************************************************************/
//IPersistStream
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetClassID(
        OUT CLSID *pClassID)
    {        
        TRACE("CQuickLaunchBand::GetClassID(0x%p)\n", pClassID);
        
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->GetClassID(pClassID);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsDirty()
    {        
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->IsDirty();
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Load(
        IN IStream *pStm)
    {
        TRACE("CQuickLaunchBand::Load called\n");
        
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->Load(pStm);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {       
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->Save(pStm, fClearDirty);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("CQuickLaunchBand::GetSizeMax called\n");        
       
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->GetSizeMax(pcbSize);
    }
    

/*****************************************************************************/
//IWinEventHandler
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        TRACE("CQuickLaunchBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
                
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContainsWindow(
        IN HWND hWnd)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {         
         //Internal CISFBand Calls
         CComPtr<IWinEventHandler> pWEH;
         HRESULT hr = m_punkISFB->QueryInterface(IID_IWinEventHandler, (void**)&pWEH);
         if (FAILED(hr)) return hr;

         return pWEH->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsWindowOwner(HWND hWnd)
    {        
        //Internal CISFBand Calls
        CComPtr<IWinEventHandler> pWEH;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IWinEventHandler, (void**)&pWEH);
        if (FAILED(hr)) return hr;

        return pWEH->IsWindowOwner(hWnd);
    }
    
/*****************************************************************************/
// *** IOleCommandTarget methods ***
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {        
        //Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IOleCommandTarget, (void**)&pOCT);
        if (FAILED(hr)) return hr;

        return pOCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {       
        //Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IOleCommandTarget, (void**)&pOCT);
        if (FAILED(hr)) return hr;

        return pOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }  

