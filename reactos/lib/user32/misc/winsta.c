/* $Id: winsta.c,v 1.12 2003/11/09 13:50:03 navaraf Exp $
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


/*
 * @implemented
 */
WINBOOL STDCALL
CloseWindowStation(HWINSTA hWinSta)
{
  return(NtUserCloseWindowStation(hWinSta));
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnumWindowStationsA(ENUMWINDOWSTATIONPROCA lpEnumFunc,
		    LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnumWindowStationsW(ENUMWINDOWSTATIONPROCW lpEnumFunc,
		    LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWINSTA STDCALL
GetProcessWindowStation(VOID)
{
  return NtUserGetProcessWindowStation();
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
HWINSTA STDCALL
OpenWindowStationW(LPWSTR lpszWinSta,
		   WINBOOL fInherit,
		   ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING WindowStationName;

  RtlInitUnicodeString(&WindowStationName, lpszWinSta);

  return NtUserOpenWindowStation(&WindowStationName, dwDesiredAccess);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetProcessWindowStation(HWINSTA hWinSta)
{
  return NtUserSetProcessWindowStation(hWinSta);
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SetWindowStationUser(
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4
  )
{
  return NtUserSetWindowStationUser(Unknown1, Unknown2, Unknown3, Unknown4);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
LockWindowStation(HWINSTA hWinSta)
{
  return NtUserLockWindowStation(hWinSta);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
UnlockWindowStation(HWINSTA hWinSta)
{
  return NtUserUnlockWindowStation(hWinSta);
}

/* EOF */

