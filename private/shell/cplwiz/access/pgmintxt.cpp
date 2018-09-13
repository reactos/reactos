#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgMinTxt.h"

CMinTextPg::CMinTextPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_LKPREV_MINTEXTTITLE, IDS_LKPREV_MINTEXTSUBTITLE)
{
	m_dwPageId = IDD_FNTWIZMINTEXT;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	
	memset(&m_lfFont, 0, sizeof(m_lfFont));
}


CMinTextPg::~CMinTextPg(
	VOID
	)
{
}

LRESULT
CMinTextPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	NONCLIENTMETRICS ncm;
	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	m_lfFont = ncm.lfMenuFont;

	g_Options.m_nMinimalFontSize = 8;
	m_lfFont.lfHeight = -MulDiv(g_Options.m_nMinimalFontSize, g_Options.m_nLogPixelsY, 72);


	m_hFont = CreateFontIndirect(&m_lfFont);

	SetWindowFont(GetDlgItem(m_hwnd, IDC_STATICTEXT), m_hFont, TRUE);

	UpdateControls();
	return 1;
}


void CMinTextPg::UpdateControls()
{
	HFONT hNewFont = CreateFontIndirect(&m_lfFont);
	SetWindowFont(GetDlgItem(m_hwnd, IDC_STATICTEXT), hNewFont, TRUE);

	// Destroy the old font
	if(m_hFont)
		DeleteObject(m_hFont);
	m_hFont = hNewFont;

}


LRESULT
CMinTextPg::OnCommand(
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
	case IDC_BTN_INCREASE_SIZE:
		g_Options.m_nMinimalFontSize = min(24, g_Options.m_nMinimalFontSize + 2);
		m_lfFont.lfHeight = -MulDiv(g_Options.m_nMinimalFontSize, g_Options.m_nLogPixelsY, 72);
		UpdateControls();
		lResult = 0;
		break;
	case IDC_BTN_DECREASE_SIZE:
		g_Options.m_nMinimalFontSize = max(8, g_Options.m_nMinimalFontSize - 2);
		m_lfFont.lfHeight = -MulDiv(g_Options.m_nMinimalFontSize, g_Options.m_nLogPixelsY, 72);
		UpdateControls();
		lResult = 0;
		break;

	default:
		break;
	}

	return lResult;
}
