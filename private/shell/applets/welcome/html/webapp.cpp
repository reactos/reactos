//////////////////////////////////////////////////////////////////////////
//
//  webapp.cpp
//
//      The purpose of this application is to demonstrate how a C++
//      application can host IE4, and manipulate the OC vtable.
//
//      This file contains the main entry point into the application and
//      the implementation of the CWebApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>    // for string compare functions
#include <htiframe.h>   // ITargetFrame2

#include "webapp.h"
#include "resource.h"
#include "crtfree.h"

// for CComVariant and ATL Conversion macros
#include <atlbase.h>
#include <atlconv.h>
#include <atlimpl.cpp>

#define WINDOW_CLASS    TEXT("_ms_WelcomeToNT5app_")

/**
*  This function is the main entry point into our application.
*
*  @return     int     Exit code.
*/
extern "C" int __stdcall main()
{
    {
        CWebApp webapp;

        OleInitialize(NULL);
        InitCommonControls();

        webapp.Register(GetModuleHandle(NULL));
        webapp.InitializeData();
        webapp.Create();
        webapp.MessageLoop();

        OleUninitialize();
    }
    ExitProcess(0);
    return 0;
}

/**
*  This method is our contstructor for our class. It initialize all
*  of the instance data.
*/
CWebApp::CWebApp()
{
    m_hInstance     = NULL;
    m_hwnd          = NULL;
    m_pContainer    = NULL;
    m_pweb          = NULL;
    m_eventCookie   = 0;
    m_pEvent        = new CEventSink(this);
    m_bVirgin = true;
    m_bUseDA = true;
}

/**
*  This method will register our window class for the application.
*
*  @param  hInstance   The application instance handle.
*
*  @return             No return value.
*/
void CWebApp::Register(HINSTANCE hInstance)
{
    WNDCLASSEX  wndclass;
    
    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_OWNDC | CS_SAVEBITS | CS_DBLCLKS;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBAPP));
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = WINDOW_CLASS;
    wndclass.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBAPP));
    
    RegisterClassEx(&wndclass);
    m_hInstance = hInstance;
}

/**
*  This method will initialize the data object.
*
*  @return         No return value.
*/
void CWebApp::InitializeData()
{
    HWND hwnd = GetDesktopWindow();
    HDC hdc = GetDC( hwnd );
    int iColors = GetDeviceCaps( hdc, NUMCOLORS );
    ReleaseDC( hwnd, hdc );
    bool bLowColor = ((iColors != -1) && (iColors < 256));

    if ( bLowColor || GetSystemMetrics(SM_SLOWMACHINE) || GetSystemMetrics(SM_REMOTESESSION) )
    {
        m_bUseDA = false;
    }

#ifndef USEDA
    // BUGBUG: hack until the DA bugs are fixed.  The ifndef allows this to be turned off by doing a
    // "set USEDA=1" in razzle before you build (very useful for testing).
    m_bUseDA = false;
#endif

    m_IniData.Init();
}

/**
*  This method will create the application window.
*
*  @return         No return value.
*/
void CWebApp::Create()
{
    //
    //  load the window title from the resource.
    //
    TCHAR szTitle[MAX_PATH];
    LoadString(m_hInstance, IDS_TITLE, szTitle, MAX_PATH);

    //
    //  set the client area to a fixed size and center the window on screen
    RECT rect = {0,0,478,322};  // desired client area (this is the spec'ed size)
    const DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER;
    
    // If HighContrast then add to height
    HIGHCONTRAST hc;
    hc.cbSize = sizeof(HIGHCONTRAST);
    hc.dwFlags = 0; // avoid random result should SPI fail

    SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0 );
    if ( hc.dwFlags & HCF_HIGHCONTRASTON )
    {
        // HighContrast mode is turned on.  This totally fucks our style sheet as most of it will
        // get ignored.  The best we can do is to resize our window so the gigantic fonts will
        // show correctly.
        rect.bottom = 450;
    }

    AdjustWindowRect( &rect, dwStyle, FALSE );
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = (GetSystemMetrics(SM_CXSCREEN)-rect.right)/2;
    rect.top = (GetSystemMetrics(SM_CYSCREEN)-rect.bottom)/2;

    m_hwnd = CreateWindow(WINDOW_CLASS,
        szTitle,
        dwStyle,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom,
        NULL,
        NULL,
        m_hInstance,
        this);
    
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/**
*  This method is our application message loop.
*
*  @return         No return value.
*/
void CWebApp::MessageLoop()
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if ( m_pContainer && S_OK == m_pContainer->TranslateAccelerator( &msg, WORD(0) ) )
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/**
*  This is the window procedure for the container application. It is used
*  to deal with all messages to our window.
*
*  @param      hwnd        Window handle.
*  @param      msg         The window message.
*  @param      wParam      Window Parameter.
*  @param      lParam      Window Parameter.
*
*  @return     LRESULT
*/
LRESULT CALLBACK CWebApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CWebApp *web = (CWebApp *)GetWindowLong(hwnd, GWL_USERDATA);
    LPCREATESTRUCT pcs;
    
    switch(msg)
    {
    case WM_NCCREATE:
        pcs = (LPCREATESTRUCT)lParam;
        web = (CWebApp *)pcs->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (long)web);
        break;
        
    case WM_CREATE:
        web->OnCreate(hwnd);
        break;
        
    case WM_DESTROY:
        web->OnDestroy();
        break;
        
    case WM_SIZE:
        web->OnSize(LOWORD(lParam), HIWORD(lParam));
        break;
        
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        web->OnKeyDown(msg, wParam, lParam);
        break;

    case WM_RBUTTONDOWN:
        return 0;

    case WM_SETFOCUS:
        web->OnSetFocus((HWND)wParam);
        break;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
*  This method is called on WM_CREATE.
*
*  @param  hwnd    Window handle for the application.
*
*  @return         No return value.
*/
void CWebApp::OnCreate(HWND hwnd)
{
    m_hwnd          = hwnd;
    m_pContainer    = new CContainer();
    
    if (m_pContainer)
    {
        m_pContainer->setParent(hwnd);
        m_pContainer->add(L"Shell.Explorer");
        m_pContainer->setVisible(TRUE);
        m_pContainer->setFocus(TRUE);
        
        //
        //  get the IWebBrowser2 interface and cache it.
        //
        IUnknown *punk = m_pContainer->getUnknown();
        if (punk)
        {
            HRESULT hr = punk->QueryInterface(IID_IWebBrowser2, (PVOID *)&m_pweb);
            punk->Release();
        }
        
        ConnectEvents();

#ifdef TEST_WITH_ABOUT_BLANK
        Navigate(L"about:blank");
#else
        if ( m_bUseDA )
        {
            Navigate(L"res://welcome.exe/welcome.htm");
        }
        else
        {
            Navigate(L"res://welcome.exe/noda.htm");
        }
#endif
    }
}

/**
*  This method handles the WM_DESTROY message.
*
*  @return         No return value.
*/
void CWebApp::OnDestroy()
{
    // remove ourselves from the per use run key if we need to
    SetRunState();

    SetWindowLong(m_hwnd, GWL_USERDATA, (long)0);
    
    //
    //  release our cached instance of the IWebBrowser2
    //  interface.
    //
    if (m_pweb)
        m_pweb->Release();
    
    //
    //  remove event connection.
    //
    DisconnectEvents();
    if (m_pEvent)
        m_pEvent->Release();
    
    //
    //  tell the container to remove IE4 and then
    //  release our reference to the container.
    //
    if (m_pContainer)
    {
        m_pContainer->remove();
        m_pContainer->Release();
    }
    
    PostQuitMessage(0);
}

/**
*  This method is called on WM_SIZE.
*
*  @param  width   The new width of the window.
*  @param  height  The new height of the window.
*
*  @return         No return value.
*/
void CWebApp::OnSize(int width, int height)
{
    if (m_pContainer)
        m_pContainer->setLocation(0, 0, width, height);
}

/**
*  This method handles the WM_KEYDOWN message.
*
*  @param  msg     The window message.
*  @param  wParam  The wParam of the message.
*  @param  lParam  The lParam of the message.
*
*  @return         No return value.
*/
void CWebApp::OnKeyDown(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!m_pContainer)
        return;
    
    MSG m;
    
    m.hwnd      = m_hwnd;
    m.message   = msg;
    m.wParam    = wParam;
    m.lParam    = lParam;
    
    m_pContainer->translateKey(m);
}

/**
*  This method handles the WM_SETFOCUS message.
*
*  @param  hwndLose The window that lost focus.
*
*  @return          No return value.
*/
void CWebApp::OnSetFocus(HWND hwndLose)
{
    if (m_pContainer)
        m_pContainer->setFocus(TRUE);
}


/**
*  This method will navigate to the specified URL.
*
*  @param  url     The url to navigate to.
*  
*  @return         No return value.
*/
void CWebApp::Navigate(BSTR url)
{
    if (!m_pweb)
        return;
    
    m_pweb->Navigate(url, NULL, NULL, NULL, NULL);
}

/**
*  This method finds a connection point for the desired interface.
*
*  @param  riid                interface id for the event interface
*
*  @return IConnectionPoint    the connection point.
*/
IConnectionPoint * CWebApp::GetConnectionPoint(REFIID riid)
{
    if (!m_pContainer)
        return NULL;
    
    IUnknown *punk = m_pContainer->getUnknown();
    if (!punk)
        return NULL;
    
    IConnectionPointContainer   *pcpc;
    IConnectionPoint            *pcp = NULL;
    
    HRESULT hr = punk->QueryInterface(IID_IConnectionPointContainer, (PVOID *)&pcpc);
    if (SUCCEEDED(hr))
    {
        pcpc->FindConnectionPoint(riid, &pcp);
        pcpc->Release();
    }
    
    punk->Release();
    return pcp;
}

/**
*  This method will wire the connection point to the advise.
*
*  @return         No return value.
*/
void CWebApp::ConnectEvents()
{
    if (!m_pContainer || !m_pEvent)
        return;
    
    IUnknown *punk = m_pContainer->getUnknown();
    if (!punk)
        return;
    
    IConnectionPoint *pcp;
    pcp = GetConnectionPoint(DIID_DWebBrowserEvents2);
    if (!pcp)
        return;
    
    pcp->Advise(m_pEvent, &m_eventCookie);
    pcp->Release();
    
    punk->Release();
}

/**
*  Removes the event wiring.
*
*  @return         No return value.
*/
void CWebApp::DisconnectEvents()
{
    IConnectionPoint    *pcp;
    
    pcp = GetConnectionPoint(DIID_DWebBrowserEvents2);
    if (!pcp)
        return;
    
    pcp->Unadvise(m_eventCookie);
    pcp->Release();
}

void CWebApp::eventBeforeNavigate2(VARIANT* URL, VARIANT_BOOL* Cancel)
{
    if ( VT_BSTR != URL->vt )
    {
        return;
    }

    TCHAR * szAppName = W2T(URL->bstrVal);

    if (szAppName && (0==StrCmpN(szAppName,_T("app://"),6)))
    {
        int iIndex = szAppName[6] - TEXT('0');

        m_IniData.UpdateAndRun( iIndex );

        *Cancel = -1;   // -1 == TRUE for VARIANT_BOOLs
    }
}

HRESULT CWebApp::SetBaseDirHack()
{
    HRESULT hr;
    LPDISPATCH lpdisp = NULL;

    hr = m_pweb->get_Document(&lpdisp);
    if (lpdisp)
    {
        IHTMLDocument2 * pDoc = NULL;

        hr = lpdisp->QueryInterface( IID_IHTMLDocument2, (void**)&pDoc );
        if (SUCCEEDED(hr))
        {
            IHTMLElement * pElement = NULL;
            hr = pDoc->get_body( &pElement );
            if (SUCCEEDED(hr))
            {
                TCHAR szWindir[MAX_PATH];
                int cch;
                cch = GetWindowsDirectory( szWindir, MAX_PATH );
                // handle the rare case where the %windir% is a root directory ("c:\")
                if ( cch && szWindir[--cch] == TEXT('\\') )
                {
                    szWindir[cch] = 0;
                }
                CComBSTR bstrAttrib = TEXT("basedir");
                CComVariant varValue(szWindir);
                pElement->setAttribute( bstrAttrib, varValue, -1 );
                pElement->Release();
            }
            pDoc->Release();
        }
        lpdisp->Release();
    }
    return hr;
}

void CWebApp::eventDocumentComplete(VARIANT* URL)
{
    const WCHAR szDestURL[] = L"res://welcome.exe/";
    const int cchDest = lstrlen( szDestURL );

    if ((URL->vt==VT_BSTR) && (0==StrCmpNW(URL->bstrVal,szDestURL,cchDest)) && m_bVirgin)
    {
        // place a bunch of these:
        // <div id=menuitem onmouseover="Select();" onclick="Click();"
        //     header="Registration Wizard"
        //     body="Register your copy of Windows NT Workstation 5.0 now.">
        //     imgindex="1"
        //     appindex="1">
        //   Registration Wizard
        // </div>
        // into the document

        IHTMLElement * pItem = NULL;

        SetBaseDirHack();

        // Walk object model to find "cmdlist"
        if (SUCCEEDED(FindIDInDoc( _T("cmdlist"), &pItem )))
        {
            int i;
            for ( i=0; i<m_IniData.m_iItems; i++ )
            {
                if ( m_IniData[i].completed )
                    continue;

                CComBSTR bstrItem;
                TCHAR szNum[2] = TEXT("0");
                bstrItem = "<div id=menuitem onfocus=\"Select();\" onmouseover=\"Select();\" onkeydown=\"KeyDown();\" onclick=\"Click();\" header=\"";
                bstrItem.Append( m_IniData[i].title );
                bstrItem.Append( "\" body=\"" );
                bstrItem.Append( m_IniData[i].description );
                bstrItem.Append( "\" imgindex=\"" );
                szNum[0] = (TCHAR)(m_IniData[i].imgindex+TEXT('0'));
                bstrItem.Append( szNum );
                bstrItem.Append( "\" appindex=\"" );
                szNum[0] = (TCHAR)(i+TEXT('0'));
                bstrItem.Append( szNum );
                bstrItem.Append( "\" tabindex=\"" );
                szNum[0] = (TCHAR)(i+TEXT('1'));
                bstrItem.Append( szNum );
                if ( m_IniData[i].flags & CIniData::WF_ALTERNATECOLOR )
                {
                    bstrItem.Append( "\" style=\"color: gray;");
                }
                bstrItem.Append( "\">" );
                bstrItem.Append( m_IniData[i].menuname?m_IniData[i].menuname:m_IniData[i].title );
                bstrItem.Append( "</div>" );

                pItem->insertAdjacentHTML( T2BSTR(_T("BeforeEnd")), bstrItem );
            }
            // Once we have inserted inner HTML we are no longer virgin
            m_bVirgin = false;
            pItem->Release();
        }
        if (SUCCEEDED(FindIDInDoc( _T("showmecheck"), &pItem )))
        {
            CComVariant varChecked(m_IniData.m_bCheckState);
            pItem->setAttribute( T2BSTR(_T("checked")), varChecked);
            pItem->Release();
        }
    }

    // Make sure the container sets the focus before we show the window
    m_pContainer->setFocus(TRUE);

    ShowWindow( m_hwnd, SW_SHOWNORMAL );
    UpdateWindow(m_hwnd);
}

HRESULT CWebApp::FindIDInDoc(LPCTSTR szItemName, IHTMLElement ** ppElement )
{
    HRESULT hr = E_FAIL;
    LPDISPATCH lpdisp = NULL;
    *ppElement = NULL;

    m_pweb->get_Document(&lpdisp);
    if (lpdisp)
    {
        IHTMLDocument2 * pDoc = NULL;

        hr = lpdisp->QueryInterface( IID_IHTMLDocument2, (void**)&pDoc );
        if (SUCCEEDED(hr))
        {
            IHTMLElementCollection * pCollection = NULL;
            hr = pDoc->get_all( &pCollection );
            if (SUCCEEDED(hr))
            {
                CComVariant varName  = szItemName;
                CComVariant varIndex(0, VT_I4);
                IDispatch * pDisp = NULL;
                hr = pCollection->item( varName, varIndex, &pDisp );
                if (SUCCEEDED(hr) && pDisp) // bug in IE4 allows pDips to be null even if S_OK is returned
                {
                    hr = pDisp->QueryInterface( IID_IHTMLElement, (void**)ppElement );
                    pDisp->Release();
                }
                else
                    hr = E_FAIL;

                pCollection->Release();
            }
            pDoc->Release();
        }
        lpdisp->Release();
    }
    return hr;
}

void CWebApp::SetRunState()
{
    DWORD bCheckPressed = TRUE;
    IHTMLElement * pCheckButton;
    if (SUCCEEDED(FindIDInDoc( _T("showmecheck"), &pCheckButton)))
    {
        CComVariant varChecked;
        if (SUCCEEDED(pCheckButton->getAttribute( T2BSTR(_T("checked")), 0, &varChecked)))
        {
            // getAttrib can return one of 3 possible types, BOOL, LONG, or BSTR.  In reality
            // "checked" should always be of type BOOL but I'm checking the other two types
            // anyway since changes to the JavaScript on the HTML page could cause a different
            // type to be returned.
            switch ( varChecked.vt )
            {
            case VT_BOOL:
            case VT_I4:
                // this will work for both bool and long since it's a union
                // NOTE: for VARIANT_BOOL's -1 is true and 0 is false.
                bCheckPressed = ( FALSE != varChecked.boolVal );
                break;
            case VT_BSTR:
                {
                    USES_CONVERSION;
                    if ( 0 != StrCmpI( W2T(varChecked.bstrVal), _T("false") ) )
                    {
                        bCheckPressed = FALSE;
                    }
                }
                break;
            }
        }
        pCheckButton->Release();
    }
    m_IniData.SetRunKey(bCheckPressed);
}
