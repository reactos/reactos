/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <windows.h>

//#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS *******************************************************************/

#define MAX_DOS_DRIVES 26

static HANDLE hCurrentDirectory = NULL;
static ULONG CurrentDrive = 0;

static WCHAR DriveDirectoryW[MAX_DOS_DRIVES][MAX_PATH] = {{0}};

static WCHAR SystemDirectoryW[MAX_PATH];
static WCHAR WindowsDirectoryW[MAX_PATH];

WINBOOL STDCALL SetCurrentDirectoryW(LPCWSTR lpPathName);

/* FUNCTIONS *****************************************************************/

DWORD STDCALL GetCurrentDriveW(DWORD nBufferLength, PWSTR lpBuffer)
{
   lpBuffer[0] = 'A' + CurrentDrive;
   lpBuffer[1] = ':';
   lpBuffer[2] = '\\';
   lpBuffer[3] = 0;
   return(4);
}

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
   uSize = lstrlenW(DriveDirectoryW[CurrentDrive]) + 2;
   if (nBufferLength > uSize)
     {
	lpBuffer[0] = 'A' + CurrentDrive;
	lpBuffer[1] = ':';
	lpBuffer[2] = 0;
	lstrcpyW(&lpBuffer[2], DriveDirectoryW[CurrentDrive]);
     }
   DPRINT("GetCurrentDirectoryW() = '%w'\n",lpBuffer);
   return uSize;
}

WINBOOL STDCALL SetCurrentDirectoryA(LPCSTR lpPathName)
{
   UINT i;
   WCHAR PathNameW[MAX_PATH];
 
   if ( lpPathName == NULL )
     return FALSE;
   if ( lstrlen(lpPathName) > MAX_PATH )
     return FALSE;
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
   PWSTR prev, current;
   
   DPRINT("SetCurrentDirectoryW(lpPathName %w)\n",lpPathName);
   
   if (lpPathName == NULL)
     return FALSE;
   
   len = lstrlenW(lpPathName);
   if (len > MAX_PATH)
     return FALSE;
  
   hDir = CreateFileW(lpPathName,
		      GENERIC_READ,
		      FILE_SHARE_READ,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_DIRECTORY,
		      NULL);
   if (hDir == NULL)
     {
	DPRINT("Failed to open directory\n");
	return(FALSE);
     }
   if (hCurrentDirectory != NULL)
     {
	CloseHandle(hCurrentDirectory);
     }
   hCurrentDirectory = hDir;
   
   DPRINT("lpPathName %w %x\n",lpPathName,lpPathName);
   if (wcslen(lpPathName) > 2 &&
       isalpha(lpPathName[0]) && 
       lpPathName[1] == ':' )
     {
	DPRINT("lpPathName %w\n",lpPathName);
	
	CurrentDrive = toupper((UCHAR)lpPathName[0]) - 'A';
	if (!(lpPathName[2] == '\\' && lpPathName[3] == 0 &&
	      DriveDirectoryW[CurrentDrive][0] != 0))
	  {
	     wcscpy(DriveDirectoryW[CurrentDrive],&lpPathName[2]);
	     len = lstrlenW(DriveDirectoryW[CurrentDrive]);
	     if (DriveDirectoryW[CurrentDrive][len-1] != '\\')
	       {
		  DriveDirectoryW[CurrentDrive][len] = '\\';
		  DriveDirectoryW[CurrentDrive][len+1] = 0;
	       }
	  }	
	return(TRUE);
     }
   if (lpPathName[0] == '\\')
     {
	wcscpy(DriveDirectoryW[CurrentDrive],lpPathName);
	return(TRUE);
     }
   
   GetCurrentDirectoryW(MAX_PATH, PathName);
   lstrcatW(PathName, lpPathName);
   len = lstrlenW(PathName);
   if (PathName[len-1] != '\\')
     {
	PathName[len] = '\\';
	PathName[len+1] = 0;
     }
   
   DPRINT("PathName %w\n",PathName);
   
   prev = NULL;
   current = &PathName[2];
   
   while (current != NULL)
     {
	if (current[1] == '.' && current[2] == '\\')
	  {
	     wcscpy(current, current+2);
	  }
	else if (current[1] == '.' && current[2] == '.' &&
		 current[3] == '\\' && prev != NULL)
	  {
	     wcscpy(prev, current+3);
	     current = prev;
	     while (prev > PathName && (*prev) != '\\')
	       {
		  prev--;
	       }
	     if (prev == PathName)
	       {
		  prev = NULL;
	       }
	  }
	else
	  {  
	     prev = current;
	     current = wcschr(current+1, (WCHAR)'\\');
	  }
     }
   
   lstrcpyW(DriveDirectoryW[CurrentDrive], &PathName[2]);   
   return TRUE;
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


