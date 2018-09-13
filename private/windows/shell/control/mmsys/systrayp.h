/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:	SYSTRAY.H
*
*  VERSION:	2.1
*
*  AUTHOR:	Tracy Sharpe / RAL
*
*  DATE:        20 Feb 1994
*
*  Public definitions of the system tray applet (battery meter, PCMCIA, etc).
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  20 Feb 1994 TCS Original implementation.
*  11/8/94     RAL Converted to systray
*
*******************************************************************************/

#ifndef _INC_SYSTRAY
#define _INC_SYSTRAY

#define SYSTRAY_CLASSNAME	   TEXT("SystemTray_Main")

//  Private tray icon notification message sent to the BatteryMeter window.
#define STWM_NOTIFYPOWER		(WM_USER + 201)
#define STWM_NOTIFYPCMCIA		(WM_USER + 202)
#define STWM_NOTIFYVOLUME		(WM_USER + 203)

//  Private tray icon notification messages sent to the BatteryMeter window.
#define STWM_ENABLESERVICE		(WM_USER + 220)
#define STWM_GETSTATE			(WM_USER + 221)


_inline BOOL SysTray_EnableService(int idSTService, BOOL fEnable)
{
    if (fEnable)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
	    static const TCHAR szSTExecFmt[] = TEXT("SYSTRAY.EXE %i");
	    TCHAR szEnableCmd[sizeof(szSTExecFmt)+10];
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOWNOACTIVATE;
        si.dwFlags = STARTF_USESHOWWINDOW;
	    wsprintf(szEnableCmd, szSTExecFmt, idSTService);
	    if (CreateProcess(NULL,szEnableCmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
	    HWND hwndST = FindWindow(SYSTRAY_CLASSNAME, NULL);
	    if (hwndST)
        {
	        SendMessage(hwndST, STWM_ENABLESERVICE, idSTService, FALSE);
	    }
    }
    return TRUE;
}


_inline BOOL SysTray_IsServiceEnabled(WPARAM idSTService)
{
    HWND hwndST = FindWindow(SYSTRAY_CLASSNAME, NULL);
    if (hwndST) {
	return((BOOL)SendMessage(hwndST, STWM_GETSTATE, idSTService, 0));
    } else {
	return(FALSE);
    }
}

#define STSERVICE_POWER 		1
#define STSERVICE_PCMCIA		2
#define STSERVICE_VOLUME		4
#define STSERVICE_ALL			7   // Internal

//
//  Flags for the PCMCIA registry entry
//
#define PCMCIA_REGFLAG_NOWARN		1

#endif // _INC_SYSTRAY
