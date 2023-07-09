HWND TrayNotifyCreate(HWND hwndParent, UINT uID, HINSTANCE hInst);
LRESULT TrayNotify(HWND hwndTray, HWND hwndFrom, PCOPYDATASTRUCT pcds, BOOL *pbRefresh);

#define TNM_GETCLOCK (WM_USER + 1)
#define TNM_HIDECLOCK (WM_USER + 2)
#define TNM_TRAYHIDE (WM_USER + 3)
#define TNM_TRAYPOSCHANGED (WM_USER + 4)
#define TNM_ASYNCINFOTIP   (WM_USER + 5)
#define TNM_ASYNCINFOTIPPOS (WM_USER + 6)
#define TNM_RUDEAPP         (WM_USER + 7)

