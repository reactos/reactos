/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#include <windows.h>
#include <wstring.h>
#include <string.h>
#include <ddk/ntddk.h>








typedef struct _FIND_FILE_INFO
{
  ULONG Offset;
  WCHAR PathName[MAX_PATH];
  WCHAR FileName[MAX_PATH];
  FILE_DIRECTORY_INFORMATION *FileDirectory;
} FIND_FILE_INFO;

typedef struct _NOTIFY_INFO
{
  HANDLE Event;
  HANDLE FileHandle;
  DWORD dwNotifyFilter;
  WINBOOL bWatchSubtree;
} NOTIFY_INFO;




typedef struct _WIN32_FIND_DATAW { 
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  WCHAR    cFileName[ MAX_PATH ]; 
  WCHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATAW, *LPWIN32_FIND_DATAW, *PWIN32_FIND_DATAW; 


WINBOOL
STDCALL
FindClose(
	  HANDLE hFind
	  )
{
	
	if ( hFind == NULL)
    		return FALSE;
	if ( hFind == (HANDLE)-1)
    		return FALSE;
	
	HeapFree(GetProcessHeap(),0,((FIND_FILE_INFO *)hFind)->FileDirectory);
	HeapFree(GetProcessHeap(),0,hFind);
	return TRUE;
}



HANDLE
STDCALL
FindFirstFileA(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	WIN32_FIND_DATAW FindFileDataW;
	WCHAR FileNameW[MAX_PATH];
	ULONG i;
	HANDLE hFind;

	i = 0;
   	while ((*lpFileName)!=0 && i < MAX_PATH)
     	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
     	}
   	FileNameW[i] = 0;
	hFind = FindFirstFileW(FileNameW,(WIN32_FIND_DATA *)&FindFileDataW);

  	lpFindFileData->dwFileAttributes = FindFileDataW.dwFileAttributes; 
  	memcpy(&lpFindFileData->ftCreationTime,&FindFileDataW.ftCreationTime,sizeof(FILETIME)); 
	memcpy(&lpFindFileData->ftLastAccessTime,&FindFileDataW.ftLastAccessTime,sizeof(FILETIME)); 
	memcpy(&lpFindFileData->ftLastWriteTime,&FindFileDataW.ftLastWriteTime,sizeof(FILETIME)); 
	lpFindFileData->nFileSizeHigh = FindFileDataW.nFileSizeHigh; 
	lpFindFileData->nFileSizeLow = FindFileDataW.nFileSizeLow; 
	lpFindFileData->dwReserved0= FindFileDataW.dwReserved0; 
	lpFindFileData->dwReserved1= FindFileDataW.dwReserved1; 
	i = 0;
   	while ((lpFindFileData->cFileName[i])!=0 && i < MAX_PATH)
   	{
		lpFindFileData->cFileName[i] = (char)FindFileDataW.cFileName[i];
		i++;
   	}
   	lpFindFileData->cFileName[i] = 0;

	i = 0;
   	while ((lpFindFileData->cAlternateFileName[i])!=0 && i < 14)
   	{
		lpFindFileData->cAlternateFileName[i] = (char)FindFileDataW.cAlternateFileName[i];
		i++;
   	}
   	lpFindFileData->cAlternateFileName[i] = 0;
	
  
	return hFind;
}

HANDLE
STDCALL
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	NTSTATUS errCode;
	IO_STATUS_BLOCK IoStatusBlock; 
	HANDLE FileHandle = NULL;
	FIND_FILE_INFO *hFind;
	WCHAR *FilePart;
	UNICODE_STRING FileNameString, PathNameString;
	OBJECT_ATTRIBUTES ObjectAttributes;


	ACCESS_MASK DesiredAccess=FILE_READ_DATA;
      
     	ULONG FileAttributes=FILE_ATTRIBUTE_DIRECTORY;
     	ULONG ShareAccess=FILE_SHARE_READ | FILE_SHARE_WRITE |  FILE_SHARE_DELETE;
     	ULONG CreateDisposition=FILE_OPEN;
     	ULONG CreateOptions=FILE_DIRECTORY_FILE;


	hFind = HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,sizeof(FIND_FILE_INFO));

	hFind->FileDirectory = HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,8192);



	/* Try to find a path and a filename in the passed filename */

    	lstrcpyW(hFind->PathName, lpFileName);
    	FilePart = wcsrchr(hFind->PathName, '\\');
 
    	if (FilePart == NULL){
       		GetCurrentDirectoryW(MAX_PATH, hFind->PathName);
       		lstrcpyW(hFind->FileName, lpFileName);
    	}
    	else {
       		lstrcpyW(hFind->FileName, &FilePart[1]);
    	}

  	hFind->Offset = 0;
	

	PathNameString.Length = lstrlenW(hFind->PathName)*sizeof(WCHAR);
   	PathNameString.Buffer = hFind->PathName;
   	PathNameString.MaximumLength = FileNameString.Length+sizeof(WCHAR);
   

    	FileNameString.Length = lstrlenW(hFind->FileName)*sizeof(WCHAR);
   	FileNameString.Buffer = hFind->FileName;
   	FileNameString.MaximumLength = FileNameString.Length+sizeof(WCHAR);


  	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &PathNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	


 

	errCode = NtCreateFile(
                &FileHandle,
                DesiredAccess,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,                           
                FileAttributes,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                NULL,                          
                0);                                     //

	if ( !NT_SUCCESS(errCode) ) {
		
		return NULL;
	}

	errCode = NtQueryDirectoryFile(
		FileHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		hFind->FileDirectory,
		8192,
		FileDirectoryInformation,
		FALSE,
		&FileNameString,
		FALSE
	);
	if ( !NT_SUCCESS(errCode) ) {
	//	printf("%x\n",errCode);
		return NULL;
	}




	if ( FindNextFileW(hFind,lpFindFileData) )
		return hFind;
	else {
		FindClose(hFind);
		return NULL;
	}
	return NULL;
}

WINBOOL
STDCALL
FindNextFileA(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	WIN32_FIND_DATAW FindFileDataW;
	ULONG i;
	
	
	hFind = FindNextFileW(hFind,(WIN32_FIND_DATA *)&FindFileDataW);

 	lpFindFileData->dwFileAttributes = FindFileDataW.dwFileAttributes; 
  	memcpy(&lpFindFileData->ftCreationTime,&FindFileDataW.ftCreationTime,sizeof(FILETIME)); 
	memcpy(&lpFindFileData->ftLastAccessTime,&FindFileDataW.ftLastAccessTime,sizeof(FILETIME)); 
	memcpy(&lpFindFileData->ftLastWriteTime,&FindFileDataW.ftLastWriteTime,sizeof(FILETIME)); 
	lpFindFileData->nFileSizeHigh = FindFileDataW.nFileSizeHigh; 
	lpFindFileData->nFileSizeLow = FindFileDataW.nFileSizeLow; 
	lpFindFileData->dwReserved0= FindFileDataW.dwReserved0; 
	lpFindFileData->dwReserved1= FindFileDataW.dwReserved1; 
	i = 0;
   	while ((lpFindFileData->cFileName[i])!=0 && i < MAX_PATH)
   	{
		lpFindFileData->cFileName[i] = (char)FindFileDataW.cFileName[i];
		i++;
   	}
   	lpFindFileData->cFileName[i] = 0;

	i = 0;
   	while ((lpFindFileData->cAlternateFileName[i])!=0 && i < 14)
   	{
		lpFindFileData->cAlternateFileName[i] = (char)FindFileDataW.cAlternateFileName[i];
		i++;
   	}
   	lpFindFileData->cAlternateFileName[i] = 0;
	
  
	return hFind;
}

WINBOOL
STDCALL
FindNextFileW(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindFileData
    )
{


	LPWIN32_FIND_DATAW  lpFindFileDataW = (LPWIN32_FIND_DATAW)lpFindFileData; 	

	FIND_FILE_INFO *FindPtr = hFind;
	FILE_DIRECTORY_INFORMATION *FileDirectory=NULL;
 
	if ( FindPtr == NULL )
    		return FALSE;
	if ( FileDirectory->NextEntryOffset == 0 ) 
		return FALSE;	

  	/* Try to find a file */
	FileDirectory = FindPtr->Offset + FindPtr->FileDirectory;
 
	
     
     	
     	FindPtr->Offset += FileDirectory->NextEntryOffset;
	
				/* We found one! */
	if (FindPtr->PathName[0] != L'\0')
	{
		wcscpy(lpFindFileDataW->cFileName, FindPtr->PathName);
		wcscat(lpFindFileDataW->cFileName, L"\\");
		wcscat(lpFindFileDataW->cFileName, FileDirectory->FileName);
	}
	else
          	wcscpy(lpFindFileDataW->cFileName, FileDirectory->FileName);

	
		
        wcscpy(lpFindFileDataW->cAlternateFileName, L"");
	lpFindFileData->dwReserved0 = 0;
	lpFindFileData->dwReserved1 = 0;
	return TRUE;
  
}






HANDLE
STDCALL
FindFirstChangeNotificationA(
    LPCSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
	ULONG i;

	WCHAR PathNameW[MAX_PATH];
	

	

    	i = 0;
   	while ((*lpPathName)!=0 && i < MAX_PATH)
     	{
		PathNameW[i] = *lpPathName;
		lpPathName++;
		i++;
     	}
   	PathNameW[i] = 0;
	return FindFirstChangeNotificationW(PathNameW, bWatchSubtree, dwNotifyFilter );

}

HANDLE
STDCALL
FindFirstChangeNotificationW(
    LPCWSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
	NTSTATUS errCode;

	
	IO_STATUS_BLOCK IoStatusBlock;
	NOTIFY_INFO *NotifyHandle;
	WCHAR Buffer[100];
	ULONG BufferSize = 100;

	NotifyHandle = HeapAlloc(GetProcessHeap(),0,sizeof(NOTIFY_INFO));

	NotifyHandle->Event = CreateEventW(NULL,FALSE,FALSE,NULL);
	NotifyHandle->FileHandle = CreateFileW(lpPathName,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);

	NotifyHandle->dwNotifyFilter = dwNotifyFilter;
	NotifyHandle->bWatchSubtree = bWatchSubtree;

	errCode = NtNotifyChangeDirectoryFile(
		NotifyHandle->FileHandle,
		NotifyHandle->Event, 
		NULL, 
		NULL, 
		&IoStatusBlock,
		Buffer,
		BufferSize,
		NotifyHandle->dwNotifyFilter,
		NotifyHandle->bWatchSubtree
	);
	return NotifyHandle;
}

WINBOOL 
STDCALL
FindNextChangeNotification(
	HANDLE hChangeHandle 	
	)
{
		NTSTATUS errCode;

	
	IO_STATUS_BLOCK IoStatusBlock;
	NOTIFY_INFO *NotifyHandle = hChangeHandle;

	WCHAR Buffer[100];
	ULONG BufferSize = 100;
	EVENT_BASIC_INFORMATION EventBasic;
	ULONG ReturnLength;
	
	

	NtQueryEvent(NotifyHandle->Event,EventBasicInformation,&EventBasic,sizeof(EVENT_BASIC_INFORMATION),&ReturnLength);
	
	if ( EventBasic.Signaled == TRUE ) {
		ResetEvent(NotifyHandle->Event);
		return TRUE;
	}

	errCode = NtNotifyChangeDirectoryFile(
		NotifyHandle->FileHandle,
		NotifyHandle->Event, 
		NULL, 
		NULL, 
		&IoStatusBlock,
		Buffer,
		BufferSize,
		NotifyHandle->dwNotifyFilter,
		(BOOLEAN)NotifyHandle->bWatchSubtree
	);
		
	return FALSE;
}

WINBOOL
STDCALL
FindCloseChangeNotification(
			    HANDLE hChangeHandle
			    )
{
	HeapFree(GetProcessHeap(),0,hChangeHandle);
	return TRUE;

}















