/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <string.h>
#include <wstring.h>
#include <ddk/rtl.h>


WINBOOL
STDCALL
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	return CreateDirectoryExA(NULL,lpPathName,lpSecurityAttributes);
}

WINBOOL
STDCALL
CreateDirectoryExA(
    LPCSTR lpTemplateDirectory,
    LPCSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	WCHAR TemplateDirectoryW[MAX_PATH];
	WCHAR NewDirectoryW[MAX_PATH];
	ULONG i;
	i = 0;
   	while ((*lpTemplateDirectory)!=0 && i < MAX_PATH)
     	{
		TemplateDirectoryW[i] = *lpTemplateDirectory;
		lpTemplateDirectory++;
		i++;
     	}
   	TemplateDirectoryW[i] = 0;

	i = 0;
   	while ((*lpNewDirectory)!=0 && i < MAX_PATH)
     	{
		NewDirectoryW[i] = *lpNewDirectory;
		lpNewDirectory++;
		i++;
     	}
   	NewDirectoryW[i] = 0;
	return CreateDirectoryExW(TemplateDirectoryW,NewDirectoryW,lpSecurityAttributes);
}


WINBOOL
STDCALL
CreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	
	return CreateDirectoryExW(NULL,lpPathName,lpSecurityAttributes);	
}

WINBOOL
STDCALL
CreateDirectoryExW(
    LPCWSTR lpTemplateDirectory,
    LPCWSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	NTSTATUS errCode;
	HANDLE DirectoryHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING DirectoryNameString;

	if ( lpTemplateDirectory != NULL ) {
		// get object attributes from template directory
	}


	DirectoryNameString.Length = lstrlenW(lpNewDirectory)*sizeof(WCHAR);
	DirectoryNameString.Buffer = (WCHAR *)lpNewDirectory;
	DirectoryNameString.MaximumLength = DirectoryNameString.Length;
	
	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &DirectoryNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;


	errCode = NtCreateDirectoryObject(
    		&DirectoryHandle,
    		GENERIC_ALL,
    		&ObjectAttributes
    	);
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}



WINBOOL
STDCALL
RemoveDirectoryA(
    LPCSTR lpPathName
    )
{
	WCHAR PathNameW[MAX_PATH];
	ULONG i;
	i = 0;
   	while ((*lpPathName)!=0 && i < MAX_PATH)
     	{
		PathNameW[i] = *lpPathName;
		lpPathName++;
		i++;
     	}
   	PathNameW[i] = 0;	
	return RemoveDirectoryW(PathNameW);
}


WINBOOL
STDCALL
RemoveDirectoryW(
    LPCWSTR lpPathName
    )
{
	NTSTATUS errCode;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING PathNameString;

	PathNameString.Length = lstrlenW(lpPathName)*sizeof(WCHAR);
	PathNameString.Buffer = (WCHAR *)lpPathName;
	PathNameString.MaximumLength = PathNameString.Length;
	
	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &PathNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	errCode = NtDeleteFile(
		&ObjectAttributes
	);
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

DWORD
STDCALL
GetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )
{
	
}



DWORD
STDCALL
GetFullPathNameW(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )
{
}

DWORD
STDCALL
GetShortPathNameA(
		  LPCSTR lpszLongPath,
		  LPSTR  lpszShortPath,
		  DWORD    cchBuffer
		  )
{
	//1 remove unicode chars and spaces
	//2 remove preceding and trailing periods. 
	//3 remove embedded periods except the last one
	
	//4 Split the string in two parts before and after the period
	//	truncate the part before the period to 6 chars and add ~1
	//	truncate the part after the period to 3 chars
	//3 Put the new name in uppercase
	
	//4 Increment the ~1 string if the resulting name allready exists

}	

DWORD
STDCALL
GetShortPathNameW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD    cchBuffer
    )
{
	
}

DWORD
STDCALL
SearchPathA(
    LPCSTR lpPath,
    LPCSTR lpFileName,
    LPCSTR lpExtension,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )
{
}

DWORD 
STDCALL
SearchPathW(
    LPCWSTR lpPath,	 
    LPCWSTR lpFileName,	 
    LPCWSTR lpExtension, 
    DWORD nBufferLength, 
    LPWSTR lpBuffer,	
    LPWSTR *lpFilePart 	  
   )
{
	
 //1.	The directory from which the application loaded. 
 //2.	The current directory. 
 //3.	Windows NT: The 32-bit Windows system directory. Use the GetSystemDirectory function to get the path of this directory. The name of this directory is SYSTEM32.
 //4.	Windows NT only: The 16-bit Windows system directory. There is no Win32 function that obtains the path of this directory, but it is searched. The name of this directory is SYSTEM.
 //5.	The Windows directory. Use the GetWindowsDirectory function to get the path of this directory. 
 //6.	The directories that are listed in the PATH environment variable. 
	NTSTATUS errCode;
	DWORD retCode = 0;
	HANDLE FileHandle = NULL;

	ULONG i,j;

	WCHAR BufferW[MAX_PATH];
	WCHAR FileAndExtensionW[MAX_PATH];
	WCHAR *EnvironmentBufferW = NULL;
	
	UNICODE_STRING PathString;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;





	if ( lpPath == NULL )  {

		// check the directory from which the application loaded

		if ( GetCurrentDirectoryW( MAX_PATH, BufferW  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0 )
				return retCode;
		}
		if ( GetSystemDirectoryW( BufferW,	  MAX_PATH  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength,  lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0 )
				return retCode;
		}

		if ( GetWindowsDirectoryW( BufferW,	  MAX_PATH  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0  )
				return retCode;
		}
	
		j = GetEnvironmentVariableW(L"Path",EnvironmentBufferW,0);
		EnvironmentBufferW = (WCHAR *) HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,(j+1)*sizeof(WCHAR));

		j = GetEnvironmentVariableW(L"Path",EnvironmentBufferW,j+1);

		for(i=0;i<j;i++) {
			if ( EnvironmentBufferW[i] == L';' )
				EnvironmentBufferW[i] = 0;
		}
		i = 0;
		while ( retCode == 0  && i < j ) {
			if (  EnvironmentBufferW[i] != 0 )
				retCode = SearchPathW(&EnvironmentBufferW[i],lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			i += lstrlenW(&EnvironmentBufferW[i]) + 1;
			
			
		}
		
		
		HeapFree(GetProcessHeap(),0,EnvironmentBufferW);

		

		return retCode;

	}
	else {

		FileAndExtensionW[0] = 0;
		lpBuffer[0] = 0;
		i = lstrlenW(lpFileName);
		j = lstrlenW(lpPath);

		if ( i + j + 8 < nBufferLength )
			return i + j + 9;

		if ( lpExtension != NULL ) {
			if ( lpFileName[i-4] != L'.' ) {
				memcpy(FileAndExtensionW,lpFileName,(i+1)*sizeof(WCHAR));
				lstrcatW(FileAndExtensionW,lpExtension);
			}
			else
				memcpy(FileAndExtensionW,lpFileName,(i+1)*sizeof(WCHAR));
		}
		else
			memcpy(FileAndExtensionW,lpFileName,(i+1)*sizeof(WCHAR));

		
		
		if ( lpPath[0] == L'\\' ) 
			lstrcatW(lpBuffer,lpPath);
		else {
			lstrcatW(lpBuffer,L"\\??\\");
			lstrcatW(lpBuffer,lpPath);
		}
	
		//printf("%S\n",FileAndExtensionW);


		i = lstrlenW(lpBuffer);
		if ( lpBuffer[i-1] != L'\\' ) {
			lpBuffer[i] = L'\\';
			lpBuffer[i+1] = 0;
		}
		if ( lpFilePart != NULL )
			*lpFilePart = &lpBuffer[lstrlenW(lpBuffer)+1];
		lstrcatW(lpBuffer,FileAndExtensionW);
		//printf("%S\n",lpBuffer);

		PathString.Buffer = lpBuffer;
		PathString.Length = lstrlenW(PathString.Buffer)*sizeof(WCHAR);
   		PathString.MaximumLength = PathString.Length;
   
   
    	

  		ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
		ObjectAttributes.RootDirectory = NULL;
   		ObjectAttributes.ObjectName = &PathString;
		ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
		ObjectAttributes.SecurityDescriptor = NULL;
		ObjectAttributes.SecurityQualityOfService = NULL;

		errCode = NtOpenFile(
		&FileHandle,
		GENERIC_ALL|FILE_LIST_DIRECTORY,
		&ObjectAttributes,
		&IoStatusBlock,   
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,         
   		0		                                                                
		);

	

		if ( !NT_SUCCESS(errCode) ) {
			return 0;
		}


	}

	return lstrlenW(lpBuffer);

}