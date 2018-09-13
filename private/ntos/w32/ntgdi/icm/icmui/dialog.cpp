/******************************************************************************

  Source File:	Dialog.CPP

  Implements the CDialog class.  See Dialog.H for class definitions and details

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96	a-robkj@microsoft.com Created it
  12-11-96  a-robkj@microsoft.com Implemented hook

******************************************************************************/

#include	"ICMUI.H"

//	CDialog member functions

//	Class constructor- just save things, for now

CDialog::CDialog(HINSTANCE hiWhere, int id, HWND hwndParent) {
	m_idMain = id;
	m_hwndParent = hwndParent;
	m_hiWhere = hiWhere;
	m_bIsModal = FALSE;
	m_hwnd = NULL;
    m_dpHook = NULL;
    m_lpHook = 0;
}

CDialog::CDialog(CDialog &cdOwner, int id) {
	m_idMain = id;
	m_hwndParent = cdOwner.m_hwnd;
	m_hiWhere = cdOwner.m_hiWhere;
	m_bIsModal = FALSE;
	m_hwnd = NULL;
    m_dpHook = NULL;
    m_lpHook = 0;
}

//	Class destructor- clean up the window, if it is modeless.

CDialog::~CDialog() {
	Destroy();
}

//	Modal Dialog Box

LONG	CDialog::DoModal() {
	m_bIsModal = TRUE;
	return	(LONG)DialogBoxParam(m_hiWhere, MAKEINTRESOURCE(m_idMain), m_hwndParent,
		CDialog::DialogProc, (LPARAM) this);
}

//	Modeless dialog box creation

void	CDialog::Create() {
	if	(!m_bIsModal && m_hwnd)
		return;	//	We'va already got one!

	m_bIsModal = FALSE;
	CreateDialogParam(m_hiWhere, MAKEINTRESOURCE(m_idMain),
		m_hwndParent, CDialog::DialogProc, (LPARAM) this);
}

//	Modeless dialog box  destruction

void	CDialog::Destroy() {
	if	(!m_bIsModal && m_hwnd) {
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
}

//	Dialog Procedure- this is a static private method.  This means
//	that all instances of this class (including derived classes) share
//	this code (no pointers needed) and that only instances of this
//	class (not even derived classes) can find it.

INT_PTR CALLBACK	CDialog::DialogProc(HWND hwndMe, UINT uMsg, WPARAM wp,
										  LPARAM lp) {

	CDialog	*pcdMe = (CDialog *) GetWindowLongPtr(hwndMe, DWLP_USER);

    //  If there is a hook procedure, it can either ignore or filter a
    //  message by returning FALSE, or it can handle itself by returning
    //  TRUE.  WM_INITDALOG hook processing occurs AFTER all of our other
    //  calls are made, and we allow the base class to define the LPARAM
    //  that is passed in to the hook.
    //  Because we do not have a pointer to the base class, we will miss
    //  messages sent before WM_INITDIALOG (specifically WM_SETFONT)

    if  (uMsg != WM_INITDIALOG && pcdMe && pcdMe -> m_dpHook &&
            (*pcdMe -> m_dpHook)(hwndMe, uMsg, wp, lp))
        return  TRUE;

	switch	(uMsg) {

		case	WM_INITDIALOG:

			//	The lp is the this pointer for the caller

			pcdMe = (CDialog *) lp;

			SetWindowLongPtr(hwndMe, DWLP_USER, (LONG_PTR)pcdMe);
			pcdMe -> m_hwnd = hwndMe;

			//	Derived classes override OnInit to initialize the dialog

			if  (!pcdMe -> m_dpHook)
                return	pcdMe -> OnInit();
            else {
                //  If there is a hook procedure, we will call that after the
                //  override- if the override returned FALSE, so must we
                BOOL    bReturn = pcdMe -> OnInit();
                return  (*pcdMe -> m_dpHook)(hwndMe, uMsg, wp,
                    pcdMe -> m_lpHook) && bReturn;
            }

		case	WM_COMMAND:

			return	pcdMe -> OnCommand(HIWORD(wp), LOWORD(wp), (HWND) lp);

		case	WM_NOTIFY:

			return	pcdMe -> OnNotify((int) wp, (LPNMHDR) lp);

        case    WM_HELP:

            return  pcdMe -> OnHelp((LPHELPINFO) lp);

        case    WM_CONTEXTMENU:

            return  pcdMe -> OnContextMenu((HWND) wp);

	}

	return	FALSE;
}

//	Moves the window into position (needed to get dialogs positioned proeprly
//	in tab control display area).

void	CDialog::Adjust(RECT& rc) {
	SetWindowPos(m_hwnd, HWND_TOP, rc.left, rc.top, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
}
