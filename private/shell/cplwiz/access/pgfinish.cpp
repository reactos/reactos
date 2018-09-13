#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgfinish.h"


FinishWizPg::FinishWizPg( 
						 LPPROPSHEETPAGE ppsp
						 ) : WizardPage(ppsp, 0, 0)
{
	m_dwPageId = IDD_WIZFINISH;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


FinishWizPg::~FinishWizPg(
						  VOID
						  )
{
}



LRESULT
FinishWizPg::OnCommand(
					   HWND hwnd,
					   WPARAM wParam,
					   LPARAM lParam
					   )
{
	LRESULT lResult = 1;
	
	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID 	 = LOWORD(wParam);
	HWND hwndCtl	 = (HWND)lParam;
	
#if 0
	switch(wCtlID)
	{
	case IDC_BTN_FINISH_APPLY:
		EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_FINISH_APPLY), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_TXT_FINISH_APPLY), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_FINISH_UNDO), TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_TXT_FINISH_UNDO), TRUE);
		lResult = 0;
		break;
		
	case IDC_BTN_FINISH_UNDO:
		EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_FINISH_UNDO), FALSE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_TXT_FINISH_UNDO), FALSE);
		lResult = 0;
		break;
		
	default:
		break;
	}
#endif
	
	return lResult;
}






LRESULT 
FinishWizPg::OnPSN_WizFinish(
							 HWND hwnd, 
							 INT idCtl, 
							 LPPSHNOTIFY pnmh
							 )
{
	return 0;
}

LRESULT
FinishWizPg::OnInitDialog(
						  HWND hwnd,
						  WPARAM wParam,
						  LPARAM lParam
						  )
{
	
	return 1;
}


