////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef __MENUBAR_H
#define __MENUBAR_H

#include "UIsubclass.h"

#include "UIFlatBar.h"

//////////////////
// CMenuBar uses this private class to intercept messages on behalf
// of its owning frame, as well as the MDI client window. Conceptually,
// these should be two different hooks, but I want to save code.
//
class CMenuBarFrameHook : public CSubclassWnd {
protected:
   friend class CMenuBar;
   CMenuBar* m_pMenuBar;
   CMenuBarFrameHook();
   ~CMenuBarFrameHook();
   BOOL Install(CMenuBar* pMenuBar, HWND hWndToHook);
   virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);
};

//////////////////
// CMenuBar implements an Office 97-style menu bar. Use it the way you would
// a CToolBar, only you need not call LoadToolbar. All you have to do is
//
// * Create the CMenuBar from your OnCreate or OnCreateBands handler.
//
// * Call LoadMenu to load a menu. This will set your frame's menu to NULL.
//
// * Implemenent your frame's PreTranslateMessage function, to call
//   CMenuBar::TranslateFrameMessage. 
//
class CTRL_EXT_CLASS CMenuBar : public CFlatToolBar {
public:
	BOOL	 m_bAutoRemoveFrameMenu;		 // set frame's menu to NULL

	CMenuBar();
	~CMenuBar();

	// You must call this from your frame's PreTranslateMessage fn
	virtual BOOL TranslateFrameMessage(MSG* pMsg);

	HMENU LoadMenu(HMENU hmenu);				// load menu
	HMENU LoadMenu(LPCTSTR lpszMenuName);	// ...from resource file
	HMENU LoadMenu(UINT nID) {
		return LoadMenu(MAKEINTRESOURCE(nID));
	}
	HMENU GetMenu() { return m_hmenu; }					// get current menu

	enum TRACKINGSTATE { // menubar has three states:
		TRACK_NONE = 0,   // * normal, not tracking anything
		TRACK_BUTTON,     // * tracking buttons (F10/Alt mode)
		TRACK_POPUP       // * tracking popups
	};

	TRACKINGSTATE GetTrackingState(int& iPopup) {
		iPopup = m_iPopupTracking; return m_iTrackingState;
	}
	static BOOL bTRACE;						 // set TRUE to see TRACE msgs

protected:
	friend class CMenuBarFrameHook;

	CMenuBarFrameHook m_frameHook;		 // hooks frame window messages
	CStringArray		m_arStrings;		 // array of menu item names
	HMENU					m_hmenu;				 // the menu

	// menu tracking stuff:
	int	 m_iPopupTracking;				 // which popup I'm tracking if any
	int	 m_iNewPopup;						 // next menu to track
	BOOL	 m_bProcessRightArrow;			 // process l/r arrow keys?
	BOOL	 m_bProcessLeftArrow;			 // ...
	BOOL	 m_bEscapeWasPressed;			 // user pressed escape to exit menu
	CPoint m_ptMouse;							 // mouse location when tracking popup
	HMENU	 m_hMenuTracking;					 // current popup I'm tracking

	TRACKINGSTATE m_iTrackingState;		 // current tracking state

	// helpers
	void  RecomputeToolbarSize();
	void	RecomputeMenuLayout();
	void	UpdateFont();
	int	GetNextOrPrevButton(int iButton, BOOL bPrev);
	void	SetTrackingState(TRACKINGSTATE iState, int iButton=-1);
	void	TrackPopup(int iButton);
	void	ToggleTrackButtonMode();
	void	CancelMenuAndTrackNewOne(int iButton);
	void  OnMenuSelect(HMENU hmenu, UINT nItemID);
	CPoint ComputeMenuTrackPoint(const CRect& rcButn, TPMPARAMS& tpm);

	BOOL	IsValidButton(int iButton) const
		{ return 0 <= iButton && iButton < GetButtonCount(); }

	virtual BOOL OnMenuInput(MSG& m);	 // handle popup menu input

	// overrides
	virtual void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);
	int HitTest(CPoint p) const;

	// command/message handlers
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateMenuButton(CCmdUI* pCmdUI);
	afx_msg LRESULT OnSetMenuNull(WPARAM wp, LPARAM lp);

	static LRESULT CALLBACK MenuInputFilter(int code, WPARAM wp, LPARAM lp);
	
	DECLARE_DYNAMIC(CMenuBar)
	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

#endif
