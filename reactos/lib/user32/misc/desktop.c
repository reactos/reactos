/* $Id: desktop.c,v 1.1 2001/06/12 17:50:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/desktop.c
 * PURPOSE:         Desktops
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <debug.h>


WINBOOL
STDCALL
CloseDesktop(
  HDESK hDesktop)
{
  return NtUserCloseDesktop(hDesktop);
}

HDESK
STDCALL
CreateDesktopA(
  LPCSTR lpszDesktop,
  LPCSTR lpszDevice,
  LPDEVMODE pDevmode,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;

	if (lpszDesktop != NULL) {
		RtlInitAnsiString(&DesktopNameA, (LPSTR)lpszDesktop);
		RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
  } else {
    RtlInitUnicodeString(&DesktopNameU, NULL);
  }

  hDesktop = CreateDesktopW(
    DesktopNameU.Buffer,
    NULL,
    pDevmode,
    dwFlags,
    dwDesiredAccess,
    lpsa);

	RtlFreeUnicodeString(&DesktopNameU);

  return hDesktop;
}

HDESK
STDCALL
CreateDesktopW(
  LPCWSTR lpszDesktop,
  LPCWSTR lpszDevice,
  LPDEVMODE pDevmode,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  UNICODE_STRING DesktopName;
  HWINSTA hWinSta;
  HDESK hDesktop;

  hWinSta = NtUserGetProcessWindowStation();

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  hDesktop = NtUserCreateDesktop(
    &DesktopName,
    dwFlags,
    dwDesiredAccess,
    lpsa,
    hWinSta);

  return hDesktop;
}

WINBOOL
STDCALL
EnumDesktopWindows(
  HDESK hDesktop,
  ENUMWINDOWSPROC lpfn,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDesktopsA(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDesktopsW(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

HDESK
STDCALL
GetThreadDesktop(
  DWORD dwThreadId)
{
  return NtUserGetThreadDesktop(dwThreadId, 0);
}

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

WINBOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return NtUserPaintDesktop(hdc);
}

WINBOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return NtUserSetThreadDesktop(hDesktop);
}

WINBOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return NtUserSwitchDesktop(hDesktop);
}

/* EOF */
