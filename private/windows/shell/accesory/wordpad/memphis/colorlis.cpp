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

/*
int BASED_CODE CColorMenu::indexMap[17] = {
	0, 		//black
	19, 	//white
	13, 	//red
	14, 	//green
	16, 	//blue
	15, 	//yellow
	17, 	//magenta
	18, 	//cyan
	1, 		//dark red
	2, 		//dark green
	4, 		//dark blue
	3, 		//light brown
	5, 		//purple
	6, 		//dark cyan
	7, 		//light gray
	12, 	//gray
	0};		//automatic
*/

int BASED_CODE CColorMenu::indexMap[17] = {
	0, 		//black
	1, 		//dark red
	2, 		//dark green
	3, 		//light brown
	4, 		//dark blue
	5, 		//purple
	6, 		//dark cyan
	12, 	//gray
	7, 		//light gray
	13, 	//red
	14, 	//green
	15, 	//yellow
	16, 	//blue
	17, 	//magenta
	18, 	//cyan
	19, 	//white
	0};		//automatic

CColorMenu::CColorMenu()
{
	VERIFY(CreatePopupMenu());
	ASSERT(GetMenuItemCount()==0);
	for (int i=0; i<=16;i++)
		VERIFY(AppendMenu(MF_OWNERDRAW, ID_COLOR0+i, (LPCTSTR)(ID_COLOR0+i)));
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
		if (pPal->GetPaletteEntries(indexMap[id-ID_COLOR0], 1, &pe) == 0)
			return ::GetSysColor(COLOR_WINDOWTEXT);
		else
			return RGB(pe.peRed,pe.peGreen,pe.peBlue);
//		return PALETTEINDEX(CColorMenu::indexMap[id-ID_COLOR0]);
	}
}

void CColorMenu::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS->CtlType == ODT_MENU);
	UINT id = (UINT)(WORD)lpDIS->itemID;
	ASSERT(id == lpDIS->itemData);
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
	ASSERT(id == lpMIS->itemData);
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
