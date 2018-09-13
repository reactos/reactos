//////////////////////////////////////////////////////////////////////////
//
//  dlgapp.cpp
//
//      This file contains the main entry point into the application and
//      the implementation of the CDlgApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>    // for string compare functions
#include <debug.h>
#include <tchar.h>

#pragma hdrstop

#include "dlgapp.h"
#include "resource.h"

#define WINDOW_CLASS    TEXT("_BerksWin2kAutorunApp_")

//////////////////////////////////////////////////////////////////////////
// Global Data
//////////////////////////////////////////////////////////////////////////

bool    g_bTaskRunning = false;     // true when we have a running task open
int     g_iSelectedItem = -1;       // 
WNDPROC g_fnBtnProc = NULL;         // the window proc for a button.

//////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////

LONG_PTR CALLBACK ButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//////////////////////////////////////////////////////////////////////////
// Metrics
// Our metrics are constants that never change.  All sizes are in pixels (as per the spec):
//////////////////////////////////////////////////////////////////////////

#define c_cyLogoImage                   87  // height of the branding image.
#define c_cyFadeBar                     6   // height of the color fade bar
#define c_cyBranding                    (c_cyLogoImage+c_cyFadeBar) // height of top region that contains our branding images and the fade bar
#define c_cxCheckTextLeftEdge           29  // width from left edge for checkbox text label

#define c_cxMenuItemPadding             10  // width from left of menu item to left of text and right of text to right of menu item
#define c_cyMenuItemPadding             5   // height from top of menu item to top of text and bottom of text to bottom of menu item
#define c_cyMenuItemSpacing             1   // gap from top of one menu item to bottom of next item

#define c_cyBarToTitlePadding           12  // vertical padding from botton of fade bar to top of title text
#define c_cyTitleToBodyPadding          6   // vertical padding from bottom of title text to top of body text
#define c_cyBodyToBottomPadding         53  // vertical padding from body of body text to bottom of client area
#define c_cxRightPanelPadding           16  // generic horizontal padding used on both edges of the right pane

#define c_cyCheckTextToBottomPadding    8   // vertical padding from bottom of "Show at startup" text to bottom of client area

#define c_cxCheckBox                    13
#define c_cyCheckBox                    12


// Code to ensure only one instance of a particular window is running
HANDLE CheckForOtherInstance(HINSTANCE hInstance)
{
    TCHAR   szCaption[128];
    HANDLE  hMutex;

    LoadString(hInstance, IDS_TITLE, szCaption, 128);

    // We don't want to launch autorun when winnt32 is running.  The standard way
    // to check for this is the following mutex, which winnt32 creates:

    hMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, TEXT("Winnt32 Is Running") );

    if ( hMutex )
    {
        // The mutex exists, this means winnt32 is running so we shouldn't.
        // REVIEW: Should we try to findwindow and activate winnt32?
        CloseHandle( hMutex );
        return 0;
    }

    // We create a named mutex with our window caption just as a way to check
    // if we are already running autorun.exe.  Only if we are the first to
    // create the mutex do we continue.

    hMutex = CreateMutex (NULL, FALSE, szCaption);

    if ( !hMutex )
    {
        // failed to create the mutex
        return 0;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Mutex created but by someone else, activate that window
        HWND hwnd = FindWindow( WINDOW_CLASS, szCaption );
        SetForegroundWindow(hwnd);
        CloseHandle(hMutex);
        return 0;
    }

    // we are the first
    return hMutex;
}


/**
*  This function is the main entry point into our application.
*
*  @return     int     Exit code.
*/

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLin, int nShowCmd )
{
    HANDLE hMutex = CheckForOtherInstance(hInstance);

    if ( hMutex )
    {
        CDlgApp dlgapp;
        dlgapp.Register(hInstance);
        if ( dlgapp.InitializeData() )
        {
            dlgapp.Create(nShowCmd);
            dlgapp.MessageLoop();
        }

        CloseHandle(hMutex);
    }
    return 0;
}

typedef DWORD (WINAPI *PFNGETLAYOUT)(HDC);                   // gdi32!GetLayout
typedef DWORD (WINAPI *PFNSETLAYOUT)(HDC, DWORD);            // gdi32!SetLayout

/**
*  This function gets the DC layout.
*
*  @return     DWORD     DC layout.
*/
DWORD Mirror_GetLayout( HDC hdc )
{
    DWORD dwRet=0;
    static PFNGETLAYOUT pfnGetLayout=NULL;
    static BOOL bTriedToLoadBefore = FALSE;

    if( (NULL == pfnGetLayout) && !bTriedToLoadBefore)
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnGetLayout = (PFNGETLAYOUT)GetProcAddress(hmod, "GetLayout");

        bTriedToLoadBefore = TRUE;    
    }

    if( pfnGetLayout )
        dwRet = pfnGetLayout( hdc );

    return dwRet;
}


/**
*  This function sets the DC layout.
*
*  @return     DWORD     old DC layout.
*/
DWORD Mirror_SetLayout( HDC hdc , DWORD dwLayout )
{
    DWORD dwRet=0;
    static PFNSETLAYOUT pfnSetLayout=NULL;
    static BOOL bTriedToLoadBefore = FALSE;

    if( (NULL == pfnSetLayout) && !bTriedToLoadBefore)
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnSetLayout = (PFNSETLAYOUT)GetProcAddress(hmod, "SetLayout");

        bTriedToLoadBefore = TRUE;            
    }

    if( pfnSetLayout )
        dwRet = pfnSetLayout( hdc , dwLayout );

    return dwRet;
}


/**
*  This method is our contstructor for our class. It initialize all
*  of the instance data.
*/
CDlgApp::CDlgApp()
{
    m_hInstance     = NULL;
    m_hwnd          = NULL;

    m_bHighContrast = false;

    m_hfontTitle = NULL;
    m_hfontMenu  = NULL;
    m_hfontBody  = NULL;

    m_hbrMenuItem   = NULL;
    m_hbrMenuBorder = NULL;
    m_hbrRightPanel = NULL;

    m_szDefTitle[0] = NULL;
    m_szDefBody[0] = NULL;

    // In theory, all of these metrics could be adjusted to resize the window.  Resizing wouldn't
    // effect the paddings and spacings so these are defined above as constants.  In the real
    // world we are only resizing vertically to adjust for oversized content.  These are more to
    // allow future expansion.
    m_cxClient = 478;       // width of the client area
    m_cyClient = 322;       // This is currently the only metirc we actually adjust.
    m_cxLeftPanel = 179;    // width of the panel that contains the menu items.
    m_hdcTop = NULL;
    m_hcurHand = NULL;

    m_bLowColor = false;
    m_iColors = -1;
    m_hpal = NULL;
}

CDlgApp::~CDlgApp()
{
    DeleteObject(m_hfontTitle);
    DeleteObject(m_hfontMenu);
    DeleteObject(m_hfontBody);

    DeleteObject(m_hbrMenuItem);
    DeleteObject(m_hbrMenuBorder);
    DeleteObject(m_hbrRightPanel);

    DeleteDC(m_hdcTop);
}

/**
*  This method will register our window class for the application.
*
*  @param  hInstance   The application instance handle.
*
*  @return             No return value.
*/
void CDlgApp::Register(HINSTANCE hInstance)
{
    WNDCLASS  wndclass;

    m_hInstance = hInstance;
    
    wndclass.style          = CS_OWNDC;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBAPP));
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = WINDOW_CLASS;

    RegisterClass(&wndclass);
}

/**
*  This method will initialize the data object.
*
*  @return         No return value.
*/
bool CDlgApp::InitializeData()
{
    // Determine if we should use Direct Animaiton to display our intro graphics.
    // We don't use DA on slow machines, machines with less than 256 color displays,
    // and hydra terminals.  For everything else we use DA.
    HWND hwnd = GetDesktopWindow();
    HDC hdc = GetDC( hwnd );
    m_iColors = GetDeviceCaps( hdc, NUMCOLORS );
    m_bLowColor = ((m_iColors != -1) && (m_iColors <= 256));
    if ( m_bLowColor )
    {
        m_hpal = CreateHalftonePalette(hdc);
    }
    ReleaseDC( hwnd, hdc );

    // Initialize the items from the INI file.
    if ( !m_DataSrc.Init() )
    {
        // this is a sign from the data source that we should exit
        return false;
    }

    // Are we in accesibility mode?  This call won't work on NT 4.0 because this flag wasn't known.
    HIGHCONTRAST hc;
    hc.cbSize = sizeof(HIGHCONTRAST);
    hc.dwFlags = 0; // avoid random result should SPI fail
    if ( SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0 ) )
    {
        m_bHighContrast = ( hc.dwFlags & HCF_HIGHCONTRASTON );
    }
    else
    {
        // we must be on NT 4.0 or below.  Just assume we aren't in high contrast mode.
        ASSERT( false == m_bHighContrast );
    }

    // Set the color table based on our HighContrast mode setting.
    SetColorTable();

    // create the fonts that we need to use.
    CreateWelcomeFonts(hdc);

    // create the image for the top region
    CreateBrandingBanner();

    // we pre load the background images so they draw more quickly.
    LoadBkgndImages();

    // load the resource strings that we always need
    LoadString( m_hInstance, IDS_DEFTITLE, m_szDefTitle, ARRAYSIZE(m_szDefTitle) );
    LoadString( m_hInstance, IDS_DEFBODY, m_szDefBody, ARRAYSIZE(m_szDefBody) );

    m_hcurHand = LoadCursor( m_hInstance, MAKEINTRESOURCE(IDC_BRHAND) );

    return true;
}

BOOL CDlgApp::SetColorTable()
{
    if ( m_bHighContrast )
    {
        // set to high contrast values
        m_hbrMenuItem   = (HBRUSH)(COLOR_BTNFACE+1);
        m_hbrMenuBorder = (HBRUSH)(COLOR_BTNSHADOW+1);
        m_hbrRightPanel = (HBRUSH)(COLOR_WINDOW+1);

        m_crMenuText    = GetSysColor(COLOR_BTNTEXT);
        m_crNormalText  = GetSysColor(COLOR_WINDOWTEXT);
        m_crTitleText   = m_crNormalText;
        m_crSelectedText= GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
        m_crMenuText    = RGB(0,0,0);
        m_crNormalText  = RGB(0,0,0);
        m_crSelectedText= RGB(0x80, 0x80, 0x80);    // default value for COLOR_GRAYTEXTs
        m_crTitleText   = RGB(51,102,153);

        m_hbrRightPanel = (HBRUSH)GetStockObject( WHITE_BRUSH );

        if ( m_bLowColor )
        {
            if (m_iColors <= 16)
            {
                // Set to colors that work well in 16 color mode.
                HBITMAP hbmp;
                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_16MENU), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_hbrMenuItem   = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);

                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_16BORDER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_hbrMenuBorder = CreatePatternBrush( hbmp );
                DeleteObject(hbmp);
//
//                if ( WeAreRunningOnWin95 )
//                    m_crMenuText    = RGB(255,255,255);
            }
            else
            {
                // Set to colors that work well in 256 color mode.  Use colors from the halftone palette.
                HBITMAP hbmp;
                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_256MENU), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_hbrMenuItem   = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);

                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_256BORDER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_hbrMenuBorder = CreatePatternBrush( hbmp );
                DeleteObject(hbmp);
            }
        }
        else
        {
            m_hbrMenuItem   = CreateSolidBrush( RGB(166,202,240) );
            m_hbrMenuBorder = CreateSolidBrush( m_crTitleText );
        }
    }

    return TRUE;
}


BOOL CDlgApp::CreateWelcomeFonts(HDC hdc)
{
    LOGFONT lf;
    CHARSETINFO csInfo;
    TCHAR szFontSize[6];

    memset(&lf,0,sizeof(lf));
    lf.lfWeight = FW_BOLD;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH|FF_SWISS;
    LoadString( m_hInstance, IDS_FONTFACE, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName) );

    // Set charset
    if (TranslateCharsetInfo((DWORD*)GetACP(), &csInfo, TCI_SRCCODEPAGE) == 0)
    {
        csInfo.ciCharset = 0;
    }
    lf.lfCharSet = (BYTE)csInfo.ciCharset;

    // TODO:  If user has accesibility large fonts turned on then scale the font sizes.

    LoadString( m_hInstance, IDS_CYTITLEFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -_ttoi(szFontSize);
    m_hfontTitle = CreateFontIndirect(&lf);

    LoadString( m_hInstance, IDS_CYMENUITEMFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -_ttoi(szFontSize);
    m_hfontMenu  = CreateFontIndirect(&lf);

    lf.lfWeight = FW_NORMAL;
    LoadString( m_hInstance, IDS_CYBODYFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -_ttoi(szFontSize);
    m_hfontBody  = CreateFontIndirect(&lf);

    return TRUE;
}

BOOL CDlgApp::CreateBrandingBanner()
{
    HBITMAP hbm;
    int iBitmap;
    m_hdcTop = CreateCompatibleDC(NULL);
    if ( m_bLowColor && (m_iColors <= 16) )
    {
        iBitmap = IDB_BANNER16;
    }
    else
    {
        iBitmap = IDB_BANNER;
    }

    hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(iBitmap), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
    SelectObject( m_hdcTop, hbm );

    return TRUE;
}

BOOL CDlgApp::LoadBkgndImages()
{
    BITMAP bm;

    for (int i=0; i<4; i++)
    {
        m_aBkgnd[i].hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_BKGND0+i), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
        // REVIEW: are all these the same size?  If yes, skip this part and use a constant:
        GetObject(m_aBkgnd[i].hbm,sizeof(bm),&bm);
        m_aBkgnd[i].cx = bm.bmWidth;
        m_aBkgnd[i].cy = bm.bmHeight;
    }

    return TRUE;
}

BOOL CDlgApp::AdjustToFitFonts(DWORD dwStyle)
{
    RECT rect;
    int cyLowestBodyPoint = 0;
    HDC hdc = GetDC(m_hwnd);

    // now based on the users prefered font size we allow these sizes to adjust slightly

    HFONT hfontOld = (HFONT)SelectObject(hdc,m_hfontTitle);
    int iMenuItemTop = c_cyBranding;
    for (int i=0; i<m_DataSrc.m_iItems; i++ )
    {
        rect.left = m_cxLeftPanel+c_cxRightPanelPadding;
        rect.top = c_cyBranding + c_cyBarToTitlePadding;
        rect.right = m_cxClient-c_cxRightPanelPadding;
        SelectObject(hdc,m_hfontTitle);
        DrawText(hdc,m_DataSrc[i].GetTitle(),-1,&rect,DT_CALCRECT|DT_WORDBREAK); // this computes rcLargestTitle.bottom

        rect.left = m_cxLeftPanel+c_cxRightPanelPadding;
        rect.top = rect.bottom + c_cyTitleToBodyPadding;
        rect.right = m_cxClient-c_cxRightPanelPadding;
        SelectObject(hdc,m_hfontBody);
        DrawText(hdc,m_DataSrc[i].GetDescription(),-1,&rect,DT_CALCRECT|DT_WORDBREAK); // this computes rcLargestBody.bottom
        if ( rect.bottom > cyLowestBodyPoint )
            cyLowestBodyPoint = rect.bottom;

        rect.left = c_cxMenuItemPadding;
        rect.top = iMenuItemTop+c_cyMenuItemPadding;
        rect.right = m_cxLeftPanel-c_cxMenuItemPadding;
        SelectObject(hdc,m_hfontMenu);
        DrawText(hdc,m_DataSrc[i].GetMenuName(),-1,&rect,DT_CALCRECT|DT_WORDBREAK);

        HWND hwnd;
        hwnd = GetDlgItem(m_hwnd, IDM_MENUITEM1+i);
        SetWindowPos(
                hwnd,
                NULL,
                0,
                iMenuItemTop,
                m_cxLeftPanel,
                rect.bottom + c_cyMenuItemPadding + 1 + c_cyMenuItemSpacing - iMenuItemTop,   // +1 to improve centering (due to drop letters)
                SWP_NOZORDER );

        iMenuItemTop = rect.bottom + c_cyMenuItemPadding + 1 + c_cyMenuItemSpacing;
    }

    // store the bottom most menu point.  Needed for drawing the background rect later.
    m_cyBottomOfMenuItems = iMenuItemTop;

    // we now adjust the height of our window to accomadate the largest item.
    if ( cyLowestBodyPoint + c_cyBodyToBottomPadding > m_cyClient )
        m_cyClient = cyLowestBodyPoint + c_cyBodyToBottomPadding;

    // if the menu items plus the check box are too tall, make the dialog taller
    if ( iMenuItemTop + c_cyCheckTextToBottomPadding > m_cyClient )
        m_cyClient = iMenuItemTop + c_cyCheckTextToBottomPadding;

    // restore the DC to its original value
    SelectObject(hdc,hfontOld);

    //  set the client area to a fixed size and center the window on screen
    rect.left = 0;
    rect.top = 0;
    rect.right = m_cxClient;
    rect.bottom = m_cyClient;

    AdjustWindowRect( &rect, dwStyle, FALSE );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    RECT rcDesktop;
    SystemParametersInfo(SPI_GETWORKAREA,0, &rcDesktop, FALSE);
    rect.left = (rcDesktop.left+rcDesktop.right-rect.right)/2;
    rect.top = (rcDesktop.top+rcDesktop.bottom-rect.bottom)/2;

    SetWindowPos(m_hwnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, 0);

    return TRUE;
}

/**
*  This method will create the application window.
*
*  @return         No return value.
*/
void CDlgApp::Create(int nCmdShow)
{
    //
    //  load the window title from the resource.
    //
    TCHAR szTitle[MAX_PATH];
    LoadString(m_hInstance, IDS_TITLE, szTitle, MAX_PATH);

    const DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_CLIPCHILDREN;
    
    m_hwnd = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            WINDOW_CLASS,
            szTitle,
            dwStyle,
            0,
            0,
            0,
            0,
            NULL,
            NULL,
            m_hInstance,
            this);

    // We created the window with zero size, now we adjust that size to take into
    // acount the selected font size, etc.
    AdjustToFitFonts(dwStyle);

    ShowWindow(m_hwnd, nCmdShow);

    // give our data source the opertunity to display a startup sequence
    m_DataSrc.ShowSplashScreen( m_hwnd );

    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);
}

/**
*  This method is our application message loop.
*
*  @return         No return value.
*/
void CDlgApp::MessageLoop()
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // IsDialogMessage cannot understand the concept of ownerdraw default pushbuttons.  It treats
        // these attributes as mutually exclusive.  As a result, we handle this ourselves.  We want
        // whatever control has focus to act as the default pushbutton.
        if ( (WM_KEYDOWN == msg.message) && (VK_RETURN == msg.wParam) )
        {
            HWND hwndFocus = GetFocus();
            if ( hwndFocus )
            {
                SendMessage(m_hwnd, WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndFocus), BN_CLICKED), (LPARAM)hwndFocus);
            }
            continue;
        }

        if ( IsDialogMessage(m_hwnd, &msg) )
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
LRESULT CALLBACK CDlgApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CDlgApp *web = (CDlgApp *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
    case WM_NCCREATE:
        web = (CDlgApp *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LRESULT)web);
        break;

    case WM_CREATE:
        return web->OnCreate(hwnd);

    case WM_DESTROY:
        return web->OnDestroy();

    case WM_ACTIVATE:
        return web->OnActivate(wParam);

    case WM_PAINT:
        return web->OnPaint((HDC)wParam);

    case WM_ERASEBKGND:
        return web->OnEraseBkgnd((HDC)wParam);

    case WM_MOUSEMOVE:
        return web->OnMouseMove(LOWORD(lParam), HIWORD(lParam), (DWORD)wParam);

    case WM_SETCURSOR:
        return web->OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_COMMAND:
    case WM_SYSCOMMAND:
        if ( web->OnCommand(LOWORD(wParam)) )
            return 0;
        break;

    case WM_DRAWITEM:
        return web->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);

    case WM_QUERYNEWPALETTE:
        return web->OnQueryNewPalette();

    case WM_PALETTECHANGED:
        return web->OnPaletteChanged((HWND)wParam);
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
LRESULT CDlgApp::OnCreate(HWND hwnd)
{
    int i;
    m_hwnd = hwnd;

    // Create one window for each button.  These windows will get resized and moved
    // after we call AdjustToFitFonts.
    for (i=0; i<m_DataSrc.m_iItems; i++)
    {
        hwnd = CreateWindowEx(
                0,
                TEXT("BUTTON"),
                m_DataSrc.m_data[i].GetMenuName(),
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_MULTILINE|BS_OWNERDRAW,
                0,0,0,0,
                m_hwnd,
                NULL,
                m_hInstance,
                NULL );

        SetWindowLongPtr(hwnd, GWLP_ID, IDM_MENUITEM1 + i);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfontMenu, 0);
        g_fnBtnProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ButtonWndProc);
    }
    hwnd = GetDlgItem(m_hwnd,IDM_MENUITEM4);
    SetFocus(hwnd);

    // Create two static text controls, one for the title and one for the body.  The
    // only purpose these serve is to allow screen readers to read the correct text.

    return 0;
}

/**
*  This method handles the WM_DESTROY message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnDestroy()
{
    // Shutdown the data source.
    m_DataSrc.Uninit(0);

    // ensure this is the last message we care about
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
    
    PostQuitMessage(0);

    return 0;
}

LRESULT CDlgApp::OnActivate(WPARAM wParam)
{
    // Note: we are actually checking two things, the HIWORD(wParam) must be zero (i.e. window not minimized)
    // and the LOWORD(wParam) must be one of the following two values (i.e. window being activated):
    if ( WA_ACTIVE == wParam || WA_CLICKACTIVE == wParam)
    {
        HWND hwnd;
        hwnd = GetDlgItem(m_hwnd,IDM_MENUITEM4);
        SetFocus(hwnd);
    }
    return 0;
}

/**
*  This method handles the WM_PAINT message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnPaint(HDC hdc)
{
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd,&ps);
    EndPaint(m_hwnd,&ps);

    return 0;
}

// winver 0x0500 definition
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP            (DWORD)0x80000000
#endif // NOMIRRORBITMAP
#ifndef LAYOUT_RTL
#define LAYOUT_RTL                              0x00000001 // Right to left
#endif // LAYOUT_RTL
/**
*  This method handles the WM_ERASEBKGND message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnEraseBkgnd(HDC hdc)
{
    RECT rect;
    HPALETTE hpalOld = NULL;

    if ( m_hpal )
    {
        hpalOld = SelectPalette(hdc, m_hpal, FALSE);
        RealizePalette(hdc);
    }

    SetMapMode(hdc, MM_TEXT);
    SetBkMode(hdc, TRANSPARENT);

    // Draw the branding area:
    DWORD dwRop  = SRCCOPY;
    if(Mirror_GetLayout(hdc) & LAYOUT_RTL)
    {
        dwRop |= NOMIRRORBITMAP;
    }
    BitBlt( hdc,0,0,m_cxClient,c_cyBranding, m_hdcTop,0,0, dwRop );

    // Draw the left pane:
    // fill rect for background below menu items
    rect.left = 0;
    rect.top = m_cyBottomOfMenuItems;
    rect.right = m_cxLeftPanel;
    rect.bottom = m_cyClient;
    FillRect(hdc, &rect, m_hbrMenuItem);

    // Draw the right pane:
    // fill right pane's background
    rect.left = m_cxLeftPanel;
    rect.top = c_cyBranding;
    rect.right = m_cxClient;
    rect.bottom = m_cyClient;
    FillRect(hdc, &rect, m_hbrRightPanel);

    // draw background image
    if ( !m_bHighContrast )
    {
        int iImgIndex;

        if ( -1 == g_iSelectedItem )
        {
            iImgIndex = 0;
        }
        else
        {
            iImgIndex = m_DataSrc.m_data[g_iSelectedItem].GetImgIndex();
        }

        HDC hdcBkgnd = CreateCompatibleDC(hdc);
        Mirror_SetLayout(hdcBkgnd, 0);        
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBkgnd, m_aBkgnd[iImgIndex].hbm);
        BitBlt( hdc,
                m_cxClient-m_aBkgnd[iImgIndex].cx,
                m_cyClient-m_aBkgnd[iImgIndex].cy,
                m_aBkgnd[iImgIndex].cx,
                m_aBkgnd[iImgIndex].cy,
                hdcBkgnd,0,0, SRCCOPY );
        SelectObject(hdcBkgnd,hbmOld);
        DeleteDC(hdcBkgnd);
    }

    // draw title text
    rect.top = c_cyBranding + c_cyBarToTitlePadding;
    rect.left = m_cxLeftPanel + c_cxRightPanelPadding;
    rect.right = m_cxClient - c_cxRightPanelPadding;
    rect.bottom = m_cyClient;
    HFONT hfontOld = (HFONT)SelectObject(hdc,m_hfontTitle);
    SetTextColor(hdc,m_crTitleText);
    rect.top += c_cyTitleToBodyPadding +
        DrawText(hdc,((-1==g_iSelectedItem)?m_szDefTitle:m_DataSrc[g_iSelectedItem].GetTitle()),-1,&rect,DT_NOCLIP|DT_WORDBREAK);

    // draw body text
    SelectObject(hdc,m_hfontBody);
    SetTextColor(hdc,m_crNormalText);
    DrawText(hdc,((-1==g_iSelectedItem)?m_szDefBody:m_DataSrc[g_iSelectedItem].GetDescription()),-1,&rect,DT_NOCLIP|DT_WORDBREAK);

    // restore the DC to its original value
    SelectObject(hdc,hfontOld);
    if(hpalOld)
        SelectPalette(hdc, hpalOld, FALSE);

    return TRUE;
}

LRESULT CDlgApp::OnMouseMove(int x, int y, DWORD fwKeys)
{
    // if a task is running then we leave the menu item for that task selected until that
    // task finishes running instead of doing the following logic.
    if ( !g_bTaskRunning )
    {
        // Didn't move over one of our menu items, select the default text.
        if (-1 != g_iSelectedItem)
        {
            g_iSelectedItem = -1;
            HWND hwnd = GetDlgItem(m_hwnd,IDM_MENUITEM4);
            SetFocus(hwnd);

            InvalidateRect(m_hwnd, NULL, TRUE);
        }
    }

    return 0;
}

LRESULT CDlgApp::OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg)
{
    if ( !g_bTaskRunning )
    {
        if ( hwnd != m_hwnd )
        {
            SetCursor(m_hcurHand);
            return TRUE;
        }
    }

    SetCursor(LoadCursor(NULL,IDC_ARROW));
    return TRUE;
}

LRESULT CDlgApp::OnCommand(int wID)
{
    int iNewSelectedItem = g_iSelectedItem;
    bool bRun = false;

    switch(wID)
    {
    case IDM_MENUITEM1:
    case IDM_MENUITEM2:
    case IDM_MENUITEM3:
    case IDM_MENUITEM4:
    case IDM_MENUITEM5:
    case IDM_MENUITEM6:
    case IDM_MENUITEM7:
        bRun = true;
        g_iSelectedItem = wID - IDM_MENUITEM1;
        // g_iSelectedItem should be a real menu item now, but just to make sure:
        ASSERT( (g_iSelectedItem < m_DataSrc.m_iItems) && (g_iSelectedItem >= 0) );
        break;

    default:
        // When we hit this then this isn't a message we care about.  We return FALSE which
        // tells our WndProc to call DefWndProc which makes everything happy.
        return FALSE;
    }

    if ( !g_bTaskRunning )
    {
        if ( iNewSelectedItem != g_iSelectedItem )
        {
            InvalidateRect(m_hwnd, NULL, TRUE);
        }

        if ( bRun )
        {
            g_bTaskRunning = TRUE;
            m_DataSrc.Invoke( g_iSelectedItem, m_hwnd );
            g_bTaskRunning = FALSE;
        }
    }
    else
    {
        // currently the only commands that are valid while another task is running are
        // IDM_SHOWCHECK and anything that goes to the default handler above.  Everything
        // else will come to here and cause a message beep
        MessageBeep(0);
    }

    return TRUE;
}

LRESULT CDlgApp::OnQueryNewPalette()
{
    if ( m_hpal )
    {
        HDC hdc = GetDC(m_hwnd);
        HPALETTE hpalOld = SelectPalette(hdc, m_hpal, FALSE);
        UnrealizeObject(m_hpal);
        RealizePalette(hdc);
        InvalidateRect(m_hwnd, NULL, TRUE);
        UpdateWindow(m_hwnd);
        if(hpalOld)
            SelectPalette(hdc, hpalOld, FALSE);
        ReleaseDC(m_hwnd, hdc);
        return TRUE;
    }
    return FALSE;
}

LRESULT CDlgApp::OnPaletteChanged(HWND hwnd)
{
    if ( m_hpal && (m_hwnd != hwnd) )
    {
        HDC hdc = GetDC(m_hwnd);
        HPALETTE hpalOld = SelectPalette(hdc, m_hpal, FALSE);
        RealizePalette(hdc);
        UpdateColors(hdc);
        if (hpalOld)
            SelectPalette(hdc, hpalOld, FALSE);
        ReleaseDC(m_hwnd, hdc);
    }
    return TRUE;
}

LRESULT CDlgApp::OnDrawItem(UINT iCtlID, LPDRAWITEMSTRUCT pdis)
{
    RECT rect = pdis->rcItem;
    int i = iCtlID - IDM_MENUITEM1;
    HPALETTE hpalOld = NULL;

    ASSERT( (i < m_DataSrc.m_iItems) && (i >= 0) );

    if ( m_hpal )
    {
        hpalOld = SelectPalette(pdis->hDC, m_hpal, FALSE);
        RealizePalette(pdis->hDC);
    }

    rect.bottom -= c_cyMenuItemSpacing;

    FillRect( pdis->hDC, &rect, (pdis->itemState & ODS_FOCUS)?m_hbrRightPanel:m_hbrMenuItem );
    
    rect.top = rect.bottom;
    rect.bottom += c_cyMenuItemSpacing;
    FillRect( pdis->hDC, &rect, m_hbrMenuBorder );

    rect.top = pdis->rcItem.top;

    // draw menu item text
    rect.left += c_cxMenuItemPadding;
    rect.top += c_cyMenuItemPadding;
    rect.right -= c_cxMenuItemPadding;

    SetBkMode(pdis->hDC, TRANSPARENT);
    SetTextColor(
            pdis->hDC,
            ((m_DataSrc[i].m_dwFlags&WF_ALTERNATECOLOR)?m_crSelectedText:
            ((pdis->itemState & ODS_FOCUS)?m_crNormalText:m_crMenuText)));

    DrawText(pdis->hDC,m_DataSrc[i].GetMenuName(),-1,&rect,DT_NOCLIP|DT_WORDBREAK);

    if ( pdis->itemState & ODS_FOCUS )
    {
        if ( m_bHighContrast )
        {
            rect.left -= 1;
            rect.top -= 2;
            rect.right += 1;
            rect.bottom -= 2;
            DrawFocusRect(pdis->hDC,&rect);
        }
    }

    if ( hpalOld )
    {
        SelectPalette(pdis->hDC, hpalOld, FALSE);
    }

    return TRUE;
}

LONG_PTR CALLBACK ButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDlgApp *web = (CDlgApp *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
        if ( !g_bTaskRunning )
        {
            int iID = ((int)GetWindowLongPtr(hwnd, GWLP_ID)) - IDM_MENUITEM1;
            
            if ( iID != g_iSelectedItem )
            {
                SetFocus(hwnd);
            }
        }
        break;

    case WM_SETFOCUS:
        if ( !g_bTaskRunning )
        {
            int iID = ((int)GetWindowLongPtr(hwnd, GWLP_ID)) - IDM_MENUITEM1;
            
            if ( iID != g_iSelectedItem )
            {
                g_iSelectedItem = iID;

                InvalidateRect(GetParent(hwnd), NULL, TRUE);

                ASSERT( (g_iSelectedItem < 7) && (g_iSelectedItem >= 0) );
            }
        }
        break;
    }

    return CallWindowProc(g_fnBtnProc, hwnd, uMsg, wParam, lParam);
}
