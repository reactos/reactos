void KillConfigDesktopDlg();
LRESULT CALLBACK DesktopTBSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConfigDesktopDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowHideDeskBtnOnQuickLaunch(BOOL fShownOnTray);
BOOL GetDesktopSCFPaths(LPTSTR lpszDeskQL, LPTSTR lpszSystem);

extern HWND g_hdlgDesktopConfig;
extern BOOL g_fShouldShowConfigDesktop;
extern WNDPROC g_DesktopTBProc;

