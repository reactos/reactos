/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <wstring.h>
#include <string.h>

#define MAX_ENVIRONMENT_VARS 255

typedef struct _ENV_ELEMENT 
{
	UNICODE_STRING Name;
	UNICODE_STRING Value;
} ENV_ELEMENT;

ENV_ELEMENT Environment[MAX_ENVIRONMENT_VARS+1];
UINT nEnvVar = 0;

int wcsncmp2(CONST WCHAR *s, CONST WCHAR *t,UINT n);

DWORD
STDCALL
GetEnvironmentVariableW(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    )
{
	UINT NameLen;
	UINT i;
	NameLen = lstrlenW(lpName);
	i = 0;

	while (i < nEnvVar) 
	{
		if ( wcsncmp2(Environment[i].Name.Buffer,lpName,min(NameLen,Environment[i].Name.Length)) != 0 ) {
			lstrcpynW(lpBuffer,Environment[i].Value.Buffer,min(nSize,Environment[i].Value.Length));
			
			return lstrlenW(lpBuffer);
			
		}
		i++;
	}
	return 0;
	
}


int wcsncmp2(CONST WCHAR *s, CONST WCHAR *t,UINT n)
{
	for(;towupper(*s) == towupper(*t) && n > 0; s++, t++, n--)
		if ( *s == 0 )
			return 0;
	return *s - *t;
} 


WINBOOL
STDCALL
SetEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpValue
    )
{
	UINT NameLen, ValueLen;
	UINT i;
	WCHAR *NameBuffer;
	WCHAR *ValueBuffer;
	NameLen = lstrlenW(lpName);
	ValueLen = lstrlenW(lpValue);
	i = 0;

	while (i < nEnvVar) 
	{
		if ( wcsncmp2(Environment[i].Name.Buffer,lpName,min(NameLen,Environment[i].Name.Length)) != 0 ) {
			lstrcpynW(Environment[i].Value.Buffer,lpValue,min(ValueLen,Environment[i].Value.MaximumLength));
			return TRUE;
			
		}
		i++;
	}

	if ( nEnvVar > MAX_ENVIRONMENT_VARS )
		return FALSE;
	NameBuffer = (WCHAR *)HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,MAX_PATH*sizeof(WCHAR));
	ValueBuffer = (WCHAR *)HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,1024*sizeof(WCHAR));
	
	Environment[i].Name.Buffer = NameBuffer;
	Environment[i].Name.MaximumLength = MAX_PATH;
	lstrcpynW(Environment[i].Name.Buffer,lpValue,min(NameLen,Environment[i].Name.MaximumLength));
	Environment[i].Name.Length = NameLen;

	Environment[i].Value.Buffer = ValueBuffer;
	Environment[i].Value.MaximumLength = 1024;
	lstrcpynW(Environment[i].Value.Buffer,lpValue,min(ValueLen,Environment[i].Value.MaximumLength));
	Environment[i].Value.Length = ValueLen;
	
	
	nEnvVar++;
	
	return TRUE;

}

DWORD 
STDCALL
GetVersion(VOID)
{
	DWORD Version = 0;
	OSVERSIONINFO VersionInformation;
	GetVersionExW(&VersionInformation);
	
	Version |= ( VersionInformation.dwMajorVersion << 8 );
	Version |= VersionInformation.dwMinorVersion;
	
	Version |= ( VersionInformation.dwPlatformId << 16 );

	return Version;
		
}



WINBOOL 
STDCALL
GetVersionExW(
    LPOSVERSIONINFO lpVersionInformation 	
   )
{
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	lpVersionInformation->dwMajorVersion = 4;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwBuildNumber = 12;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	lstrcpyW((WCHAR *)lpVersionInformation->szCSDVersion,L"ReactOs Pre-Alpha 12");
	return TRUE;
}

WINBOOL 
STDCALL
GetVersionExA(
    LPOSVERSIONINFO lpVersionInformation 	
   )
{
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	lpVersionInformation->dwMajorVersion = 4;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwBuildNumber = 12;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	lstrcpyA((char *)lpVersionInformation->szCSDVersion,"ReactOs Pre-Alpha 12");
	return TRUE;
}

