f////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CFixMFCToolBar fixes sizing bugs in MFC CToolBar, so it works with
// modern toolbars in versions of comctl32.dll > 4.70.
//
// Most of this code is copied from MFC, with slight modifications marked "PD"
//
#include "StdAfx.h"
#include "UIFixTB.h"
#include "UIModulVer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CFixMFCToolBar, CToolBar)

BEGIN_MESSAGE_MAP(CFixMFCToolBar, CToolBar)
	ON_MESSAGE(TB_SETBITMAPSIZE, OnSetBitmapSize)
	ON_MESSAGE(TB_SETBUTTONSIZE, OnSetButtonSize)
	ON_MESSAGE(WM_SETTINGCHANGE, OnSettingChange)
	ON_MESSAGE(WM_SETFONT,		  OnSettingChange)
END_MESSAGE_MAP()

/////////////////
// This function gets the version number of comctl32.dll.
//
static int GetVerComCtl32()
{
	CModuleVersion ver;
	DLLVERSIONINFO dvi;
	VERIFY(ver.DllGetVersion(_T("comctl32.dll"), dvi));
	return dvi.dwMajorVersion*100 + dvi.dwMinorVersion;
}

int CFixMFCToolBar::iVerComCtl32 = GetVerComCtl32();

CFixMFCToolBar::CFixMFCToolBar()
{
	m_bShowDropdownArrowWhenVertical = FALSE;
}

CFixMFCToolBar::~CFixMFCToolBar()
{
}

//////////////////
// These functions duplicate functionalityin VC 6.0
// Need to set transparent/flat style before setting
// button/image size or font to allow zero-height border.
//
LRESULT CFixMFCToolBar::OnSetBitmapSize(WPARAM, LPARAM lp)
{
	return OnSizeHelper(m_sizeImage, lp);
}
LRESULT CFixMFCToolBar::OnSetButtonSize(WPARAM, LPARAM lp)
{
	return OnSizeHelper(m_sizeButton, lp);
}
LRESULT CFixMFCToolBar::OnSettingChange(WPARAM, LPARAM lp)
{
	return OnSizeHelper(CSize(0,0), lp);
}
LRESULT CFixMFCToolBar::OnSizeHelper(CSize& sz, LPARAM lp)
{
	ASSERT(iVerComCtl32 > 0);
	BOOL bModStyle =FALSE;
	DWORD dwStyle =0;
	if (iVerComCtl32 >= 471) {
		dwStyle = GetStyle();
		bModStyle = ModifyStyle(0, TBSTYLE_TRANSPARENT|TBSTYLE_FLAT);		
	}

	LRESULT lRet = Default();
	if (lRet)
		sz = lp;

	if (bModStyle)
		SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

	return lRet;
}

//////////////////
// **PD**
// This is the all-important function that gets the true size of a button,
// instead of using m_sizeButton. And it's virtual, so you can override if
// my algorithm doesn't work, as will surely be the case in some circumstances.
//
CSize CFixMFCToolBar::GetButtonSize(TBBUTTON* pData, int iButton)
{
	ASSERT(iVerComCtl32 > 0);

	// Get the actual size of the button, not what's in m_sizeButton.
	// Make sure to do SendMessage instead of calling MFC's GetItemRect,
	// which has all sorts of bad side-effects! (Go ahead, take a look at it.)
	// 
	CRect rc;
	SendMessage(TB_GETITEMRECT, iButton, (LPARAM)&rc);
	CSize sz = rc.Size();

	////////////////
	// Now must do special case for various versions of comctl32.dll,
	//
	DWORD dwStyle = pData[iButton].fsStyle;
	if ((pData[iButton].fsState & TBSTATE_WRAP)) {
		if (dwStyle & TBSTYLE_SEP) {
			// this is the last separator in the row (eg vertically docked)
			// fudge the height, and ignore the width. TB_GETITEMRECT will return
			// size = (8 x 22) even for a separator in vertical toolbar
			//
			if (iVerComCtl32 <= 470)
				sz.cy -= 3;		// empircally good fudge factor
			else if (iVerComCtl32 != 471)
				sz.cy = sz.cx;
			sz.cx = 0;			// separator takes no width if it's the last one

		} else if (dwStyle & TBSTYLE_DROPDOWN &&
			!m_bShowDropdownArrowWhenVertical) {
			// ignore width of dropdown
			sz.cx = 0;
		}
	}
	return sz;
}

//////////////////
// **PD**
// Part 2 of correction for MFC is to recalculate everything when the bar
// goes from docked to undocked because the AdjustSize calculation happens
// when the bar is in the old state, and thus wrong. After the bar is
// docked/undocked, I'll recalculate with the new style and commit the change.
//
void CFixMFCToolBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle)
{
	CToolBar::OnBarStyleChange(dwOldStyle, dwNewStyle);
	
	if (dwOldStyle != dwNewStyle) {
		DWORD dwMode = 0;
		if ((dwNewStyle & CBRS_SIZE_DYNAMIC) && (dwNewStyle & CBRS_FLOATING))
			dwMode = LM_HORZ | LM_MRUWIDTH;
		else if (dwNewStyle & CBRS_ORIENT_HORZ)
			dwMode = LM_HORZ | LM_HORZDOCK;
		else
			dwMode =  LM_VERTDOCK;

		CalcDynamicLayout(-1, dwMode | LM_COMMIT);
	}
}

////////////////////////////////////////////////////////////////
// Stuff below is copied from MFC; only mod is to call GetButtonSize.

#ifdef _MAC
	#define CX_OVERLAP  1
#else
	#define CX_OVERLAP  0
#endif

CSize CFixMFCToolBar::CalcSize(TBBUTTON* pData, int nCount)
{
	ASSERT(pData != NULL && nCount > 0);

	CPoint cur(0,0);
	CSize sizeResult(0,0);
	int cyTallestOnRow = 0;

	for (int i = 0; i < nCount; i++)
	{
		if (pData[i].fsState & TBSTATE_HIDDEN)
			continue;

		// **PD**
		// Load actual size of button into a local variable
		// called m_sizeButton. C++ will use this instead of
		// CToolBar::m_sizeButton.
		//
		CSize m_sizeButton = GetButtonSize(pData, i);

		// **PD**
		// I also changed the logic below to be more correct.
		cyTallestOnRow = max(cyTallestOnRow, m_sizeButton.cy);
		sizeResult.cx = max(cur.x + m_sizeButton.cx, sizeResult.cx);
		sizeResult.cy = max(cur.y + m_sizeButton.cy, sizeResult.cy);

		cur.x += m_sizeButton.cx - CX_OVERLAP;

		if (pData[i].fsState & TBSTATE_WRAP)
		{
			cur.x = 0;
			cur.y += cyTallestOnRow;
			cyTallestOnRow = 0;
			if (pData[i].fsStyle & TBSTYLE_SEP)
				cur.y += m_sizeButton.cy;
		}
	}
	return sizeResult;
}

int CFixMFCToolBar::WrapToolBar(TBBUTTON* pData, int nCount, int nWidth)
{
	ASSERT(pData != NULL && nCount > 0);

	int nResult = 0;
	int x = 0;
	for (int i = 0; i < nCount; i++)
	{
		pData[i].fsState &= ~TBSTATE_WRAP;

		if (pData[i].fsState & TBSTATE_HIDDEN)
			continue;

		int dx, dxNext;

		// **PD**
		// Load actual size of button into a local variable
		// called m_sizeButton. C++ will use this instead of
		// CToolBar::m_sizeButton.
		//
		CSize m_sizeButton = GetButtonSize(pData, i);

		dx = m_sizeButton.cx;
		dxNext = dx - CX_OVERLAP;

		if (x + dx > nWidth)
		{
			BOOL bFound = FALSE;
			for (int j = i; j >= 0  &&  !(pData[j].fsState & TBSTATE_WRAP); j--)
			{
				// Find last separator that isn't hidden
				// a separator that has a command ID is not
				// a separator, but a custom control.
				if ((pData[j].fsStyle & TBSTYLE_SEP) &&
					(pData[j].idCommand == 0) &&
					!(pData[j].fsState & TBSTATE_HIDDEN))
				{
					bFound = TRUE; i = j; x = 0;
					pData[j].fsState |= TBSTATE_WRAP;
					nResult++;
					break;
				}
			}
			if (!bFound)
			{
				for (int j = i - 1; j >= 0 && !(pData[j].fsState & TBSTATE_WRAP); j--)
				{
					// Never wrap anything that is hidden,
					// or any custom controls
					if ((pData[j].fsState & TBSTATE_HIDDEN) ||
						((pData[j].fsStyle & TBSTYLE_SEP) &&
						(pData[j].idCommand != 0)))
						continue;

					bFound = TRUE; i = j; x = 0;
					pData[j].fsState |= TBSTATE_WRAP;
					nResult++;
					break;
				}
				if (!bFound)
					x += dxNext;
			}
		}
		else
			x += dxNext;
	}
	return nResult + 1;
}

//////////////////////////////////////////////////////////////////////////
// **PD**
// Functions below are NOT actually modified. They're only here because they
// calls the modified functions above, which are NOT virtual.
//////////////////////////////////////////////////////////////////////////

void  CFixMFCToolBar::SizeToolBar(TBBUTTON* pData, int nCount, int nLength, BOOL bVert)
{
	ASSERT(pData != NULL && nCount > 0);

	if (!bVert)
	{
		int nMin, nMax, nTarget, nCurrent, nMid;

		// Wrap ToolBar as specified
		nMax = nLength;
		nTarget = WrapToolBar(pData, nCount, nMax);

		// Wrap ToolBar vertically
		nMin = 0;
		nCurrent = WrapToolBar(pData, nCount, nMin);

		if (nCurrent != nTarget)
		{
			while (nMin < nMax)
			{
				nMid = (nMin + nMax) / 2;
				nCurrent = WrapToolBar(pData, nCount, nMid);

				if (nCurrent == nTarget)
					nMax = nMid;
				else
				{
					if (nMin == nMid)
					{
						WrapToolBar(pData, nCount, nMax);
						break;
					}
					nMin = nMid;
				}
			}
		}
		CSize size = CalcSize(pData, nCount);
		WrapToolBar(pData, nCount, size.cx);
	}
	else
	{
		CSize sizeMax, sizeMin, sizeMid;

		// Wrap ToolBar vertically
		WrapToolBar(pData, nCount, 0);
		sizeMin = CalcSize(pData, nCount);

		// Wrap ToolBar horizontally
		WrapToolBar(pData, nCount, 32767);
		sizeMax = CalcSize(pData, nCount);

		while (sizeMin.cx < sizeMax.cx)
		{
			sizeMid.cx = (sizeMin.cx + sizeMax.cx) / 2;
			WrapToolBar(pData, nCount, sizeMid.cx);
			sizeMid = CalcSize(pData, nCount);

			if (nLength < sizeMid.cy)
			{
				if (sizeMin == sizeMid)
				{
					WrapToolBar(pData, nCount, sizeMax.cx);
					return;
				}
				sizeMin = sizeMid;
			}
			else if (nLength > sizeMid.cy)
				sizeMax = sizeMid;
			else
				return;
		}
	}
}

struct _AFX_CONTROLPOS
{
	int nIndex, nID;
	CRect rectOldPos;
};

CSize CFixMFCToolBar::CalcLayout(DWORD dwMode, int nLength)
{
	ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));
	if (dwMode & LM_HORZDOCK)
		ASSERT(dwMode & LM_HORZ);

	int nCount;
	TBBUTTON* pData;
	CSize sizeResult(0,0);

	// Load Buttons
	{
		nCount = SendMessage(TB_BUTTONCOUNT, 0, 0);
		if (nCount != 0)
		{
			int i;
			pData = new TBBUTTON[nCount];
			for (i = 0; i < nCount; i++)
				GetButton(i, &pData[i]); // **PD** renamed from _GetButton
		}
	}

	if (nCount > 0)
	{
		if (!(m_dwStyle & CBRS_SIZE_FIXED))
		{
			BOOL bDynamic = m_dwStyle & CBRS_SIZE_DYNAMIC;

			if (bDynamic && (dwMode & LM_MRUWIDTH))
				SizeToolBar(pData, nCount, m_nMRUWidth);
			else if (bDynamic && (dwMode & LM_HORZDOCK))
				SizeToolBar(pData, nCount, 32767);
			else if (bDynamic && (dwMode & LM_VERTDOCK))
				SizeToolBar(pData, nCount, 0);
			else if (bDynamic && (nLength != -1))
			{
				CRect rect; rect.SetRectEmpty();
				CalcInsideRect(rect, (dwMode & LM_HORZ));
				BOOL bVert = (dwMode & LM_LENGTHY);
				int nLen = nLength + (bVert ? rect.Height() : rect.Width());

				SizeToolBar(pData, nCount, nLen, bVert);
			}
			else if (bDynamic && (m_dwStyle & CBRS_FLOATING))
				SizeToolBar(pData, nCount, m_nMRUWidth);
			else
				SizeToolBar(pData, nCount, (dwMode & LM_HORZ) ? 32767 : 0);
		}

		sizeResult = CalcSize(pData, nCount);

		if (dwMode & LM_COMMIT)
		{
			_AFX_CONTROLPOS* pControl = NULL;
			int nControlCount = 0;
			BOOL bIsDelayed = m_bDelayedButtonLayout;
			m_bDelayedButtonLayout = FALSE;

			for(int i = 0; i < nCount; i++)
				if ((pData[i].fsStyle & TBSTYLE_SEP) && (pData[i].idCommand != 0))
					nControlCount++;

			if (nControlCount > 0)
			{
				pControl = new _AFX_CONTROLPOS[nControlCount];
				nControlCount = 0;

				for(int i = 0; i < nCount; i++)
				{
					if ((pData[i].fsStyle & TBSTYLE_SEP) && (pData[i].idCommand != 0))
					{
						pControl[nControlCount].nIndex = i;
						pControl[nControlCount].nID = pData[i].idCommand;

						CRect rect;
						GetItemRect(i, &rect);
						ClientToScreen(&rect);
						pControl[nControlCount].rectOldPos = rect;

						nControlCount++;
					}
				}
			}

			if ((m_dwStyle & CBRS_FLOATING) && (m_dwStyle & CBRS_SIZE_DYNAMIC))
				m_nMRUWidth = sizeResult.cx;
			for (i = 0; i < nCount; i++)
				SetButton(i, &pData[i]); // **PD** renamed from _SetButton

			if (nControlCount > 0)
			{
				for (int i = 0; i < nControlCount; i++)
				{
					CWnd* pWnd = GetDlgItem(pControl[i].nID);
					if (pWnd != NULL)
					{
						CRect rect;
						pWnd->GetWindowRect(&rect);
						CPoint pt = rect.TopLeft() - pControl[i].rectOldPos.TopLeft();
						GetItemRect(pControl[i].nIndex, &rect);
						pt = rect.TopLeft() + pt;
						pWnd->SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
					}
				}
				delete[] pControl;
			}
			m_bDelayedButtonLayout = bIsDelayed;
		}
		delete[] pData;
	}

	//BLOCK: Adjust Margins
	{
		CRect rect; rect.SetRectEmpty();
		CalcInsideRect(rect, (dwMode & LM_HORZ));
		sizeResult.cy -= rect.Height();
		sizeResult.cx -= rect.Width();

		CSize size = CControlBar::CalcFixedLayout((dwMode & LM_STRETCH), (dwMode & LM_HORZ));
		sizeResult.cx = max(sizeResult.cx, size.cx);
		sizeResult.cy = max(sizeResult.cy, size.cy);
	}
	return sizeResult;
}

CSize CFixMFCToolBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	DWORD dwMode = bStretch ? LM_STRETCH : 0;
	dwMode |= bHorz ? LM_HORZ : 0;

	return CalcLayout(dwMode);
}

CSize CFixMFCToolBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	if ((nLength == -1) && !(dwMode & LM_MRUWIDTH) && !(dwMode & LM_COMMIT) &&
		((dwMode & LM_HORZDOCK) || (dwMode & LM_VERTDOCK)))
	{
		return CalcFixedLayout(dwMode & LM_STRETCH, dwMode & LM_HORZDOCK);
	}
	return CalcLayout(dwMode, nLength);
}

/////////////////////////////////////////////////////////////////////////////
// CToolBar attribute access

// **PD** I renamed this from _GetButton.
//
void CFixMFCToolBar::GetButton(int nIndex, TBBUTTON* pButton) const
{
	CToolBar* pBar = (CToolBar*)this;
	VERIFY(pBar->SendMessage(TB_GETBUTTON, nIndex, (LPARAM)pButton));
	// TBSTATE_ENABLED == TBBS_DISABLED so invert it
	pButton->fsState ^= TBSTATE_ENABLED;
}

// **PD** I renamed this from _SetButton.
//
void CFixMFCToolBar::SetButton(int nIndex, TBBUTTON* pButton)
{
	// get original button state
	TBBUTTON button;
	VERIFY(SendMessage(TB_GETBUTTON, nIndex, (LPARAM)&button));

	// prepare for old/new button comparsion
	button.bReserved[0] = 0;
	button.bReserved[1] = 0;
	// TBSTATE_ENABLED == TBBS_DISABLED so invert it
	pButton->fsState ^= TBSTATE_ENABLED;
	pButton->bReserved[0] = 0;
	pButton->bReserved[1] = 0;

	// nothing to do if they are the same
	if (memcmp(pButton, &button, sizeof(TBBUTTON)) != 0)
	{
		// don't redraw everything while setting the button
		DWORD dwStyle = GetStyle();
		ModifyStyle(WS_VISIBLE, 0);
		VERIFY(SendMessage(TB_DELETEBUTTON, nIndex, 0));
		VERIFY(SendMessage(TB_INSERTBUTTON, nIndex, (LPARAM)pButton));
		ModifyStyle(0, dwStyle & WS_VISIBLE);

		// invalidate appropriate parts
		if (((pButton->fsStyle ^ button.fsStyle) & TBSTYLE_SEP) ||
			((pButton->fsStyle & TBSTYLE_SEP) && pButton->iBitmap != button.iBitmap))
		{
			// changing a separator
			Invalidate(FALSE);
		}
		else
		{
			// invalidate just the button
			CRect rect;
			if (SendMessage(TB_GETITEMRECT, nIndex, (LPARAM)&rect))
				InvalidateRect(rect, FALSE);    // don't erase background
		}
	}
}
