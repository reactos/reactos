/********************************************************************
*	Module:	winui.cpp. This is part of WinUI.
*
*	Purpose:	WinUI main module. Contains procedures relative to User Interface 
*			and general purpose procedures.
*
*	Authors:	Manu B.
*
*	License:	WinUI is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
*	Revisions:	
*			Manu B. 11/19/01	OpenDlg & SaveDlg enhancement.
*			Manu B. 12/07/01	CIniFile created.
*			Manu B. 12/15/01	CWinApp created.
*
********************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <ctype.h>
#include "winui.h"

// Callback procedures.
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

// Declare a CMessageBox instance to use anywhere in the whole app.
CMessageBox	MsgBox;


/********************************************************************
*	Function:	char *StpCpy(char *dest, const char *src).
*
*	Purpose:	stpcpy clone.
*
*	Revisions:	
*
********************************************************************/
char *StpCpy(char *dest, const char *src){
	while (*src != '\0'){
		*dest = *src;
		src++;
		dest++;
	}
return dest;
}

size_t strcpylen(char *dest, const char *src){
	char * start = dest;
	while (*src != '\0'){
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
return (dest-start);
}


/********************************************************************
*	Class:	CChrono.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CChrono::CChrono(){
	_time = 0;
}

CChrono::~CChrono(){
}

void CChrono::Start(void){
	_time = ::GetTickCount();
}

DWORD CChrono::Stop(void){
	DWORD diff = ::GetTickCount() - _time;
	_time = 0;
return diff;
}


/********************************************************************
*	Class:	Base Window class.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CWindow::CWindow(){
	_pParent		= NULL;
	_hInst 		= 0;
	_hWnd 		= 0;
	_lParam		= NULL;
}

CWindow::~CWindow(){
}

HWND CWindow::CreateEx(
				CWindow * pWindow, 
				DWORD dwExStyle, 
				LPCTSTR lpClassName, 
				LPCTSTR lpWindowName, 
				DWORD dwStyle, 
				int x, int y, int nWidth, int nHeight, 
				HMENU hMenu, 
				LPVOID lpParam){

	// Store a pointer to parent class and to lpParam.
	_pParent 	= pWindow;
	_lParam 	= lpParam;

	// Get parent class handles.
	HWND hParent;
	HINSTANCE hInst;

	if(_pParent){
		// Have a parent window.
		hParent 	= _pParent->_hWnd;
		hInst 	= _pParent->_hInst;
	}else{
		// Parent window is desktop.
		hParent = 0;
		hInst = GetModuleHandle(NULL);
	}

	_hWnd = CreateWindowEx(
		dwExStyle, 
		lpClassName, 
		lpWindowName, 
		dwStyle,
		x, 
		y, 
		nWidth, 
		nHeight, 
		hParent, 
		(HMENU) hMenu,
		hInst, 
		this); // Retrieve lpParam using this->_lParam.

return _hWnd;
}

HWND CWindow::GetId(void){
return _hWnd;
}

LONG CWindow::SetLong(int nIndex, LONG dwNewLong){
return ::SetWindowLong(_hWnd, nIndex, dwNewLong);
}

LONG CWindow::GetLong(int nIndex){
return ::GetWindowLong(_hWnd, nIndex);
}

LRESULT CWindow::SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam){
return ::SendMessage(_hWnd, Msg, wParam, lParam);
}

bool CWindow::SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags){
return ::SetWindowPos(_hWnd, hInsertAfter, x, y, width, height, uFlags);
}

bool CWindow::Show(int nCmdShow){
return ::ShowWindow(_hWnd, nCmdShow);
}

bool CWindow::Hide(void){
return ::ShowWindow(_hWnd, SW_HIDE);
}

HWND CWindow::SetFocus(void){
return ::SetFocus(_hWnd);
}

/********************************************************************
*	Class:	CWinBase.
*
*	Purpose:	Base Application class.
*
*	Revisions:	
*
********************************************************************/
CWinBase::CWinBase(){
	hPrevInst	= 0;
	lpCmdLine	= NULL;
	nCmdShow	= SW_SHOW;

	isWinNT 	= false;
	strcpy(	appName, 	"CWinBase");
}

CWinBase::~CWinBase(){
}

bool CWinBase::Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
												int nCmdShow){
	_hInst 	= hInstance;
	hPrevInst	= hPrevInstance;
	lpCmdLine	= lpCmdLine;
	nCmdShow	= nCmdShow;
return true;
}	

bool CWinBase::SetName(char * name, char * version){
	strcpy(appName, name);
	strcat(appName, " ");

	if(version)
		strcat(appName, version);
return true;
}	

bool CWinBase::IsWinNT(void){

	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO), 0, 0, 0, 0, ""};
	GetVersionEx(&osv);

	isWinNT = (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
return isWinNT;
}	

void CWinBase::ParseCmdLine(char * outBuff){
	int len = 0;
	outBuff[len] = '\0';

	LPTSTR cmdLine = GetCommandLine();
	
	while (*cmdLine){
		if (*cmdLine == '\"'){
			cmdLine++;
			while (*cmdLine && *cmdLine != '\"'){
				outBuff[len] = *cmdLine;
				len++;
				cmdLine++;
			}
			break;
		}else{
			while (*cmdLine && *cmdLine != ' '){
				outBuff[len] = *cmdLine;
				len++;
				cmdLine++;
			}
			break;
		}
	}
	outBuff[len] = '\0';
	SplitFileName(outBuff, 0);
return;
}


/********************************************************************
*	Class:	CSDIBase.
*
*	Purpose:	Base SDI class.
*
*	Revisions:	
*
********************************************************************/
CSDIBase::CSDIBase(){
mainClass[0] 	= 0;
hAccel 		= NULL;
}

CSDIBase::~CSDIBase(){
}

int CSDIBase::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
												int nCmdShow){
	Init(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	if (!CustomInit()){
		Release();
		return 1;
	}

	MSG Msg;
	if (CreateUI()){
		while(GetMessage(&Msg, NULL, 0, 0)){
			if (!TranslateAccelerator(_hWnd, hAccel, &Msg)){
			 TranslateMessage(&Msg);
			 DispatchMessage(&Msg);
			}
		}
	}else{
		MsgBox.DisplayFatal("CreateUI() failed !");
	}

	Release();
return Msg.wParam;
}

bool CSDIBase::CustomInit(void){
return true;
}

bool CSDIBase::Release(void){
return true;
}

bool CSDIBase::CreateUI(void){
return false;
}

bool CSDIBase::MainRegisterEx(const char * className){
	strcpy(mainClass, className);

	// Default values.
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.lpfnWndProc		= MainWndProc;
	wc.hInstance		= _hInst;
	wc.lpszClassName	= mainClass;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(CSDIBase *);
return RegisterClassEx(&wc);
}

LRESULT CALLBACK CSDIBase::CMainWndProc(UINT Message, WPARAM wParam, LPARAM lParam){

	switch (Message){
		case WM_DESTROY:
		PostQuitMessage (0);
		return 0;
	}
	
return DefWindowProc(_hWnd, Message, wParam, lParam);
}

/********************************************************************
*	Callback procedure.
********************************************************************/
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
	// Get a pointer to the CWindow class that owns this window.
	CSDIBase * pSdiBase = (CSDIBase *) (GetWindowLong(hwnd, 0));

	if (pSdiBase == 0){	// The pointer isn't set yet.
		if (Message == WM_NCCREATE){
			// First, get the CSDIBase pointer.
			pSdiBase = (CSDIBase *) ((CREATESTRUCT *)lParam)->lpCreateParams;

			// Attach pSdiBase and lParam to the window.
			SetWindowLong(hwnd, 0, (LONG)(pSdiBase));
			SetWindowLong(hwnd, GWL_USERDATA, (LONG) pSdiBase->_lParam);

			// Store window handle.
			pSdiBase->_hWnd = hwnd;

			// Let Windows continue the job.
			return pSdiBase->CMainWndProc(Message, wParam, lParam);
		}else{
			return DefWindowProc(hwnd, Message, wParam, lParam);
		}
	}
return pSdiBase->CMainWndProc(Message, wParam, lParam);
}


/********************************************************************
*	Class:	CMDIBase.
*
*	Purpose:	Base MDI class.
*
*	Revisions:	
*
********************************************************************/
CMDIBase::CMDIBase(){
}

CMDIBase::~CMDIBase(){
}

bool CMDIBase::ChildRegisterEx(const char * className){
	strcpy(MdiClient.childClass, className);

	// Default values.
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.lpfnWndProc		= ChildWndProc;
	wc.hInstance		= _hInst;
	wc.lpszClassName	= className;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 8;//sizeof(CMDIBase *);
return RegisterClassEx(&wc);
}

int CMDIBase::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
												int nCmdShow){
	Init(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	if (!CustomInit()){
		Release();
		return 1;
	}

	MSG Msg;
	if (CreateUI()){
		while(GetMessage(&Msg, NULL, 0, 0)){
			if (!TranslateAccelerator(_hWnd, hAccel, &Msg)){
				if (!TranslateMDISysAccel(MdiClient.GetId(), &Msg)){
					 TranslateMessage(&Msg);
					 DispatchMessage(&Msg);
				}
			}
		}
	}else{
		MsgBox.DisplayFatal("CreateUI() failed !");
	}

	Release();
return Msg.wParam;
}

bool CMDIBase::CustomInit(void){
return true;
}

bool CMDIBase::Release(void){
return true;
}

bool CMDIBase::CreateUI(void){
return false;
}

LRESULT CALLBACK CMDIBase::CMainWndProc(UINT, WPARAM, LPARAM){
return 0;
}

LRESULT CALLBACK CMDIBase::CChildWndProc(CWindow *, UINT, WPARAM, LPARAM){
return 0;
}

/********************************************************************
*	Child window callback procedure.
********************************************************************/
LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
	// Get a pointer to the CWindow class that owns this window.
	CMDIChild * pMdiChild = (CMDIChild *) (GetWindowLong(hwnd, 0));

	if (pMdiChild == 0){	// The pointer isn't set yet.
		if (Message == WM_NCCREATE){
			// First, get the CMDIChild pointer.
			pMdiChild = (CMDIChild *) ((MDICREATESTRUCT *) ((CREATESTRUCT *)lParam)->lpCreateParams)->lParam;

			// Attach pMdiChild and lParam to the window.
			//MsgBox.DisplayLong((long) pMdiChild);

			SetWindowLong(hwnd, 0, (long)(pMdiChild));
			SetWindowLong(hwnd, GWL_USERDATA, (long) pMdiChild->_lParam);

			// Store window handle.
			pMdiChild->_hWnd = hwnd;

			// Attach to a childList so that the list can destroy all MdiChild objects.
			((CMDIClient *) pMdiChild->_pParent)->childList.InsertFirst(pMdiChild);

			// Let Windows continue the job.
			return pMdiChild->_pFrame->CChildWndProc(pMdiChild, Message, wParam, lParam);
		}else{
			return DefWindowProc(hwnd, Message, wParam, lParam);
		}

	// Free pMdiChild object.
	}else if (Message == WM_NCDESTROY){
		((CMDIClient *) pMdiChild->_pParent)->childList.Destroy(pMdiChild);
	}
return pMdiChild->_pFrame->CChildWndProc(pMdiChild, Message, wParam, lParam);
}


/********************************************************************
*	Class:	CMDIClient.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CMDIClient::CMDIClient(){
	initialized = false;
	childClass[0] = 0;
}

CMDIClient::~CMDIClient(){
}

void CMDIClient::Init(int menuIndex, UINT idFirstChild){
	nPos = menuIndex;

	// ID of first created child.
	ccs.idFirstChild = idFirstChild;
	initialized = true;
}

HWND CMDIClient::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId){

	if (!pWindow || !initialized)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	// Get our "Window menu" handle.
	ccs.hWindowMenu  = GetSubMenu(GetMenu(_pParent->_hWnd), nPos);

	_hWnd = CreateWindowEx(
		dwExStyle,
		"mdiclient", 
		NULL,
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd,
		(HMENU) resId,
		_hInst, 
		(LPVOID) &ccs);
	
return _hWnd;
}

LPARAM CMDIClient::GetParam(LPARAM lParam){
	MDICREATESTRUCT *mcs = (MDICREATESTRUCT *) ((CREATESTRUCT *) lParam)->lpCreateParams;
return mcs->lParam;
}


/********************************************************************
*	Class:	CMDIChild.
*
*	Purpose:	Base Windows Class.
*
*	Revisions:	
*
********************************************************************/
CMDIChild::CMDIChild(){
	_pFrame = NULL;
}

CMDIChild::~CMDIChild(){
}

HWND CMDIChild::CreateEx(CMDIClient * pMdiClient, DWORD dwExStyle, DWORD dwStyle, char * caption, UINT resId, LPVOID lParam){

	if (!pMdiClient || !pMdiClient->initialized)
		return 0;

	// Store pointers, lParam and _hInst.
	_pParent = pMdiClient;
	_pFrame = (CMDIBase *) pMdiClient->_pParent; // Owner of CChildWndProc.
	_lParam = lParam;
	_hInst = _pParent->_hInst;

	HWND hwnd = CreateWindowEx(
				dwExStyle,
				pMdiClient->childClass,
				caption,
				dwStyle,
				0, 
				0, 
				100, 
				100, 
				_pParent->_hWnd, 
				(HMENU) resId,
				_hInst,
				this);
	
return hwnd;
}


/********************************************************************
*	Class:	CDlgBase.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CDlgBase::CDlgBase(){
}

CDlgBase::~CDlgBase(){
}

HWND CDlgBase::Create(CWindow * pWindow, WORD wResId, RECT * Pos, LPARAM lParam){
	HWND hwnd = 0;
	if (pWindow){
		hwnd = CreateParam(pWindow, wResId, lParam);
		if (Pos)
			SetPosition(0, Pos->left, Pos->top, Pos->right, Pos->bottom, 0);
	}
return hwnd;
}

int CDlgBase::CreateModal(CWindow * pWindow, WORD wResId, LPARAM lParam){
	if (pWindow == NULL)
		return 0;

	// Don't create a modal dialog if a modeless one already exists.
	else if (_hWnd != 0)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;
	_lParam = (LPVOID) lParam;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(wResId), _pParent->_hWnd,
				(DLGPROC) DlgProc, (long) this);
}

HWND CDlgBase::CreateParam(CWindow * pWindow, WORD wResId, LPARAM lParam){
	if (pWindow == NULL)
		return 0;
	// Don't create a dialog twice.
	else if (_hWnd != 0)
		return _hWnd;

	// Store a pointer to parent class.
	_pParent = pWindow;
	_lParam = (LPVOID) lParam;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	HWND hwnd = CreateDialogParam(_hInst,
	                                   MAKEINTRESOURCE(wResId),
	                                   _pParent->_hWnd,
	                                   (DLGPROC) DlgProc,
	                                   (long) this);
return hwnd;
}

BOOL CDlgBase::EndDlg(int nResult){
	if (_hWnd){
		BOOL result = ::EndDialog(_hWnd, nResult);
		_hWnd = 0;
		return result;
	}
return false;
}

HWND CDlgBase::GetItem(int nIDDlgItem){
return ::GetDlgItem(_hWnd, nIDDlgItem);
}

BOOL CDlgBase::SetItemText(HWND hItem, LPCTSTR lpString){
return ::SendMessage(hItem, WM_SETTEXT, 0, (long)lpString);
}

UINT CDlgBase::GetItemText(HWND hItem, LPTSTR lpString, int nMaxCount){
return ::SendMessage(hItem, WM_GETTEXT, nMaxCount, (long)lpString);
}

LRESULT CALLBACK CDlgBase::CDlgProc(UINT Message, WPARAM, LPARAM){

	switch(Message){
		case WM_INITDIALOG:
			return TRUE;
	
		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}


/********************************************************************
*	Class:	CTabbedDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CTabbedDlg::CTabbedDlg(){
	_hWndTab = 0;
	Pos.left 	= 0;
	Pos.top 	= 0;
	Pos.right 	= 100;
	Pos.bottom	= 100;
}

CTabbedDlg::~CTabbedDlg(){
}

bool CTabbedDlg::SetChildPosition(HWND hChild){
	if (!_hWndTab)
		return false;
	// Get tab's display area.
	RECT area;
	::GetWindowRect(_hWndTab, &area);
	::ScreenToClient(_hWnd, (POINT *) &area.left);
	::ScreenToClient(_hWnd, (POINT *) &area.right);
	::SendMessage(_hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM) &area);
	::CopyRect(&Pos, &area);

	// Get child dialog's rect.
	RECT child;
	::GetWindowRect(hChild, &child);
	::ScreenToClient(_hWnd, (POINT *) &child.left);
	::ScreenToClient(_hWnd, (POINT *) &child.right);

	// Center child dialog.
	int childWidth = child.right-child.left;
	int childHeight = child.bottom-child.top;
	int hMargin = ((area.right-area.left)-childWidth)/2;
	int vMargin = ((area.bottom-area.top)-childHeight)/2;
	Pos.left += hMargin;
	Pos.top += vMargin;
	Pos.right = childWidth;
	Pos.bottom = childHeight;

return ::SetWindowPos(hChild, 0, Pos.left, Pos.top, Pos.right, Pos.bottom, 0);
}

void CTabbedDlg::OnNotify(int, LPNMHDR notify){
	// Dispatch tab control messages.
	switch (notify->code){
		case TCN_SELCHANGING:
			OnSelChanging(notify);
		break;

		case TCN_SELCHANGE:
			OnSelChange(notify);
		break;
	}
}

void CTabbedDlg::OnSelChanging(LPNMHDR notify){
	// Hide child dialog that is deselected.
	if (notify->hwndFrom == _hWndTab){
		CWindow * pPaneDlg = (CWindow *) GetParam();
		if (pPaneDlg){
			if (pPaneDlg->_hWnd)
				pPaneDlg->Hide();
		}
	}
}

void CTabbedDlg::OnSelChange(LPNMHDR notify){
	// Show child dialog that is selected.
	if (notify->hwndFrom == _hWndTab){
		CWindow * pPaneDlg = (CWindow *) GetParam();
		if (pPaneDlg){
			if (pPaneDlg->_hWnd)
				pPaneDlg->Show();
				pPaneDlg->SetFocus();
		}
	}
}

LPARAM CTabbedDlg::GetParam(void){
	if (!_hWndTab)
		return false;
	int iItem = ::SendMessage(_hWndTab, TCM_GETCURSEL, 0, 0);

	tcitem.mask = TCIF_PARAM;
	BOOL result = ::SendMessage(_hWndTab, TCM_GETITEM, iItem, (long) &tcitem);
	if (result)
		return tcitem.lParam;
return 0;
}


/********************************************************************
*	Callback procedure.
********************************************************************/
LRESULT CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam){
	// Get a pointer to the CWindow class that owns this window.
	CDlgBase * pDlgBase = (CDlgBase *) (GetWindowLong(hwnd, DWL_USER));

	if (pDlgBase == 0){	// The pointer isn't set yet.
		if (Message == WM_INITDIALOG){
			// First, get the CDlgBase pointer.
			pDlgBase = (CDlgBase *) lParam;

			// Attach pDlgBase and lParam to the window.
			SetWindowLong(hwnd, DWL_USER, (LONG) pDlgBase);
			SetWindowLong(hwnd, GWL_USERDATA, (LONG) pDlgBase->_lParam);

			// Store window handle.
			pDlgBase->_hWnd = hwnd;

			// Let Windows continue the job.
			return pDlgBase->CDlgProc(Message, wParam, (LONG) pDlgBase->_lParam);
		}else{
			return FALSE;
		}
	}
return pDlgBase->CDlgProc(Message, wParam, lParam);
}


/********************************************************************
*	Class:	CToolBar.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CToolBar::CToolBar(){
}

CToolBar::~CToolBar(){
}

HWND CToolBar::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId){

	if (!pWindow)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle, 
		TOOLBARCLASSNAME, 
		NULL, 
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd, 
		(HMENU) resId,
		_hInst, 
		NULL); 

	if(_hWnd)
	// For backward compatibility. 
		::SendMessage(_hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 
return _hWnd; 
} 
 
LRESULT CToolBar::AddBitmap(UINT resId, int nBmp, HINSTANCE hInstance){
	if (!_hWnd)
		return -1;

	// Add the bitmap containing button images to the toolbar. 
	TBADDBITMAP tbab; 
	if (hInstance == HINST_COMMCTRL)
		tbab.hInst	= hInstance; 
	else
		tbab.hInst	= _hInst; 
	tbab.nID	= resId;
return ::SendMessage(_hWnd, TB_ADDBITMAP, (WPARAM) nBmp, (WPARAM) &tbab); 
}

BOOL CToolBar::AddButtons(TBBUTTON * tbButtons, UINT numButtons){
	if (!_hWnd)
		return FALSE;

	// Add the buttons to the toolbar. 
return ::SendMessage(_hWnd, TB_ADDBUTTONS, (WPARAM) numButtons, (LPARAM) tbButtons); 
}


/********************************************************************
*	Class:	CStatusBar.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CStatusBar::CStatusBar(){
	numParts = 0;
}

CStatusBar::~CStatusBar(){
}

HWND CStatusBar::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId){

	if (!pWindow)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle,
		STATUSCLASSNAME, 
		NULL,
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd,
		(HMENU) resId,
		_hInst,
		NULL);

return _hWnd;
}

void CStatusBar::SetParts(int nParts, int * aWidths){
	numParts = nParts;
	::SendMessage(_hWnd, SB_SETPARTS, nParts, (LPARAM) aWidths);
}

void CStatusBar::WriteString(char * string, int part){
	if (part <= numParts)
		::SendMessage(_hWnd, SB_SETTEXT, part, (LPARAM) string);
}

void CStatusBar::WriteLong(long number, int part){
	char longbuf[10];
	itoa (number, longbuf, 10);
	WriteString(longbuf, part);
}


/********************************************************************
*	Class:	CTabCtrl.
*
*	Purpose:	Tab control.
*
*	Revisions:	
*
********************************************************************/
CTabCtrl::CTabCtrl(){
}

CTabCtrl::~CTabCtrl(){
}

HWND CTabCtrl::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId, LPVOID lpParam){

	if (!pWindow)
		return 0;

	// TODO also use lpParam for each control ?
	// Store a pointer to parent class and to lpParam.
	_pParent 	= pWindow;
	_lParam 	= lpParam;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle,
		WC_TABCONTROL, 
		NULL,
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd,
		(HMENU) resId,
		_hInst,
		lpParam);

return _hWnd;
}

int CTabCtrl::InsertItem(int iItem, UINT mask, DWORD dwState, DWORD dwStateMask, 
				LPTSTR pszText, int cchTextMax, int iImage, LPARAM lParam){

	tcitem.mask 			= mask;

		#if (_WIN32_IE >= 0x0300)
		tcitem.dwState 		= dwState;
		tcitem.dwStateMask 	= dwStateMask;
		#else
		tcitem.lpReserved1 = 0;
		tcitem.lpReserved2 = 0;
		#endif

	tcitem.pszText 			= pszText;
	tcitem.cchTextMax 		= cchTextMax;
	tcitem.iImage 			= iImage;
	tcitem.lParam 			= lParam;

return ::SendMessage(_hWnd, TCM_INSERTITEM, iItem, (long) &tcitem);
}

BOOL CTabCtrl::SetItem_Param(int iItem, LPARAM lParam){

	tcitem.mask = TCIF_PARAM;
	tcitem.lParam = lParam;

return ::SendMessage(_hWnd, TCM_SETITEM, iItem, (long) &tcitem);
}

int CTabCtrl::GetCurSel(void){

return ::SendMessage(_hWnd, TCM_GETCURSEL, 0, 0);
}

LPARAM CTabCtrl::GetItem_Param(int iItem){

	tcitem.mask = TCIF_PARAM;
	BOOL result = ::SendMessage(_hWnd, TCM_GETITEM, iItem, (long) &tcitem);
	if (result)
		return tcitem.lParam;
return 0;
}


/********************************************************************
*	Class:	CTreeView.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CTreeView::CTreeView(){
}

CTreeView::~CTreeView(){
}

HWND CTreeView::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId, LPVOID lpParam){

	if (!pWindow)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle,
		WC_TREEVIEW, 
		NULL,
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd,
		(HMENU) resId,
		_hInst,
		lpParam);

return _hWnd;
}

HTREEITEM CTreeView::CreateItem(HTREEITEM hParent, HTREEITEM hInsertAfter, int iImage, LPTSTR pszText, LPARAM lParam){

	tvi.hParent			= hParent; 
	tvi.hInsertAfter		= hInsertAfter;

	tvi.item.mask		= TVIF_STATE | TVIF_TEXT | TVIF_IMAGE
					| TVIF_SELECTEDIMAGE | TVIF_PARAM ;
	tvi.item.stateMask 	= TVIS_EXPANDED;
	tvi.item.state 		= TVIS_EXPANDED;
	tvi.item.pszText		= pszText; 
	tvi.item.iImage		= iImage; 
	tvi.item.iSelectedImage	= iImage; 
	tvi.item.lParam		= (LPARAM) lParam; 

return (HTREEITEM) ::SendMessage(_hWnd, TVM_INSERTITEM, 0, (LPARAM)&tvi);
}

LPARAM CTreeView::GetSelectedItemParam(void){
	_TvItem.hItem  = TreeView_GetSelection(_hWnd);
	_TvItem.mask   = TVIF_PARAM;
	_TvItem.lParam = 0; 
	TreeView_GetItem(_hWnd, (long) &_TvItem); 
return _TvItem.lParam;
}

/********************************************************************
*	Class:	CListView.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CListView::CListView(){
	lastRow = 0;
}

CListView::~CListView(){
}

HWND CListView::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId){

	if (!pWindow)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle, 
		WC_LISTVIEW, 
		NULL, 
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd, 
		(HMENU) resId,
		_hInst, 
		NULL);

return _hWnd;
}

void CListView::Clear(void){
	// Win9x ListView clear is too slow.
	int numitems = ::SendMessage(_hWnd, LVM_GETITEMCOUNT, 0, 0);
	for (int n = 0; n < numitems; n++){
		::SendMessage(_hWnd, LVM_DELETEITEM, (WPARAM) 0, 0);
	}
	lastRow = 0;
}


/********************************************************************
*	Class:	CScintilla.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CScintilla::CScintilla(){
}

CScintilla::~CScintilla(){
}

HWND CScintilla::CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle, UINT resId, LPVOID lpParam){

	if (!pWindow)
		return 0;

	// Store a pointer to parent class.
	_pParent = pWindow;

	// Get parent class handles.
	_hInst = _pParent->_hInst;

	_hWnd = CreateWindowEx(
		dwExStyle,
		"Scintilla", 
		NULL,
		dwStyle,
		0, 
		0, 
		100, 
		100, 
		_pParent->_hWnd,
		(HMENU) resId,
		_hInst,
		lpParam);

return _hWnd;
}


/********************************************************************
*	Class:	CSplitter.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CSplitter::CSplitter(){
	Pane1 	= NULL;
	Pane2 	= NULL;
	size		= 4;
	p		= 0;
	isActive	= false;
	initialized	= false;
}

CSplitter::~CSplitter(){
}

void CSplitter::Init(CWindow * pane1, CWindow * pane2, bool vertical, 
				int barPos, int barMode){
	Pane1 = pane1;
	Pane2 = pane2;

	if(Pane1 && Pane2)
		initialized = true;

	isVertical = vertical;

	p = barPos;
	mode = barMode;
}

bool CSplitter::Show(int){
return false;
}

bool CSplitter::Hide(void){
return false;
}

bool CSplitter::SetPosition(HWND, int x, int y, int width, int height, UINT){
	pos.left	= x; 
	pos.top	= y; 
	pos.right	= width; 
	pos.bottom = height; 

	if(isVertical)
		SetVertPosition();
	else
		SetHorzPosition();
return true;
}

void CSplitter::SetVertPosition(void){
	psize			= p+size;

	barPos.left		= pos.left+p; 
	barPos.top		= pos.top; 
	barPos.right	= size; 
	barPos.bottom 	= pos.bottom;

	Pane1->SetPosition(0,	
		pos.left,
		pos.top,
					p,
		pos.bottom,
		0);

	Pane2->SetPosition(0,
		pos.left		+psize,
		pos.top, 
		pos.right	-psize,
		pos.bottom,
		0);
}

void CSplitter::SetHorzPosition(void){
	psize			= p+size;

	barPos.left		= pos.left; 
	barPos.top		= pos.top+pos.bottom-psize; 
	barPos.right	= pos.right; 
	barPos.bottom 	= size;

	Pane1->SetPosition(0,
		pos.left,
		pos.top,
		pos.right,
		pos.bottom	-psize,
		0);

	Pane2->SetPosition(0,
		pos.left,
		pos.top		+pos.bottom-p, 
		pos.right,
					p,
		0);
}

bool CSplitter::HaveMouse(HWND hwnd, short, short){
	POINT ptCursor;
	::GetCursorPos(&ptCursor);
	POINT ptClient = ptCursor;
	::ScreenToClient(hwnd, &ptClient);

	if (	ptClient.x >= barPos.left 
		&& ptClient.x <= barPos.left+barPos.right
		&& ptClient.y >= barPos.top
		&& ptClient.y <= barPos.top+barPos.bottom){
	return true;
	}
return false;
}

bool CSplitter::OnSetCursor(HWND hwnd, LPARAM){
	if(HaveMouse(hwnd, 0, 0)){
		if (isVertical){
			::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			return true;
		}else{
			::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
			return true;
		}
	}
return false;
}

bool CSplitter::OnLButtonDown(HWND hwnd, short xPos, short yPos){
	if (HaveMouse(hwnd, 0, 0)){
		// Begin of mouse capture.
		isActive = true;
		::SetCapture(hwnd);

		// Save initial mouse position.
		initialPos.x = xPos;
		initialPos.y = yPos;

		// Converts bar position for Xor bar.
		POINT xorPos;
		RECT rect;
		xorPos.x = barPos.left;
		xorPos.y = barPos.top;
	
		ClientToWindow(hwnd, &xorPos, &rect);

		// Draw the Xor bar.
		HDC hdc;
		hdc = GetWindowDC(hwnd);
		DrawXorBar(hdc, xorPos.x, xorPos.y, barPos.right, barPos.bottom);

		// Save initial position for future erasing.
		initialXorPos.x = xorPos.x;
		initialXorPos.y = xorPos.y;

		ReleaseDC(hwnd, hdc);
	}
return isActive;
}

void CSplitter::OnLButtonUp(HWND hwnd, short xPos, short yPos){
	if (isActive) {
	
		// Erase the Xor bar *********************
		HDC hdc;
		hdc = GetWindowDC(hwnd);
		DrawXorBar(hdc, initialXorPos.x, initialXorPos.y, barPos.right, barPos.bottom);
		ReleaseDC(hwnd, hdc);

		// Move Splitter to new position *******************
		newPos.x = xPos;
		newPos.y = yPos;

		if(isVertical)
			deltaPos = newPos.x - initialPos.x;
		else
			deltaPos = newPos.y - initialPos.y;/**/

		if (deltaPos != 0)
			Move(barPos.left + deltaPos, barPos.top + deltaPos);

		// End of capture.
		::ReleaseCapture();
		isActive = false;
	}
}

void CSplitter::OnMouseMove(HWND hwnd, short xPos, short yPos){
	if (isActive) {
		newPos.x = xPos;
		newPos.y = yPos;
	
		// Draw the Xor bar *********************
		POINT xorPos;
		RECT rect;

		if(isVertical){
			deltaPos = newPos.x - initialPos.x;
			xorPos.x = barPos.left + deltaPos;
			xorPos.y = barPos.top;

			// Convert coordinates.
			ClientToWindow(hwnd, &xorPos, &rect);
		
			// Convert rect.
			OffsetRect(&rect, -rect.left, -rect.top);
		
			if(xorPos.x < 20) 
				xorPos.x = 20;
			if(xorPos.x > rect.right-24) 
				xorPos.x = rect.right-24;
		}else{
			deltaPos = newPos.y - initialPos.y;
			xorPos.x = barPos.left;
			xorPos.y = barPos.top + deltaPos;

			// Convert coordinates.
			ClientToWindow(hwnd, &xorPos, &rect);
		
			// Convert rect.
			OffsetRect(&rect, -rect.left, -rect.top);
	
			if(xorPos.y < 20) 
				xorPos.y = 20;
			if(xorPos.y > rect.bottom-24) 
				xorPos.y = rect.bottom-24;
		}

		HDC hdc;
		hdc = GetWindowDC(hwnd);
		DrawXorBar(hdc, initialXorPos.x, initialXorPos.y, barPos.right, barPos.bottom);
		DrawXorBar(hdc, xorPos.x, xorPos.y, barPos.right, barPos.bottom);
		initialXorPos.x = xorPos.x;
		initialXorPos.y = xorPos.y;
		ReleaseDC(hwnd, hdc);
	}
}

void CSplitter::Move(int mouseX, int mouseY){

	if(isVertical){
		p = mouseX;
		SetVertPosition();
	}else{
		p = pos.bottom - size - mouseY+pos.top;
		SetHorzPosition();
	}
}

void CSplitter::ClientToWindow(HWND hwnd, POINT * pt, RECT * rect){
	// Bar position.
	ClientToScreen(hwnd, pt);
	// Window rect.
	GetWindowRect(hwnd, rect);

	// Convert the bar coordinates relative to the top-left of the window.
	pt->x -= rect->left;
	pt->y -= rect->top;
}

void CSplitter::DrawXorBar(HDC hdc, int x1, int y1, int width, int height){
	static WORD _dotPatternBmp[8] = 
	{ 
		0x00aa, 0x0055, 0x00aa, 0x0055, 
		0x00aa, 0x0055, 0x00aa, 0x0055
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);
	
	SetBrushOrgEx(hdc, x1, y1, 0);
	hbrushOld = (HBRUSH)SelectObject(hdc, hbr);
	
	PatBlt(hdc, x1, y1, width, height, PATINVERT);
	
	SelectObject(hdc, hbrushOld);
	
	DeleteObject(hbr);
	DeleteObject(hbm);
}


/********************************************************************
*	Class:	CBitmap.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CBitmap::CBitmap(){
	hBitmap = NULL;
}

CBitmap::~CBitmap(){
	if (hBitmap)
		DeleteObject(hBitmap);
}

HBITMAP CBitmap::Load(CWindow * pWindow, LPCTSTR lpBitmapName){
	if (!hBitmap)	// Don't create twice.
		hBitmap = ::LoadBitmap(pWindow->_hInst, lpBitmapName);	
return hBitmap;
}

HBITMAP CBitmap::Load(CWindow * pWindow, WORD wResId){
	if (!hBitmap)	//Don't create twice.
		hBitmap = ::LoadBitmap(pWindow->_hInst, MAKEINTRESOURCE(wResId));	
return hBitmap;
}

BOOL CBitmap::Destroy(void){
	BOOL result = false;
	if (hBitmap)
		result = DeleteObject(hBitmap);
return result;
}


/********************************************************************
*	Class:	CImageList.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CImageList::CImageList(){
	hImgList = NULL;
}

CImageList::~CImageList(){
	if (hImgList)
		ImageList_Destroy(hImgList);
}

HIMAGELIST CImageList::Create(int cx, int cy, UINT flags, int cInitial, int cGrow){
	if (!hImgList)	// Don't create twice.
		hImgList = ::ImageList_Create(cx, cy, flags, cInitial, cGrow);	
return hImgList;
}

int CImageList::AddMasked(CBitmap * pBitmap, COLORREF crMask){
return ImageList_AddMasked(hImgList, pBitmap->hBitmap, crMask);
}


/********************************************************************
*	Class:	CMessageBox.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CMessageBox::CMessageBox(){
	hParent = 0;
	strcpy(caption, "CMessageBox");
}

CMessageBox::~CMessageBox(){
}

void CMessageBox::SetParent(HWND hwnd){
	hParent = hwnd;
}

void CMessageBox::SetCaption(char * string){
	strcpy(caption, string);
}

void CMessageBox::DisplayString(char * string, char * substring, UINT uType){
	if(!substring){
		MessageBox (hParent, string, caption, uType);
	}else{
		sprintf(msgBuf, string, substring);
		MessageBox (hParent, msgBuf, caption, uType);
	}
}

void CMessageBox::DisplayFatal(char * string, char * substring){
	DisplayString(string, substring, MB_OK | MB_ICONERROR);
}

void CMessageBox::DisplayWarning(char * string, char * substring){
	DisplayString(string, substring, MB_OK | MB_ICONWARNING);
}

void CMessageBox::DisplayLong(long number){
	char longbuf[10];
	itoa (number, longbuf, 10);
	DisplayString(longbuf);
}

void CMessageBox::DisplayRect(RECT * rect){
	if (!rect)
		return;
	sprintf(msgBuf, "left: %ld top: %ld right: %ld bottom: %ld", rect->left, rect->top, rect->right, rect->bottom);
	MessageBox (hParent, msgBuf, caption, MB_OK);
}

int CMessageBox::Ask(char * question, bool canCancel){

	if (canCancel){
		// Cancel button.
		return MessageBox(hParent, question, caption, MB_YESNOCANCEL | MB_ICONWARNING);
	}else{
		// No cancel.
		return MessageBox(hParent, question, caption, MB_YESNO | MB_ICONWARNING);
	}
}

int CMessageBox::AskToSave(bool canCancel){

	char question[] = "Save changes ?";

	if (canCancel){
		// Cancel button.
		return Ask(question, true);
	}else{
		// No cancel.
		return Ask(question, false);
	}
}


/********************************************************************
*	Class:	CFileDlgBase.
*
*	Purpose:	Open/Save Dlg.
*
*	Revisions:	
*
********************************************************************/
CFileDlgBase::CFileDlgBase(){
	ofn.lStructSize = sizeof(OPENFILENAME);	// DWORD
	Reset();
}

CFileDlgBase::~CFileDlgBase(){
}

void CFileDlgBase::Reset(void){
	// Set methods.
	ofn.lpstrTitle 		= 0;					// LPCTSTR
	ofn.nFilterIndex 		= 0;					// DWORD
	ofn.lpstrFilter 		= 0;					// LPCTSTR
	ofn.lpstrDefExt 		= 0;					// LPCTSTR
	ofn.Flags 			= 0;					// DWORD

	ofn.lpstrInitialDir 		= 0;					// LPCTSTR

	// Get methods.
	nNextFileOffset		= 0;
	ofn.nFileOffset 		= 0;					// WORD
	ofn.nFileExtension 	= 0;					// WORD

	// Unused.
	ofn.hInstance 		= 0;					// HINSTANCE
	ofn.lpstrCustomFilter 	= 0;					// LPTSTR
	ofn.nMaxCustFilter 	= 0;					// DWORD
	ofn.lpstrFileTitle 		= 0;					// LPTSTR
	ofn.nMaxFileTitle 		= 0;					// DWORD
	ofn.lCustData 		= 0;					// DWORD
	ofn.lpfnHook 		= 0;					// LPOFNHOOKPROC
	ofn.lpTemplateName	= 0;					// LPCTSTR
}

void CFileDlgBase::SetData(char * filter, char * defExt, DWORD flags){
	SetFilter(filter);
	SetDefExt(defExt);
	SetFlags(flags);
}

void CFileDlgBase::SetTitle(char * title){
	ofn.lpstrTitle = title;
}

void CFileDlgBase::SetFilterIndex(DWORD filterIndex){
	ofn.nFilterIndex	= filterIndex;
}

void CFileDlgBase::SetFilter(char * filter){
	ofn.lpstrFilter = filter;
}

void CFileDlgBase::SetDefExt(char * defExt){
	ofn.lpstrDefExt = defExt;
}

void CFileDlgBase::SetFlags(DWORD flags){
	ofn.Flags = flags;
}

void CFileDlgBase::SetInitialDir(char * lpstrInitialDir){
	ofn.lpstrInitialDir = lpstrInitialDir;
}

WORD CFileDlgBase::GetFileOffset(void){
return ofn.nFileOffset;
}

WORD CFileDlgBase::GetFileExtension(void){
return ofn.nFileExtension;
}

WORD CFileDlgBase::GetNextFileOffset(void){
	// Analyses a "path\0file1\0file2\0file[...]\0\0" string returned by Open/SaveDlg.
	char * srcFiles = ofn.lpstrFile;
	if (srcFiles){
		if ((ofn.Flags & OFN_ALLOWMULTISELECT) == OFN_ALLOWMULTISELECT){
			// Initialize first call.
			if (!nNextFileOffset)
				nNextFileOffset = ofn.nFileOffset;
			// Parse the string.
			while (srcFiles[nNextFileOffset] != '\0')
				nNextFileOffset++;
			// Get next char.
			nNextFileOffset++;
			// End of string ?
			if (srcFiles[nNextFileOffset] == '\0')
				nNextFileOffset = 0;
			return nNextFileOffset;
		}
	}
return 0;
}

bool CFileDlgBase::OpenFileName(CWindow * pWindow, char * pszFileName, DWORD nMaxFile){
	if (!pWindow)
		return false;

	ofn.hwndOwner 		= pWindow->_hWnd;		// HWND
	ofn.lpstrFile 		= pszFileName;			// LPTSTR
	ofn.nMaxFile 		= nMaxFile;				// DWORD

return ::GetOpenFileName(&ofn);
}

bool CFileDlgBase::SaveFileName(CWindow * pWindow, char * pszFileName, DWORD nMaxFile){
	if (!pWindow)
		return false;
	pszFileName[0] = '\0';
	ofn.hwndOwner 		= pWindow->_hWnd;		// HWND
	ofn.lpstrFile 		= pszFileName;			// LPTSTR
	ofn.nMaxFile 		= nMaxFile;				// DWORD

return ::GetSaveFileName(&ofn);
}


/********************************************************************
*	Class:	CPath.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CPath::CPath(){
}

CPath::~CPath(){
}

bool CPath::ChangeDirectory(char * dir){
	return SetCurrentDirectory(dir);
}


/********************************************************************
*	Paths.
********************************************************************/
void SplitFileName(char * dirName, char * fileName){
	int n;
	int len = strlen (dirName);

	for (n = len; n > 2; n--){
		if (dirName[n] == '\\' ){
			if (fileName)
				strcpy (fileName, &dirName[n+1]);
			dirName[n+1] = 0;
			break;
		}
	}
}

bool ChangeFileExt(char * fileName, char * ext){
	int len = strlen(fileName);
	for (int n=len; n > 0; n--){
		if (fileName[n] == '.'){
			fileName[n+1] = 0;
			strcat(fileName, ext);
			return true;
		}
	}
return false;	// No file extension.
}


/********************************************************************
*	Class:	CIniFile.
*
*	Purpose:	"GetPrivateProfileString()" like procedure.
*
*	Revisions:	
*
********************************************************************/
CIniFile::CIniFile(){
	buffer	= NULL;
	pcurrent 	= NULL;
	psection 	= NULL;
}

CIniFile::~CIniFile(){
	Close();
}

void CIniFile::Close(void){
	if (buffer)
		free(buffer);
	buffer	= NULL;
	pcurrent 	= NULL;
	psection 	= NULL;
}

bool CIniFile::Load(char * fullPath){
	if (buffer)
		Close();

	FILE * file;

	/* FIXME: 	ZeroMemory because open/close projects can cause
			GetString to found data from previous projects. */
//	ZeroMemory(buffer, MAX_BLOC_SIZE);

	file = fopen(fullPath, "rb");
	if (file){
		buffer = (char *) malloc(MAX_BLOC_SIZE);
		if (!buffer)
			return false;
		fread(buffer, 1, MAX_BLOC_SIZE, file);
		if (0 != fread(buffer, 1, sizeof(buffer), file)){
			MsgBox.DisplayString("CIniFile ERROR: OVERFOW");
			fclose(file);
			Close();
			return false;
		}
		fclose(file);

		// Initialize current pointer.
		pcurrent = buffer;
		return true;
	}
return false;
}

int CIniFile::GetInt(char * key, char * section){

	char integer[32];

	if (GetString(integer, key, section)){
		return atoi(integer);
	}

return 0;
}

//******************************************************
//	Reminder :
//	GetPrivateProfileString("Section", "Key", "default_string", data, size, iniFilePath);
//******************************************************
bool CIniFile::GetString(char * data, char * key, char * section){
	/* Copies data and returns true if successfull */
	char *pstop;
	if (section){
		/* Parse from pcurrent until the end of buffer */
		pstop = pcurrent;
		for ( ; ; ){
			if (*pcurrent == '\0'){
				/* Parse again from beginning to pstop */
				pcurrent = buffer;
				for ( ; ; ){
					if (FindSection(pcurrent, section))
						break; // Found !
					if (pcurrent >= pstop)
						return false; // Not found !
				}
			}
			if (FindSection(pcurrent, section))
				break; // Found !
		}
	}

	if (psection){
		/* Section Found */
		pstop = pcurrent;
		//MsgBox.DisplayString(pcurrent);
		for ( ; ; ){
			if (*pcurrent == '\0' || *pcurrent == '['){
				/* Parse again from beginning to pstop */
				pcurrent = psection;
				for ( ; ; ){
					if (FindData(pcurrent, key, data))
						break; // Found !
					if (pcurrent >= pstop)
						return false; // Not found !
				}
			}
			if (FindData(pcurrent, key, data))
				break; // Found !
		}
	}
	//MsgBox.DisplayString(pcurrent);
return true;
}

bool CIniFile::FindSection(char * s, char * section){
	/* Search the section through one line of text */
	/* Returns true if successful, false otherwise */
	/* This procedure increments current pointer to the end of line */

	if (!section)
		return false;

	bool sectionFound = false;
	psection = NULL;

	/* Skip spaces and comments */
	s = SkipUnwanted(s);

	/* End of buffer ? */
	if (*s == '\0'){
		pcurrent = s;
		return false;

	/* A section ? */
	}else if (*s == '['){
		s++;
		if (*s == '\0'){
			pcurrent = s;
			return false;
		}

		/* Parse the section name */
		int len = strlen(section);
		if (!strncmp(s, section, len)){
			s+=len;
			if (*s == ']'){
				/* Section found ! */
				sectionFound = true;
			}
		}
	}

	/* Increment current pointer until the end of current line */
	while (*s != '\0' && *s != '\n')
		s++;
	pcurrent = s;

	if (sectionFound)
		psection = pcurrent;
return sectionFound;
}

bool CIniFile::FindData(char * s, char * key, char * data){
	/* Search the key through one line of text */
	/* Returns true if successful, false otherwise */
	/* This procedure increments current pointer to the end of line */

	bool keyFound = false;
	/* Skip spaces and comments */
	s = SkipUnwanted(s);

	/* End of buffer ? End of section ? */
	if (*s == '\0' || *s == '['){
		pcurrent = s;
		return false;

	/* Search the key */
	}else{
		int len = strlen(key);
		/* Compare key and s */
		if (!strncmp(s, key, len)){
			s+=len;
			/* Search an '=' sign */
			while (isspace(*s)){
				if (*s == '\n')
					break;
				s++;
			}

			if (*s == '='){
				/* Key found ! */
				keyFound = true;
				s++;
				while (isspace(*s)){
					if (*s == '\n')
						break;
					s++;
				}

				/* Copy data ! */
				s += CopyData(data, s);
			}
		}
	}

	/* Increment current pointer until the end of current line */
	while (*s != '\0' && *s != '\n')
		s++;
	pcurrent = s;
return keyFound;
}

long CIniFile::CopyData(char * data, char * s){
/* Returns the number of characters copied not including the terminating null char */
	char *pdata = data;
	while (*s != '\0' && *s != '\r' && *s != '\n'){
		*pdata = *s;
		pdata++;
		s++;

		/* @@ Overflow @@
		n++;
		if (n == n_max){
			*data = '\0';
			return 
		}*/
	}
	*pdata = '\0';
return (pdata-data);
}

char * CIniFile::SkipUnwanted(char *s){
	for ( ; ; ){		
		/* End of buffer ? */
		if (*s == '\0')
			return s;
		if (isspace(*s)){
			/* Skip spaces */
			s++;
		}else if (*s == ';'){
			/* Skip comments */
			char * end_of_line = strchr(s, '\n');
			if (end_of_line){
				/* End of comment */
				s = end_of_line+1;
				continue;
			}else{
				/* End of buffer */
				return end_of_line;
			}
		/* Done */
		}else{
			return s;
		}
	}
}


/********************************************************************
*	Class:	CShellDlg.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CShellDlg::CShellDlg(){
	// Get shell task allocator.
	if(SHGetMalloc(&pMalloc) == (HRESULT) E_FAIL)
		pMalloc = NULL;
}

CShellDlg::~CShellDlg(){
	// Decrements the reference count.
	if (pMalloc)
		pMalloc->Release();
}

bool CShellDlg::BrowseForFolder(CWindow * pWindow, LPSTR pszDisplayName, 
		LPCSTR lpszTitle, UINT ulFlags, BFFCALLBACK lpfn, LPARAM lParam, int iImage){

	// Initialize output buffer.
	*pszDisplayName = '\0';
	// BROWSEINFO.
	if (!pWindow)
		bi.hwndOwner = 0;
	else
		bi.hwndOwner = pWindow->_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = pszDisplayName;
	bi.lpszTitle = lpszTitle;
	bi.ulFlags = ulFlags;
	bi.lpfn = lpfn;
	bi.lParam = lParam;
	bi.iImage = iImage;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(!pidl)
		return false;
	
	if(!SHGetPathFromIDList(pidl, pszDisplayName))
		return false;

	pMalloc->Free(pidl);
return true;
}

CCriticalSection::CCriticalSection(){
	::InitializeCriticalSection(&cs);
}

CCriticalSection::~CCriticalSection(){
	::DeleteCriticalSection(&cs);
}

void CCriticalSection::Enter(){
	::EnterCriticalSection(&cs);
}

void CCriticalSection::Leave(){
	::LeaveCriticalSection(&cs);
}

