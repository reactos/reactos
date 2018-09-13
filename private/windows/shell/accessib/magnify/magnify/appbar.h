/******************************************************************************
Module name: AppBar.h
Written by:  Jeffrey Richter
Purpose: 	 AppBar base class description file.
******************************************************************************/

#ifndef __APPBAR_H__
#define __APPBAR_H__


// An AppBar can be in one of 6 states shown in the table below: 
// State 			Description
// ----------- 	-----------------------------------------------------
// ABE_UNKNOWN 	The Appbar is in an unknown state 
// 					(usually during construction/destruction)
// ABE_FLOAT		The AppBar is floating on the screen
// ABE_LEFT 		The Appbar is docked on the left   edge of the screen
// ABE_TOP			The Appbar is docked on the top	  edge of the screen
// ABE_RIGHT		The Appbar is docked on the right  edge of the screen
// ABE_BOTTOM		The Appbar is docked on the bottom edge of the screen

// The ABE_edge state macros are defined in SHELLAPI.H as follows:
// #define ABE_LEFT			0
// #define ABE_TOP			1
// #define ABE_RIGHT 		2
// #define ABE_BOTTOM		3

// The ABE_UNKNOWN and ABE_FLOAT macros are defined here as follows:
#define ABE_UNKNOWN				((UINT) -1)
#define ABE_FLOAT 				((UINT) -2)


///////////////////////////////////////////////////////////////////////////////


// An AppBar can have several behavior flags as shown below: 
// Flag								 Description
// ----------- 					 ------------------------------------------------
// ABF_ALLOWLEFTRIGHT			 Allow dock on left/right of screen
// ABF_ALLOWTOPBOTTOM			 Allow dock on top/bottom of screen
// ABF_ALLOWANYEDGE				 Allow dock on any edge of screen
// ABF_ALLOWFLOAT 				 Allow float in the middle of screen
// ABF_ALLOWANYWHERE 			 Allow dock and float
// ABF_MIMICTASKBARAUTOHIDE	 Follow Autohide state of TaskBar
// ABF_MIMICTASKBARALWAYSONTOP Follow AlwaysOnTop state of TaskBar


#define ABF_ALLOWLEFTRIGHT 			0x00000001
#define ABF_ALLOWTOPBOTTOM 			0x00000002
#define ABF_ALLOWANYEDGE				(ABF_ALLOWLEFTRIGHT | ABF_ALLOWTOPBOTTOM)
#define ABF_ALLOWFLOAT					0x00000004
#define ABF_ALLOWANYWHERE				(ABF_ALLOWANYEDGE | ABF_ALLOWFLOAT)
#define ABF_MIMICTASKBARAUTOHIDE 	0x00000010
#define ABF_MIMICTASKBARALWAYSONTOP 0x00000020


///////////////////////////////////////////////////////////////////////////////

typedef struct {
		DWORD m_cbSize;			// Size of this structure
		UINT	m_uState;			// ABE_UNKNOWN, ABE_FLOAT, or ABE_edge
		BOOL	m_fAutohide;		// Should AppBar be auto-hidden when docked?
		BOOL	m_fAlwaysOnTop;	// Should AppBar always be on top?
		UINT	m_auDimsDock[4];	// Width/height for docked bar on 4 edges
		CRect m_rcFloat;			// Floating rectangle (in screen coordinates)
	} APPBARSTATE;

class CAppBar : public CDialog {

public:	// Static, AppBar-specific helper functions
	// Returns TRUE if uEdge is ABE_LEFT or ABE_RIGHT, else FALSE is returned
	static BOOL IsEdgeLeftOrRight (UINT uEdge);

	// Returns TRUE if uEdge is ABE_TOP or ABE_BOTTOM, else FALSE is returned
	static BOOL IsEdgeTopOrBottom (UINT uEdge);

	// Forces the shell to update its AppBar list and the workspace area
	static void ResetSystemKnowledge (void);

	// Returns a proposed edge or ABE_FLOAT based on ABF_* flags and a 
	// point specified in screen coordinates).
	static UINT GetEdgeFromPoint (DWORD fdwFlags, CPoint pt);


protected:	// Internal implementation state variables
	// Registered window message for the AppBar's callback notifications
	static UINT s_uAppBarNotifyMsg;

	// AppBar's class-specific constants
	enum { AUTOHIDETIMERID = 1, AUTOHIDETIMERINTERVAL = 400 };

	// See the OnAppBarCallbackMsg function for usage.
	DWORD m_fdwTaskBarState;

	// The structure below contains all of the AppBar settings that
	// can be saved/loaded in/from the Registry.
	APPBARSTATE *PAPPBARSTATE;
	APPBARSTATE m_abs;			// This AppBar's state info

	DWORD m_fdwFlags; 			// See the ABF_* flags above
	CSize m_szSizeInc;			// Descrete width/height size increments

	// We need a member variable which tracks the proposed state of the
	// AppBar while the user is moving it, deciding where to position it.
	// While not moving, this member must contain ABE_UNKNOWN so that 
	// GetState() returns the current state contained in m_ps.m_uState.
	// While moving the AppBar, m_uStateProposedPrev contains the 
	// proposed state based on the position of the AppBar.  The proposed 
	// state becomes the new state when the user stops moving the AppBar.
	UINT m_uStateProposedPrev;

	// We need a member variable which tracks whether a full screen 
	// application window is open
	BOOL m_fFullScreenAppOpen;

	// We need a member variable which tracks whether our autohide window 
	// is visible or not
	BOOL m_fAutoHideIsVisible;

	// Window is entirely hidden
	BOOL m_fHidden;
	
public:	// Public member functions
	// Constructs an AppBar
	CAppBar (UINT nIDTemplate, CWnd* pParent = NULL);

	// Returns which edge we're autohidden on or ABE_NONE
	UINT GetAutohideEdge (void);

	// Sets Autohide & AlwaysOnTop to match a specified state
	void MimicState (DWORD fdwStateChangedMask, DWORD fdwState);

	// Forces the appbar's visual appearance to match it's internal state
	void SetState (void);

	// Changes the AppBar's state to ABE_UNKNOWN, ABE_FLOAT or an ABE_edge
	void SetState (UINT uState);

	// Changes the AppBar's window to reflect the persistent state info
	void SetState (APPBARSTATE& abs);

	// Retrieves the AppBar's state.  If the AppBar is being positioned, its
	// proposed state is returned instead.
	UINT GetState (void);

	// Retrieves the AppBar's entire state.  To change many state variables,
	// call this function, change the variables, and then call SetState.
	void GetState (APPBARSTATE* pabs);

	// Gets the AppBar's Autohide state
	BOOL IsBarAutohide (void);

	// Gets the AppBar's always-on-top state
	BOOL IsBarAlwaysOnTop (void);

	// Gets the AppBar's floating rectangle
	void GetFloatRect (CRect* prc);

	// Gets the AppBar's docked width/height dimension
	int  GetDockedDim (UINT uEdge);

	// Hides the AppBar
	void SetHidden(BOOL fHidden);

protected:	// Internal implementation functions
	// This function simplifies calling the shell's SHAppBarMessage function
	UINT SHAppBarMessage (DWORD dwMessage, UINT uEdge = ABE_UNKNOWN, 
		LPARAM lParam = 0, CRect *rc = NULL);

	// Get a state (ABE_FLOAT or ABE_edge) from a point (screen coordinates)
	UINT CalcProposedState (const CPoint& pt);

	// Get a retangle position (screen coordinates) from a proposed state
	void GetRect (UINT uStateProposed, CRect* prcProposed);

	// Adjust the AppBar's location to account for autohide
	// Returns TRUE if rectangle was adjusted.
	BOOL AdjustLocationForAutohide (BOOL fShow, CRect* prc);

	// If AppBar is Autohide and docked, show/hide the AppBar.
	void ShowHiddenAppBar (BOOL fShow = TRUE);

	// When Autohide AppBar is shown/hidden, slide in/out of view
	void SlideWindow (const CRect& rc);


protected:	// Overridable functions
	// Called when the AppBar's proposed state changes.
	virtual void OnAppBarStateChange(BOOL fProposed, UINT uStateProposed);

	// Called if user attempts to dock an Autohide AppBar on
	// an edge that already contains an Autohide AppBar
	virtual void OnAppBarForcedToDocked(void);

	// Called when AppBar gets an ABN_FULLSCREENAPP notification
	virtual void OnABNFullScreenApp (BOOL fOpen);

	// Called when AppBar gets an ABN_POSCHANGED notification
	virtual void OnABNPosChanged (void);

	// Called when AppBar gets an ABN_STATECHANGE notification
	virtual void OnABNStateChange (DWORD fdwStateChangedMask, DWORD fdwState);

	// Called when AppBar gets an ABN_WINDOWARRANGE notification
	virtual void OnABNWindowArrange (BOOL fBeginning);

// Dialog Data
	//{{AFX_DATA(CAppBar)
	enum { IDD = 0 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	 // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAppBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnGetMinMaxInfo( MINMAXINFO FAR* lpMMI );
	//}}AFX_MSG
	afx_msg LRESULT OnAppBarCallbackMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEnterSizeMove(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnExitSizeMove(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSizing(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMoving(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
// Inline functions.  See above for descriptions


inline BOOL CAppBar::IsEdgeLeftOrRight (UINT uEdge) { 
	return((uEdge == ABE_LEFT) || (uEdge == ABE_RIGHT)); 
}

inline BOOL CAppBar::IsEdgeTopOrBottom (UINT uEdge) { 
	return((uEdge == ABE_TOP) || (uEdge == ABE_BOTTOM)); 
}

inline void CAppBar::SetState (void) {
	SetState(GetState());
}

inline UINT CAppBar::GetState (void) { 
	return((m_uStateProposedPrev != ABE_UNKNOWN) 
		? m_uStateProposedPrev : m_abs.m_uState); 
}

inline void CAppBar::GetState (APPBARSTATE* pabs) {
	DWORD dwSizeCaller = pabs->m_cbSize;
	CopyMemory(pabs, &m_abs, pabs->m_cbSize);
	pabs->m_cbSize = dwSizeCaller;
}

inline BOOL CAppBar::IsBarAutohide (void) {
	return(m_abs.m_fAutohide); 
}

inline BOOL CAppBar::IsBarAlwaysOnTop (void) { 
	return(m_abs.m_fAlwaysOnTop); 
}

inline void CAppBar::GetFloatRect (CRect* prc) {
	*prc = m_abs.m_rcFloat; 
}

inline int CAppBar::GetDockedDim (UINT uEdge) {
	return(m_abs.m_auDimsDock[uEdge]);
}

inline int AbsoluteValue(int n) { 
	return((n < 0) ? -n : n); 
}


#endif


//////////////////////////////// End of File //////////////////////////////////
