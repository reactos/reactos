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

#include "welcome.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

class CDlgApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle
        HWND            m_hwndCheck;        // window handle for the check box
        CDataSource     m_DataSrc;          // info from ini and registry about display items
        CDefItem        m_DefItem;          // Our default background pane
        DRAWINFO        m_DI;               // a bunch of setting controlling drawing

        HCURSOR         m_hcurHand;

        int             m_cxClient;
        int             m_cyClient;
        int             m_cxLeftPanel;
        int             m_cyBottomOfMenuItems;

        HDC             m_hdcTop;           // Memory DC used for storing and painting the top image

        TCHAR           m_szCheckText[MAX_PATH];
        TCHAR           m_szExit[MAX_PATH];
#ifndef WINNT
        TCHAR           m_szLogo[MAX_PATH];
#endif

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
        LRESULT OnPaint(HDC hdc);
        LRESULT OnEraseBkgnd(HDC hdc);
        LRESULT OnMouseMove(int x, int y, DWORD fwKeys);
        LRESULT OnLButtonDown(int x, int y, DWORD fwKeys);
        LRESULT OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg);
        LRESULT OnCommand(int wID);
        LRESULT OnQueryNewPalette();
        LRESULT OnPaletteChanged(HWND hwnd);
        LRESULT OnDrawItem(UINT iCtlID, LPDRAWITEMSTRUCT pdis);
        LRESULT OnActivate(BOOL bActivate);

        // helper functions
        BOOL SetColorTable();
        BOOL CreateWelcomeFonts(HDC hdc);
        BOOL CreateBrandingBanner();
        BOOL AdjustToFitFonts(DWORD dwStyle);
};
