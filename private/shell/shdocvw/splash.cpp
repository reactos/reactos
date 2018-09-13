/* Copyright 1997 Microsoft */

#include "priv.h"
#include "resource.h"

#include <mluisupp.h>

#define TIMER_TIMEOUT      1
#define SPLASHWM_DISMISS    WM_USER

////////////////////////////////////////////////////////////////////////////
//
//  InitLF -- modified from comdlg32\fonts.c, used by ShowSplashScreen
//
//  Initalize a LOGFONT structure to some base generic regular type font.
//
////////////////////////////////////////////////////////////////////////////

VOID InitLF(
    LPLOGFONT lplf)
{
    lplf->lfEscapement = 0;
    lplf->lfOrientation = 0;
    lplf->lfCharSet = DEFAULT_CHARSET;
    lplf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lplf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lplf->lfQuality = DEFAULT_QUALITY;
    lplf->lfPitchAndFamily = DEFAULT_PITCH;
    lplf->lfItalic = 0;
    lplf->lfWeight = FW_NORMAL;
    lplf->lfStrikeOut = 0;
    lplf->lfUnderline = 0;
    lplf->lfWidth = 0;            // otherwise we get independant x-y scaling

    lplf->lfFaceName[0] = 0;
    MLLoadString(IDS_SPLASH_FONT, lplf->lfFaceName, ARRAYSIZE(lplf->lfFaceName));

    TCHAR szTmp[16];
    MLLoadString(IDS_SPLASH_SIZE, szTmp, ARRAYSIZE(szTmp));
    lplf->lfHeight = StrToInt(szTmp);
}

BOOL g_fShown = FALSE;

class CIESplashScreen : public ISplashScreen
{
protected:
    HBITMAP  _hbmSplash;     // The bitmap to display.
    HBITMAP  _hbmOld;
    HDC      _hdc;
    HWND     _hwnd;
    LONG     _cRef;

public:
    CIESplashScreen( HRESULT * pHr );
    ~CIESplashScreen();

    STDMETHOD (QueryInterface)(THIS_ REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef) ( THIS );
    STDMETHOD_(ULONG, Release) ( THIS );

    STDMETHOD ( Show ) ( HINSTANCE hinst, UINT idResHi, UINT idResLow, HWND * phwshnd );
    STDMETHOD ( Dismiss ) ( void );
    
    static LRESULT s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL _RegisterWindowClass(void);
    HWND ShowSplashScreen(HINSTANCE hinst, UINT idResHi, UINT idResLow);

};


CIESplashScreen::CIESplashScreen(HRESULT * pHr) : _cRef (1)
{
    DllAddRef();
    *pHr = NOERROR;
}

CIESplashScreen::~CIESplashScreen()
{
    if (_hdc)
    {
        // select the previous hbm we got when we put the splash in there,
        // so that we can now destroy the hbitmap
        SelectObject( _hdc, _hbmOld );
        DeleteObject(_hdc);
    }

    // destroy the hbitmpa, can only do this if we have deselected it above...
    if (_hbmSplash)
        DeleteObject(_hbmSplash);

    DllRelease();
}

STDMETHODIMP CIESplashScreen::QueryInterface (REFIID riid, void ** ppv)
{
    HRESULT hr = NOERROR;
    if ( IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ISplashScreen ))
    {
        *ppv = SAFECAST( this, ISplashScreen *);
        this->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CIESplashScreen::AddRef ( )
{
    _cRef ++;
    return _cRef;
}

STDMETHODIMP_(ULONG) CIESplashScreen::Release ( )
{
    _cRef --;
    if ( !_cRef )
    {
        delete this;
        return 0;
    }
    return _cRef;
}

STDMETHODIMP CIESplashScreen::Show ( HINSTANCE hinst, UINT idResHi, UINT idResLow, HWND * phwnd )
{
    if ( !phwnd )
    {
        return E_INVALIDARG;
    }
    
    // First thing to do is to see if see if browser or a splash screen up...
    if ( g_fShown )
        return NULL;
    
    *phwnd = ShowSplashScreen( hinst, idResHi, idResLow );
    
    return ( *phwnd ? NOERROR : E_UNEXPECTED );
}

STDMETHODIMP CIESplashScreen::Dismiss ( void )
{
    if ( _hwnd )
    {
        // Synchronously dismiss the splash screen then post a message to
        // destroy the window.
        SendMessage(_hwnd, SPLASHWM_DISMISS, 0, 0);
        PostMessage(_hwnd, WM_CLOSE, 0, 0);
    }
    return S_OK;
}

HWND CIESplashScreen::ShowSplashScreen( HINSTANCE hinst, UINT idResHi, UINT idResLow )
{
    // don't show splash screen for IE in intergrated mode or if it's been disabled 
    // by the admin
    if (
        ( (WhichPlatform() == PLATFORM_INTEGRATED) && (hinst == HINST_THISDLL) ) ||
        ( SHRestricted2(REST_NoSplash, NULL, 0) )
       )
    {
        return NULL;
    }
    
    if (!_RegisterWindowClass())
        return NULL;

    // provide default bitmap resource ID's for IE
    if (hinst == HINST_THISDLL)
    {
        if (idResHi == -1)
            idResHi = IDB_SPLASH_IEXPLORER_HI;
        if (idResLow == -1)
            idResLow = IDB_SPLASH_IEXPLORER;
    }
            
     // Now load the appropriate bitmap depending on colors, only use the 256 colour splash
     // if we are greater than 256 colours (such as 32K or 65K upwards), this will mean we don't have
     // to flash the palette just to put up the splash screen.
    _hbmSplash = (HBITMAP)LoadImage(hinst, MAKEINTRESOURCE((GetCurColorRes() > 8) ? idResHi : idResLow), 
                                    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    _hdc = CreateCompatibleDC(NULL);
    
    if (!_hbmSplash || !_hdc)
        return NULL;

    // remember the old hbitmap so we can select it back before we delete the bitmap..
    _hbmOld = (HBITMAP) SelectObject(_hdc, _hbmSplash);

    // set font and color for text
    LOGFONT lf;
    HFONT   hfont;
    
    InitLF(&lf);
    hfont = CreateFontIndirect(&lf);
    if ( hfont == NULL ) // show the bitmap without text if we can't create the font
        goto Done;

    // select the new font and remember the old one
    hfont = (HFONT)SelectObject(_hdc, hfont);
    
    SetTextColor(_hdc, RGB(0,0,0));
    SetBkColor(_hdc, RGB(255,255,255));
    SetBkMode(_hdc, TRANSPARENT);
    
    // draw the text on top of the selected bitmap
    TCHAR   szText[512], szY[32];
    RECT    rect;
    
    MLLoadString(IDS_SPLASH_Y1, szY, ARRAYSIZE(szY));
    MLLoadString(IDS_SPLASH_STR1, szText, ARRAYSIZE(szText));
    SetRect(&rect, 104, StrToInt(szY), 386, StrToInt(szY) + 10);
    DrawText(_hdc, szText, -1, &rect, DT_TOP | DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
    DrawText(_hdc, szText, -1, &rect, DT_TOP | DT_LEFT | DT_SINGLELINE);

    MLLoadString(IDS_SPLASH_Y2, szY, ARRAYSIZE(szY));
    MLLoadString(IDS_SPLASH_STR2, szText, ARRAYSIZE(szText));
    SetRect(&rect, 104, StrToInt(szY), 386, 400);
    DrawText(_hdc, szText, -1, &rect, DT_TOP | DT_LEFT | DT_CALCRECT);
    DrawText(_hdc, szText, -1, &rect, DT_TOP | DT_LEFT);

    // select back the old font and delete the new one
    DeleteObject(SelectObject(_hdc, hfont));
     
Done:
    // we now have everything in the DC, ready for painting.
    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TEXT("CIESplashScreen"), NULL, 
                           WS_OVERLAPPED | WS_CLIPCHILDREN,
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                           NULL, (HMENU)NULL, HINST_THISDLL, this);
    if (_hwnd)
        ShowWindow(_hwnd, SW_NORMAL);

    return _hwnd;
}

LRESULT CIESplashScreen::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    CIESplashScreen *piess = (CIESplashScreen*)GetWindowLong(hwnd, 0);

    if (!piess && (uMsg != WM_CREATE))
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        DllAddRef();        // make sure we are not unloaded while in dialog
        if (lParam)
        {
            DWORD dwExStyles;

            piess = (CIESplashScreen*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)piess);

            //
            // Turn off mirroring for the GUI splash screen bitmap.
            //
            if ((dwExStyles=GetWindowLong(hwnd, GWL_EXSTYLE))&RTL_MIRRORED_WINDOW)
            {
                SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyles&~RTL_MIRRORED_WINDOW);
            }

            // Now lets try to center the window on the screen.
            BITMAP bm;

            GetObject(piess->_hbmSplash, sizeof(bm), &bm);

            SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & 
                          ~(WS_CAPTION|WS_SYSMENU|WS_BORDER|WS_THICKFRAME));
            SetWindowPos(hwnd, HWND_TOP, 
                         (GetSystemMetrics(SM_CXSCREEN) - bm.bmWidth) / 2, 
                         (GetSystemMetrics(SM_CYSCREEN) - bm.bmHeight) / 2, 
                         bm.bmWidth, bm.bmHeight, 0);

            // Set a 5 second timer to time it out.
            SetTimer(hwnd, TIMER_TIMEOUT, 15000, NULL);
        }
        g_fShown = TRUE;
        break;

    case WM_NCDESTROY:
        // the splash screen has left the building
        g_fShown = FALSE;
        
        DllRelease();
        break;

    case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            HDC hdc = (HDC)wParam;

            BitBlt((HDC)hdc, 0, 0, rc.right, rc.bottom, piess->_hdc, 0, 0, SRCCOPY);

            return 1;
        }
        break;

    case WM_TIMER:
        // Now assume it is the right one.
        KillTimer( hwnd, TIMER_TIMEOUT );
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        break;

    case SPLASHWM_DISMISS:
        // Hide ourselves and remove our reference to piess - it may be gone at any point
        // after this call.
        ShowWindow(hwnd, SW_HIDE);
        SetWindowLong(hwnd, 0, 0);
        break;

    case WM_ACTIVATE:
        if ( wParam == WA_INACTIVE && hwnd != NULL )
        {
            KillTimer( hwnd, TIMER_TIMEOUT );

            // create a new timer for 2 seconds after loss of activation...
            SetTimer( hwnd, TIMER_TIMEOUT, 2000, NULL );
            break;
        }
        // drop through
    
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


BOOL CIESplashScreen::_RegisterWindowClass(void)
{
    WNDCLASS wc = {0};

    //wc.style         = 0;
    wc.lpfnWndProc   = s_WndProc ;
    //wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(CIESplashScreen *);
    wc.hInstance     = g_hinst ;
    //wc.hIcon         = NULL ;
    //wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    //wc.lpszMenuName  = NULL ;
    wc.lpszClassName = TEXT("CIESplashScreen");

    return SHRegisterClass(&wc);
}

STDAPI CIESplashScreen_CreateInstance(IUnknown * pUnkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr = E_FAIL;
    CIESplashScreen * pSplash = new CIESplashScreen( & hr );
    if ( !pSplash )
    {
        return E_OUTOFMEMORY;
    }
    if ( FAILED( hr ))
    {
        delete pSplash;
        return hr;
    }
    
    *ppunk = SAFECAST(pSplash, ISplashScreen *);
    return NOERROR;
}

STDAPI SHCreateSplashScreen(ISplashScreen **ppSplash)
{
    return CIESplashScreen_CreateInstance(NULL, (IUnknown **)ppSplash, NULL );
}
