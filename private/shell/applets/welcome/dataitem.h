//
//
//

#pragma once

#include <debug.h>

enum {
    WF_PERUSER          = 0x0001,   // item is per user as opposed to per machine
    WF_ADMINONLY        = 0x0002,   // only show item if user is an admin
    WF_ALTERNATECOLOR   = 0x1000,   // show menu item text in the "visited" color
    WF_DISABLED         = 0x2000,   // Treated normally except cannot be launched
};

typedef struct tagDrawInfo
{
    bool            bHighContrast;    // true if high contrast options should be used
    bool            bLowColor;        // true if we are in 256 or less color mode.
    HPALETTE        hpal;             // palette to use if in palette mode
    int             iColors;          // -1, 16, or 256 depending on the color mode we are in.

#ifdef WINNT
    bool            bTerminalServer;  // true if we are on a Terminal Server client.
#endif

    HFONT           hfontTitle;       // Font used to draw the title
    HFONT           hfontMenu;        // Font used to draw the menu items
    HFONT           hfontBody;        // Font used to draw the body
    HFONT           hfontCheck;       // Font used to draw the checkbox label
#ifndef WINNT
    HFONT           hfontLogo;        // Font used to draw "Second Edition" in the logo
#endif

    HBRUSH          hbrMenuItem;      // Brush used to draw background of menu items
    HBRUSH          hbrMenuBorder;    // Brush used to draw the dark area behind the menu items
    HBRUSH          hbrRightPanel;    // Brush used to draw the background of the right panel

    COLORREF        crNormalText;
    COLORREF        crTitleText;
    COLORREF        crSelectedText;

} DRAWINFO, * LPDRAWINFO;

extern LPDRAWINFO g_pdi;

class CDataItem
{
public:
    CDataItem();
    ~CDataItem();

    TCHAR * GetTitle()      { return m_pszTitle; }
    TCHAR * GetMenuName()   { return m_pszMenuName?m_pszMenuName:m_pszTitle; }
    TCHAR * GetDescription(){ return m_pszDescription; }
    TCHAR   GetAccel()      { return m_chAccel; }

    BOOL SetData( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex );
    BOOL Invoke(HWND hwnd);
    int  GetMinHeight(HDC hdc, int cx, int cyBottomPad);
    bool DrawPane(HDC hdc, LPRECT prc);


    // flags
    //
    // This var is a bit mask of the following values
    //  PERUSER     True if item must be completed on a per user basis
    //              False if it's per machine
    //  ADMINONLY   True if this item can only be run by an admin
    //              False if all users should do this
    DWORD   m_dwFlags;

protected:
    TCHAR * m_pszTitle;
    TCHAR * m_pszMenuName;
    TCHAR * m_pszDescription;
    TCHAR   m_chAccel;
    int     m_iImage;

    TCHAR * m_pszCmdLine;
    TCHAR * m_pszArgs;

    HBITMAP m_hBitmap;
    DWORD   m_cxBitmap;
    DWORD   m_cyBitmap;
};


typedef struct tagDefItemData
{
    LPTSTR  pszDescription;
    int     iAniCtrl;
    HBITMAP hBitmap;
    HBITMAP hBitMask;
    int     cxBitmap;
    int     cyBitmap;

} DEFITEMDATA, * LPDEFITEMDATA;

class CDefItem
{
public:
    CDefItem();
    ~CDefItem();

    int  Init();
    int  GetMinHeight(HDC hdc, int cx, int cyBottomPad);
    bool DrawPane(HDC hdc, LPRECT prc);
    int  Next(HWND hwnd);
    bool Deactivate();              // if active content is being displayed, hide it

    UINT m_uRotateSpeed;            // number of milliseconds before changing tip

protected:
    TCHAR *         m_pszTitle;     // The title is always the same
    LPDEFITEMDATA   m_pdid;         // array of items
    int             m_cItems;       // the number of items in the array pointed to by m_pdid    
    int             m_iCurItem;     // The current item.
    HWND            m_hwndAni;      // handle to the Animate Control window
    int             m_cyBottomPad;  // padding below bitmaps
    BOOL            m_bPositioned;  // we lazy create the animation control
    HKEY            m_hkey;         // hkey into which we store our current item
    bool            m_bActivePane;  // true if we are showing "active" content, such as an AVI.
};
