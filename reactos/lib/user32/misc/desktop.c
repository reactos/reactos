/* $Id: desktop.c,v 1.31 2004/03/07 11:59:43 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/desktop.c
 * PURPOSE:         Desktops
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */
#include <string.h>
#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <rosrtl/devmode.h>
#include <rosrtl/logfont.h>
#include <malloc.h>
#include <math.h>

/*
 * @implemented
 */
int STDCALL
GetSystemMetrics(int nIndex)
{
  return(NtUserGetSystemMetrics(nIndex));
}


/*
 * @unimplemented
 */
BOOL STDCALL SetDeskWallpaper(LPCSTR filename)
{
	return SystemParametersInfoA(SPI_SETDESKWALLPAPER,0,(PVOID)filename,1);
}
/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch (uiAction)
    {
      case SPI_GETWORKAREA:
        {
           return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
        }
      case SPI_GETNONCLIENTMETRICS:
        {
           LPNONCLIENTMETRICSA nclma = (LPNONCLIENTMETRICSA)pvParam;
           NONCLIENTMETRICSW nclmw;
           nclmw.cbSize = sizeof(NONCLIENTMETRICSW);

           if (!SystemParametersInfoW(uiAction, sizeof(NONCLIENTMETRICSW),
                                      &nclmw, fWinIni))
             return FALSE;

           nclma->iBorderWidth = nclmw.iBorderWidth;
           nclma->iScrollWidth = nclmw.iScrollWidth;
           nclma->iScrollHeight = nclmw.iScrollHeight;
           nclma->iCaptionWidth = nclmw.iCaptionWidth;
           nclma->iCaptionHeight = nclmw.iCaptionHeight;
           nclma->iSmCaptionWidth = nclmw.iSmCaptionWidth;
           nclma->iSmCaptionHeight = nclmw.iSmCaptionHeight;
           nclma->iMenuWidth = nclmw.iMenuWidth;
           nclma->iMenuHeight = nclmw.iMenuHeight;
           RosRtlLogFontW2A(&(nclma->lfCaptionFont), &(nclmw.lfCaptionFont));
           RosRtlLogFontW2A(&(nclma->lfSmCaptionFont), &(nclmw.lfSmCaptionFont));
           RosRtlLogFontW2A(&(nclma->lfMenuFont), &(nclmw.lfMenuFont));
           RosRtlLogFontW2A(&(nclma->lfStatusFont), &(nclmw.lfStatusFont));
           RosRtlLogFontW2A(&(nclma->lfMessageFont), &(nclmw.lfMessageFont));
           return TRUE;
        }
      case SPI_GETICONTITLELOGFONT:
        {
           LOGFONTW lfw;
           if (!SystemParametersInfoW(uiAction, 0, &lfw, fWinIni))
             return FALSE;
           RosRtlLogFontW2A(pvParam, &lfw);
           return TRUE;
        }
    }

  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


/*
 * @implemented
 */
BOOL
STDCALL
CloseDesktop(
  HDESK hDesktop)
{
  return NtUserCloseDesktop(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopA(LPCSTR lpszDesktop,
	       LPCSTR lpszDevice,
	       LPDEVMODEA pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;
  DEVMODEW DevmodeW;

  if (lpszDesktop != NULL) 
    {
      RtlInitAnsiString(&DesktopNameA, (LPSTR)lpszDesktop);
      RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
    } 
  else 
    {
      RtlInitUnicodeString(&DesktopNameU, NULL);
    }

  RosRtlDevModeA2W ( &DevmodeW, pDevmode );

  hDesktop = CreateDesktopW(DesktopNameU.Buffer,
			    NULL,
			    &DevmodeW,
			    dwFlags,
			    dwDesiredAccess,
			    lpsa);

  RtlFreeUnicodeString(&DesktopNameU);
  return(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopW(LPCWSTR lpszDesktop,
	       LPCWSTR lpszDevice,
	       LPDEVMODEW pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  UNICODE_STRING DesktopName;
  HWINSTA hWinSta;
  HDESK hDesktop;

  hWinSta = NtUserGetProcessWindowStation();

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  hDesktop = NtUserCreateDesktop(&DesktopName,
				 dwFlags,
				 dwDesiredAccess,
				 lpsa,
				 hWinSta);

  return(hDesktop);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDesktopsA(
  HWINSTA hwinsta,
  DESKTOPENUMPROCA lpEnumFunc,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDesktopsW(
  HWINSTA hwinsta,
  DESKTOPENUMPROCW lpEnumFunc,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HDESK
STDCALL
GetThreadDesktop(
  DWORD dwThreadId)
{
  return NtUserGetThreadDesktop(dwThreadId, 0);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopA(
  LPSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;

	if (lpszDesktop != NULL) {
		RtlInitAnsiString(&DesktopNameA, lpszDesktop);
		RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
  } else {
    RtlInitUnicodeString(&DesktopNameU, NULL);
  }

  hDesktop = OpenDesktopW(
    DesktopNameU.Buffer,
    dwFlags,
    fInherit,
    dwDesiredAccess);

	RtlFreeUnicodeString(&DesktopNameU);

  return hDesktop;
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopW(
  LPWSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING DesktopName;

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  return NtUserOpenDesktop(
    &DesktopName,
    dwFlags,
    dwDesiredAccess);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return NtUserOpenInputDesktop(
    dwFlags,
    fInherit,
    dwDesiredAccess);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return NtUserPaintDesktop(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return NtUserSetThreadDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return NtUserSwitchDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
	return NtUserSetShellWindowEx(hwndShell, hwndShellListView);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindow(HWND hwndShell)
{
	return SetShellWindowEx(hwndShell, hwndShell);
}


/*
 * @implemented
 */
HWND STDCALL
GetShellWindow(VOID)
{
	return NtUserGetShellWindow();
}


/* EOF */
