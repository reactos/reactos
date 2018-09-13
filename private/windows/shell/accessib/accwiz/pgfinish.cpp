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
FinishWizPg::OnInitDialog(
						  HWND hwnd,
						  WPARAM wParam,
						  LPARAM lParam
						  )
{
	g_Options.ReportChanges(GetDlgItem(hwnd, IDC_SZCHANGES));
	
	return 1;
}

LRESULT
FinishWizPg::OnPSN_SetActive(
							 HWND hwnd, 
							 INT idCtl, 
							 LPPSHNOTIFY pnmh
							 )
{
	// Call the base class
	WizardPage::OnPSN_SetActive(hwnd, idCtl, pnmh);

	g_Options.ReportChanges(GetDlgItem(hwnd, IDC_SZCHANGES));

	return TRUE;
}


BOOL FinishWizPg::OnMsgNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
	// Hope the site addr is not more than 256 characters
	TCHAR webAddr[256];
	LoadString(g_hInstDll, IDS_ENABLEWEB, webAddr, 256);

	ShellExecute(hwnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
	return 0;
}

