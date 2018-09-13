//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       app.cxx
//
//  Contents:   application functionality.
//
//  Created:    02/20/98    philco   
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_FACTORY_HXX_
#define X_FACTORY_HXX_
#include "factory.hxx"
#endif

#ifndef X_SERVER_HXX_
#define X_SERVER_HXX_
#include "server.hxx"
#endif

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_APPRC_H_
#define X_APPRC_H_
#include "apprc.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_REGKEY_HXX_
#define X_REGKEY_HXX_
#include "regkey.hxx"
#endif

#ifndef X_PEERS_HXX_
#define X_PEERS_HXX_
#include "peers.hxx"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

DeclareTag(tagHTAWndMsg, "HTA", "HtmlApp Windows Messages")

CHTMLApp::CHTMLApp()
{
    // Default window styles
    _dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    _windowState = SW_NORMAL;

    _fShowInTaskBar = TRUE;
    _fContextMenu   = TRUE;

    // Set defaults for pending move/resize operations
    _pMovePending.x = _pMovePending.y = LONG_MIN;
    _pSizePending.cx = _pSizePending.cy = LONG_MIN;
}

ULONG
CHTMLApp::SubRelease()
{
    if (--_ulAllRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        return 0;
    }
    return _ulAllRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Register
//
//  Synopsis:   Creates registry entries under various keys for this app
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::Register()
{
    HRESULT hr = S_OK;
    CRegKey key, subKey, serverKey;
    TCHAR aryAppPath[MAX_PATH+1];
    TCHAR arydefIcon[MAX_PATH+3];
    long lRet = ERROR_SUCCESS;
    
    // Remove any previous registration information
    Unregister();
    
    GetModuleFileName(_hinst, aryAppPath, MAX_PATH);

    // append icon resource ID to app path
    _tcscpy(arydefIcon, aryAppPath);
    _tcscat(arydefIcon, SZ_REG_DEF_ICON_ID);

    // Update file extension
    lRet = key.Create(HKEY_CLASSES_ROOT, SZ_REG_FILE_EXT);
    TESTREG(lRet);
    key.SetValue(SZ_REG_PROGID, NULL);
    key.SetValue(SZ_REG_CONTENT_TYPE, TEXT("Content Type"));

    // Update file association actions
    lRet= key.Create(HKEY_CLASSES_ROOT, SZ_REG_PROGID);
    TESTREG(lRet);
    key.SetValue(SZ_REG_FILE_READABLE_STRING, NULL);

    // Add a key to indicate the default icon for this file type
    lRet = subKey.Create(key, TEXT("DefaultIcon"));
    TESTREG(lRet);
    subKey.SetValue(arydefIcon, NULL);
        
    // Add a shell action key
    lRet = subKey.Create(key, TEXT("Shell\\Open\\Command"));
    TESTREG(lRet);

    TCHAR aryOpenCmd[MAX_PATH+4];
    _tcscpy(aryOpenCmd, aryAppPath);
    _tcscat(aryOpenCmd, TEXT(" \"%1\" %*"));
    subKey.SetValue(aryOpenCmd, NULL);

    // Register COM server information
    lRet = key.Open(HKEY_CLASSES_ROOT, TEXT("clsid"));
    TESTREG(lRet);

    // add our clsid
    lRet = subKey.Create(key, SZ_SERVER_CLSID);
    TESTREG(lRet);
    subKey.SetValue(SZ_REG_FILE_READABLE_STRING, NULL);

    // Now add all the other server goo
    lRet = serverKey.Create(subKey, TEXT("DefaultIcon"));
    TESTREG(lRet);
    serverKey.SetValue(arydefIcon, NULL);

    lRet = serverKey.Create(subKey, TEXT("LocalServer32"));
    TESTREG(lRet);
    serverKey.SetValue(aryAppPath, NULL);

    lRet = serverKey.Create(subKey, TEXT("Version"));
    TESTREG(lRet);            
    serverKey.SetValue(SZ_APPLICATION_VERSION, NULL);

    lRet = serverKey.Create(subKey, TEXT("ProgID"));
    TESTREG(lRet);            
    serverKey.SetValue(SZ_REG_PROGID, NULL);

    // Update content-type information
    lRet = key.Open(HKEY_CLASSES_ROOT, TEXT("MIME\\Database\\Content Type"));
    TESTREG(lRet);

    lRet = subKey.Create(key, SZ_REG_CONTENT_TYPE);
    TESTREG(lRet);
    subKey.SetValue(SZ_SERVER_CLSID, TEXT("CLSID"));                    
    subKey.SetValue(SZ_REG_FILE_EXT, TEXT("Extension"));                    

Cleanup:
    if (lRet != ERROR_SUCCESS)
    {
        // On failure, don't leave an inconsistent registry state
        Unregister();  
        hr = E_FAIL;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Unregister
//
//  Synopsis:   Removes registry entries under various keys for this app.
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::Unregister()
{
    HRESULT hr = S_OK;
    CRegKey key;
    long lRet = ERROR_SUCCESS;
    
    lRet = key.Open(HKEY_CLASSES_ROOT, NULL);
    if (lRet == ERROR_SUCCESS)
    {
        key.RecurseDeleteKey(SZ_REG_FILE_EXT);
        key.RecurseDeleteKey(SZ_REG_PROGID);
    }
    
    lRet = key.Open(HKEY_CLASSES_ROOT, TEXT("clsid"));
    if (lRet == ERROR_SUCCESS)
        key.RecurseDeleteKey(SZ_SERVER_CLSID);

    lRet = key.Open(HKEY_CLASSES_ROOT, TEXT("MIME\\Database\\Content Type"));
    if (lRet == ERROR_SUCCESS)
        key.RecurseDeleteKey(SZ_REG_CONTENT_TYPE);

    if (lRet != ERROR_SUCCESS)
        hr = E_FAIL;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::RegisterWindowClasses
//
//  Synopsis:   Registers window class for app's frame.
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::RegisterWindowClasses()
{
    WNDCLASS    wc;
    HICON hIcon = 0;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;                                                                            
    wc.hInstance = _hinst;

    if (_cstrAppIcon)
    {
        hIcon = ExtractIcon(_hinst, _cstrAppIcon, 0);
    }

    // If we don't have a custom icon (or it couldn't be loaded), use the
    // default windows application icon.
    if (hIcon == (HICON)1 || hIcon == (HICON)0)
    {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    wc.hIcon = hIcon;

    wc.lpszMenuName =  MAKEINTRESOURCE(IDR_PADMENU);
    wc.lpszClassName = _cstrAppName ? _cstrAppName : SZ_APPLICATION_WNDCLASS;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);

    if (!RegisterClass(&wc))
    {
        return E_FAIL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Init
//
//  Synopsis:   One-time initialization 
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::Init(HINSTANCE hInst, LPTSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    CCurs curs(IDC_APPSTARTING);

    _hinst = hInst;
    _windowState = nCmdShow;
    _cstrCmdLine.Set(lpszCmdLine);
    
    hr = THR(OleInitialize(NULL));

    // Need to remember whether this succeeded.  If it fails, don't
    // call OleUninitialize when the app shuts down.
    if (hr == S_OK)
        _fOLEInitialized = TRUE;

    TEST(hr);

    // COM will pass "/Embedding" as the command line if we are
    // launched via that means.  Register our class factory in 
    // this case.
    if (_tcsiequal(_cstrCmdLine, _T("/Embedding")) ||
        _tcsiequal(_cstrCmdLine, _T("-Embedding")))
    {
        _cstrCmdLine.Free();  // command line shouldn't contain embedding switch.
        _Factory.Register();
    }
    else if (_tcsiequal(_cstrCmdLine, _T("/Register")) ||
        _tcsiequal(_cstrCmdLine, _T("-Register")))
    {
        Register();
        hr = E_FAIL;  // Used to indicate the app should exit immediately
        goto Cleanup;        
    }
    else if (_tcsiequal(_cstrCmdLine, _T("/Unregister")) ||
        _tcsiequal(_cstrCmdLine, _T("-Unregister")))
    {
        Unregister();
        hr = E_FAIL;  // Used to indicate the app should exit immediately
        goto Cleanup;        
    }
    else
    {
        // We are being launched from the command line, or as the result of a
        // file extension association.  The command line argument should contain
        // an URL.

        IMoniker *pMk = NULL;
        
        hr = CreateHTAMoniker(&pMk);
        
        TEST(hr);
        
        hr = _Factory.CreateInstance(NULL, IID_IPersistMoniker, (void **)&_pServer);
        TEST(hr);

        _pServer->Load(TRUE, pMk, NULL, 0);

        ReleaseInterface(((IPersistMoniker *)_pServer));
        _pServer = NULL;
    }
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Passivate
//
//  Synopsis:   General cleanup that occurs before dtor
//
//----------------------------------------------------------------------------

void CHTMLApp::Passivate()
{
    // general app cleanup
    
    // Then passivate the subobjects
    _Client.Passivate();
    _Frame.Passivate();
    _Factory.Passivate();

    if (_pServer)
    {
        Assert(!_pServer->GetRefs());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Terminate
//
//  Synopsis:   Called when app terminates.
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::Terminate()
{
    HRESULT hr = S_OK;
    
    Passivate();
    
    if (_pServer)
    {
        ReleaseInterface(((IPersistMoniker *)_pServer));
        _pServer = NULL;
    }

    if (_fOLEInitialized)
        OleUninitialize();

    // If there is a hidden parent window, destroy it now.
    if (_hwndParent)
       DestroyWindow(_hwndParent);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Run
//
//  Synopsis:   Message pump
//
//----------------------------------------------------------------------------

void CHTMLApp::Run()
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (S_FALSE == _Frame.TranslateKeyMsg(&msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::CreateHTAMoniker
//
//  Synopsis:   Creates an URL moniker based on the command line provided by
//              the system.
//
//  Notes:      This function is invoked only when an HTA is launched from a
//              command line or by the shell.
//----------------------------------------------------------------------------

HRESULT CHTMLApp::CreateHTAMoniker(IMoniker ** ppm)
{
    HRESULT hr = S_OK;
    CStr cstrPath;

    Assert(ppm);

    // Make a copy, remove command-line arguments and quotes.
    cstrPath.Set(_cstrCmdLine);
    PathRemoveArgs(cstrPath);
    PathUnquoteSpaces(cstrPath);
    
#ifndef UNICODE
    // Convert ansi URL to Unicode
    WCHAR awch[pdlUrlLen];
    UnicodeFromMbcs(awch, pdlUrlLen, cstrPath);
    hr = ::CreateURLMoniker(NULL, awch, ppm);
#else
    hr = ::CreateURLMoniker(NULL, cstrPath, ppm);
#endif

    TEST(hr);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::RunHTMLApplication
//
//  Synopsis:   Given a moniker, creates the frame window and an instance of 
//              trident.  Loads the moniker and activates Trident as a doc object.
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::RunHTMLApplication(IMoniker *pMk)
{
	HRESULT hr = S_OK;

	// A moniker is required.  Otherwise, what would we load?
	if (!pMk)
	{
		hr = E_POINTER;
		goto Cleanup;
	}
	
    hr = THR(_Client.Create(CLSID_HTADoc));
    TEST(hr);
    
    hr = THR(_Client.Load(pMk));
    TEST(hr);
    
Cleanup:
	RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::GetViewRect
//
//  Synopsis:   Provides the current client rect.
//
//----------------------------------------------------------------------------

void CHTMLApp::GetViewRect(OLERECT *prc)
{
    if (prc)
    {
        ::GetClientRect(_hwnd, prc);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::Resize
//
//  Synopsis:   Resizes the doc object to fill the new size of our frame window.
//
//----------------------------------------------------------------------------

void CHTMLApp::Resize()
{
    _Frame.Resize();
    _Client.Resize(); 
}


HRESULT CHTMLApp::Pending_moveTo(long x, long y)
{
    Assert(!IsWindow(_hwnd));
    
    _pMovePending.x = x;
    _pMovePending.y = y;

    return S_OK;
}

HRESULT CHTMLApp::Pending_moveBy(long x, long y)
{
    HRESULT hr = E_PENDING;
    
    Assert(!IsWindow(_hwnd));
    
    // Can't move by a relative amount until we have a non-default rect.
    if (_pMovePending.x != LONG_MIN && _pMovePending.y != LONG_MIN)
    {
        _pMovePending += CSize(x, y);
        hr = S_OK;
    }
    
    return hr;
}

HRESULT CHTMLApp::Pending_resizeTo(long x, long y)
{
    Assert(!IsWindow(_hwnd));
    
    _pSizePending.SetSize(x, y);

    return S_OK;
}

HRESULT CHTMLApp::Pending_resizeBy(long x, long y)
{
    HRESULT hr = E_PENDING;
    
    Assert(!IsWindow(_hwnd));
    
    // Can't resize by a relative amount until we have a non-default rect.
    if (_pSizePending.cx != LONG_MIN && _pSizePending.cy != LONG_MIN)
    {
        _pSizePending += CSize(x, y);
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::CreateAppWindow
//
//  Synopsis:   Creates the application's frame window
//
//----------------------------------------------------------------------------

HRESULT CHTMLApp::CreateAppWindow()
{
    HRESULT hr = S_OK;
    
    // We might have already created the frame window.  This function is called from
    // the IDM_PARSECOMPLETE exec command handler and also from the behavior notification
    // function.  The idea is to get the frame window up ASAP.
    if (_fWait || ::IsWindow(_hwnd))
        goto Cleanup;
        
    // Handle single instance behavior
    if (_fSingleInstance && PriorInstance())
    {
        ::PostQuitMessage(0);
        return E_FAIL;
    }

    // Register our window class
    hr = RegisterWindowClasses();
    TEST(hr);

    // Create a hidden window to parent the app to if it shouldn't show up
    // in the task bar.
    if (!_fShowInTaskBar)
    {
        _hwndParent = CreateWindowEx(
            0,
            _cstrAppName ? _cstrAppName : SZ_APPLICATION_WNDCLASS,
            _T(""),
            0,
            0, 0, 0, 0,
            NULL,
            NULL,
            _hinst,
            this);

       if (!_hwndParent)
       {
            hr = E_FAIL;
            goto Cleanup;
       }
       Assert(::IsWindow(_hwndParent));

    }

    // Create main application window.
    _hwnd = CreateWindowEx(
            _dwStyleEx,
            _cstrAppName ? _cstrAppName : SZ_APPLICATION_WNDCLASS,
            _T(""),
            _dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            _hwndParent,
            NULL,
            _hinst,
            this);

    Assert(::IsWindow(_hwnd));
    if (!_hwnd)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Morph to a popup style...
    _dwStyle |= WS_POPUP;
    ::SetWindowLong(_hwnd, GWL_STYLE, _dwStyle);
    ::SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    
    // There will be a pending move if script has already called MoveTo before
    // this window was created.
    if ((_pMovePending.x != LONG_MIN) && (_pMovePending.y != LONG_MIN))
    {
        // Move to new location.
        SetWindowPos(_hwnd,
                 NULL, 
                 _pMovePending.x, 
                 _pMovePending.y, 
                 0, 
                 0, 
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE );
    }
    
    // There will be a pending size if script has already called resizeTo before
    // this window was created.
    if ((_pSizePending.cx != LONG_MIN) && (_pSizePending.cy != LONG_MIN))
    {
        // resize according to pending information.
        SetWindowPos(_hwnd,
                 NULL, 
                 0, 
                 0, 
                 _pSizePending.cx, 
                 _pSizePending.cy, 
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE );
    }

    // If we are running on NT5, then we need to tell the window that we are
    // interested in UIState messages.  This should only be called once per top
    // level window
    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT && g_dwPlatformVersion >= 0x00050000)
    {
        SendMessage(_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
    }

    ShowFrameWindow(_windowState);
    _Client.Show();

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::ShowFrameWindow
//
//  Synopsis:   Shows the application's frame window
//
//----------------------------------------------------------------------------

void CHTMLApp::ShowFrameWindow(int windowState)
{
    ShowWindow(_hwnd, windowState);
    UpdateWindow(_hwnd); 
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::SetTitle
//
//  Synopsis:   Sets the caption for frame window.
//
//----------------------------------------------------------------------------

void CHTMLApp::SetTitle(TCHAR * pchTitle)
{
    TCHAR szBuf[512];

    GetWindowText(_hwnd, szBuf, ARRAY_SIZE(szBuf));
    if (!_tcsequal(szBuf, pchTitle))
    {
        SetWindowText(_hwnd, pchTitle);
    }
}

// Get application attributes from the behavior.
void CHTMLApp::SetAttributes(CAppBehavior *pApp)
{
    CDoc *  pDoc; 
    HRESULT hr;

    // Computed values
    _dwStyle = pApp->GetStyles();
    _dwStyleEx = pApp->GetExtendedStyles();
    
    // Get these directly from the attribute array.
    _fSingleInstance = pApp->GetAAsingleInstance();
    _fShowInTaskBar  = pApp->GetAAshowInTaskBar();
    _fContextMenu    = pApp->GetAAcontextMenu();

    _windowState = pApp->GetAAwindowState();
    _cstrAppName.Set(pApp->GetAAapplicationName());
    _cstrAppIcon.Set(pApp->GetAAicon());
    _cstrAppVersion.Set(pApp->GetAAversion());
    
    hr = THR(theApp._Client._pUnk->QueryInterface(CLSID_HTMLDocument, (void**)&pDoc));

    if (SUCCEEDED(hr))
    {
        // Set IDocHostUIHandler Flags
        //
        // Note that the default value of scrollFlat is 
        // different than the other properties. It is "no"
        // by default.
        //
        if (pApp->GetAAscrollFlat())  
        {
            pDoc->_dwFlagsHostInfo |= DOCHOSTUIFLAG_FLAT_SCROLLBAR;
        }

        if (!pApp->GetAAinnerBorder())
        {
            pDoc->_dwFlagsHostInfo |= DOCHOSTUIFLAG_NO3DBORDER;
        }

        if (!pApp->GetAAscroll())
        {
            pDoc->_dwFlagsHostInfo |= DOCHOSTUIFLAG_SCROLL_NO;
        }

        if (!pApp->GetAAselection())  // Acts like a dialog: no selection
        {
            pDoc->_dwFlagsHostInfo |= DOCHOSTUIFLAG_DIALOG;
        }
    }

    // We now have enough information to create the App's main frame window.
    CreateAppWindow();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::PriorInstance
//
//  Synopsis:   Searches for another window that has a class name matching the
//              application name attribute for this instance.  Returns TRUE if
//              such a window is found.  Also brings that window (and any associated
//              popups) to the foreground.
//
//----------------------------------------------------------------------------

BOOL CHTMLApp::PriorInstance()
{
    BOOL fRet = FALSE;

    // We need an application name to determine if a prior instance is running
    if (!_cstrAppName)
        return fRet;
        
    HWND hwndPrev = ::FindWindow(_cstrAppName, NULL);
    
    if (hwndPrev)
    {
        HWND hwndChild = ::GetLastActivePopup(hwndPrev);

        // if previous instance is minimized, restore it.
        if (::IsIconic(hwndPrev))
            ::ShowWindow(hwndPrev, SW_RESTORE);

        // Bring the previous instance (or its last popup window) to the fore.
        ::SetForegroundWindow(hwndChild);

        // Previous instance was running...
        fRet = TRUE;
    }
    
    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::HandleClose
//
//  Synopsis:   Handles closing the application.  Causes onBeforeUnload to be fired.
//
//----------------------------------------------------------------------------

void
CHTMLApp::HandleClose(HWND hwnd)
{
    Assert(IsWindow(hwnd) && (_hwnd == hwnd));

    CVariant varOut;

    _Client.SendCommand(NULL, OLECMDID_ONUNLOAD, OLECMDEXECOPT_PROMPTUSER, NULL, &varOut);
    if ((V_VT(&varOut) == VT_BOOL) && (V_BOOL(&varOut)) != VARIANT_TRUE)
        return;

    DestroyWindow(hwnd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLApp::WndProc
//
//  Synopsis:   WndProc for the frame.
//
//----------------------------------------------------------------------------

LRESULT CHTMLApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        TraceTag((tagHTAWndMsg, "WM_CLOSE"));
        theApp.HandleClose(hwnd);
        break;
        
    case WM_SYSCOMMAND:
        TraceTag((tagHTAWndMsg, "WM_SYSCOMMAND"));

        if (wParam == SC_CLOSE)
            theApp.HandleClose(hwnd);
        else
            return DefWindowProc(hwnd, msg, wParam, lParam);
        break;
        
    case WM_DESTROY:
        TraceTag((tagHTAWndMsg, "WM_DESTROY"));
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        TraceTag((tagHTAWndMsg, "WM_SIZE"));
        theApp.Resize();
        break;

    case WM_ACTIVATE:
        {
            TraceTag((tagHTAWndMsg, "WM_ACTIVATE"));
            
            CDoc*   pDoc; 
            HRESULT hr;
            IOleInPlaceActiveObject* pActiveObj;

            // Activate or Deactivate the document.  This is needed 
            // in order to make focus adornments work correctly on 
            // Win9x and NT4
            //
            hr = THR(theApp._Client._pUnk->QueryInterface(CLSID_HTMLDocument, (void**)&pDoc));
            if (SUCCEEDED(hr))
            {
                hr = pDoc->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pActiveObj);
                if (SUCCEEDED(hr))
                {
                    pActiveObj->OnFrameWindowActivate((WA_INACTIVE == LOWORD(wParam)) ? FALSE : TRUE);
                    ReleaseInterface(pActiveObj);
                }
            }
        }
        break;
        
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        if (msg == WM_QUERYNEWPALETTE)
        {
            TraceTag((tagHTAWndMsg, "WM_QUERYNEWPALETTE"));
        }
        else
        {
            TraceTag((tagHTAWndMsg, "WM_PALETTECHANGED"));
        }
                                
        return theApp._Client.DelegateMessage(msg, wParam, lParam);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

   return 0;
}


