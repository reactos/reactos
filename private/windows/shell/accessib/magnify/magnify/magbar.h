/******************************************************************************
Module name: MagBar.h
Written by:  Jeffrey Richter
Purpose: 	 ShellRun class description file.
******************************************************************************/


#if !defined(AFX_MAGBAR_H__B3056D65_965F_11D0_B287_00A0C90DA742__INCLUDED_)
#define AFX_MAGBAR_H__B3056D65_965F_11D0_B287_00A0C90DA742__INCLUDED_

#include "AppBar.h"
#include "ZoomRect.h"


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define USEONESTRETCHBLT 1


class CMagnifyDlg;


class CMagBar : public CAppBar {
public:
	void ForceHideCursor(BOOL bForceHideCursor)
	{
		m_bForceHideCursor = bForceHideCursor;
	}
protected:	// Internal implementation state variables
	BOOL m_bForceHideCursor;
	// CSRBar's class-specific constants
	static const TCHAR m_szRegSubkey[];
	CSize m_szMinTracking;	// The minimum size of the client area
#ifdef USEONESTRETCHBLT
	CDC m_dcOffScreen;
	HBITMAP m_hbmOffScreen;
#else
	CDC m_dcIconBuffer;
	CBitmap m_bmIcon;
#endif
	BOOL m_bSizingOrMoving;
	CZoomRect m_wndZoomRect;

	DWORD m_dwLastMouseMoveTrack;

public:	// Public member functions
	CMagBar(CMagnifyDlg* pwndSettings);

protected:	// Internal implementation functions
	void HideFloatAdornments (BOOL fHide);

protected:	// Overridable functions
	void OnAppBarStateChange (BOOL fProposed, UINT uStateProposed);

// Dialog Data
	//{{AFX_DATA(CMagBar)
	enum { IDD = IDD_STATIONARY };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagBar)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	 // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetupOffScreenDC();
	void CMagBar::OnCancel();
	LRESULT OnEventXMove(WPARAM wParam, LPARAM lParam, BOOL fLastShowPos);
	LRESULT OnEventCaretMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnEventFocusMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnEventMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnEventForceMove(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CMagBar)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnAppbarCopyToClipboard();
	afx_msg void OnClose();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnAppbarExit();
	afx_msg void OnAppbarOptions();
	afx_msg void OnAppbarHide();
	afx_msg void OnSysColorChange();
	//}}AFX_MSG
	afx_msg LRESULT OnEnterSizeMove(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnExitSizeMove(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	CPalette m_hpalPhysical;
	BOOL m_fShowCrossHair;
	int m_cxZoomed, m_cyZoomed;
	POINT m_ptZoom;
	BOOL m_fShowZoomRect;	// Persistent: true if we show (via DrawZoomRect) where the zoomed area is.
	CDC m_dcMem;

private:
	CMagnifyDlg* m_pwndSettings;
	UINT MagLevel();
	BOOL TrackText();
	BOOL TrackSecondaryFocus();
	BOOL TrackCursor();
	BOOL TrackFocus();
	BOOL InvertColors();
	CString m_str;
	CString m_strD;
	
		
	enum { eRefreshTimerId = 55, eRefreshTimerInterval = 500 /* milliseconds */};

	VOID DoTheZoomIn(CDC* pdc, BOOL fShowCrossHair);
	VOID ZoomView (WPARAM ZoomChangeCode);
	VOID MoveView(INT nDirectionCode, BOOL fFast, BOOL fPeg);
	VOID DrawZoomRect();
	VOID CalcZoomedSize();

public:
	VOID ZoomChanged() {CalcZoomedSize();SetupOffScreenDC();}
	VOID CopyToClipboard();
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGBAR_H__B3056D65_965F_11D0_B287_00A0C90DA742__INCLUDED_)


//////////////////////////////// End of File //////////////////////////////////
