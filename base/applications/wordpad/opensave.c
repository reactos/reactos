#include "precomp.h"

static OPENFILENAME ofn;

/*
 * Initialize file open / save structure
 */
VOID FileInitialize(HWND hwnd)
{
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize   = sizeof(OPENFILENAME);
    ofn.hwndOwner     = hwnd;
    ofn.nMaxFile      = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrDefExt   = _T("bmp");
}


static BOOL
DoWriteFile(LPCTSTR pszFileName)
{
    return TRUE;
}


BOOL
DoOpenFile(HWND hwnd,
           LPTSTR szFileName,
           LPTSTR szTitleName)
{
	DWORD err;

	static TCHAR Filter[] = _T("All documents (*.txt,*.rtf)\0*.txt;*.rtf\0") \
		_T("Rich Text Document (*.rtf)\0*.rtf\0") \
		_T("Text Document (*.txt)\0*.txt\0");


	ofn.lpstrFilter = Filter;
	ofn.lpstrFile = szFileName;
	ofn.lpstrFileTitle = szTitleName;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
	    return TRUE;
	}

	err = CommDlgExtendedError();

	if (err != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Open file failed"), NULL, 0);

    return FALSE;
}



BOOL
DoSaveFile(HWND hwnd)
{
	TCHAR szFileName[MAX_PATH] = _T("");
	static TCHAR Filter[] = _T("Rich Text Document (*.rtf)\0*.rtf\0") \
		_T("Text Document (*.txt)\0*.txt\0");

	ofn.lpstrFilter = Filter;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
        if (DoWriteFile(szFileName))
            return TRUE;
	}

	if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Save to file failed"), NULL, 0);

	return FALSE;
}

