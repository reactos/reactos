//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       app.hxx
//
//  Contents:   CHTMLApp
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#ifndef __APP_HXX__
#define __APP_HXX__

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_CLIENT_HXX_
#define X_CLIENT_HXX_
#include "client.hxx"
#endif

#ifndef X_FACTORY_HXX_
#define X_FACTORY_HXX_
#include "factory.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_POINT_HXX_
#define X_POINT_HXX_
#include "point.hxx"
#endif

// Forward declarations
class CHTAClassFactory;
class CAppBehavior;

class CServerObject;
class CClient;

// Global application object
extern CHTMLApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Memory meter support
MtExtern(CHTMLApp)

//+---------------------------------------------------------------------------
//
//  Class:      CHTMLApp
//
//	Purpose:	Creates the main application window, message pump and
//           	hosts Trident as a document object.
//
//	Notes:
//
//----------------------------------------------------------------------------

class CHTMLApp
{

private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:

    NO_COPY(CHTMLApp);
    DECLARE_CLASS_TYPES(CHTMLApp, CBase)

    CHTMLApp();
    
    void Passivate();
    
    ULONG       SubAddRef() { return ++_ulAllRefs; }
    ULONG       SubRelease();
    ULONG       GetObjectRefs() { return _ulRefs; }

    HRESULT RegisterWindowClasses();
    HRESULT Init(HINSTANCE hinst, LPTSTR szCmdLine, int nCmdShow);
    HRESULT Terminate();
    HRESULT CreateHTAMoniker(IMoniker ** ppm);
    
	// Self-registration helpers
    HRESULT Register();
    HRESULT Unregister();

    // Load and run an HTML application.
    HRESULT RunHTMLApplication(LPMONIKER pMK);
    
	// Creates the HTA host application frame window
    HRESULT CreateAppWindow();
    void    ShowFrameWindow(int windowState);
    
    void SetAttributes(CAppBehavior *pApp);
    void SetTitle(TCHAR * pchTitle);
    BOOL PriorInstance();
    
	// WndProc and message pump
    static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Run();

    void GetViewRect(OLERECT *prc);
    void HandleClose(HWND hwnd);
    void Resize();

    // Pending operations
    HRESULT Pending_moveTo(long x, long y);
    HRESULT Pending_moveBy(long x, long y);
    HRESULT Pending_resizeTo(long x, long y);
    HRESULT Pending_resizeBy(long x, long y);
    
    // inline functions
    void Wait(BOOL fWait) { _fWait = fWait; }
    void Close()          { ::PostMessage(_hwnd, WM_CLOSE, 0, 0); }
    LPTSTR cmdLine()      { return _cstrCmdLine; }

    BOOL contextMenu()    { return _fContextMenu; }
    
public:

    HWND                _hwnd;          // HWND of the frame window
    HWND                _hwndParent;    // HWND of the frame window's hidden parent
    HINSTANCE           _hinst;         // Our HINSTANCE.

    CPoint              _pMovePending;
    CSize               _pSizePending;
    
    // subobjects
    CHTMLAppFrame       _Frame;
    CClient             _Client;
    CHTAClassFactory    _Factory;
    CBehaviorFactory    _PeerFactory;
    
private:

    // refcounting
    ULONG               _ulRefs;
    ULONG               _ulAllRefs;

    CServerObject *     _pServer;

    // Frame window styles
    DWORD               _dwStyle;
    DWORD               _dwStyleEx;

    // APPLICATION tag values
    BOOL                _fSingleInstance;
    BOOL                _fShowInTaskBar;
    BOOL                _fContextMenu;

    int                 _windowState;
    CStr                _cstrAppName;
    CStr                _cstrAppIcon;
    CStr                _cstrAppVersion;
    CStr                _cstrCmdLine;
    
private:
    // misc. data
    BOOL                _fOLEInitialized : 1;
    BOOL                _fWait : 1;
};

#endif // __APP_HXX__


