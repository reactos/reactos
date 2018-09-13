/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       POWERCFP.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Public declarations for PowerCfg notification interface. Systray uses
*   this interface to notify PowerCfg that the user has changed something.
*
*******************************************************************************/

//  Private PowerCfg notification message.
#define PCWM_NOTIFYPOWER                (WM_USER + 201)

#define IDS_POWERPOLICIESTITLE 400

/*******************************************************************************
*
*  PowerCfg_Notify
*
*  DESCRIPTION:
*   Called by Systray to notify PowerCfg that something has changed.
*
*  PARAMETERS:
*
*******************************************************************************/

_inline BOOL PowerCfg_Notify(void)
{
    HINSTANCE hInst;
    static  LPTSTR lpszWndName;
    TCHAR   szBuf[128];
    HWND    hwnd, hwndPC;
    int     iLen;

    // first time initialization of the PowerCfg top level property sheet title.
    if (!lpszWndName) {
        if (hInst = LoadLibrary(TEXT("powercfg.cpl"))) {
            iLen = LoadString(hInst, IDS_POWERPOLICIESTITLE, szBuf, sizeof(szBuf));
            if (iLen) {
                if (lpszWndName = LocalAlloc(0, (iLen + 1) * sizeof(TCHAR))) {
                    lstrcpy(lpszWndName, szBuf);
                }
            }
        }
    }

    // Notify the child of the top level window.
    if (lpszWndName) {
        hwndPC = FindWindow(WC_DIALOG, lpszWndName);
        if (hwndPC) {
            hwnd = GetWindow(hwndPC, GW_CHILD);
            if (hwnd) {
                return (BOOL)SendMessage(hwnd, PCWM_NOTIFYPOWER, 0, 0);
            }
        }
    }
    return FALSE;
}


