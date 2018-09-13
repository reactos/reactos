#ifndef FILECOPY_H
#define FILECOPY_H

BOOL GetInstallPath(LPTSTR szDirOfSrc);
void wsStartWait();
void wsEndWait();
int fDialog(int id, HWND hwnd, DLGPROC fpfn);
UINT wsCopyError(int n, LPTSTR szFile);
UINT wsInsertDisk(LPTSTR Disk, LPTSTR szSrcPath);
INT_PTR wsDiskDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam);
UINT wsCopySingleStatus(int msg, DWORD_PTR n, LPTSTR szFile);
INT_PTR wsExistDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam);

#endif

