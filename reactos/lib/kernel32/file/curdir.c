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
#include <kernel32/kernel32.h>


/* GLOBALS *******************************************************************/

#define MAX_DOS_DRIVES 26

WCHAR CurrentDirectoryW[MAX_PATH] = {0,};
HANDLE hCurrentDirectory = NULL;
int drive= 2;

char DriveDirectoryW[MAX_DOS_DRIVES][MAX_PATH] = { {"A:\\"},{"B:\\"},{"C:\\"},{"D:\\"},
					{"E:\\"},{"F:\\"},{"G:\\"},{"H:\\"},
					{"I:\\"},{"J:\\"},{"K:\\"},{"L:\\"},
					{"M:\\"},{"N:\\"},{"O:\\"},{"P:\\"},
					{"Q:\\"},{"R:\\"},{"S:\\"},{"T:\\"},
					{"U:\\"},{"V:\\"},{"W:\\"},{"X:\\"},
					{"Y:\\"},{"Z:\\"} };

WCHAR SystemDirectoryW[MAX_PATH];

WCHAR WindowsDirectoryW[MAX_PATH];


WINBOOL
STDCALL
SetCurrentDirectoryW(
    LPCWSTR lpPathName
    );

/* FUNCTIONS *****************************************************************/
 
DWORD STDCALL GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer)
{
   UINT uSize,i;
   if ( lpBuffer == NULL ) 
	return 0;
 

   uSize = lstrlenW(CurrentDirectoryW); 
   if ( nBufferLength > uSize ) {
	i = 0;
   	while ((CurrentDirectoryW[i])!=0 && i < MAX_PATH)
     	{
		lpBuffer[i] = (unsigned char)CurrentDirectoryW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
   }
   return uSize;
}

DWORD STDCALL GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
   UINT uSize;
   
   DPRINT("CurrentDirectoryW %w\n",CurrentDirectoryW);
   
   if ( lpBuffer == NULL ) 
	return 0;
   uSize = lstrlenW(CurrentDirectoryW); 
   if ( nBufferLength > uSize )
   	lstrcpynW(lpBuffer,CurrentDirectoryW,uSize);
   
   DPRINT("GetCurrentDirectoryW() = %w\n",lpBuffer);
   
   return uSize;
}

WINBOOL STDCALL SetCurrentDirectoryA(LPCSTR lpPathName)
{
   UINT i;
   WCHAR PathNameW[MAX_PATH];
 
   
   if ( lpPathName == NULL )
		return FALSE;
   if ( strlen(lpPathName) > MAX_PATH )
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


WINBOOL
STDCALL
SetCurrentDirectoryW(
    LPCWSTR lpPathName
    )
{
   int len;
   int i,j;
   HANDLE hDirOld = hCurrentDirectory;
   WCHAR PathName[MAX_PATH];

   if ( lpPathName == NULL )
		return FALSE;
   len = lstrlenW(lpPathName);
   if ( len > MAX_PATH )
		return FALSE;
  
   if ( len == 2 && isalpha(lpPathName[0]) && lpPathName[1] == ':'  ) {
	    len = lstrlenW(CurrentDirectoryW);
	   	for(i=0;i<len+1;i++)
			DriveDirectoryW[drive][i] = CurrentDirectoryW[i];
		drive = toupper((char)lpPathName[0]) - 'A';
		len = lstrlenW(DriveDirectoryW[drive]);
		for(i=0;i<len+1;i++)
			CurrentDirectoryW[i] = DriveDirectoryW[drive][i];	
		if ( hDirOld  != NULL )
			CloseHandle(hDirOld);
		return TRUE;
   }
   if ( lpPathName[0] == '.' && lpPathName[1] == '\\') {
		lstrcpyW(PathName,CurrentDirectoryW);
		lstrcatW(PathName,&lpPathName[2]);
   }
   else if ( lpPathName[0] == '.' && lpPathName[1] == '.' ) {
		lstrcpyW(PathName,CurrentDirectoryW);
		lstrcatW(PathName,lpPathName);
   }
   else if ( lpPathName[0] != '.' && lpPathName[1] != ':' ) {
		lstrcpyW(PathName,CurrentDirectoryW);
		lstrcatW(PathName,lpPathName);
		
	}
   else
		lstrcpyW(PathName,CurrentDirectoryW);

  len = lstrlenW(PathName);
  for(i=0;i<len+1;i++) {
	  if ( PathName[i] == '.' && PathName[i+1] == '.' )
		  if ( i + 2 < len && PathName[i+2] != '\\' &&  PathName[i+2] != 0 )
				PathName[i+2] = 0;
  }


   if ( len > 0 && PathName[len-1] == L'\\' ) {
		PathName[len-1] = 0;
   }
   len = lstrlenW(PathName);
   for(i=3;i<len-2 && PathName[i] != 0;i++) {
		if ( PathName[i] == L'\\' && PathName[i+1] == L'.' && PathName[i+2] == L'.'  ) {
			for(j = i-1;j>=2 && PathName[j] != '\\';j-- ) {}
			PathName[j+1] = 0;
			if ( i+4 < len ) {
				PathName[i+3] = 0;
				lstrcatW(PathName,&PathName[i+4]);
			}
			len = lstrlenW(PathName);
		}
   }
			
   hCurrentDirectory = CreateFileW(PathName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_ATTRIBUTE_DIRECTORY,NULL);
   if ( hCurrentDirectory == - 1 ) {
		hCurrentDirectory = hDirOld;
		DPRINT("%d\n",GetLastError());
		return FALSE; 
   }
   else
		CloseHandle(hDirOld);

   
   lstrcpyW(CurrentDirectoryW,PathName);
   lstrcatW(CurrentDirectoryW,L"\\");

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


