//
// CDlg
// 
// FelixA
//
// Used to be CDialog
//


#include "pch.h"
#include "dlg.h"

///////////////////////////////////////////////////////////////////////////////
//
// Sets the lParam to the 'this' pointer
// wraps up PSN_ messages and calls virtual functions
// calls off to your overridable DlgProc
//

BOOL CALLBACK CDlg::BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CDlg * pSV = (CDlg*)GetWindowLong(hDlg,DWL_USER);

	switch (uMessage)
	{
		case WM_INITDIALOG:
		{
			pSV=(CDlg*)lParam;
			pSV->SetWindow(hDlg);
			SetWindowLong(hDlg,DWL_USER,(LPARAM)pSV);
			pSV->OnInit();
		}
		break;

		// Override the Do Command to get a nice wrapped up feeling.
		case WM_COMMAND:
			if(pSV)
				return pSV->DoCommand(LOWORD(wParam),HIWORD(wParam));
		break;

		case WM_NOTIFY:
			if(pSV)
				return pSV->DoNotify((NMHDR FAR *)lParam);
		break;

		case WM_DESTROY:
			if(pSV)
				pSV->Destroy();
		break;
	}

	if(pSV)
		return pSV->DlgProc(hDlg,uMessage,wParam,lParam);
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// You can override this DlgProc if you want to handle specific messages
//
BOOL CALLBACK CDlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Below are just default handlers for the virtual functions.
//
int CDlg::DoCommand(WORD wCmdID,WORD hHow)
{
	switch( wCmdID )
	{
		case IDOK:
		case IDCANCEL:
			EndDialog(wCmdID);
		break;
	}
	return 1;	// not handled, just did Apply work.
}

void CDlg::OnInit()
{
}

CDlg::CDlg(int DlgID, HWND hWnd, HINSTANCE hInst)
: m_DlgID(DlgID),
  m_hParent(hWnd),
  m_Inst(hInst),
  m_bCreatedModeless(FALSE),
  m_hDlg(0)
{
}

//
//
//
int CDlg::Do()
{
	m_bCreatedModeless=FALSE;
	return DialogBoxParam( m_Inst,  MAKEINTRESOURCE(m_DlgID), m_hParent, (DLGPROC)BaseDlgProc, (LPARAM)this);
}

HWND CDlg::CreateModeless()
{
	if(m_hDlg)
		return m_hDlg;

	HWND hWnd=CreateDialogParam(m_Inst, MAKEINTRESOURCE(m_DlgID), m_hParent, (DLGPROC)BaseDlgProc,  (LPARAM)this);
	if(hWnd)
		m_bCreatedModeless=TRUE;
	return hWnd;
}

int CDlg::DoNotify(NMHDR * pHdr)
{
	return FALSE;
}

void CDlg::Destroy()
{
	if(m_bCreatedModeless)
	{
		if(m_hDlg)
			m_hDlg=NULL;
	}
}

CDlg::~CDlg()
{
	if(m_hDlg)
		DestroyWindow(m_hDlg);
}

void CDlg::SetDlgID(UINT id)
{
	m_DlgID=id;
}

void CDlg::SetInstance(HINSTANCE hInst)
{
	m_Inst=hInst;
}

