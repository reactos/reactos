/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       SYSTRAY.H
*
*  VERSION:     2.1
*
*  AUTHOR:      Tracy Sharpe / RAL
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
*  10/23/95    Shawnb Unicode enabled
*
*******************************************************************************/

#ifndef _INC_SYSTRAY
#define _INC_SYSTRAY

#define SYSTRAY_CLASSNAME          TEXT ("SystemTray_Main")

//  Private tray icon notification message sent to the BatteryMeter window.
#define STWM_NOTIFYPOWER                (WM_USER + 201)
#define STWM_NOTIFYPCMCIA               (WM_USER + 202)
#define STWM_NOTIFYVOLUME               (WM_USER + 203)
#define STWM_NOTIFYSTICKYKEYS           (WM_USER + 204)
#define STWM_NOTIFYMOUSEKEYS            (WM_USER + 205)
#define STWM_NOTIFYFILTERKEYS           (WM_USER + 206)
#define STWM_NOTIFYCSC                  (WM_USER + 207)
#define STWM_NOTIFYUSBUI                (WM_USER + 208)

//  Private tray icon notification messages sent to the BatteryMeter window.
#define STWM_ENABLESERVICE              (WM_USER + 220)
#define STWM_GETSTATE                   (WM_USER + 221)

#define STSERVICE_POWER                 1
#define STSERVICE_PCMCIA                2
#define STSERVICE_VOLUME                4
#define STSERVICE_CSC                   8
#define STSERVICE_USBUI                 16
#define STSERVICE_ALL                   31   // Internal

//
//  Flags for the PCMCIA registry entry
//
#define PCMCIA_REGFLAG_NOWARN           1

//      Prototypes
_inline BOOL SysTray_EnableService(int idSTService, BOOL fEnable)
{
	HWND hwndST = FindWindow(SYSTRAY_CLASSNAME, NULL);
	if (hwndST) 
	{
		SendMessage(hwndST, STWM_ENABLESERVICE, idSTService, fEnable);
		return TRUE;
	}
	else
	{
		if (fEnable) 
		{
			static const TCHAR szOPEN[]     = TEXT ("open");
			static const TCHAR szFILE[]     = TEXT ("SYSTRAY.EXE");
			static const TCHAR szFORMAT[]   = TEXT ("%i");
			TCHAR       szPARAMS[10];
			HINSTANCE   hInst;
      
			wsprintf (szPARAMS, szFORMAT, idSTService);

			hInst = ShellExecute (NULL, szOPEN, szFILE,
								  szPARAMS, NULL, SW_SHOWNOACTIVATE);
			if (hInst <= (HINSTANCE)32)
				return FALSE;
		}
		return TRUE;
	} 
} // End SysTray_EnableService


_inline BOOL SysTray_IsServiceEnabled(WPARAM idSTService)
{
   HWND hwndST = FindWindow(SYSTRAY_CLASSNAME, NULL);
   if (hwndST) 
   {
      return((BOOL)SendMessage(hwndST, STWM_GETSTATE, idSTService, 0));
   } 
   else 
   {
      return (FALSE);
   }
} // End SysTray_IsServiceEnabled


#endif // _INC_SYSTRAY
