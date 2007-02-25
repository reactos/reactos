/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * PROGRAMMER:      Eric Kohl
 *                  Filip Navara
 *                  Steven Edwards
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* GLOBAL VARIABLES **********************************************************/

UNICODE_STRING SystemDirectory;
UNICODE_STRING WindowsDirectory;


/* FUNCTIONS *****************************************************************/




/*
 * @implemented
 */
DWORD
STDCALL
GetCurrentDirectoryA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
   WCHAR BufferW[MAX_PATH];
   DWORD ret;

   ret = GetCurrentDirectoryW(MAX_PATH, BufferW);

   if (!ret) return 0;
   if (ret > MAX_PATH)
   {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      return 0;
   }

   return FilenameW2A_FitOrFail(lpBuffer, nBufferLength, BufferW, ret+1);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetCurrentDirectoryW (
	DWORD	nBufferLength,
	LPWSTR	lpBuffer
	)
{
	ULONG Length;

	Length = RtlGetCurrentDirectory_U (nBufferLength * sizeof(WCHAR),
	                                   lpBuffer);

	return (Length / sizeof (WCHAR));
}



/*
 * @implemented
 */
BOOL
STDCALL
SetCurrentDirectoryA (
	LPCSTR	lpPathName
	)
{
   PWCHAR PathNameW;

   DPRINT("setcurrdir: %s\n",lpPathName);

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
      return FALSE;

   return SetCurrentDirectoryW(PathNameW);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCurrentDirectoryW (
	LPCWSTR	lpPathName
	)
{
	UNICODE_STRING UnicodeString;
	NTSTATUS Status;

	RtlInitUnicodeString (&UnicodeString,
	                      lpPathName);

	Status = RtlSetCurrentDirectory_U (&UnicodeString);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 *
 * NOTE: Windows returns a dos/short (8.3) path
 */
DWORD
STDCALL
GetTempPathA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
   WCHAR BufferW[MAX_PATH];
   DWORD ret;

   ret = GetTempPathW(MAX_PATH, BufferW);

   if (!ret)
      return 0;

   if (ret > MAX_PATH)
   {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      return 0;
   }

   return FilenameW2A_FitOrFail(lpBuffer, nBufferLength, BufferW, ret+1);
}


/*
 * @implemented
 *
 * ripped from wine
 */
DWORD
STDCALL
GetTempPathW (
	DWORD	count,
   LPWSTR   path
	)
{
   WCHAR tmp_path[MAX_PATH];
   WCHAR tmp_full_path[MAX_PATH];
   UINT ret;

   DPRINT("GetTempPathW(%lu,%p)\n", count, path);

   if (!(ret = GetEnvironmentVariableW( L"TMP", tmp_path, MAX_PATH )))
     if (!(ret = GetEnvironmentVariableW( L"TEMP", tmp_path, MAX_PATH )))
         if (!(ret = GetCurrentDirectoryW( MAX_PATH, tmp_path )))
             return 0;

   if (ret > MAX_PATH)
   {
     SetLastError(ERROR_FILENAME_EXCED_RANGE);
     return 0;
   }

   ret = GetFullPathNameW(tmp_path, MAX_PATH, tmp_full_path, NULL);
   if (!ret) return 0;

   if (ret > MAX_PATH - 2)
   {
     SetLastError(ERROR_FILENAME_EXCED_RANGE);
     return 0;
   }

   if (tmp_full_path[ret-1] != '\\')
   {
     tmp_full_path[ret++] = '\\';
     tmp_full_path[ret]   = '\0';
   }

   ret++; /* add space for terminating 0 */

   if (count)
   {
     lstrcpynW(path, tmp_full_path, count);
     if (count >= ret)
         ret--; /* return length without 0 */
     else if (count < 4)
         path[0] = 0; /* avoid returning ambiguous "X:" */
   }

   DPRINT("GetTempPathW returning %u, %s\n", ret, path);
   return ret;

}


/*
 * @implemented
 */
UINT
STDCALL
GetSystemDirectoryA (
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
   return FilenameU2A_FitOrFail(lpBuffer, uSize, &SystemDirectory);
}


/*
 * @implemented
 */
UINT
STDCALL
GetSystemDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	Length = SystemDirectory.Length / sizeof (WCHAR);

	if (lpBuffer == NULL)
		return Length + 1;

	if (uSize > Length)	{
		memmove (lpBuffer,
		         SystemDirectory.Buffer,
		         SystemDirectory.Length);
		lpBuffer[Length] = 0;

		return Length;	  //good: ret chars excl. nullchar
	}

	return Length+1;	 //bad: ret space needed incl. nullchar
}

/*
 * @implemented
 */
UINT
STDCALL
GetWindowsDirectoryA (
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
   return FilenameU2A_FitOrFail(lpBuffer, uSize, &WindowsDirectory);
}


/*
 * @implemented
 */
UINT
STDCALL
GetWindowsDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	Length = WindowsDirectory.Length / sizeof (WCHAR);

	if (lpBuffer == NULL)
		return Length + 1;

	if (uSize > Length)
	{
		memmove (lpBuffer,
		         WindowsDirectory.Buffer,
		         WindowsDirectory.Length);
		lpBuffer[Length] = 0;

		return Length;	  //good: ret chars excl. nullchar
	}

	return Length+1;	//bad: ret space needed incl. nullchar
}

/*
 * @implemented
 */
UINT
STDCALL
GetSystemWindowsDirectoryA(
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
    return GetWindowsDirectoryA( lpBuffer, uSize );
}

/*
 * @implemented
 */
UINT
STDCALL
GetSystemWindowsDirectoryW(
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
    return GetWindowsDirectoryW( lpBuffer, uSize );
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
#ifdef _WIN64
    DPRINT1("GetSystemWow64DirectoryW is UNIMPLEMENTED!\n");
    return 0;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
#endif
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )
{
#ifdef _WIN64
   WCHAR BufferW[MAX_PATH];
   UINT ret;

   ret = GetSystemWow64DirectoryW(BufferW, MAX_PATH);

   if (!ret) return 0;
   if (ret > MAX_PATH)
   {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      return 0;
   }

   return FilenameW2A_FitOrFail(lpBuffer, uSize, BufferW, ret+1);
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
#endif
}

/* EOF */
