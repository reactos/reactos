/* $Id: winsta.c,v 1.10 2003/08/21 16:04:26 weiden Exp $
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
  HWINSTA res;
  UNICODE_STRING WindowStationName;
  HMENU SysMenuTemplate;
  HMODULE hUser32;
  
  RtlInitUnicodeString(&WindowStationName, lpwinsta);
  
  res = NtUserCreateWindowStation(&WindowStationName,
				   dwDesiredAccess,
				   lpsa, 0, 0, 0);
				   
  hUser32 = GetModuleHandleW(L"user32.dll");
  SysMenuTemplate = LoadMenuW(hUser32, L"SYSMENU");
				   
  if(SysMenuTemplate)
  {
    NtUserCallTwoParam((DWORD)res, (DWORD)SysMenuTemplate, 
                       TWOPARAM_ROUTINE_SETWINSTASYSMENU);
    /* we don't need the menu anymore, it's been cloned */
    DestroyMenu(SysMenuTemplate);
  }
    
  return res;
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

/* EOF */

