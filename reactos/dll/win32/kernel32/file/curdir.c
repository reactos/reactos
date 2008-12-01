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
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/* GLOBAL VARIABLES **********************************************************/

UNICODE_STRING SystemDirectory;
UNICODE_STRING WindowsDirectory;


/* FUNCTIONS *****************************************************************/




/*
 * @implemented
 */
DWORD
WINAPI
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
WINAPI
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
WINAPI
SetCurrentDirectoryA (
	LPCSTR	lpPathName
	)
{
   PWCHAR PathNameW;

   TRACE("setcurrdir: %s\n",lpPathName);

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
      return FALSE;

   return SetCurrentDirectoryW(PathNameW);
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
GetTempPathW (
	DWORD	count,
   LPWSTR   path
	)
{
    static const WCHAR tmp[]  = { 'T', 'M', 'P', 0 };
    static const WCHAR temp[] = { 'T', 'E', 'M', 'P', 0 };
    static const WCHAR userprofile[] = { 'U','S','E','R','P','R','O','F','I','L','E',0 };
    WCHAR tmp_path[MAX_PATH];
    UINT ret;

    TRACE("%u,%p\n", count, path);

    if (!(ret = GetEnvironmentVariableW( tmp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( temp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( userprofile, tmp_path, MAX_PATH )) &&
        !(ret = GetWindowsDirectoryW( tmp_path, MAX_PATH )))
        return 0;

   if (ret > MAX_PATH)
   {
     SetLastError(ERROR_FILENAME_EXCED_RANGE);
     return 0;
   }

   ret = GetFullPathNameW(tmp_path, MAX_PATH, tmp_path, NULL);
   if (!ret) return 0;

   if (ret > MAX_PATH - 2)
   {
     SetLastError(ERROR_FILENAME_EXCED_RANGE);
     return 0;
   }

   if (tmp_path[ret-1] != '\\')
   {
     tmp_path[ret++] = '\\';
     tmp_path[ret]   = '\0';
   }

   ret++; /* add space for terminating 0 */

   if (count)
   {
     lstrcpynW(path, tmp_path, count);
     if (count >= ret)
         ret--; /* return length without 0 */
     else if (count < 4)
         path[0] = 0; /* avoid returning ambiguous "X:" */
   }

   TRACE("GetTempPathW returning %u, %s\n", ret, path);
   return ret;

}


/*
 * @implemented
 */
UINT
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
GetSystemWow64DirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
#ifdef _WIN64
    ERR("GetSystemWow64DirectoryW is UNIMPLEMENTED!\n");
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
WINAPI
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
