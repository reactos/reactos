//////////////////////////////////////////////////////////////////////////
//
//  webapp.cpp
//
//      The purpose of this application is to demonstrate how a C++
//      application can host IE4, and manipulate the OC vtable.
//
//      This file contains the implementation of the CWebApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>    // for path, url, and string functions
#include <shlwapip.h>   // for SHTCharToUnicode

#include "webapp.h"
#include "resource.h"
#include "crtfree.h"

#define WINDOW_CLASS    TEXT("BerksDiscoverTour")
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

//////////////////////////////////////////////////////////////////////////
// Global Data
//////////////////////////////////////////////////////////////////////////
extern TCHAR g_szPath[MAX_PATH];         // The tour location.  Ex. "e:\discover\default.htm" or "\\ntbuilds\...\discover\default.htm"


//////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Source
//////////////////////////////////////////////////////////////////////////

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
}

/**
*  This method will register our window class for the application.
*
*  @param  hInstance   The application instance handle.
*
*  @return             TRUE on success, FALSE otherwise.
*/
BOOL CWebApp::Register(HINSTANCE hInstance)
{
    WNDCLASSEX  wndclass;
    m_hInstance = hInstance;
    
    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = 0;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TOUR));
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = WINDOW_CLASS;
    wndclass.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TOUR));
    
    return (0 != RegisterClassEx(&wndclass));
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
    //  set the window area to a fixed size and center the window on screen
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        WINDOW_CLASS,
        szTitle,
        WS_POPUP | WS_VISIBLE,
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
    CWebApp *web = (CWebApp *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    LPCREATESTRUCT pcs;
    
    switch(msg)
    {
    case WM_NCCREATE:
        pcs = (LPCREATESTRUCT)lParam;
        web = (CWebApp *)pcs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)web);
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

    case WM_USER+0:
        web->OnExit();
        break;

    case WM_USER+1:
        web->OnRelease();
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
        
        TCHAR szURL[MAX_PATH+12];
        WCHAR wszUrl[MAX_PATH+12];
        DWORD dwSize = ARRAYSIZE(szURL);

        UrlCreateFromPath(g_szPath, szURL, &dwSize, URL_FILE_USE_PATHURL);
        SHTCharToUnicode(szURL, wszUrl, ARRAYSIZE(wszUrl));
        Navigate(wszUrl);
    }

    UpdateWindow(hwnd);
}

/**
*  This method handles the WM_DESTROY message.
*
*  @return         No return value.
*/
void CWebApp::OnDestroy()
{
    OnExit();
    
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
    {
        m_pContainer->setLocation(0, 0, width, height);
    }
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
    {
        m_pContainer->setFocus(TRUE);
    }
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

void CWebApp::OnExit()
{
    //
    //  release our cached instance of the IWebBrowser2
    //  interface.
    //
    if (m_pweb)
    {
        m_pweb->Release();
        m_pweb = NULL;
    }
    
    //
    //  tell the container to remove IE4 and then
    //  release our reference to the container.
    //
    if (m_pContainer)
    {
        m_pContainer->remove();
        m_pContainer->Release();
        m_pContainer = NULL;
    }
}

void CWebApp::OnRelease()
{
    DestroyWindow(m_hwnd);
}