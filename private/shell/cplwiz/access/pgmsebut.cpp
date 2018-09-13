#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgMseBut.h"

CMouseButtonPg::CMouseButtonPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_MSEWIZBUTTONCONFIGTITLE, IDS_MSEWIZBUTTONCONFIGSUBTITLE)
{
	m_dwPageId = IDD_MSEWIZBUTTONCONFIG;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CMouseButtonPg::~CMouseButtonPg(
	VOID
	)
{
}

LRESULT
CMouseButtonPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	if(g_Options.m_schemePreview.m_bSwapMouseButtons)
		Button_SetCheck(GetDlgItem(m_hwnd, MOUSE_LEFTHAND), TRUE);
	else
		Button_SetCheck(GetDlgItem(m_hwnd, MOUSE_RIGHTHAND), TRUE);

	RECT rc;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_OBJECTMENU), &rc);
	m_ptRight.x = rc.left;
	m_ptRight.y = rc.top;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_SELECTDRAG), &rc);
	m_ptLeft.x = rc.left;
	m_ptLeft.y = rc.top;
	ScreenToClient(m_hwnd, &m_ptRight);
	ScreenToClient(m_hwnd, &m_ptLeft);


	UpdateControls();
	return 1;
}


void CMouseButtonPg::UpdateControls()
{
	if(Button_GetCheck(GetDlgItem(m_hwnd, MOUSE_RIGHTHAND)))
	{
		SetWindowPos(GetDlgItem(m_hwnd, IDC_OBJECTMENU), NULL, m_ptRight.x, m_ptRight.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(m_hwnd, IDC_SELECTDRAG), NULL, m_ptLeft.x, m_ptLeft.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_MOUSEPIC1), TRUE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_MOUSEPIC2), FALSE);
	}
	else
	{
		SetWindowPos(GetDlgItem(m_hwnd, IDC_OBJECTMENU), NULL, m_ptLeft.x, m_ptLeft.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(m_hwnd, IDC_SELECTDRAG), NULL, m_ptRight.x, m_ptRight.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_MOUSEPIC1), FALSE);
		ShowWindow(GetDlgItem(m_hwnd, IDC_MOUSEPIC2), TRUE);
	}

}


LRESULT
CMouseButtonPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	switch(wCtlID)
	{
	case MOUSE_LEFTHAND:
	case MOUSE_RIGHTHAND:
		UpdateControls();
		break;

	}

	return lResult;
}

LRESULT
CMouseButtonPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	g_Options.m_schemePreview.m_bSwapMouseButtons = Button_GetCheck(GetDlgItem(m_hwnd, MOUSE_LEFTHAND));
	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
