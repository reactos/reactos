// IDD_FOLDER_OPTIONS_GENERAL
INT_PTR
CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

// IDD_FOLDER_OPTIONS_VIEW
INT_PTR CALLBACK
FolderOptionsViewDlg(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

// IDD_FOLDER_OPTIONS_FILETYPES
INT_PTR CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

HBITMAP Create24BppBitmap(HDC hDC, INT cx, INT cy);
HBITMAP BitmapFromIcon(HICON hIcon, INT cx, INT cy);
HBITMAP CreateCheckImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE);
HBITMAP CreateCheckMask(HDC hDC);
HBITMAP CreateRadioImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE);
HBITMAP CreateRadioMask(HDC hDC);

extern LPCWSTR g_pszShell32;
extern LPCWSTR g_pszSpace;
