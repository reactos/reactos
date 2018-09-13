// ipframe.h : interface of the CInPlaceFrame class
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

class CWordPadResizeBar : public COleResizeBar
{
public: 
	void SetMinSize(CSize size) {m_tracker.m_sizeMin = size;}
};

class CInPlaceFrame : public COleIPFrameWnd
{
	DECLARE_DYNCREATE(CInPlaceFrame)
public:
	CInPlaceFrame() {};

// Attributes
public:
	CToolBar m_wndToolBar;
	CFormatBar m_wndFormatBar;
	CRulerBar m_wndRulerBar;
	CWordPadResizeBar m_wndResizeBar;
	COleDropTarget m_dropTarget;

// Operations
public:
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual void CalcWindowRect(LPRECT lpClientRect, 
		UINT nAdjustType = adjustBorder);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceFrame)
	public:
	virtual BOOL OnCreateControlBars(CFrameWnd* pWndFrame, CFrameWnd* pWndDoc);
	virtual void RepositionFrame(LPCRECT lpPosRect, LPCRECT lpClipRect);
	//}}AFX_VIRTUAL

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL CreateToolBar(CWnd* pWndFrame);
	BOOL CreateFormatBar(CWnd* pWndFrame);
	BOOL CreateRulerBar(CWnd* pWndFrame);

// Generated message map functions
protected:
	//{{AFX_MSG(CInPlaceFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnHelpFinder();
	afx_msg void OnCharColor();
	afx_msg void OnPenToggle();
	//}}AFX_MSG
	LRESULT OnResizeChild(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnBarState(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
