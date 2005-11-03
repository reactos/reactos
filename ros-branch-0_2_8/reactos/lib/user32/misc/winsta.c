/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/winsta.c
 * PURPOSE:         Window stations
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      04-06-2001  CSH  Created
 */

#include <user32.h>

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
 * Common code for EnumDesktopsA/W and EnumWindowStationsA/W
 */
BOOL FASTCALL
EnumNamesW(HWINSTA WindowStation,
           NAMEENUMPROCW EnumFunc,
	   LPARAM Context,
           BOOL Desktops)
{
   char Buffer[256];
   PVOID NameList;
   PWCHAR Name;
   NTSTATUS Status;
   ULONG RequiredSize;
   ULONG CurrentEntry, EntryCount;
   BOOL Ret;

   /*
    * Check parameters
    */
   if (NULL == WindowStation && Desktops)
   {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   /*
    * Try with fixed-size buffer
    */
   Status = NtUserBuildNameList(WindowStation, sizeof(Buffer), Buffer, &RequiredSize);
   if (NT_SUCCESS(Status))
   {
      /* Fixed-size buffer is large enough */
      NameList = (PWCHAR) Buffer;
   }
   else if (Status == STATUS_BUFFER_TOO_SMALL)
   {
      /* Allocate a larger buffer */
      NameList = HeapAlloc(GetProcessHeap(), 0, RequiredSize);
      if (NULL == NameList)
      {
         return FALSE;
      }
      /* Try again */
      Status = NtUserBuildNameList(WindowStation, RequiredSize, NameList, NULL);
      if (! NT_SUCCESS(Status))
      {
         HeapFree(GetProcessHeap(), 0, NameList);
         SetLastError(RtlNtStatusToDosError(Status));
         return FALSE;
      }
   }
   else
   {
      /* Some unrecognized error occured */
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
   }

   /*
    * Enum the names one by one
    */
   EntryCount = *((DWORD *) NameList);
   Name = (PWCHAR) ((PCHAR) NameList + sizeof(DWORD));
   Ret = TRUE;
   for (CurrentEntry = 0; CurrentEntry < EntryCount && Ret; ++CurrentEntry)
   {
      Ret = (*EnumFunc)(Name, Context);
      Name += wcslen(Name) + 1;
   }

   /*
    * Cleanup
    */
   if (NameList != Buffer)
   {
      HeapFree(GetProcessHeap(), 0, NameList);
   }

   return Ret;
}


/* For W->A conversion */
typedef struct tagENUMNAMESASCIICONTEXT
{
   NAMEENUMPROCA UserEnumFunc;
   LPARAM UserContext;
} ENUMNAMESASCIICONTEXT, *PENUMNAMESASCIICONTEXT;

/*
 * Callback used by Ascii versions. Converts the Unicode name to
 * Ascii and then calls the user callback
 */
BOOL CALLBACK
EnumNamesCallback(LPWSTR Name, LPARAM Param)
{
   PENUMNAMESASCIICONTEXT Context = (PENUMNAMESASCIICONTEXT) Param;
   char FixedNameA[32];
   LPSTR NameA;
   int Len;
   BOOL Ret;

   /*
    * Determine required size of Ascii string and see if we can use
    * fixed buffer
    */
   Len = WideCharToMultiByte(CP_ACP, 0, Name, -1, NULL, 0, NULL, NULL);
   if (Len <= 0)
   {
      /* Some strange error occured */
      return FALSE;
   }
   else if (Len <= sizeof(FixedNameA))
   {
      /* Fixed-size buffer is large enough */
      NameA = FixedNameA;
   }
   else
   {
      /* Allocate a larger buffer */
      NameA = HeapAlloc(GetProcessHeap(), 0, Len);
      if (NULL == NameA)
      {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }
   }

   /*
    * Do the Unicode ->Ascii conversion
    */
   if (0 == WideCharToMultiByte(CP_ACP, 0, Name, -1, NameA, Len, NULL, NULL))
   {
      /* Something went wrong, clean up */
      if (NameA != FixedNameA)
      {
         HeapFree(GetProcessHeap(), 0, NameA);
      }
      return FALSE;
   }

   /*
    * Call user callback
    */
   Ret = Context->UserEnumFunc(NameA, Context->UserContext);

   /*
    * Clean up
    */
   if (NameA != FixedNameA)
   {
      HeapFree(GetProcessHeap(), 0, NameA);
   }

   return Ret;
}

/*
 * Common code for EnumDesktopsA and EnumWindowStationsA
 */
BOOL FASTCALL
EnumNamesA(HWINSTA WindowStation,
           NAMEENUMPROCA EnumFunc,
	   LPARAM Context,
           BOOL Desktops)
{
   ENUMNAMESASCIICONTEXT PrivateContext;

   PrivateContext.UserEnumFunc = EnumFunc;
   PrivateContext.UserContext = Context;

   return EnumNamesW(WindowStation, EnumNamesCallback, (LPARAM) &PrivateContext, Desktops);
}

/*
 * @implemented
 */
BOOL STDCALL
EnumWindowStationsA(WINSTAENUMPROCA EnumFunc,
		    LPARAM Context)
{
   return EnumNamesA(NULL, EnumFunc, Context, FALSE);
}


/*
 * @implemented
 */
BOOL STDCALL
EnumWindowStationsW(WINSTAENUMPROCW EnumFunc,
		    LPARAM Context)
{
   return EnumNamesW(NULL, EnumFunc, Context, FALSE);
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

