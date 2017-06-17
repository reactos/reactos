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

//CQuickLaunchBand

    CQuickLaunchBand::CQuickLaunchBand() : m_hWnd(NULL), m_BandID(0) {}

    CQuickLaunchBand::~CQuickLaunchBand() {}

/*****************************************************************************/
//ATL Construct

    HRESULT CQuickLaunchBand::FinalConstruct()
    {
        //MessageBox(0, L"CQuickLaunchBand::FinalConstruct Begin!", L"Testing", MB_OK | MB_ICONINFORMATION);

        HRESULT hr = CoCreateInstance(CLSID_ISFBand, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &m_punkISFB);
        if (SUCCEEDED(hr))
        {
            //MessageBox(0, L"CoCreateInstance success!", L"Testing", MB_OK | MB_ICONINFORMATION);

            CComPtr<IShellFolderBand> pISFB;
            hr = m_punkISFB->QueryInterface(IID_IShellFolderBand, (void**) &pISFB);
            if (SUCCEEDED(hr))
            {
               // MessageBox(0, L"IID_ISFBand query success!", L"Testing", MB_OK | MB_ICONINFORMATION);

                CComPtr<IShellFolder> pISF;
                hr = SHGetDesktopFolder(&pISF);
                if (SUCCEEDED(hr))
                {
                    //MessageBox(0, L"pisf success!", L"Testing", MB_OK | MB_ICONINFORMATION);

                    //LPITEMIDLIST pidl;
                    //hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
                    //if (SUCCEEDED(hr))
                    //{
                        //MessageBox(0, L"pidl success!", L"Testing", MB_OK | MB_ICONINFORMATION);

                        pISFB->InitializeSFB(pISF, 0);
                    //}
                }                                               
            }            
        }
        
       // MessageBox(0, L"CQuickLaunchBand::FinalConstruct End!", L"Testing", MB_OK | MB_ICONINFORMATION);
        return hr;
    }

//IObjectWithSite
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::SetSite(IUnknown *pUnkSite)
    {
        MessageBox(0, L"CQuickLaunchBand::SetSite called!", L"Testing", MB_OK | MB_ICONINFORMATION);

       /* HRESULT hRet;
        HWND hwndParent;

        TRACE("CQuickLaunchBand::SetSite(0x%p)\n", pUnkSite);

        hRet = IUnknown_GetWindow(pUnkSite, &hwndParent);
        if (FAILED(hRet))
        {
            TRACE("Querying site window failed: 0x%x\n", hRet);
            return hRet;
        }
        m_Site = pUnkSite;        
        m_hWnd = CreateWindowEx(0, L"BUTTON", L">>", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, 50, 50, hwndParent, 0, m_hInstance, 0);
        SetWindowSubclass(hwndParent, MyWndProc, 0, 0); //when button is clicked, parent receives WM_COMMAND, and thus subclassed to show a test message box
        */
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
        //MessageBox(0, L"GetSite called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        TRACE("CQuickLaunchBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

       /* HRESULT hr;
        if (m_Site != NULL)
        {
            hr = m_Site->QueryInterface(riid, ppvSite);
            if (FAILED(hr)) return hr;
        }*/

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
        //MessageBox(0, L"GetWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        /*if (!m_hWnd)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;*/

        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->GetWindow(phwnd);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */

        //MessageBox(0, L"ContextSensitiveHelp called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->ContextSensitiveHelp(fEnterMode);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ShowDW(
        IN BOOL bShow)
    {
        //MessageBox(0, L"ShowDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        /*if (m_hWnd)
        {
            ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        }*/

        //Internal CISFBand Calls
        CComPtr<IDeskBand> pIDB;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IDeskBand, (void**)&pIDB);
        if (FAILED(hr)) return hr;

        return pIDB->ShowDW(bShow);       
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::CloseDW(
        IN DWORD dwReserved)
    {
        //MessageBox(0, L"CloseDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        /*if (m_hWnd)
        {
            ShowWindow(m_hWnd, SW_HIDE);
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }*/

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
        /* No need to implement this method */

        //MessageBox(0, L"ResizeBorderDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

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
        //MessageBox(0, L"GetBandInfo called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);

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

        //MessageBox(0, L"SetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

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

        //MessageBox(0, L"GetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
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

        //MessageBox(0, L"OnPosRectChangeDB called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
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
        //MessageBox(0, L"GetClassID called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        TRACE("CQuickLaunchBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the quick launch band */
        /* *pClassID = CLSID_QuickLaunchBand; */
        
        //Internal CISFBand Calls
        CComPtr<IPersistStream> pIPS;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IPersistStream, (void**)&pIPS);
        if (FAILED(hr)) return hr;

        return pIPS->GetClassID(pClassID);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsDirty()
    {
        /* The object hasn't changed since the last save! */

        //MessageBox(0, L"IsDirty called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

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
        /* Nothing to do */

        //MessageBox(0, L"Load called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
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
        /* Nothing to do */

        //MessageBox(0, L"Save called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

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
        //pcbSize->QuadPart = 0;

        //MessageBox(0, L"GetSizeMax called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

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
        
        //MessageBox(0, L"ProcessMessage called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContainsWindow(
        IN HWND hWnd)
    {
        //MessageBox(0, L"ContainsWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        /*if (hWnd == m_hWnd ||
            IsChild(m_hWnd, hWnd))
        {
            TRACE("CQuickLaunchBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }*/

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {        
        //MessageBox(0, L"OnWinEvent called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
         //UNIMPLEMENTED;
        
         //Internal CISFBand Calls
         CComPtr<IWinEventHandler> pWEH;
         HRESULT hr = m_punkISFB->QueryInterface(IID_IWinEventHandler, (void**)&pWEH);
         if (FAILED(hr)) return hr;

         return pWEH->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsWindowOwner(HWND hWnd)
    {
        //MessageBox(0, L"IsWindowOwner called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //return (hWnd == m_hWnd) ? S_OK : S_FALSE;

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
        //MessageBox(0, L"QueryStatus called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //UNIMPLEMENTED;

        //Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IOleCommandTarget, (void**)&pOCT);
        if (FAILED(hr)) return hr;

        return pOCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        /*if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        {
            return S_OK;
        }

        if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
        {
            return S_OK;
        }*/

        //MessageBox(0, L"Exec called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        //Internal CISFBand Calls
        CComPtr<IOleCommandTarget> pOCT;
        HRESULT hr = m_punkISFB->QueryInterface(IID_IOleCommandTarget, (void**)&pOCT);
        if (FAILED(hr)) return hr;

        return pOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }  

