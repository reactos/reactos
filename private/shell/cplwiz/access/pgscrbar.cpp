#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgScrBar.h"

CScrollBarPg::CScrollBarPg(
						   LPPROPSHEETPAGE ppsp
						   ) : WizardPage(ppsp, IDS_LKPREV_SCROLLBARTITLE, IDS_LKPREV_SCROLLBARSUBTITLE)
{
	m_dwPageId = IDD_FNTWIZSCROLLBAR;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);

	m_nCurValueIndex = 0;
}


CScrollBarPg::~CScrollBarPg(
							VOID
							)
{
}

LRESULT
CScrollBarPg::OnInitDialog(
						   HWND hwnd,
						   WPARAM wParam,
						   LPARAM lParam
						   )
{
	LoadArrayFromStringTable(IDS_LKPREV_SCROLLSIZES, m_rgnValues, &m_nCountValues);

	RECT rcDummy;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_STATICDUMMYLOCATION), &rcDummy);
	MapWindowPoints(NULL, m_hwnd,(LPPOINT) &rcDummy, 2);

	int nSizeTotal = 0;
	for(int i=0;i<m_nCountValues;i++)
		nSizeTotal += 3 * m_rgnValues[i];

	int nSpaceBetween = (rcDummy.right - rcDummy.left - nSizeTotal) / (m_nCountValues - 1);

	int nTemp = rcDummy.left;
	for(i=0;i<m_nCountValues;i++)
	{
		m_rgrcScrollBars[i].top = rcDummy.top;
		m_rgrcScrollBars[i].bottom = rcDummy.bottom;
		m_rgrcScrollBars[i].left = nTemp;
		m_rgrcScrollBars[i].right = m_rgrcScrollBars[i].left + 3 * m_rgnValues[i];
		nTemp = m_rgrcScrollBars[i].right + nSpaceBetween;
	}

	// JMC: ToDo  - Set m_nCurValueIndex based on the current NONCLIENTMETRICS
	m_nCurValueIndex = 0;

	UpdateControls();
	return 1;
}


void CScrollBarPg::UpdateControls()
{
}


LRESULT
CScrollBarPg::OnCommand(
						HWND hwnd,
						WPARAM wParam,
						LPARAM lParam
						)
{
	LRESULT lResult = 1;
	
	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID 	 = LOWORD(wParam);
	HWND hwndCtl	 = (HWND)lParam;
	
#if 0
	switch(wCtlID)
	{
	case IDC_TK_ENABLE:
		// These commands require us to re-enable/disable the appropriate controls
		UpdateControls();
		lResult = 0;
		break;
		
	default:
		break;
	}
#endif
	
	return lResult;
}




LRESULT
CScrollBarPg::HandleMsg(
						 HWND hwnd,
						 UINT uMsg,
						 WPARAM wParam,
						 LPARAM lParam
						 )
{
	switch(uMsg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(m_hwnd, &ps);
			
			// Set the color for drawing the scroll bar
			COLORREF clrrefScrollBar = GetSysColor(COLOR_SCROLLBAR);
			clrrefScrollBar = GetSysColor(COLOR_3DHILIGHT);
			COLORREF clrrefSelected = GetSysColor(COLOR_HIGHLIGHT);
			COLORREF clrrefOld = SetBkColor(hdc, clrrefScrollBar);
			
			for(int i=0;i<m_nCountValues;i++)
			{
				RECT rcOriginal = m_rgrcScrollBars[i];
				
				// Draw border
				DrawEdge(hdc, &rcOriginal, EDGE_RAISED, BF_BOTTOMRIGHT| BF_ADJUST);
				DrawEdge(hdc, &rcOriginal, BDR_RAISEDINNER, BF_FLAT | BF_BOTTOMRIGHT | BF_ADJUST);
				DrawEdge(hdc, &rcOriginal, BDR_RAISEDINNER, BF_FLAT | BF_BOTTOMRIGHT | BF_ADJUST);
				
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
				rc.left = m_rgrcScrollBars[i].left;
				hbr = (HBRUSH)DefWindowProc(m_hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)m_hwnd);
				hbrOld = (HBRUSH)SelectObject(hdc, hbr);
				hpenOld = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
				//				ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
				Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
				SelectObject(hdc, hbrOld);
				SelectObject(hdc, hpenOld);
			}
			
			
			// Draw the focus
			RECT rc = m_rgrcScrollBars[m_nCurValueIndex];
			InflateRect(&rc, 8, 8);
			
			RECT rcTemp;
			
			// Use 'selected' color for scroll bar selection
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
			
			EndPaint(m_hwnd, &ps);
			return 1;
		}
		break;
	case WM_LBUTTONDOWN:
		{
			POINT pt;
			pt.x = LOWORD(lParam);  // horizontal position of cursor 
			pt.y = HIWORD(lParam);  // vertical position of cursor 

			// Find out if we have hit a scroll bar
			for(int i=0;i<m_nCountValues;i++)
			{
				if(PtInRect(&m_rgrcScrollBars[i], pt))
					break;
			}

			if(i<m_nCountValues)
			{
				// We've hit inside a scroll bar rectangle

				// Calc Old Rect for selected
				RECT rcOldSelected = m_rgrcScrollBars[m_nCurValueIndex];
				InflateRect(&rcOldSelected, 8, 8);

				// Calc New Rect for hilight
				m_nCurValueIndex = i;
				RECT rcNewSelected = m_rgrcScrollBars[m_nCurValueIndex];
				InflateRect(&rcNewSelected, 8, 8);

				// Erase Old Rect
				InvalidateRect(m_hwnd, &rcOldSelected, TRUE); // We must erase so the background overdraws the hilight

				// Draw New Rect
				InvalidateRect(m_hwnd, &rcNewSelected, FALSE); // We don't have to erase since we are just adding the hilight
			}
		}
		break;
	default:
		break;
	}
	return 0;
}


