////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CCoolBar implements coolbars for MFC.
//
#include "StdAfx.h"
#include "UICoolBar.h"
#include "UIModulVer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// if you want to see extra TRACE diagnostics, set below to TRUE
BOOL CCoolBar::bTRACE = FALSE;

#ifdef _DEBUG
#define CBTRACEFN			\
	CTraceFn __fooble;	\
	if (bTRACE)				\
		TRACE
#define CBTRACE			\
	if (bTRACE)				\
		TRACE
#else
#define CBTRACEFN TRACE
#define CBTRACE   TRACE
#endif

IMPLEMENT_DYNAMIC(CCoolBar, CControlBar)

BEGIN_MESSAGE_MAP(CCoolBar, CControlBar)
	//{{AFX_MSG_MAP(CCoolBar)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(RBN_HEIGHTCHANGE, OnHeightChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCoolBar::CCoolBar()
{
}

CCoolBar::~CCoolBar()
{
}

//////////////////
// Create coolbar
//
BOOL CCoolBar::Create(CWnd* pParentWnd, DWORD dwStyle,
	DWORD dwAfxBarStyle, UINT nID)
{
	ASSERT_VALID(pParentWnd);   // must have a parent

	// dynamic coolbar not supported
	dwStyle &= ~CBRS_SIZE_DYNAMIC;

	// save the style (this code copied from MFC--probably unecessary)
	m_dwStyle = dwAfxBarStyle;
	if (nID == AFX_IDW_TOOLBAR)
		m_dwStyle |= CBRS_HIDE_INPLACE;

	// MFC requires these:
	dwStyle |= CCS_NODIVIDER|CCS_NOPARENTALIGN;

	// initialize cool common controls
	static BOOL bInit = FALSE;
	if (!bInit) {
		HMODULE h = ::GetModuleHandle(_T("ComCtl32"));
		ASSERT(h);

		typedef BOOL (CALLBACK* INITCC)(INITCOMMONCONTROLSEX*);
		INITCC pfn = (INITCC)GetProcAddress(h, "InitCommonControlsEx");
		if (pfn) {
			INITCOMMONCONTROLSEX sex;
			sex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			sex.dwICC = ICC_COOL_CLASSES;
			(*pfn)(&sex);
		}
		bInit = TRUE;
	}

	// Finally create the cool bar using given style and parent.
	CRect rc;
	rc.SetRectEmpty();
	return CWnd::CreateEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		dwStyle, rc, pParentWnd, nID);
}

//////////////////
// Invalidate coolbar: invalidate children too
//
void CCoolBar::Invalidate(BOOL bErase)
{
	HWND hWndChild = ::GetWindow(m_hWnd, GW_CHILD);
	while (hWndChild != NULL) {
		::InvalidateRect(hWndChild, NULL, bErase);
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT);
	}
	CControlBar::Invalidate(bErase);
}

//////////////////
// Set the background bitmap. This sets the background for all bands and
// sets the RBBS_FIXEDBMP style so it looks like one background.
//
void CCoolBar::SetBackgroundBitmap(CBitmap* pBitmap)
{
	HBITMAP hbm = (HBITMAP)pBitmap->GetSafeHandle();
	CRebarBandInfo rbbi;
	int n = GetBandCount();
	for (int i=0; i< n; i++) {
		rbbi.fMask  = RBBIM_STYLE;
		GetBandInfo(i, &rbbi);
		rbbi.fMask  |= RBBIM_BACKGROUND;
		rbbi.fStyle |= RBBS_FIXEDBMP;
		rbbi.hbmBack = hbm;
		SetBandInfo(i, &rbbi);
	}
}

//////////////////
// Set background/foreground colors for all bands
//
void CCoolBar::SetColors(COLORREF clrFG, COLORREF clrBG)
{
	CRebarBandInfo rbbi;
	rbbi.fMask = RBBIM_COLORS;
	rbbi.clrFore = clrFG;
	rbbi.clrBack = clrBG;
	int n = GetBandCount();
	for (int i=0; i< n; i++)
		SetBandInfo(i, &rbbi);
}

//////////////////
// Insert band. This overloaded version lets you create a child window band.
//
BOOL CCoolBar::InsertBand(CWnd* pWnd, CSize szMin, int cx,
	LPCTSTR lpText, int iPos, BOOL bNewRow)
{
	CRebarBandInfo rbbi;
	rbbi.fMask = RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_TEXT;
	if (cx>0) {
		rbbi.fMask |= RBBIM_SIZE;
		rbbi.cx = cx;
	}
	if (bNewRow) {
		rbbi.fMask  |= RBBIM_STYLE;
		rbbi.fStyle |= RBBS_BREAK;
	}
	rbbi.hwndChild =  pWnd->GetSafeHwnd();
	rbbi.cxMinChild = szMin.cx;
	rbbi.cyMinChild = szMin.cy;
	rbbi.lpText = (LPTSTR)lpText;
	return InsertBand(iPos, &rbbi);
}

//////////////////
// Handle WM_CREATE: call virtual fn so derived class can create bands.
//
int CCoolBar::OnCreate(LPCREATESTRUCT lpcs)
{
	return (CControlBar::OnCreate(lpcs)==-1 || !OnCreateBands()) ? -1 : 0;
}

//////////////////
// Standard UI handler updates any controls in the coolbar.
//
void CCoolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

/////////////////
// These two functions are called by MFC to calculate the layout of
// the main frame. Since CCoolBar is not designed to be dynamic, the
// size is always fixed, and the same as the window size. 
//
CSize CCoolBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	return CalcFixedLayout(dwMode & LM_STRETCH, dwMode & LM_HORZ);
}

CSize CCoolBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CRect rc;
	GetWindowRect(&rc);
	CSize sz(bHorz && bStretch ? 0x7FFF : rc.Width(),
		!bHorz && bStretch ? 0x7FFF : rc.Height());
	return sz;
}

//////////////////
// Handle RBN_HEIGHTCHANGE notification: pass to virtual fn w/nicer args.
//
void CCoolBar::OnHeightChange(NMHDR* pNMHDR, LRESULT* pRes)
{
	CRect rc;
	GetWindowRect(&rc);
	OnHeightChange(rc);
	*pRes = 0; // why not?
}

//////////////////
// Height changed: Default implementation does the right thing for MFC
// doc/view: notify parent frame by posting a WM_SIZE message. This will
// cause the frame to do RecalcLayout. The message must be posted, not sent,
// because the coolbar could send RBN_HEIGHTCHANGE while the user is sizing,
// which would be in the middle of a CFrame::RecalcLayout, and RecalcLayout
// doesn't let you re-enter it. Posting guarantees that CFrameWnd can finish
// any recalc it may be in the middle of *before* handling my posted WM_SIZE.
// Very confusing, but that's MFC for you.
//
void CCoolBar::OnHeightChange(const CRect& rcNew)
{
	CWnd* pParent = GetParent();
	CRect rc;
	pParent->GetWindowRect(&rc);
	pParent->PostMessage(WM_SIZE, 0, MAKELONG(rc.Width(),rc.Height()));
}

void CCoolBar::OnPaint()
{
	Default();	// bypass CControlBar
}

BOOL CCoolBar::OnEraseBkgnd(CDC* pDC)
{
	return (BOOL)Default();  // bypass CControlBar
}
