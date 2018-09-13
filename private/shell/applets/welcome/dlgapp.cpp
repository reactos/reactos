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
#include <windowsx.h>   // for windows macros
#include <commctrl.h>
#include <shlwapi.h>    // for string compare functions
#include <shlobj.h>
#include <debug.h>

#pragma hdrstop

#include "dlgapp.h"
#include "resource.h"

#define WINDOW_CLASS    TEXT("_BerksWin2kWelcomeApp_")
#define WELCOMETIPTIMERID   100


//////////////////////////////////////////////////////////////////////////
// Global Data
//////////////////////////////////////////////////////////////////////////

bool        g_bTaskRunning = false;     // true when we have a running task open
int         g_iSelectedItem = -1;       // 
WNDPROC     g_fnBtnProc = NULL;         // the window proc for a button.
LPDRAWINFO  g_pdi = NULL;               // Info needed to draw the window

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
#define c_cyBodyToBottomPadding         5   // vertical padding from body of body text to top of exit button
#define c_cxRightPanelPadding           16  // generic horizontal padding used on both edges of the right pane

#define c_cyCheckTextToBottomPadding    8   // vertical padding from bottom of "Show at startup" text to bottom of client area

#define c_cxCheckBox                    13
#define c_cyCheckBox                    12

#define c_cxExitToRightEdge             8   // padding from exit button to right edge of client
#define c_cyExitToBottomEdge            8   // padding from exit button to bottom edge of client

#define c_cxMinExitWidth                43  // the smallest width we set the exit button to
#define c_cyMinExitHeight               20  // the smallest height we set the exit button to

#define c_cxExitButtonPadding           8   // 1 pixel border + 3 pixels room for focus rect on each side of the text
#define c_cyExitButtonPadding           8   // same spacing as above


// Code to ensure only one instance of a particular window is running
HANDLE CheckForOtherInstance(HINSTANCE hInstance)
{
    TCHAR   szCaption[128];
    HANDLE  hMutex;

    LoadString(hInstance, IDS_TITLE, szCaption, 128);

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

        HWND hwnd = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
        if ( hwnd )
        {
            UINT uMsg = RegisterWindowMessage(TEXT("Welcome Finished"));

            PostMessage(hwnd, uMsg, 0,0);
        }
    }
    return 0;
}

/**
*  This method is our contstructor for our class. It initialize all
*  of the instance data.
*/
CDlgApp::CDlgApp()
{
    m_hInstance     = NULL;
    m_hwnd          = NULL;

    m_DI.bHighContrast = false;

    m_DI.hfontTitle = NULL;
    m_DI.hfontMenu  = NULL;
    m_DI.hfontBody  = NULL;
    m_DI.hfontCheck = NULL;

    m_DI.hbrMenuItem   = NULL;
    m_DI.hbrMenuBorder = NULL;
    m_DI.hbrRightPanel = NULL;

    m_DI.bLowColor = false;
    m_DI.iColors = -1;
    m_DI.hpal = NULL;

    g_pdi = &m_DI;

    // In theory, all of these metrics could be adjusted to resize the window.  Resizing wouldn't
    // effect the paddings and spacings so these are defined above as constants.  In the real
    // world we are only resizing vertically to adjust for oversized content.  These are more to
    // allow future expansion.
    m_cxClient = 478;       // width of the client area
    m_cyClient = 322;       // This is currently the only metirc we actually adjust.
    m_cxLeftPanel = 179;    // width of the panel that contains the menu items.
    m_hdcTop = NULL;
    m_hcurHand = NULL;
}

CDlgApp::~CDlgApp()
{
    DeleteObject(m_DI.hfontTitle);
    DeleteObject(m_DI.hfontMenu);
    DeleteObject(m_DI.hfontBody);
    DeleteObject(m_DI.hfontCheck);
#ifndef WINNT
    DeleteObject(m_DI.hfontLogo);
#endif

    DeleteObject(m_DI.hbrMenuItem);
    DeleteObject(m_DI.hbrMenuBorder);
    DeleteObject(m_DI.hbrRightPanel);

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
    WNDCLASSEX  wndclass;

    m_hInstance = hInstance;
    
    wndclass.cbSize         = sizeof(wndclass);
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
    wndclass.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBAPP));
    
    RegisterClassEx(&wndclass);
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
    m_DI.iColors = GetDeviceCaps( hdc, NUMCOLORS );
    m_DI.bLowColor = ((m_DI.iColors != -1) && (m_DI.iColors <= 256));
    if ( m_DI.bLowColor )
    {
        m_DI.hpal = CreateHalftonePalette(hdc);
    }
    ReleaseDC( hwnd, hdc );

    // Are we in accesibility mode?  This call won't work on NT 4.0 because this flag wasn't known.
    HIGHCONTRAST hc;
    hc.cbSize = sizeof(HIGHCONTRAST);
    hc.dwFlags = 0; // avoid random result should SPI fail
    if ( SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0 ) )
    {
        m_DI.bHighContrast = ( hc.dwFlags & HCF_HIGHCONTRASTON );
    }
    else
    {
        // we must be on NT 4.0.  Just assume we aren't in high contrast mode.
        ASSERT( false == m_DI.bHighContrast );
    }

#ifdef WINNT
    m_DI.bTerminalServer = GetSystemMetrics(SM_REMOTESESSION)?true:false;
#endif

    // Initialize the items from the INI file.
    if ( !m_DataSrc.Init() )
    {
        // this is a sign from the data source that we should exit
        return false;
    }

    if ( !m_DefItem.Init() )
    {
        // This can happen in low memory conditions
        return false;
    }

    // Set the color table based on our HighContrast mode setting.
    SetColorTable();

    // create the fonts that we need to use.
    CreateWelcomeFonts(hdc);

    // create the image for the top region
    CreateBrandingBanner();

    // load the resource strings that we always need
    LoadString( m_hInstance, IDS_CHECKTEXT, m_szCheckText, ARRAYSIZE(m_szCheckText) );
    LoadString( m_hInstance, IDS_EXIT, m_szExit, ARRAYSIZE(m_szExit) );
#ifndef WINNT
    LoadString( m_hInstance, IDS_LOGOTEXT, m_szLogo, ARRAYSIZE(m_szLogo));
#endif

    m_hcurHand = LoadCursor( m_hInstance, MAKEINTRESOURCE(IDC_BRHAND) );

    return true;
}

BOOL CDlgApp::SetColorTable()
{
    if ( m_DI.bHighContrast )
    {
        // set to high contrast values
        m_DI.hbrMenuItem   = (HBRUSH)(COLOR_BTNFACE+1);
        m_DI.hbrMenuBorder = (HBRUSH)(COLOR_BTNSHADOW+1);
        m_DI.hbrRightPanel = (HBRUSH)(COLOR_WINDOW+1);

        m_DI.crNormalText  = GetSysColor(COLOR_WINDOWTEXT);
        m_DI.crTitleText   = m_DI.crNormalText;
        m_DI.crSelectedText= GetSysColor(COLOR_GRAYTEXT);
    }
    else
    {
        m_DI.crNormalText  = RGB(0,0,0);
        m_DI.crSelectedText= RGB(0x80, 0x80, 0x80); // default value for COLOR_GRAYTEXT
        m_DI.crTitleText   = RGB(51,102,153);

        m_DI.hbrRightPanel = (HBRUSH)GetStockObject( WHITE_BRUSH );

        if ( m_DI.bLowColor )
        {
            if (m_DI.iColors <= 16)
            {
                // Set to colors that work well in 16 color mode.
                HBITMAP hbmp;
                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_16MENU), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_DI.hbrMenuItem   = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);

                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_16BORDER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_DI.hbrMenuBorder = CreatePatternBrush( hbmp );
                DeleteObject(hbmp);
            }
            else
            {
                // Set to colors that work well in 256 color mode.  Use colors from the halftone palette.
                HBITMAP hbmp;
                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_256MENU), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_DI.hbrMenuItem   = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);

                hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_256BORDER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
                m_DI.hbrMenuBorder = CreatePatternBrush( hbmp );
                DeleteObject(hbmp);
            }
        }
        else
        {
            m_DI.hbrMenuItem   = CreateSolidBrush( RGB(166,202,240) );
            m_DI.hbrMenuBorder = CreateSolidBrush( m_DI.crTitleText );
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
    lf.lfHeight  = -StrToInt(szFontSize);
    m_DI.hfontTitle = CreateFontIndirect(&lf);

    LoadString( m_hInstance, IDS_CYMENUITEMFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -StrToInt(szFontSize);
    m_DI.hfontMenu  = CreateFontIndirect(&lf);

    lf.lfWeight = FW_NORMAL;
    LoadString( m_hInstance, IDS_CYBODYFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -StrToInt(szFontSize);
    m_DI.hfontBody  = CreateFontIndirect(&lf);

    LoadString( m_hInstance, IDS_CYCHECKTEST, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -StrToInt(szFontSize);
    m_DI.hfontCheck = CreateFontIndirect(&lf);

#ifndef WINNT
    LoadString( m_hInstance, IDS_CYLOGOFONT, szFontSize, ARRAYSIZE(szFontSize) );
    lf.lfHeight  = -StrToInt(szFontSize);
    m_DI.hfontLogo = CreateFontIndirect(&lf);
#endif

    return TRUE;
}

BOOL CDlgApp::CreateBrandingBanner()
{
    HBITMAP hbm;
    int iBitmap;
    m_hdcTop = CreateCompatibleDC(NULL);
    if ( m_DI.bLowColor && (m_DI.iColors <= 16) )
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

BOOL CDlgApp::AdjustToFitFonts(DWORD dwStyle)
{
    RECT rect;
    RECT rcExit = {0};
    int cyLowestBodyPoint = 0;
    int cyCheckTextHeight = 0;
    HDC hdc = GetDC(m_hwnd);

    // now based on the users prefered font size we allow these sizes to adjust slightly

    // determine the space required for the exit button
    HFONT hfontOld = (HFONT)SelectObject(hdc,m_DI.hfontMenu);
    DrawTextEx(hdc,m_szExit,-1,&rcExit,DT_CALCRECT|DT_SINGLELINE,NULL);

    // we pad the room needed for the text so that we can draw the border and a focus rect
    rcExit.right += c_cxExitButtonPadding;
    rcExit.bottom += c_cyExitButtonPadding;

    // we have a minimum size that we never make the button smaller than just to make sure it's visible
    if ( rcExit.right < c_cxMinExitWidth)
        rcExit.right = c_cxMinExitWidth;
    if ( rcExit.bottom < c_cyMinExitHeight)
        rcExit.bottom = c_cyMinExitHeight;

    // We can't initialize the default item until after we create the window.
    cyLowestBodyPoint = m_DefItem.GetMinHeight(hdc, m_cxClient-m_cxLeftPanel, c_cyBodyToBottomPadding+rcExit.bottom+c_cyExitToBottomEdge ) + c_cyBranding;
    int iMenuItemTop = c_cyBranding;
    for (int i=0; i<m_DataSrc.m_iItems; i++ )
    {
        rect.bottom = m_DataSrc[i].GetMinHeight(hdc, m_cxClient-m_cxLeftPanel, c_cyBodyToBottomPadding+rcExit.bottom+c_cyExitToBottomEdge ) + c_cyBranding;
        if ( rect.bottom > cyLowestBodyPoint )
            cyLowestBodyPoint = rect.bottom;

        rect.left = c_cxMenuItemPadding;
        rect.top = iMenuItemTop+c_cyMenuItemPadding;
        rect.right = m_cxLeftPanel-c_cxMenuItemPadding;
        SelectObject(hdc,m_DI.hfontMenu);
        DrawTextEx(hdc,m_DataSrc[i].GetMenuName(),-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);

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
    if ( cyLowestBodyPoint > m_cyClient )
        m_cyClient = cyLowestBodyPoint;

    // See how big the checkbox text is given our calculated width
    rect.left = c_cxCheckTextLeftEdge;
    rect.top = 0;
    rect.right = m_cxLeftPanel; // this can go right up to the edge.
    SelectObject(hdc,m_DI.hfontCheck);
    DrawTextEx(hdc,m_szCheckText,-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);
    cyCheckTextHeight = rect.bottom + c_cyCheckTextToBottomPadding;

    // if the menu items plus the check box are too tall, make the dialog taller
    if ( iMenuItemTop + cyCheckTextHeight + c_cyCheckTextToBottomPadding > m_cyClient )
        m_cyClient = iMenuItemTop + cyCheckTextHeight + c_cyCheckTextToBottomPadding;

    // Now possition the exit button
    SetWindowPos(
            GetDlgItem(m_hwnd, IDM_EXIT),
            NULL,
            m_cxClient-(rcExit.right+c_cxExitToRightEdge),
            m_cyClient-(rcExit.bottom+c_cyExitToBottomEdge),
            rcExit.right,
            rcExit.bottom,
            SWP_NOZORDER);

    // Now possition the checkbox based on the window height
    SetWindowPos(
            m_hwndCheck,
            NULL,
            ((c_cxCheckTextLeftEdge-c_cxCheckBox)/2),
            m_cyClient-cyCheckTextHeight,
            m_cxLeftPanel-((c_cxCheckTextLeftEdge-c_cxCheckBox)/2),
            cyCheckTextHeight-c_cyCheckTextToBottomPadding+2,
            SWP_NOZORDER );

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

    SetWindowPos(m_hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right, rect.bottom, 0);

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

    // We don't want to show the same item twice in a row so do a Next right away.
    // We must do the Next before we show the window because in the uninitialized
    // case the value of m_DefItem.m_iCurItem will be -1, which won't draw correctly.
    m_DefItem.Next(m_hwnd);

    ShowWindow(m_hwnd, nCmdShow);

    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);
    SetTimer(m_hwnd, WELCOMETIPTIMERID, m_DefItem.m_uRotateSpeed, NULL);
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

    case WM_NCDESTROY:
        // ensure this is the last message we care about
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        break;

    case WM_PAINT:
        return web->OnPaint((HDC)wParam);

    case WM_ERASEBKGND:
        return web->OnEraseBkgnd((HDC)wParam);

    case WM_MOUSEMOVE:
        return web->OnMouseMove(LOWORD(lParam), HIWORD(lParam), (DWORD)wParam);

    case WM_SETCURSOR:
        return web->OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_LBUTTONDOWN:
        return web->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);

    case WM_COMMAND:
    case WM_SYSCOMMAND:
        if ( web->OnCommand(LOWORD(wParam)) )
            return 0;
        break;

    case WM_CTLCOLORSTATIC:
        if ( (HWND)lParam == web->m_hwndCheck ) 
        {
            if ( web->m_DI.hpal )
            {
                SelectPalette((HDC)wParam, web->m_DI.hpal, FALSE);
                RealizePalette((HDC)wParam);
            }
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)web->m_DI.hbrMenuItem;
        }
        else
        {
            return (LRESULT)web->m_DI.hbrRightPanel;
        }
        break;

    case WM_DRAWITEM:
        return web->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);

    case WM_QUERYNEWPALETTE:
        return web->OnQueryNewPalette();

    case WM_PALETTECHANGED:
        return web->OnPaletteChanged((HWND)wParam);

    case WM_TIMER:
        web->m_DefItem.Next(hwnd);
        break;

    case WM_ACTIVATE:
        return web->OnActivate(LOWORD(wParam));
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
LRESULT CDlgApp::OnCreate(HWND hwndDlg)
{
    int i;
    HWND hwnd;
    m_hwnd = hwndDlg;

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
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_DI.hfontMenu, 0);
        g_fnBtnProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ButtonWndProc);
    }

    // Create the exit button
    hwnd = CreateWindowEx(
            0,
            TEXT("BUTTON"),
            m_szExit,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_OWNERDRAW,
            0,0,0,0,
            m_hwnd,
            NULL,
            m_hInstance,
            NULL );
    SetWindowLongPtr(hwnd, GWLP_ID, IDM_EXIT);
    SendMessage(hwnd, WM_SETFONT, (WPARAM)m_DI.hfontMenu, 0);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ButtonWndProc);

    // Create the checkbox window and possition it correctly
    m_hwndCheck = CreateWindowEx(
            0,
            TEXT("BUTTON"),
            m_szCheckText,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX|BS_MULTILINE|BS_VCENTER,
            0,0,0,0,
            m_hwnd,
            NULL,
            m_hInstance,
            NULL );
    ASSERT( IDM_SHOWCHECK == IDM_MENUITEM1 - 1 );
    SetWindowLongPtr(m_hwndCheck, GWLP_ID, IDM_SHOWCHECK);
    SendMessage( m_hwndCheck, WM_SETFONT, (WPARAM)m_DI.hfontCheck, 0 );
    SendMessage( m_hwndCheck, BM_SETCHECK, m_DataSrc.m_bCheckState?BST_CHECKED:BST_UNCHECKED, 0 );
    SetFocus(m_hwndCheck);
    SetWindowLongPtr(m_hwndCheck, GWLP_WNDPROC, (LONG_PTR)ButtonWndProc);

#ifndef WINNT
    TCHAR szDumbAssFilename[MAX_PATH];
    SHGetSpecialFolderPath(NULL, szDumbAssFilename, CSIDL_APPDATA, FALSE);
    PathAppend(szDumbAssFilename, TEXT("Microsoft\\Welcome\\welcom98.wav"));
    PlaySound( szDumbAssFilename, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT );
#endif

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
    DWORD bCheckPressed;
    bCheckPressed = (BST_CHECKED == SendMessage( m_hwndCheck, BM_GETCHECK, 0,0));
    m_DataSrc.Uninit(bCheckPressed);

#ifndef WINNT
    PlaySound( NULL, NULL, 0 );
#endif

    PostQuitMessage(0);

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

/**
*  This method handles the WM_ERASEBKGND message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnEraseBkgnd(HDC hdc)
{
    RECT rect;
    HPALETTE hpalOld = NULL;

    if ( m_DI.hpal )
    {
        hpalOld = SelectPalette(hdc, m_DI.hpal, FALSE);
        RealizePalette(hdc);
    }

    SetMapMode(hdc, MM_TEXT);
    SetBkMode(hdc, TRANSPARENT);

    // Draw the branding area:
    BitBlt( hdc,0,0,m_cxClient,c_cyBranding, m_hdcTop,0,0, SRCCOPY );
#ifndef WINNT
    {
        HFONT hfontOld = (HFONT)SelectObject(hdc, m_DI.hfontLogo);
        rect.left = 300;
        rect.top = (c_cyBranding-18-18);
        rect.right = m_cxClient;
        rect.bottom = c_cyBranding;
        DrawTextEx(hdc, m_szLogo, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_TOP, NULL);
        SelectObject(hdc, hfontOld);
    }
#endif
    // Draw the left pane:
    // fill rect for background below menu items
    rect.left = 0;
    rect.top = m_cyBottomOfMenuItems;
    rect.right = m_cxLeftPanel;
    rect.bottom = m_cyClient;
    FillRect(hdc, &rect, m_DI.hbrMenuItem);

    // Draw the right pane:
    // fill right pane's background
    rect.left = m_cxLeftPanel;
    rect.top = c_cyBranding;
    rect.right = m_cxClient;
    rect.bottom = m_cyClient;
    if ( 0 > g_iSelectedItem )
    {
        m_DefItem.DrawPane(hdc, &rect);
    }
    else
    {
        m_DefItem.Deactivate();
        m_DataSrc[g_iSelectedItem].DrawPane(hdc, &rect);
    }

    // restore the DC to its original value
    if(hpalOld)
        SelectPalette(hdc, hpalOld, FALSE);

    return TRUE;
}

LRESULT CDlgApp::OnMouseMove(int x, int y, DWORD fwKeys)
{
    // calling ShowWindow will send a WM_MOUSEMOVE even if the mouse didn't move
    // which messes up our focus so we need to ensure the mouse actually moved.
    static int xOld = -1;
    static int yOld = -1;

    // if a task is running then we leave the menu item for that task selected until that
    // task finishes running instead of doing the following logic.
    if ( !g_bTaskRunning )
    {
        if ( (x != xOld) || (y != yOld) )
        {
            // Didn't move over one of our menu items, select the default text.
            if (0 <= g_iSelectedItem)
            {
                g_iSelectedItem = -1;
                SetFocus(m_hwndCheck);

                InvalidateRect(m_hwnd, NULL, TRUE);
            }

            xOld = x;
            yOld = y;
        }
    }

    return 0;
}

LRESULT CDlgApp::OnLButtonDown(int x, int y, DWORD fwKeys)
{
    if ( (x > m_cxLeftPanel) && ( y > c_cyBranding) )
    {
        SetTimer(m_hwnd, WELCOMETIPTIMERID, m_DefItem.m_uRotateSpeed, NULL);
        m_DefItem.Next(m_hwnd);
    }
    return 0;
}

LRESULT CDlgApp::OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg)
{
    if ( !g_bTaskRunning )
    {
        if ( hwnd != m_hwnd )
        {
            if ( hwnd != m_hwndCheck )
            {
                SetCursor(m_hcurHand);
                return TRUE;
            }
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
    case IDM_EXIT:
        // NOTE: Do not call DestroyWindow(m_hwnd), doing so causes Win9x to AV.  Instead
        // simply call the OnDestroy method directy.
        OnDestroy();
        return TRUE;

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

            // For some reason focus isn't going to the right place when we get re-activated.
            // Also, we need to repaint the window and changing it's focus state will do that.
            SetFocus(GetDlgItem(m_hwnd,wID));
        }
    }
    else
    {
        // currently the only commands that are valid while another task is running are
        // IDM_SHOWCHECK and anything that goes to the default handler above.  Everything
        // else will come to here and cause a message beep
        g_iSelectedItem = iNewSelectedItem;
        MessageBeep(0);
    }

    return TRUE;
}

LRESULT CDlgApp::OnQueryNewPalette()
{
    if ( m_DI.hpal )
    {
        HDC hdc = GetDC(m_hwnd);
        HPALETTE hpalOld = SelectPalette(hdc, m_DI.hpal, FALSE);
        UnrealizeObject(m_DI.hpal);
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
    if ( m_DI.hpal && (m_hwnd != hwnd) )
    {
        HDC hdc = GetDC(m_hwnd);
        HPALETTE hpalOld = SelectPalette(hdc, m_DI.hpal, FALSE);
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
    HPALETTE hpalOld = NULL;

    if ( m_DI.hpal )
    {
        hpalOld = SelectPalette(pdis->hDC, m_DI.hpal, FALSE);
        RealizePalette(pdis->hDC);
    }

    if ( IDM_EXIT == iCtlID )
    {
        DrawEdge( pdis->hDC, &rect, BDR_RAISEDOUTER, BF_FLAT | BF_RECT | BF_ADJUST );

        FillRect( pdis->hDC, &rect, m_DI.hbrRightPanel );

        if ( pdis->itemState & ODS_FOCUS )
        {
            InflateRect(&rect, -2, -2 );
            DrawFocusRect( pdis->hDC, &rect );
        }

        SetBkMode(pdis->hDC, TRANSPARENT);
        SetTextColor( pdis->hDC, m_DI.crSelectedText);
        DrawTextEx(pdis->hDC,m_szExit,-1,&rect,DT_NOCLIP|DT_CENTER|DT_VCENTER|DT_SINGLELINE,NULL);
    }
    else
    {
        int i = iCtlID - IDM_MENUITEM1;

        ASSERT( (i < m_DataSrc.m_iItems) && (i >= 0) );

        rect.bottom -= c_cyMenuItemSpacing;

        FillRect( pdis->hDC, &rect, 
            (i==g_iSelectedItem)?m_DI.hbrRightPanel:m_DI.hbrMenuItem );
    
        rect.top = rect.bottom;
        rect.bottom += c_cyMenuItemSpacing;
        FillRect( pdis->hDC, &rect, m_DI.hbrMenuBorder );

        rect.top = pdis->rcItem.top;

        // draw menu item text
        rect.left += c_cxMenuItemPadding;
        rect.top += c_cyMenuItemPadding;
        rect.right -= c_cxMenuItemPadding;

        SetBkMode(pdis->hDC, TRANSPARENT);
        SetTextColor(
                pdis->hDC,
                ((m_DataSrc[i].m_dwFlags&WF_ALTERNATECOLOR)?m_DI.crSelectedText:m_DI.crNormalText));

        DrawTextEx(pdis->hDC,m_DataSrc[i].GetMenuName(),-1,&rect,DT_NOCLIP|DT_WORDBREAK,NULL);

        if ( i==g_iSelectedItem )
        {
            if ( m_DI.bHighContrast )
            {
                rect.left -= 1;
                rect.top -= 2;
                rect.right += 1;
                rect.bottom -= 2;
                DrawFocusRect(pdis->hDC,&rect);
            }
        }
    }

    if ( hpalOld )
    {
        SelectPalette(pdis->hDC, hpalOld, FALSE);
    }

    return TRUE;
}

LRESULT CDlgApp::OnActivate(BOOL bActivate)
{
    if ( WA_INACTIVE != bActivate )
    {
        HWND hwndFocus;

        if ( g_iSelectedItem >= 0 )
        {
            hwndFocus = GetDlgItem(m_hwnd, IDM_MENUITEM1+g_iSelectedItem);
        }
        else
        {
            hwndFocus = m_hwndCheck;
        }

        if ( hwndFocus )
        {
            SetFocus(hwndFocus);
        }
    }

    return 0;
}

LONG_PTR CALLBACK ButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDlgApp *web = (CDlgApp *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
        if ( !g_bTaskRunning )
        {
            // calling ShowWindow will send a WM_MOUSEMOVE even if the mouse didn't move
            // which messes up our focus so we need to ensure the mouse actually moved.
            static int xOld = -1;
            static int yOld = -1;

            if ( (GET_X_LPARAM(lParam) != xOld) || (GET_Y_LPARAM(lParam) != yOld) )
            {
                int iID = ((int)GetDlgCtrlID(hwnd)) - IDM_MENUITEM1;
            
                if ( iID != g_iSelectedItem )
                {
                    SetFocus(hwnd);
                }

                xOld = GET_X_LPARAM(lParam);
                yOld = GET_Y_LPARAM(lParam);
            }
        }
        break;

    case WM_SETFOCUS:
        if ( !g_bTaskRunning )
        {
            int iID = ((int)GetDlgCtrlID(hwnd)) - IDM_MENUITEM1;
            
            if ( iID != g_iSelectedItem )
            {
                g_iSelectedItem = iID;

                InvalidateRect(GetParent(hwnd), NULL, TRUE);

                ASSERT( (g_iSelectedItem < 7) && (g_iSelectedItem >= -2) );
            }
        }
        break;

    }

    return CallWindowProc(g_fnBtnProc, hwnd, uMsg, wParam, lParam);
}
