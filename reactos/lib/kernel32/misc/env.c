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
#include <wchar.h>
#include <string.h>

#include <kernel32/kernel32.h>

#define MAX_ENVIRONMENT_VARS 255
#define MAX_VALUE 1024

typedef struct _ENV_ELEMENT 
{
	UNICODE_STRING Name;
	UNICODE_STRING Value;
	WINBOOL Valid;
} ENV_ELEMENT;

ENV_ELEMENT Environment[MAX_ENVIRONMENT_VARS+1];
UINT nEnvVar = 0;

DWORD STDCALL GetEnvironmentVariableA(LPCSTR lpName,
				      LPSTR lpBuffer,
				      DWORD nSize)
{
   WCHAR BufferW[MAX_VALUE];
   WCHAR NameW[MAX_PATH];
   DWORD RetValue;
   int i=0;
   
   while ((*lpName)!=0 && i < MAX_PATH)
     {
	NameW[i] = *lpName;
	lpName++;
	i++;
     }
   NameW[i] = 0;
   
   RetValue = GetEnvironmentVariableW(NameW,BufferW,nSize);
   for(i=0;i<nSize;i++)
     lpBuffer[i] = (char)BufferW[i];	
   return RetValue;
}

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
		if ( wcsnicmp(Environment[i].Name.Buffer,lpName,min(NameLen,Environment[i].Name.Length/sizeof(WCHAR))) != 0 ) {
			lstrcpynW(lpBuffer,Environment[i].Value.Buffer,min(nSize,Environment[i].Value.Length/sizeof(WCHAR)));
			
			return lstrlenW(Environment[i].Value.Buffer);
			
		}
		i++;
	}
	return 0;
	
}



WINBOOL
STDCALL
SetEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpValue
    )
{
	WCHAR NameW[MAX_PATH];
	WCHAR ValueW[MAX_VALUE];

	int i=0;
	while ((*lpName)!=0 && i < MAX_PATH)
     	{
		NameW[i] = *lpName;
		lpName++;
		i++;
     	}
   	NameW[i] = 0;

	i = 0;
	
	while ((*lpValue)!=0 && i < MAX_PATH)
     	{
		ValueW[i] = *lpValue;
		lpValue++;
		i++;
     	}
   	ValueW[i] = 0;
	return SetEnvironmentVariableW(NameW,ValueW);

	
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
		if ( wcsnicmp(Environment[i].Name.Buffer,lpName,min(NameLen,Environment[i].Name.Length/sizeof(WCHAR))) != 0 ) {
			if ( lpValue != NULL ) {
				lstrcpynW(Environment[i].Value.Buffer,lpValue,min(ValueLen,Environment[i].Value.MaximumLength/sizeof(WCHAR)));
				return TRUE;
			}
			else {
				Environment[i].Valid = FALSE;
				Environment[i].Value.Length = 0;
				Environment[i].Name.Length = 0;
				return FALSE;
			}
				
				
			
		}
		i++;
	}

	if ( nEnvVar >= MAX_ENVIRONMENT_VARS )
		return FALSE;

	while (i < nEnvVar) 
	{
		if ( Environment[i].Valid == FALSE ) 
			break;
		i++;
	}
	if ( i == nEnvVar ) {
		NameBuffer = (WCHAR *)HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,MAX_PATH*sizeof(WCHAR) );
		ValueBuffer = (WCHAR *)HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,MAX_VALUE*sizeof(WCHAR) );
	
		Environment[i].Name.Buffer = NameBuffer;
		Environment[i].Name.MaximumLength = MAX_PATH*sizeof(WCHAR);

		Environment[i].Value.Buffer = ValueBuffer;
		Environment[i].Value.MaximumLength = MAX_VALUE*sizeof(WCHAR);
		nEnvVar++;
	}
	Environment[i].Valid = TRUE;

	lstrcpynW(Environment[i].Name.Buffer,lpValue,min(NameLen,(Environment[i].Name.MaximumLength-sizeof(WCHAR))/sizeof(WCHAR)));
	Environment[i].Name.Length = NameLen*sizeof(WCHAR);

	
	lstrcpynW(Environment[i].Value.Buffer,lpValue,min(ValueLen,(Environment[i].Value.MaximumLength-sizeof(WCHAR)))/sizeof(WCHAR));
	Environment[i].Value.Length = ValueLen*sizeof(WCHAR);
	
	
	
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
	lstrcpyW((WCHAR *)lpVersionInformation->szCSDVersion,L"Ariadne was here...");
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
	lstrcpyA((char *)lpVersionInformation->szCSDVersion,"ReactOs Pre-Alpha");
	return TRUE;
}





LPSTR STDCALL GetEnvironmentStringsA(VOID)
{
#if 0
   WCHAR *EnvironmentStringsW;
   char *EnvironmentStringsA;
   int size = 0;
   int i;
#endif
   
   return(NULL);
   
   /* FIXME: This doesn't work */
#if 0
   EnvironmentStringsW = GetEnvironmentStringsW();
   EnvironmentStringsA = (char *)EnvironmentStringsW;

   for(i=0;i<nEnvVar;i++) 
     {
	if ( Environment[i].Valid ) 
	  {
	     size += Environment[i].Name.Length;
	     size += sizeof(WCHAR); // =
	     size += Environment[i].Value.Length;
	     size += sizeof(WCHAR); // zero
	  }
     }
   size += sizeof(WCHAR);
   size /= sizeof(WCHAR);
   for(i=0;i<size;i++)
     EnvironmentStringsA[i] = (char)EnvironmentStringsW[i];	
   return EnvironmentStringsA;
#endif
}


LPWSTR STDCALL GetEnvironmentStringsW(VOID)
{
#if 0
   int size = 0;
   int i;
   WCHAR *EnvironmentString;
   WCHAR *EnvironmentStringSave;
#endif
   
   return(NULL);
   
   /* FIXME: This doesn't work, why not? */
#if 0
   for(i=0;i<nEnvVar;i++) 
     {
	if ( Environment[i].Valid ) 
	  {
	     size += Environment[i].Name.Length;
	     size += sizeof(WCHAR); // =
	     size += Environment[i].Value.Length;
	     size += sizeof(WCHAR); // zero
	  }
     }
   size += sizeof(WCHAR); // extra zero
   DPRINT("size %d\n",size);
   EnvironmentString =  (WCHAR *)HeapAlloc(GetProcessHeap(),
					   HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
					   size);
   DPRINT("EnvironmentString %x\n",EnvironmentString);
   EnvironmentStringSave = EnvironmentString;
   for(i=0;i<nEnvVar;i++) 
     {
	if ( Environment[i].Valid ) 
	  {
	     wcscpy(EnvironmentString,Environment[i].Name.Buffer);
	     wcscat(EnvironmentString,L"=");
	     wcscat(EnvironmentString,Environment[i].Value.Buffer);
	     
	     size = Environment[i].Name.Length;
	     size += sizeof(WCHAR); // =
	     size += Environment[i].Value.Length;
	     size += sizeof(WCHAR); // zero
	     EnvironmentString += (size/sizeof(WCHAR));
	  }
     }
   *EnvironmentString = 0;
   return EnvironmentStringSave;
#endif
}


WINBOOL
STDCALL
FreeEnvironmentStringsA(
			LPSTR EnvironmentStrings
			)
{
	if ( EnvironmentStrings == NULL )
		return FALSE;
	HeapFree(GetProcessHeap(),0,EnvironmentStrings);
	return TRUE;
}

WINBOOL
STDCALL
FreeEnvironmentStringsW(
    LPWSTR EnvironmentStrings
    )
{
	if ( EnvironmentStrings == NULL )
		return FALSE;
	HeapFree(GetProcessHeap(),0,EnvironmentStrings);
	return TRUE;
}

