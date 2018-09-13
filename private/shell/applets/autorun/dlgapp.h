//////////////////////////////////////////////////////////////////////////
//
//  dlgapp.h
//
//      This file contains the specification of the DlgApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "autorun.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

class CDlgApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle
        CDataSource     m_DataSrc;          // info from ini and registry about display items

        HFONT           m_hfontTitle;       // Font used to draw the title
        HFONT           m_hfontMenu;        // Font used to draw the menu items
        HFONT           m_hfontBody;        // Font used to draw the body

        HBRUSH          m_hbrMenuItem;      // Brush used to draw background of menu items
        HBRUSH          m_hbrMenuBorder;    // Brush used to draw the dark area behind the menu items
        HBRUSH          m_hbrRightPanel;    // Brush used to draw the background of the right panel

        COLORREF        m_crMenuText;       // Color of text on non-selected menu items (ususally the same as m_crNormalText)
        COLORREF        m_crNormalText;     // Color of text in right panel body and selected menu items
        COLORREF        m_crTitleText;      // Color of the title text
        COLORREF        m_crSelectedText;   // Color of menu items that have been previouly launched.
        
        HCURSOR         m_hcurHand;

        int             m_cxClient;
        int             m_cyClient;
        int             m_cxLeftPanel;
        int             m_cyBottomOfMenuItems;

        HDC             m_hdcTop;           // Memory DC used for storing and painting the top image

        // BUGBUG: Why not make these pointers to avoid adding a length limit?  Shouldn't really matter since
        // we control this text but it would be real easy to make this more general.
        TCHAR           m_szDefTitle[MAX_PATH];
        TCHAR           m_szDefBody[1024];
        TCHAR           m_szCheckText[MAX_PATH];

        bool            m_bHighContrast;    // true if high contrast options should be used
        bool            m_bLowColor;        // true if we are in 256 or less color mode.
        HPALETTE        m_hpal;             // palette to use if in palette mode
        int             m_iColors;          // -1, 16, or 256 depending on the color mode we are in.

        struct tagBkgndInfo {
            HBITMAP hbm;
            int     cx;
            int     cy;
        } m_aBkgnd[4];

    public:
        CDlgApp();
        ~CDlgApp();

        void Register(HINSTANCE hInstance);
        bool InitializeData();
        void Create(int nCmdShow);
        void MessageLoop();

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Window Messages
        LRESULT OnCreate(HWND hwnd);
        LRESULT OnDestroy();
        LRESULT OnActivate(WPARAM wParam);
        LRESULT OnPaint(HDC hdc);
        LRESULT OnEraseBkgnd(HDC hdc);
        LRESULT OnLButtonDown(int x, int y, DWORD fwKeys);
        LRESULT OnMouseMove(int x, int y, DWORD fwKeys);
        LRESULT OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg);
        LRESULT OnCommand(int wID);
        LRESULT OnQueryNewPalette();
        LRESULT OnPaletteChanged(HWND hwnd);
        LRESULT OnDrawItem(UINT iCtlID, LPDRAWITEMSTRUCT pdis);

        // helper functions
        BOOL SetColorTable();
        BOOL CreateWelcomeFonts(HDC hdc);
        BOOL CreateBrandingBanner();
        BOOL LoadBkgndImages();
        BOOL AdjustToFitFonts(DWORD dwStyle);
};
