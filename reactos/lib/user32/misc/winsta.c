/* $Id: winsta.c,v 1.14 2004/01/23 23:38:26 ekohl Exp $
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
BOOL STDCALL
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
BOOL STDCALL
EnumWindowStationsA(ENUMWINDOWSTATIONPROCA lpEnumFunc,
		    LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
EnumWindowStationsW(ENUMWINDOWSTATIONPROCW lpEnumFunc,
		    LPARAM lParam)
{
   PWCHAR Buffer;
   NTSTATUS Status;
   ULONG dwRequiredSize;
   ULONG CurrentEntry, EntryCount;

   Buffer = HeapAlloc(GetProcessHeap(), 0, 200);
   if (Buffer == NULL)
   {
      return FALSE;
   }
   Status = NtUserBuildNameList(0, 200, Buffer, &dwRequiredSize);
   if (Status == STATUS_BUFFER_TOO_SMALL)
   {
      Buffer = HeapReAlloc(GetProcessHeap(), 0, Buffer, dwRequiredSize);
      if (Buffer == NULL)
      {
         return FALSE;
      }
      Status = NtUserBuildNameList(0, dwRequiredSize, Buffer, &dwRequiredSize);
   }
   if (Status != STATUS_SUCCESS)
   {
      HeapFree(GetProcessHeap(), 0, Buffer);
      return FALSE;
   }
   EntryCount = *((DWORD *)Buffer);
   Buffer += sizeof(DWORD) / sizeof(WCHAR);
   for (CurrentEntry = 0; CurrentEntry < EntryCount; ++CurrentEntry)
   {
      (*lpEnumFunc)(Buffer, lParam);
      Buffer += wcslen(Buffer) + 1;
   }
   return TRUE;
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
		   BOOL fInherit,
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
		   BOOL fInherit,
		   ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING WindowStationName;

  RtlInitUnicodeString(&WindowStationName, lpszWinSta);

  return NtUserOpenWindowStation(&WindowStationName, dwDesiredAccess);
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL
STDCALL
LockWindowStation(HWINSTA hWinSta)
{
  return NtUserLockWindowStation(hWinSta);
}


/*
 * @implemented
 */
BOOL
STDCALL
UnlockWindowStation(HWINSTA hWinSta)
{
  return NtUserUnlockWindowStation(hWinSta);
}

/* EOF */

