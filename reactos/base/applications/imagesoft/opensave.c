#include <precomp.h>

static OPENFILENAME ofn;

/*
 * Initialize file open / save structure
 */
VOID FileInitialize(HWND hwnd)
{
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.nMaxFile          = MAX_PATH;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrDefExt       = _T("bmp");

}

/*
 * Write the file to disk
 */
BOOL DoWriteFile(LPCTSTR pszFileName)
{
    return TRUE;
}

/*
 * Read the file from disk
 */
BOOL DoReadFile(LPCTSTR pszFileName)
{
    return TRUE;
}


/*
 * Show the file open dialog
 */
VOID DoOpenFile(HWND hwnd)
{
	TCHAR szFileName[MAX_PATH] = _T("");
	static TCHAR Filter[] = _T("All image files (*.gif,*.bmp,*.jpg,*.jpeg,*.tif,*.png)\0*.gif,*.bmp,*.jpg,*.jpeg,*.tif,*.png\0") \
                            _T("All files (*.*)\0*.*\0") \
                            _T("Graphics Interchange format (*gif)\0*.gif\0") \
                            _T("Windows Bitmap (*bmp)\0*.bmp\0") \
                            _T("JPEG File Interchange Format (*jpg,*.jpeg)\0*.jpg,*.jpeg\0") \
                            _T("TAG Image File Format (*tif)\0*.tif\0") \
                            _T("Portable Network Graphics (*png)\0*.png\0\0");

	ofn.lpstrFilter = Filter;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
        if (DoReadFile(szFileName))
            return;
	}

	if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Open file failed"), NULL, 0);
}


/*
 * Show the file saveas dialog
 */
VOID DoSaveFile(HWND hwnd)
{
	TCHAR szFileName[MAX_PATH] = _T("");
	static TCHAR Filter[] = _T("Graphics Interchange format (*gif)\0*.gif\0") \
                            _T("Windows Bitmap (*bmp)\0*.bmp\0") \
                            _T("JPEG File Interchange Format (*jpg,*.jpeg)\0*.jpg,*.jpeg\0") \
                            _T("TAG Image File Format (*tif)\0*.tif\0") \
                            _T("Portable Network Graphics (*png)\0*.png\0\0");

	ofn.lpstrFilter = Filter;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
        if (DoWriteFile(szFileName))
            return;
	}

	if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Save to file failed"), NULL, 0);
}

