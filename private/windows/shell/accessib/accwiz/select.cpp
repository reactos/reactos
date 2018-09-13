//  Select.cpp

#include "pch.hxx" // PCH
#pragma hdrstop

#include "pgbase.h"
#include "AccWiz.h"
#include "resource.h"
#include "Select.h"

extern HPALETTE g_hpal3D;

// Re-Write to use owner drawn controls....:a-anilk
//////////////////////////////////////////////////////////////
// CIconSizePg member functions
//
UINT IDMap[3][2] = { 0, IDC_ICON1,
					 1, IDC_ICON2,
					 2, IDC_ICON3
					};
			

CIconSizePg::CIconSizePg(LPPROPSHEETPAGE ppsp)
				: WizardPage(ppsp, IDS_LKPREV_ICONTITLE, IDS_LKPREV_ICONSUBTITLE)
{
	m_dwPageId = IDD_PREV_ICON2;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	m_nCountValues = 3;
	m_rgnValues[0] = 32;
	m_rgnValues[1] = 48;
	m_rgnValues[2] = 64;
}

LRESULT CIconSizePg::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// Initialize the current selection..
	if(g_Options.m_schemePreview.m_nIconSize <= 32)
		m_nCurValueIndex = 0;
	else if(g_Options.m_schemePreview.m_nIconSize <= 48)
		m_nCurValueIndex = 1;
	else
		m_nCurValueIndex = 2;

	m_nCurrentHilight = m_nCurValueIndex;

	return 1;
}


// These is to set the Focus and sync the painting
LRESULT CIconSizePg::OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
	syncInit = FALSE;
	uIDEvent = SetTimer(hwnd, NULL, 100, NULL);
	return 0;
}

LRESULT CIconSizePg::OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	KillTimer(hwnd, uIDEvent);
	syncInit = TRUE;
	return 1;
}

// Selection has chnaged, So Apply for preview. 
LRESULT CIconSizePg::SelectionChanged(int nNewSelection)
{
	g_Options.m_schemePreview.m_nIconSize = m_rgnValues[nNewSelection];
	g_Options.ApplyPreview();

	return 0;
}

// Re-paints the previous radio control. 
void CIconSizePg::InvalidateRects(int PrevHilight)
{
	InvalidateRect(GetDlgItem(m_hwnd, IDMap[PrevHilight][1]), NULL, TRUE);
}

// Sets the focus to the current item in OnInitDialog. 
void CIconSizePg::SetFocussedItem(int m_nCurrentHilight)
{
	SetFocus(GetDlgItem(m_hwnd, IDMap[m_nCurrentHilight][1]));
}

// DrawItem. Handles painting checks the focussed item 
// to determine selection changes
LRESULT CIconSizePg::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT idCtrl = (UINT) wParam;
	LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT) lParam;
	int index;
	BOOL hasChanged = FALSE;

	if ( !syncInit)
		SetFocussedItem(m_nCurrentHilight);

	switch(idCtrl)
	{

	case IDC_ICON1:
		index = 0;
		break;
	
	case IDC_ICON2:
		index = 1;
		break;
		
	case IDC_ICON3:
		index = 2;
		break;

	default:
		_ASSERTE(FALSE);
	}
	
	// For each button, Check the state, And if the button is selected,
	// means that it has current focus, So Re-paint the previously hilighted and 
	// the current selected buttons....
	// Make sure we ignore the initial events so that we minimize the flicker...
	if ( (lpDrawItemStruct->itemState & ODS_FOCUS) && (m_nCurrentHilight != index))
	{
		// If focussed item!
		if ( syncInit )
		{
			// Erase the previous one...
			InvalidateRects(m_nCurrentHilight);
			m_nCurrentHilight= m_nCurValueIndex = index;
			
			SelectionChanged(m_nCurValueIndex);
		}
	}

	Draw( lpDrawItemStruct, index );

	return 1;
}



void CIconSizePg::Draw(LPDRAWITEMSTRUCT ldi, int i)
{
	HDC hdc = ldi->hDC;

	int nOldBkMode = SetBkMode(hdc, TRANSPARENT);
	int nOldAlign = SetTextAlign(hdc, TA_CENTER);
	
	RECT rcOriginal = ldi->rcItem ;
	TCHAR sz[100];
	LPCTSTR szBitmap = NULL;
	int nFontSize = 8;
	int nOffset = 0;
	HBITMAP hBitmap;

	switch(i)
	{
	case 0:
		szBitmap = __TEXT("IDB_ICON_SAMPLE_NORMAL2"); // NO NEED TO LOCALIZE

		LoadString(g_hInstDll, IDS_ICONSIZENAMENORMAL, sz, ARRAYSIZE(sz));
		nFontSize = 8; 
		nOffset = 16 + 2;
		break;
	case 1:
		szBitmap = __TEXT("IDB_ICON_SAMPLE_LARGE2"); // NO NEED TO LOCALIZE

		LoadString(g_hInstDll, IDS_ICONSIZENAMELARGE, sz, ARRAYSIZE(sz));
		nFontSize = 12; 
		nOffset = 24 + 2;
		break;
	case 2:
		szBitmap = __TEXT("IDB_ICON_SAMPLE_EXLARGE2"); // NO NEED TO LOCALIZE

		LoadString(g_hInstDll, IDS_ICONSIZENAMEEXTRALARGE, sz, ARRAYSIZE(sz));
		nFontSize = 18; 
		nOffset = 32 + 2;
		break;
	default:
		_ASSERTE(FALSE);
		break;
	}

	HFONT hFontOld = (HFONT)SelectObject(hdc, g_Options.GetClosestMSSansSerif(nFontSize, (m_nCurrentHilight == i)));
	TextOut(hdc,
		(rcOriginal.left + rcOriginal.right)/2,
		(rcOriginal.top + rcOriginal.bottom)/2 + nOffset,
		sz, lstrlen(sz));
	SelectObject(hdc, hFontOld);

	HDC hDC = CreateCompatibleDC(hdc);
	// Paint the selected Bitmap. 
	hBitmap = (HBITMAP) LoadImage( g_hInstDll, szBitmap, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_SHARED  | LR_LOADMAP3DCOLORS);
	SelectObject(hDC, hBitmap);

	BitBlt(hdc, (rcOriginal.left + rcOriginal.right)/2 - nOffset, 
				(rcOriginal.top + rcOriginal.bottom)/2 - nOffset, 100, 100, hDC, 0, 0, SRCCOPY);

	DeleteDC(hDC);

	SetTextAlign(hdc, nOldAlign);
	SetBkMode(hdc, nOldBkMode);

	//If current hi-lighted item, Then draw the bounding rectangle. 
	if ( m_nCurrentHilight == i)
	{
		DrawHilight(m_hwnd, ldi);
	}
}




/////////////////////////////////
//CScrollBarPg members 
/////////////////////////////////
//
// Map the button-ID and the selection index
//
UINT IDMapS[4][2] = { 0, IDC_SCROLL1,
					  1, IDC_SCROLL2,
					  2, IDC_SCROLL3,
					  3, IDC_SCROLL4
					};

CScrollBarPg::CScrollBarPg(LPPROPSHEETPAGE ppsp)
		: WizardPage(ppsp, IDS_LKPREV_SCROLLBARTITLE, IDS_LKPREV_SCROLLBARSUBTITLE)
{
	m_dwPageId = IDD_FNTWIZSCROLLBAR;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	
	// Initializes the scroll bar widths and number of elements from string table. 
	LoadArrayFromStringTable(IDS_LKPREV_SCROLLSIZES, m_rgnValues, &m_nCountValues);

}


LRESULT CScrollBarPg::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	m_nCurValueIndex = m_nCountValues - 1;
	
	// Compute the current scroll bar type...
	for(int i=0; i < m_nCountValues; i++)
	{
		if(g_Options.m_schemePreview.m_PortableNonClientMetrics.m_iScrollWidth <= m_rgnValues[i])
		{
			m_nCurValueIndex = i;
			break;
		}
	}

	m_nCurrentHilight = m_nCurValueIndex;
	return 1;
}


// When page set active, Start Timer to set the Focus and ignore the 
// Hilighted events....
LRESULT CScrollBarPg::OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
	syncInit = FALSE;
	uIDEvent = SetTimer(hwnd, NULL, 100, NULL);

	return 0;
}

// Timer Handler
LRESULT CScrollBarPg::OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	KillTimer(hwnd, uIDEvent);
	syncInit = TRUE;
	return 1;
}

// Apply new settings...
LRESULT CScrollBarPg::SettingChanged(int nNewSelection)
{
	int nNewValue = (int) m_rgnValues[nNewSelection];

	g_Options.m_schemePreview.m_PortableNonClientMetrics.m_iScrollWidth = nNewValue;
	g_Options.m_schemePreview.m_PortableNonClientMetrics.m_iScrollHeight = nNewValue;
	g_Options.m_schemePreview.m_PortableNonClientMetrics.m_iBorderWidth = nNewSelection; 
	
	g_Options.ApplyPreview();

	return 0;
}

// Set the current focussed item....
void CScrollBarPg::SetFocussedItem(int m_nCurrentHilight)
{
	SetFocus(GetDlgItem(m_hwnd, IDMapS[m_nCurrentHilight][1]));
}

// Erase the previous one....
void CScrollBarPg::InvalidateRects(int PrevHilight)
{
	InvalidateRect(GetDlgItem(m_hwnd, IDMapS[PrevHilight][1]), NULL, TRUE);
}

// Owner Draw message
LRESULT CScrollBarPg::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT idCtrl = (UINT) wParam;
	LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT) lParam;
	int index;

	if ( !syncInit)
		SetFocussedItem(m_nCurrentHilight);

	switch(idCtrl)
	{

	case IDC_SCROLL1:
		index = 0;
		break;
	
	case IDC_SCROLL2:
		index = 1;
		break;
		
	case IDC_SCROLL3:
		index = 2;
		break;

	case IDC_SCROLL4:
		index = 3;
		break;

	default:
		// Error
		_ASSERTE(FALSE);
	}
	
	// For each button, Check the state, And if the button is selected,
	// means that it has current focus, So Re-paint the previously hilighted and 
	// the current selected buttons....
	// Make sure we ignore the initial events so that we minimize the flicker...
	if ( (lpDrawItemStruct->itemState & ODS_FOCUS) && (m_nCurrentHilight != index))
	{
		if ( syncInit )
		{
			// Erase the previous one...
			InvalidateRects(m_nCurrentHilight);

			m_nCurrentHilight= m_nCurValueIndex = index;
			SettingChanged(m_nCurValueIndex);
			// dirty = TRUE;
		}
	}

	Draw( lpDrawItemStruct, index );

	return 1;
}

// Paints the scroll bars and the selected item
void CScrollBarPg::Draw(LPDRAWITEMSTRUCT ldi, int i)
{
	HDC hdc = ldi->hDC;

	RECT rcOriginal = ldi->rcItem ;
	RECT rci = rcOriginal;
	InflateRect(&rcOriginal, -10, -10);
	
	// Draw border
	DrawEdge(hdc, &rcOriginal, EDGE_RAISED, BF_BOTTOMRIGHT| BF_ADJUST);
	DrawEdge(hdc, &rcOriginal, BDR_RAISEDINNER, BF_FLAT | BF_BOTTOMRIGHT | BF_ADJUST);
	DrawEdge(hdc, &rcOriginal, BDR_RAISEDINNER, BF_FLAT | BF_BOTTOMRIGHT | BF_ADJUST);
	
	// Adjust for the border
	rcOriginal.right -= i;
	rcOriginal.bottom -= i;
	
	// Adjust to the width of the scroll bar
	rcOriginal.left = rcOriginal.right - m_rgnValues[i];
	
	RECT rc = rcOriginal;
	
	
	// Drop the top
	rc.bottom = rc.top + m_rgnValues[i];
	DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLUP);
	
	// Draw the middle
	rc.top = rc.bottom;
	rc.bottom = rcOriginal.bottom - 2 * m_rgnValues[i];
	HBRUSH hbr = (HBRUSH)DefWindowProc(m_hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)m_hwnd);
	HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
	HPEN hpenOld = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
	//				ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	SelectObject(hdc, hbrOld);
	SelectObject(hdc, hpenOld);
	
	// Draw the bottom
	rc.top = rc.bottom;
	rc.bottom = rc.top + m_rgnValues[i];
	DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLDOWN);
	
	// Draw the thumb
	rc.top = rc.bottom;
	rc.bottom = rc.top + m_rgnValues[i];
	DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
	
	// Draw the right arrow
	rc.right = rc.left;
	rc.left = rc.right - m_rgnValues[i];
	DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLRIGHT);
	
	// Draw the middle of the bottom scroll bar
	rc.right = rc.left;
	rc.left = rci.left + 10;
	hbr = (HBRUSH)DefWindowProc(m_hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)m_hwnd);
	hbrOld = (HBRUSH)SelectObject(hdc, hbr);
	hpenOld = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
	//				ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	SelectObject(hdc, hbrOld);
	SelectObject(hdc, hpenOld);

	//If current hi-lighted item, Then draw the bounding rectangle. 
	if ( m_nCurrentHilight == i)
	{
		DrawHilight(m_hwnd, ldi);
	}
}

// Global function to draw the hilighted rectangle....
void DrawHilight(HWND hWnd, LPDRAWITEMSTRUCT ldi)
{
	HDC hdc = ldi->hDC;
	UINT clrH = COLOR_HIGHLIGHT;

	HPALETTE hpalOld = NULL;

	SaveDC(hdc);

	if (g_hpal3D)
	{
		hpalOld = SelectPalette(hdc, g_hpal3D, TRUE);
		RealizePalette(hdc);
	}

	// Set the color for drawing the scroll bar
	COLORREF clrrefOld = SetBkColor(hdc, GetSysColor(COLOR_3DHILIGHT));
	COLORREF clrrefOldText = SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

	// OnDraw(hdc);
	
	// Draw the focus
	RECT rc = ldi->rcItem;
	InflateRect(&rc, -2, -2);
	
	RECT rcTemp;
	
	// If current window not in focus
	if ( GetForegroundWindow() != GetParent(hWnd) )
		clrH = COLOR_GRAYTEXT;

	HWND hwF = GetFocus();

	// Use 'selected' color for scroll bar selection
	COLORREF clrrefSelected = GetSysColor(COLOR_GRAYTEXT); 
	
	if ( (hwF != NULL) && (GetParent(hwF) == hWnd))
		clrrefSelected = GetSysColor(COLOR_HIGHLIGHT); // Use 'Gray' or 'Selected'

	SetBkColor(hdc, clrrefSelected);
	
	// Draw left
	rcTemp = rc;
	rcTemp.right = rcTemp.left + 5;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
	
	// Draw top
	rcTemp = rc;
	rcTemp.bottom = rcTemp.top + 5;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
	
	// Draw right
	rcTemp = rc;
	rcTemp.left = rcTemp.right - 5;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
	
	// Draw bottom
	rcTemp = rc;
	rcTemp.top = rcTemp.bottom - 5;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcTemp, NULL, 0, NULL);
	
	// Reset the color from drawing the scroll bar
	SetBkColor(hdc, clrrefOld);
	SetTextColor(hdc, clrrefOldText);

	if (hpalOld)
	{
		hpalOld = SelectPalette(hdc, hpalOld, FALSE);
		RealizePalette(hdc);
	}

	RestoreDC(hdc, -1);
}