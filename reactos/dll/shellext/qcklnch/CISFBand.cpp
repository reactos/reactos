/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CISFBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

//Subclassing Button

    LRESULT CALLBACK MyWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                MessageBox(0, L"Button Clicked!!", L"Testing", MB_OK | MB_ICONINFORMATION);
            }            
        } 
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

//CISFBand

    CISFBand::CISFBand() : m_hWnd(NULL), m_BandID(0) {}

    CISFBand::~CISFBand() {}

/*****************************************************************************/

//IObjectWithSite
    HRESULT STDMETHODCALLTYPE CISFBand::SetSite(IUnknown *pUnkSite)
    {
        //MessageBox(0, L"CISFBand::SetSite called!", L"Testing", MB_OK | MB_ICONINFORMATION);

       /* HRESULT hRet;
        HWND hwndParent;

        TRACE("CISFBand::SetSite(0x%p)\n", pUnkSite);

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

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {
        //MessageBox(0, L"GetSite called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //TRACE("CISFBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

       /* HRESULT hr;
        if (m_Site != NULL)
        {
            hr = m_Site->QueryInterface(riid, ppvSite);
            if (FAILED(hr)) return hr;
        }*/        

        return E_NOTIMPL;
    }

/*****************************************************************************/
//IDeskBand
    HRESULT STDMETHODCALLTYPE CISFBand::GetWindow(OUT HWND *phwnd)
    {
        //MessageBox(0, L"GetWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        /*if (!m_hWnd)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;*/        

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */

        //MessageBox(0, L"ContextSensitiveHelp called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ShowDW(
        IN BOOL bShow)
    {
        //MessageBox(0, L"ShowDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        /*if (m_hWnd)
        {
            ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        }*/
        
        return E_NOTIMPL;       
    }

    HRESULT STDMETHODCALLTYPE CISFBand::CloseDW(
        IN DWORD dwReserved)
    {
        //MessageBox(0, L"CloseDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        /*if (m_hWnd)
        {
            ShowWindow(m_hWnd, SW_HIDE);
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }*/

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {
        /* No need to implement this method */

        //MessageBox(0, L"ResizeBorderDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetBandInfo(  //Need a check
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {
        //MessageBox(0, L"GetBandInfo called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        //TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);

        return E_NOTIMPL;
    }    

 /*****************************************************************************/
 //IDeskBar   
    HRESULT STDMETHODCALLTYPE CISFBand::SetClient(
        IN IUnknown *punkClient)
    {
        //TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);

        //MessageBox(0, L"SetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetClient(
        OUT IUnknown **ppunkClient)
    {
        //TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);

        //MessageBox(0, L"GetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnPosRectChangeDB(
        IN RECT *prc)
    {
        //TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);      

        //MessageBox(0, L"OnPosRectChangeDB called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

/*****************************************************************************/
//IPersistStream
    HRESULT STDMETHODCALLTYPE CISFBand::GetClassID(
        OUT CLSID *pClassID)
    {
        //MessageBox(0, L"GetClassID called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //TRACE("CISFBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the quick launch band */
        /* *pClassID = CLSID_QuickLaunchBand; */        

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsDirty()
    {
        /* The object hasn't changed since the last save! */

        //MessageBox(0, L"IsDirty called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
       
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Load(
        IN IStream *pStm)
    {
        //TRACE("CISFBand::Load called\n");
        /* Nothing to do */

        //MessageBox(0, L"Load called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {
        /* Nothing to do */

        //MessageBox(0, L"Save called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        //TRACE("CISFBand::GetSizeMax called\n");        
        //pcbSize->QuadPart = 0;

        //MessageBox(0, L"GetSizeMax called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return E_NOTIMPL;
    }
    

/*****************************************************************************/
//IWinEventHandler
    HRESULT STDMETHODCALLTYPE CISFBand::ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        //TRACE("CISFBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
        
        //MessageBox(0, L"ProcessMessage called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContainsWindow(
        IN HWND hWnd)
    {
        //MessageBox(0, L"ContainsWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        /*if (hWnd == m_hWnd ||
            IsChild(m_hWnd, hWnd))
        {
            TRACE("CISFBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }*/

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {        
        //MessageBox(0, L"OnWinEvent called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
         //UNIMPLEMENTED;
                 
         return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsWindowOwner(HWND hWnd)
    {
        //MessageBox(0, L"IsWindowOwner called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //return (hWnd == m_hWnd) ? S_OK : S_FALSE;

        return E_NOTIMPL;
    }
    
/*****************************************************************************/
// *** IOleCommandTarget methods ***
    HRESULT STDMETHODCALLTYPE CISFBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {
        //MessageBox(0, L"QueryStatus called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        //UNIMPLEMENTED;

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
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
        
        return E_NOTIMPL;
    }  

