////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef __COOLBAR_H
#define __COOLBAR_H

//////////////////
// CCoolBar encapsulates IE common coolbar (rebar) for MFC. To use it,
//
//		* derive your own CMyCoolBar from CCoolBar
//		* implement OnCreateBands to create whatever bands you want
//		* instantiate CMyCoolBar in your frame window as you would a toolbar
//		* create and load it, etc from CMainFrame::OnCreate
//
// See MBTest for example of how to use.
//
class CTRL_EXT_CLASS CCoolBar : public CControlBar {
public:
	CCoolBar();
	virtual ~CCoolBar();

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle,
		DWORD dwAfxBarStyle = CBRS_ALIGN_TOP,
		UINT nID = AFX_IDW_TOOLBAR);

	// message wrappers
	BOOL GetBarInfo(LPREBARINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_GETBARINFO, 0, (LPARAM)lp); }

	BOOL SetBarInfo(LPREBARINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_SETBARINFO, 0, (LPARAM)lp); }

	BOOL GetBandInfo(int iBand, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_GETBANDINFO, iBand, (LPARAM)lp); }

	BOOL SetBandInfo(int iBand, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_SETBANDINFO, iBand, (LPARAM)lp); }

	BOOL InsertBand(int iWhere, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_INSERTBAND, (WPARAM)iWhere, (LPARAM)lp); }

	BOOL DeleteBand(int nWhich)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_DELETEBAND, (WPARAM)nWhich); }

	int GetBandCount()
		{ ASSERT(::IsWindow(m_hWnd));
		  return (int)SendMessage(RB_GETBANDCOUNT); }

	int GetRowCount()
		{ ASSERT(::IsWindow(m_hWnd));
	     return (int)SendMessage(RB_GETROWCOUNT); }

	int GetRowHeight(int nWhich)
		{ ASSERT(::IsWindow(m_hWnd));
	     return (int)SendMessage(RB_GETROWHEIGHT, (WPARAM)nWhich); }

	// Call these handy functions from your OnCreateBands to do stuff
	// more easily than the Windows way.
	//
	BOOL InsertBand(CWnd* pWnd, CSize szMin, int cx = 0,
		LPCTSTR lpText=NULL, int iWhere=-1, BOOL bNewRow =FALSE);
	void SetColors(COLORREF clrFG, COLORREF clrBG);
	void SetBackgroundBitmap(CBitmap* pBitmap);
	void Invalidate(BOOL bErase = TRUE); // invalidates children too

	static BOOL bTRACE;	// Set TRUE to see extra diagnostics in DEBUG code

protected:
	// YOU MUST OVERRIDE THIS in your derived class to create bands.
	virtual BOOL OnCreateBands() = 0; // return -1 if failed

	// Virtual fn called when the coolbar height changes as a result of moving
	// bands around. Override only if you want to do something different.
	virtual void OnHeightChange(const CRect& rcNew);

	// overrides to fix problems w/MFC. No need to override yourself.
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);
	virtual void  OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	// message handlers
	afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnPaint();
	afx_msg void OnHeightChange(NMHDR* pNMHDR, LRESULT* pRes);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CCoolBar)
};

//////////////////
// Programmer-friendly REBARINFO initializes itself.
//
class CRebarInfo : public REBARINFO {
public:
	CRebarInfo() {
		memset(this, 0, sizeof(REBARINFO));
		cbSize = sizeof(REBARINFO);
	}
};

//////////////////
// Programmer-friendly REBARBANDINFO initializes itself.
//
class CRebarBandInfo : public REBARBANDINFO {
public:
	CRebarBandInfo() {
		memset(this, 0, sizeof(REBARBANDINFO));
		cbSize = sizeof(REBARBANDINFO);
	}
};

#endif
