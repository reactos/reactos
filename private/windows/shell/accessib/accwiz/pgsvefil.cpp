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
			TCHAR szDefaultName[_MAX_PATH];
			LoadString(g_hInstDll, IDS_DEFAULTSAVEFILENAME, szDefaultName, ARRAYSIZE(szDefaultName));
			
			TCHAR szFilterName[_MAX_PATH];
			TCHAR szTemp[_MAX_PATH];
			ZeroMemory(szFilterName, _MAX_PATH);
			LoadString(g_hInstDll, IDS_FILTERNAME, szFilterName, ARRAYSIZE(szFilterName));
			LoadString(g_hInstDll, IDS_FILTER, szTemp, ARRAYSIZE(szTemp));

			// Copy the extension to after the filter name
			lstrcpy(&szFilterName[lstrlen(szFilterName) + 1], szTemp);

			// Double NULL terminate it
			szFilterName[lstrlen(szFilterName) + 1 + lstrlen(szTemp) + 1] = 0;
			szFilterName[lstrlen(szFilterName) + 1 + lstrlen(szTemp) + 2] = 0;


			memset(szBuf, 0, ARRAYSIZE(szBuf));
			memset(szBuf2, 0, ARRAYSIZE(szBuf));
			wsprintf(szBuf, szDefaultName);
			OPENFILENAME ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = m_hwnd;
			ofn.hInstance = g_hInstDll;
			ofn.lpstrFilter = szFilterName; 
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
			ofn.lpstrDefExt = __TEXT("acw");
			ofn.lCustData = NULL;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;
			BOOL bOk = GetSaveFileName(&ofn);
			if(bOk)
			{
				// Save the file to disk.
				HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(hFile != INVALID_HANDLE_VALUE)
				{
					DWORD dwWritten;
					WriteFile(hFile, (LPCVOID)&g_Options.m_schemePreview, sizeof(g_Options.m_schemePreview), &dwWritten, NULL);
					CloseHandle(hFile);
					// TODO: Maybe go to the next page
				}
				else
					StringTableMessageBox(m_hwnd, IDS_WIZERRORSAVINGFILETEXT, IDS_WIZERRORSAVINGFILETITLE, MB_OK);
			}

			UpdateControls();
			lResult = 0;
		}
		break;
																					 
	default:
		break;
	}

	return lResult;
}
