////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef __FLATBAR_H
#define __FLATBAR_H

#ifndef TB_SETEXTENDEDSTYLE
#error FlatBar.h requires a newer version of the SDK than you have!
#error Please update your SDK files.
#endif

#ifndef countof
#define countof(x)	(sizeof(x)/sizeof(x[0]))
#endif
#ifndef _tsizeof
#define _tsizeof(s) sizeof(s)/sizeof(TCHAR)
#endif

#include "UIFixTB.h"

// fwd ref
struct DROPDOWNBUTTON;

#define CFlatToolBarBase CFixMFCToolBar

//////////////////
// CFlatToolbar is a drop-in replacement for CToolBar that supports
// flat-style buttons and gripper handles. Use instead of CToolBar in your
// CMainFrame. CFlatTooBar's don'tK actually have to have the flat style,
// but they do by default. If you don't want the flat look, you can call
// ModifyStyle(TBSTYLE_FLAT, 0);
//
// CFlatToolBar overcomes various MFC drawing bugs that cause it not to work
// with flat-style buttons. CFlatToolBar Can also used inside a coolbar
// (unlike CToolBar).
//
// CFlatToolBar has other good stuff too, like an easy way to handle drop-down
// buttons--See MBTest for example how to use.
//
class CTRL_EXT_CLASS CFlatToolBar : public CFlatToolBarBase {
public:
	CFlatToolBar();
	virtual ~CFlatToolBar();

	static BOOL bTRACE;						// to see TRACE diagnostics

	// set these before creation:
	BOOL m_bDrawDisabledButtonsInColor;	// draw disabled buttons in color
	BOOL m_bInCoolBar;						// set if flatbar is inside coolbar

	// You must call one of these to get the flat look; if not, you must
	// set TBSTYLE_FLAT yourself.
	BOOL LoadToolBar(LPCTSTR lpszResourceName);
	BOOL LoadToolBar(UINT nIDResource)
		{ return LoadToolBar(MAKEINTRESOURCE(nIDResource)); }

	// call to add drop-down buttons
	BOOL AddDropDownButton(UINT nIDButton, UINT nIDMenu, BOOL bArrow);

	// Use these to get/set the flat style. By default, LoadToolBar calls
	// SetFlatStyle(TRUE); if you create some other way, you must call it
	// yourself.
	BOOL SetFlatStyle(BOOL bFlat) {
		return ModifyStyle(bFlat ? 0 : TBSTYLE_FLAT, bFlat ? TBSTYLE_FLAT : 0);
	}
	BOOL GetFlatStyle() {
		return (GetStyle() & TBSTYLE_FLAT)!=0;
	}

	// silly function to fake out compiler with const-ness
	LRESULT SendMessageC(UINT m, WPARAM wp=0, LPARAM lp=0) const
		{ return ((CFixMFCToolBar*)this)->SendMessage(m, wp, lp); }

	// Wrappers that are not in MFC but should be;
	// I copied these from CToolBarCtrl
	BOOL EnableButton(int nID, BOOL bEnable)
		{ return SendMessage(TB_ENABLEBUTTON, nID, MAKELPARAM(bEnable, 0)); }
	BOOL CheckButton(int nID, BOOL bCheck)
		{ return SendMessage(TB_CHECKBUTTON, nID, MAKELPARAM(bCheck, 0)); }
	BOOL PressButton(int nID, BOOL bPress)
		{ return SendMessage(TB_PRESSBUTTON, nID, MAKELPARAM(bPress, 0)); }
	BOOL HideButton(int nID, BOOL bHide)
		{ return SendMessage(TB_HIDEBUTTON, nID, MAKELPARAM(bHide, 0)); }
	BOOL Indeterminate(int nID, BOOL bIndeterminate)
		{ return SendMessage(TB_INDETERMINATE, nID, MAKELPARAM(bIndeterminate, 0)); }
	BOOL IsButtonEnabled(int nID) const
		{ return SendMessageC(TB_ISBUTTONENABLED, nID); }
	BOOL IsButtonChecked(int nID) const
		{ return SendMessageC(TB_ISBUTTONCHECKED, nID); }
	BOOL IsButtonPressed(int nID) const
		{ return SendMessageC(TB_ISBUTTONPRESSED, nID); }
	BOOL IsButtonHidden(int nID) const
		{ return SendMessageC(TB_ISBUTTONHIDDEN, nID); }
	BOOL IsButtonIndeterminate(int nID) const
		{ return SendMessageC(TB_ISBUTTONINDETERMINATE, nID); }
	BOOL SetState(int nID, UINT nState)
		{ return SendMessage(TB_SETSTATE, nID, MAKELPARAM(nState, 0)); }
	int GetState(int nID) const
		{ return SendMessageC(TB_GETSTATE, nID); }
	BOOL AddButtons(int nNumButtons, LPTBBUTTON lpButtons)
		{ return SendMessage(TB_ADDBUTTONS, nNumButtons, (LPARAM)lpButtons); }
	BOOL InsertButton(int nIndex, LPTBBUTTON lpButton)
		{ return SendMessage(TB_INSERTBUTTON, nIndex, (LPARAM)lpButton); }
	BOOL DeleteButton(int nIndex)
		{ return SendMessage(TB_DELETEBUTTON, nIndex); }
	int GetButtonCount() const
		{ return SendMessageC(TB_BUTTONCOUNT); }
	UINT CommandToIndex(UINT nID) const
		{ return SendMessageC(TB_COMMANDTOINDEX, nID); }
	void Customize()
		{ SendMessage(TB_CUSTOMIZE, 0, 0L); }
	int AddStrings(LPCTSTR lpszStrings)
		{ return SendMessage(TB_ADDSTRING, 0, (LPARAM)lpszStrings); }
	void SetButtonStructSize(int nSize)
		{ SendMessage(TB_BUTTONSTRUCTSIZE, nSize); }
	BOOL SetButtonSize(CSize sz)
		{ return SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM(sz.cx, sz.cy)); }
	BOOL SetBitmapSize(CSize sz)
		{ return SendMessage(TB_SETBITMAPSIZE, 0, MAKELPARAM(sz.cx, sz.cy)); }
	void AutoSize()
		{ SendMessage(TB_AUTOSIZE); }
	CToolTipCtrl* GetToolTips() const
		{ return (CToolTipCtrl*)CWnd::FromHandle((HWND)SendMessageC(TB_GETTOOLTIPS)); }
	void SetToolTips(CToolTipCtrl* pTip)
		{ SendMessage(TB_SETTOOLTIPS, (WPARAM)pTip->m_hWnd); }
// NO!!!--this is not the same as the MFC owner
//	void SetOwner(CWnd* pWnd)
//		{ SendMessage(TB_SETPARENT, (WPARAM)pWnd->m_hWnd); }
	void SetRows(int nRows, BOOL bLarger, LPRECT lpRect)
		{ SendMessage(TB_SETROWS, MAKELPARAM(nRows, bLarger), (LPARAM)lpRect); }
	int GetRows() const
		{ return (int) SendMessageC(TB_GETROWS); }
	BOOL SetCmdID(int nIndex, UINT nID)
		{ return SendMessage(TB_SETCMDID, nIndex, nID); }
	UINT GetBitmapFlags() const
		{ return (UINT) SendMessageC(TB_GETBITMAPFLAGS); }

	// Wrappers for some of the newer messages--not complete
	BOOL SetIndent(int indent)
		{ return SendMessage(TB_SETINDENT, indent); }
	HIMAGELIST GetImageList() const
		{ return (HIMAGELIST)SendMessageC(TB_GETIMAGELIST); }
	HIMAGELIST SetImageList(HIMAGELIST hImgList)
		{ return (HIMAGELIST)SendMessage(TB_SETIMAGELIST, 0, (LPARAM)hImgList); }
	int GetBitmap(UINT nIdButton) const
		{ return SendMessageC(TB_GETBITMAP, nIdButton); }
	DWORD SetExtendedStyle(DWORD dwStyle)
		{ return SendMessage(TB_SETEXTENDEDSTYLE, 0, dwStyle); }
	BOOL GetRect(UINT nIdButton, RECT& rc) const
		{ return SendMessageC(TB_GETRECT, nIdButton, (LPARAM)&rc); }
	DWORD GetToolbarStyle() const
		{ return SendMessageC(TB_GETSTYLE); }
	void SetToolbarStyle(DWORD dwStyle)
		{ SendMessage(TB_SETSTYLE, 0, dwStyle); }
	int HitTest(CPoint p) const
		{ return SendMessageC(TB_HITTEST, 0, (LPARAM)&p); }
	int  GetHotItem() const
		{ if (GetSafeHwnd()) return SendMessageC(TB_GETHOTITEM); return 0; }
	void SetHotItem(int iHot)
		{ if (GetSafeHwnd()) SendMessage(TB_SETHOTITEM, iHot); }
	BOOL MapAccelerator(TCHAR ch, UINT& nID) const
		{ return SendMessageC(TB_MAPACCELERATOR, (WPARAM)ch, (LPARAM)&nID); }
	CSize GetPadding() const
		{ return SendMessageC(TB_GETPADDING); }
	CSize SetPadding(CSize sz) 
		{ return SendMessage(TB_SETPADDING, 0, MAKELPARAM(sz.cx,sz.cy)); }

protected:
	CRect				 m_rcOldPos;				// used when toolbar is moved
	DROPDOWNBUTTON* m_pDropDownButtons;		// list of dropdown button/menu pairs
	BOOL				 m_bNoEntry;				// implementation hack

	// override to do your own weird drop-down buttons
	virtual void OnDropDownButton(const NMTOOLBAR& nmtb, UINT nID, CRect rc);
	DROPDOWNBUTTON* FindDropDownButton(UINT nID);

	// helpers
	virtual void InvalidateOldPos(const CRect& rcInvalid);

	afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnTbnDropDown(NMHDR* pNMHDR, LRESULT* pRes);
	afx_msg void OnWindowPosChanging(LPWINDOWPOS lpWndPos);
	afx_msg void OnWindowPosChanged(LPWINDOWPOS lpWndPos);
	afx_msg void OnNcCalcSize(BOOL bCalc, NCCALCSIZE_PARAMS*	pncp );
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CFlatToolBar)
};

#endif
