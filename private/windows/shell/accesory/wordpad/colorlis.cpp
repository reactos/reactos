// colorlis.cpp : implementation file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "wordpad.h"
#include "colorlis.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorMenu

CColorMenu::MenuInfo CColorMenu::m_menuInfo[] = 
{
    {{MSAA_MENU_SIG, 0, NULL}, 0},    //black
    {{MSAA_MENU_SIG, 0, NULL}, 1},    //dark red
    {{MSAA_MENU_SIG, 0, NULL}, 2},    //dark green
    {{MSAA_MENU_SIG, 0, NULL}, 3},    //light brown
    {{MSAA_MENU_SIG, 0, NULL}, 4},    //dark blue
    {{MSAA_MENU_SIG, 0, NULL}, 5},    //purple
    {{MSAA_MENU_SIG, 0, NULL}, 6},    //dark cyan
    {{MSAA_MENU_SIG, 0, NULL}, 12},   //gray
    {{MSAA_MENU_SIG, 0, NULL}, 7},    //light gray
    {{MSAA_MENU_SIG, 0, NULL}, 13},   //red
    {{MSAA_MENU_SIG, 0, NULL}, 14},   //green
    {{MSAA_MENU_SIG, 0, NULL}, 15},   //yellow
    {{MSAA_MENU_SIG, 0, NULL}, 16},   //blue
    {{MSAA_MENU_SIG, 0, NULL}, 17},   //magenta
    {{MSAA_MENU_SIG, 0, NULL}, 18},   //cyan
    {{MSAA_MENU_SIG, 0, NULL}, 19},   //white
    {{MSAA_MENU_SIG, 0, NULL}, 0}     //automatic
};


CColorMenu::CColorMenu()
{
    VERIFY(CreatePopupMenu());
    ASSERT(GetMenuItemCount()==0);

    CString menutext;

    for (int i = 0; i < 17; i++)
    {
        LPWSTR menutextW = new WCHAR[64];

#ifndef UNICODE
        char    menutext[64];
#else
        LPWSTR  menutext = menutextW;
#endif

        ::LoadString(
                AfxGetInstanceHandle(),
                ID_COLOR0 + i,
                menutext,
                64);

#ifndef UNICODE
        // Can't use A2W because it allocates the destination with alloca
        MultiByteToWideChar(CP_ACP, 0, menutext, -1, menutextW, 64);
#endif // !UNICODE

        m_menuInfo[i].msaa.pszWText = menutextW;
        m_menuInfo[i].msaa.cchWText = wcslen(menutextW);

        VERIFY(AppendMenu(MF_OWNERDRAW, ID_COLOR0+i, (LPCTSTR)&m_menuInfo[i]));
    }
}

COLORREF CColorMenu::GetColor(UINT id)
{
    ASSERT(id >= ID_COLOR0);
    ASSERT(id <= ID_COLOR16);
    if (id == ID_COLOR16) // autocolor
        return ::GetSysColor(COLOR_WINDOWTEXT);
    else
    {
        CPalette* pPal = CPalette::FromHandle( (HPALETTE) GetStockObject(DEFAULT_PALETTE));
        ASSERT(pPal != NULL);
        PALETTEENTRY pe;
        if (pPal->GetPaletteEntries(m_menuInfo[id-ID_COLOR0].index, 1, &pe) == 0)
            return ::GetSysColor(COLOR_WINDOWTEXT);
        else
            return RGB(pe.peRed,pe.peGreen,pe.peBlue);
    }
}

void CColorMenu::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    ASSERT(lpDIS->CtlType == ODT_MENU);
    UINT id = (UINT)(WORD)lpDIS->itemID;
    ASSERT(id >= ID_COLOR0);
    ASSERT(id <= ID_COLOR16);
    CDC dc;
    dc.Attach(lpDIS->hDC);

    CRect rc(lpDIS->rcItem);
    ASSERT(rc.Width() < 500);
    if (lpDIS->itemState & ODS_FOCUS)
        dc.DrawFocusRect(&rc);

    COLORREF cr = (lpDIS->itemState & ODS_SELECTED) ?
        ::GetSysColor(COLOR_HIGHLIGHT) :
        dc.GetBkColor();

    CBrush brushFill(cr);
    cr = dc.GetTextColor();

    if (lpDIS->itemState & ODS_SELECTED)
        dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

    int nBkMode = dc.SetBkMode(TRANSPARENT);
    dc.FillRect(&rc, &brushFill);

    rc.left += 50;
    CString strColor;
    strColor.LoadString(id);
    dc.TextOut(rc.left,rc.top,strColor,strColor.GetLength());
    rc.left -= 45;
    rc.top += 2;
    rc.bottom -= 2;
    rc.right = rc.left + 40;
    CBrush brush(GetColor(id));
    CBrush* pOldBrush = dc.SelectObject(&brush);
    dc.Rectangle(rc);

    dc.SelectObject(pOldBrush);
    dc.SetTextColor(cr);
    dc.SetBkMode(nBkMode);
    
    dc.Detach();
}

void CColorMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
    ASSERT(lpMIS->CtlType == ODT_MENU);
    UINT id = (UINT)(WORD)lpMIS->itemID;
    ASSERT(id >= ID_COLOR0);
    ASSERT(id <= ID_COLOR16);
    CDisplayIC dc;
    CString strColor;
    strColor.LoadString(id);
    CSize sizeText = dc.GetTextExtent(strColor,strColor.GetLength());
    ASSERT(sizeText.cx < 500);
    lpMIS->itemWidth = sizeText.cx + 50;
    lpMIS->itemHeight = sizeText.cy;
}
