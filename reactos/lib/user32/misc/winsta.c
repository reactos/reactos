/* $Id: winsta.c,v 1.4 2002/08/27 06:40:15 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/winsta.c
 * PURPOSE:         Window stations
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      04-06-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <debug.h>


WINBOOL STDCALL
CloseWindowStation(HWINSTA hWinSta)
{
  return(NtUserCloseWindowStation(hWinSta));
}

HWINSTA STDCALL
CreateWindowStationA(LPSTR lpwinsta,
		     DWORD dwReserved,
		     ACCESS_MASK dwDesiredAccess,
		     LPSECURITY_ATTRIBUTES lpsa)
{
  ANSI_STRING WindowStationNameA;
  UNICODE_STRING WindowStationNameU;
  HWINSTA hWinSta;
  
  if (lpwinsta != NULL) 
    {
      RtlInitAnsiString(&WindowStationNameA, lpwinsta);
      RtlAnsiStringToUnicodeString(&WindowStationNameU, &WindowStationNameA, 
				   TRUE);
    } 
  else 
    {
      RtlInitUnicodeString(&WindowStationNameU, NULL);
    }

  hWinSta = CreateWindowStationW(WindowStationNameU.Buffer,
				 dwReserved,
				 dwDesiredAccess,
				 lpsa);

  RtlFreeUnicodeString(&WindowStationNameU);
  
  return hWinSta;
}

HWINSTA STDCALL
CreateWindowStationW(LPWSTR lpwinsta,
		     DWORD dwReserved,
		     ACCESS_MASK dwDesiredAccess,
		     LPSECURITY_ATTRIBUTES lpsa)
{
  UNICODE_STRING WindowStationName;
  
  RtlInitUnicodeString(&WindowStationName, lpwinsta);
  
  return NtUserCreateWindowStation(&WindowStationName,
				   dwDesiredAccess,
				   lpsa, 0, 0, 0);
}

WINBOOL STDCALL
EnumWindowStationsA(ENUMWINDOWSTATIONPROC lpEnumFunc,
		    LPARAM lParam)
{
  return FALSE;
}

WINBOOL STDCALL
EnumWindowStationsW(ENUMWINDOWSTATIONPROC lpEnumFunc,
		    LPARAM lParam)
{
  return FALSE;
}

HWINSTA STDCALL
GetProcessWindowStation(VOID)
{
  return NtUserGetProcessWindowStation();
}

HWINSTA STDCALL
OpenWindowStationA(LPSTR lpszWinSta,
		   WINBOOL fInherit,
		   ACCESS_MASK dwDesiredAccess)
{
  ANSI_STRING WindowStationNameA;
  UNICODE_STRING WindowStationNameU;
  HWINSTA hWinSta;
  
  if (lpszWinSta != NULL) 
    {
      RtlInitAnsiString(&WindowStationNameA, lpszWinSta);
      RtlAnsiStringToUnicodeString(&WindowStationNameU, &WindowStationNameA, 
				   TRUE);
    } 
  else 
    {
      RtlInitUnicodeString(&WindowStationNameU, NULL);
    }
  
  hWinSta = OpenWindowStationW(WindowStationNameU.Buffer,
			       fInherit,
			       dwDesiredAccess);
  
  RtlFreeUnicodeString(&WindowStationNameU);
  
  return hWinSta;
}

HWINSTA STDCALL
OpenWindowStationW(LPWSTR lpszWinSta,
		   WINBOOL fInherit,
		   ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING WindowStationName;

  RtlInitUnicodeString(&WindowStationName, lpszWinSta);

  return NtUserOpenWindowStation(&WindowStationName, dwDesiredAccess);
}

WINBOOL STDCALL
SetProcessWindowStation(HWINSTA hWinSta)
{
  return NtUserSetProcessWindowStation(hWinSta);
}

/* EOF */

