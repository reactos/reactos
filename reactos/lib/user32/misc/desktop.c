/* $Id: desktop.c,v 1.20 2003/08/09 13:13:43 mf Exp $
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

/*
 * @implemented
 */
int STDCALL
GetSystemMetrics(int nIndex)
{
  return(NtUserGetSystemMetrics(nIndex));
}


/*
 * @implemented
 */
WINBOOL STDCALL
SystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  return(SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni));
}


/*
 * @implemented
 */
WINBOOL STDCALL
SystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  NONCLIENTMETRICSW *nclm;

  /* FIXME: This should be obtained from the registry */
  static LOGFONTW CaptionFont =
  { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
    0, 0, DEFAULT_QUALITY, FF_MODERN, L"Bitstream Vera Sans Bold" };

  switch (uiAction)
    {
    case SPI_GETWORKAREA:
      {
	((PRECT)pvParam)->left = 0;
	((PRECT)pvParam)->top = 0;
	((PRECT)pvParam)->right = 640;
	((PRECT)pvParam)->bottom = 480;
	return(TRUE);
      }
    case SPI_GETNONCLIENTMETRICS:
      {
        nclm = pvParam;
        memcpy(&nclm->lfCaptionFont, &CaptionFont, sizeof(CaptionFont));
        memcpy(&nclm->lfSmCaptionFont, &CaptionFont, sizeof(CaptionFont));
	return(TRUE);
      }
    }
  return(FALSE);
}


/*
 * @implemented
 */
WINBOOL
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
WINBOOL
STDCALL
EnumDesktopsA(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDesktopsW(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
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
  WINBOOL fInherit,
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
  WINBOOL fInherit,
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
  WINBOOL fInherit,
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
WINBOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return NtUserPaintDesktop(hdc);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return NtUserSetThreadDesktop(hDesktop);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return NtUserSwitchDesktop(hDesktop);
}


 /* globally stored handles to the shell windows */
HWND hwndShellWindow = 0;
HWND hwndShellListView = 0;
DWORD pidShellWindow = 0;


/*
 * @implemented
 */
HWND STDCALL
GetShellWindow(VOID)
{
	return hwndShellWindow;
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
	 /* test if we are permitted to change the shell window */
	if (pidShellWindow && GetCurrentProcessId()!=pidShellWindow)
		return FALSE;

	hwndShellWindow = hwndShell;
	hwndShellListView = hwndShellListView;

	if (hwndShell)
		pidShellWindow = GetCurrentProcessId();	/* request shell window for the calling process */
	else
		pidShellWindow = 0;	/* shell window is now free for other processes. */

	return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindow(HWND hwndShell)
{
	return SetShellWindowEx(hwndShell, 0);
}

/* EOF */
