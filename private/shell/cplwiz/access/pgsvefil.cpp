#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgSveFil.h"

CSaveToFilePg::CSaveToFilePg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZSAVETOFILETITLE, IDS_WIZSAVETOFILESUBTITLE)
{
	m_dwPageId = IDD_WIZSAVETOFILE;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CSaveToFilePg::~CSaveToFilePg(
	VOID
	)
{
}

LRESULT
CSaveToFilePg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	UpdateControls();
	return 1;
}


void CSaveToFilePg::UpdateControls()
{
	// No options
}


LRESULT
CSaveToFilePg::OnCommand(
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
	case IDC_BTNBROWSE:
		{
			// These commands require us to re-enable/disable the appropriate controls
			TCHAR szBuf[_MAX_PATH];
			TCHAR szBuf2[_MAX_PATH];
			memset(szBuf, 0, ARRAYSIZE(szBuf));
			memset(szBuf2, 0, ARRAYSIZE(szBuf));
			wsprintf(szBuf, __TEXT("test.exe"));
			OPENFILENAME ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = m_hwnd;
			ofn.hInstance = g_hInstDll;
			ofn.lpstrFilter = NULL; 
			ofn.lpstrCustomFilter = NULL;
			ofn.nMaxCustFilter = 0;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = szBuf;
			ofn.nMaxFile = _MAX_PATH;
			ofn.lpstrFileTitle = szBuf2;
			ofn.nMaxFileTitle = _MAX_PATH;
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = NULL;
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
			ofn.nFileOffset = 0;
			ofn.nFileExtension = 0;
			ofn.lpstrDefExt = NULL;
			ofn.lCustData = NULL;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;

 			BOOL bOk = GetSaveFileName(&ofn);
			UpdateControls();
			lResult = 0;
		}
		break;
																					 
	default:
		break;
	}

	return lResult;
}
