////////////////////////////////////////////////////////////////
// Microsoft Systems Journal -- December 1999
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
#include "StdAfx.h"
#include "HtmlCtrl.h"
#include "UIMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CHtmlCtrl, CUIHtmlView)
BEGIN_MESSAGE_MAP(CHtmlCtrl, CUIHtmlView)
	ON_WM_DESTROY()
	ON_WM_MOUSEACTIVATE()
	ON_MESSAGE(WM_APP_CB_IE_SEL_CHANGE,OnAppCbIeSelChange)
END_MESSAGE_MAP()

BOOL CHtmlCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.lpszClass = AfxRegisterWndClass(
				  CS_DBLCLKS,                       
				  NULL,                             
				  NULL,                             
				  NULL); 
	ASSERT(cs.lpszClass);
	BOOL bRet = CHtmlView::PreCreateWindow(cs);
	return bRet;	
}

//////////////////
// Create control in same position as an existing static control with
// the same ID (could be any kind of control, really)
//
BOOL CHtmlCtrl::CreateFromStatic(UINT nID, CWnd* pParent)
{
	CStatic wndStatic;
	if (!wndStatic.SubclassDlgItem(nID, pParent))
		return FALSE;

	// Get static control rect, convert to parent's client coords.
	CRect rc;
	wndStatic.GetWindowRect(&rc);
	pParent->ScreenToClient(&rc);
	wndStatic.DestroyWindow();

	// create HTML control (CHtmlView)
	return Create(NULL,						 // class name
		NULL,										 // title
		(WS_CHILD | WS_VISIBLE ),			 // style
		rc,										 // rectangle
		pParent,									 // parent
		nID,										 // control ID
		NULL);									 // frame/doc context not used
}

////////////////
// Override to avoid CView stuff that assumes a frame.
//
void CHtmlCtrl::OnDestroy()
{
	// This is probably unecessary since ~CHtmlView does it, but
	// safer to mimic CHtmlView::OnDestroy.
	if (m_pBrowserApp) {
		m_pBrowserApp->Release();
		m_pBrowserApp = NULL;
	}
	CWnd::OnDestroy(); // bypass CView doc/frame stuff
}

////////////////
// Override to avoid CView stuff that assumes a frame.
//
int CHtmlCtrl::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT msg)
{
	// bypass CView doc/frame stuff
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, msg);
}

//////////////////
// Override navigation handler to pass to "app:" links to virtual handler.
// Cancels the navigation in the browser, since app: is a pseudo-protocol.
//
void CHtmlCtrl::OnBeforeNavigate2( LPCTSTR lpszURL,
	DWORD nFlags,
	LPCTSTR lpszTargetFrameName,
	CByteArray& baPostedData,
	LPCTSTR lpszHeaders,
	BOOL* pbCancel )
{
	CUIHtmlView::OnBeforeNavigate2(lpszURL,nFlags,lpszTargetFrameName,baPostedData,lpszHeaders,pbCancel);
	LPCTSTR APP_PROTOCOL = _T("app:");
	int len = _tcslen(APP_PROTOCOL);
	if (_tcsnicmp(lpszURL, APP_PROTOCOL, len)==0) {
		OnAppCmd(lpszURL + len);
		*pbCancel = TRUE;
	}
}

//////////////////
// Called when the browser attempts to navigate to "app:foo"
// with "foo" as lpszWhere. Override to handle app commands.
//
void CHtmlCtrl::OnAppCmd(LPCTSTR lpszWhere)
{
	// default: do nothing
}

LRESULT CHtmlCtrl::OnAppCbIeSelChange(WPARAM wParam, LPARAM lParam)
{
	if (wParam)
		Navigate((LPCTSTR)wParam);
	return 1;
}
