/* $Id: curdir.c,v 1.19 2000/01/11 17:30:16 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <windows.h>
#include <wchar.h>
#include <ctype.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS *******************************************************************/

static HANDLE hCurrentDirectory = NULL;
static WCHAR CurrentDirectoryW[MAX_PATH];

static WCHAR SystemDirectoryW[MAX_PATH];
static WCHAR WindowsDirectoryW[MAX_PATH];

WINBOOL STDCALL SetCurrentDirectoryW(LPCWSTR lpPathName);

/* FUNCTIONS *****************************************************************/

DWORD STDCALL GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer)
{
   UINT uSize,i;
   WCHAR TempDir[MAX_PATH];
   
   if ( lpBuffer == NULL ) 
	return 0;
 
   GetCurrentDirectoryW(MAX_PATH, TempDir);
   uSize = lstrlenW(TempDir); 
   if (nBufferLength > uSize) 
     {
	i = 0;
   	while (TempDir[i] != 0)
     	{
	   lpBuffer[i] = (unsigned char)TempDir[i];
	   i++;
     	}
   	lpBuffer[i] = 0;
     }
   return uSize;
}

DWORD STDCALL GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
   UINT uSize;

   DPRINT("GetCurrentDirectoryW()\n");

   if ( lpBuffer == NULL )
	return 0;

   uSize = lstrlenW(CurrentDirectoryW);
   if (nBufferLength > uSize)
     {
	lstrcpyW(lpBuffer, CurrentDirectoryW);
     }
   if (uSize > 3 && lpBuffer[uSize - 1] == L'\\')
     {
        lpBuffer[uSize - 1] = 0;
        uSize--;
     }
   DPRINT("GetCurrentDirectoryW() = '%S'\n",lpBuffer);

   return uSize;
#if 0
	return (RtlGetCurrentDirectory_U (nBufferLength, lpBuffer) / sizeof (WCHAR));
#endif
}

WINBOOL STDCALL SetCurrentDirectoryA(LPCSTR lpPathName)
{
   UINT i;
   WCHAR PathNameW[MAX_PATH];
 
   if ( lpPathName == NULL )
     {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
     }

   if ( lstrlen(lpPathName) > MAX_PATH )
     {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
     }

   i = 0;
   while ((lpPathName[i])!=0 && i < MAX_PATH)
     {
        PathNameW[i] = (WCHAR)lpPathName[i];
        i++;
     }
   PathNameW[i] = 0;

   return SetCurrentDirectoryW(PathNameW);
}

WINBOOL STDCALL SetCurrentDirectoryW(LPCWSTR lpPathName)
{
   ULONG len;
   WCHAR PathName[MAX_PATH];
   HANDLE hDir;
   
   DPRINT("SetCurrentDirectoryW(lpPathName %S)\n",lpPathName);
   
   if (lpPathName == NULL)
     {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
     }
   
   len = lstrlenW(lpPathName);
   if (len > MAX_PATH)
     {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
     }

   if (!GetFullPathNameW (lpPathName, MAX_PATH, PathName, NULL))
     {
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
     }

   DPRINT("PathName %S\n",PathName);

   hDir = CreateFileW(PathName,
		      GENERIC_READ,
		      FILE_SHARE_READ,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_DIRECTORY,
		      NULL);
   if (hDir == INVALID_HANDLE_VALUE)
     {
	DPRINT("Failed to open directory\n");
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
     }

   if (hCurrentDirectory != NULL)
     {
	CloseHandle(hCurrentDirectory);
     }

   hCurrentDirectory = hDir;
   
   DPRINT("PathName %S\n",PathName);

   wcscpy(CurrentDirectoryW,PathName);
   len = lstrlenW(CurrentDirectoryW);
   if (CurrentDirectoryW[len-1] != '\\')
     {
        CurrentDirectoryW[len] = '\\';
        CurrentDirectoryW[len+1] = 0;
     }

   return TRUE;
#if 0
	UNICODE_STRING PathName_U;

	RtlInitUnicodeString (&PathName_U,
	                      lpPathName);

	return RtlSetCurrentDirectory_U (&PathName_U);
#endif
}

DWORD STDCALL GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer)
{
	WCHAR BufferW[MAX_PATH];
	DWORD retCode;
	UINT i;
	retCode = GetTempPathW(nBufferLength,BufferW);
	i = 0;
   	while ((BufferW[i])!=0 && i < MAX_PATH)
     	{
		lpBuffer[i] = (unsigned char)BufferW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
	return retCode;
}

DWORD STDCALL GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	WCHAR EnvironmentBufferW[MAX_PATH];
	UINT i;

	EnvironmentBufferW[0] = 0;
	i = GetEnvironmentVariableW(L"TMP",EnvironmentBufferW,MAX_PATH);
	if ( i==0 )
		i = GetEnvironmentVariableW(L"TEMP",EnvironmentBufferW,MAX_PATH);
		if ( i==0 )
			i = GetCurrentDirectoryW(MAX_PATH,EnvironmentBufferW);

	return i;
}

UINT STDCALL GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize)
{
   UINT uPathSize,i;
   if ( lpBuffer == NULL ) 
	return 0;
   uPathSize = lstrlenW(SystemDirectoryW); 
   if ( uSize > uPathSize ) {
   	i = 0;
   	while ((SystemDirectoryW[i])!=0 && i < uSize)
     	{
		lpBuffer[i] = (unsigned char)SystemDirectoryW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
   }
   
   return uPathSize;
}

UINT STDCALL GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize)
{
   UINT uPathSize,i;
   if ( lpBuffer == NULL ) 
	return 0;
   uPathSize = lstrlenW(WindowsDirectoryW); 
   if ( uSize > uPathSize ) {
   	i = 0;
   	while ((WindowsDirectoryW[i])!=0 && i < uSize)
     	{
		lpBuffer[i] = (unsigned char)WindowsDirectoryW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
   }
   return uPathSize;
}

UINT
STDCALL
GetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
   UINT uPathSize;
   if ( lpBuffer == NULL ) 
	return 0;
   uPathSize = lstrlenW(SystemDirectoryW); 
   if ( uSize > uPathSize ) 
   	lstrcpynW(lpBuffer,SystemDirectoryW,uPathSize);

   return uPathSize;
}

UINT
STDCALL
GetWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
   UINT uPathSize;
   if ( lpBuffer == NULL ) 
	return 0;
   uPathSize = lstrlenW(WindowsDirectoryW); 
   if ( uSize > uPathSize );
   	lstrcpynW(lpBuffer,WindowsDirectoryW,uPathSize);

   return uPathSize;
}

/* EOF */
