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
#include <ddk/ntddk.h>

typedef enum _FINDEX_INFO_LEVELS 
{ 
    FindExSearchNameMatch, 
    FindExSearchLimitToDirectories, 
    FindExSearchLimitToDevices, 

} FINDEX_INFO_LEVELS ; 

typedef enum _FINDEX_SEARCH_OPS 
{ 
    FindExInfoStandard 

} FINDEX_SEARCH_OPS; 

int wcharicmp ( WCHAR char1, WCHAR char2 );

WINBOOL
mfs_regexp(LPCWSTR lpFileName,LPCWSTR lpFilter);

HANDLE
STDCALL
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    );

WINBOOL
STDCALL
FindNextFileW(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindFileData
    );

HANDLE 
STDCALL
FindFirstFileExA( 
    LPCSTR lpFileName, 
    FINDEX_INFO_LEVELS fInfoLevelId, 
    LPVOID lpFindFileData, 
    FINDEX_SEARCH_OPS fSearchOp, 
    LPVOID lpSearchFilter, 
    DWORD dwAdditionalFlags 
    ); 
 
HANDLE 
STDCALL 
FindFirstFileExW( 
    LPCWSTR lpFileName, 
    FINDEX_INFO_LEVELS fInfoLevelId, 
    LPVOID lpFindFileData, 
    FINDEX_SEARCH_OPS fSearchOp, 
    LPVOID lpSearchFilter, 
    DWORD dwAdditionalFlags 
    );

typedef struct _FIND_FILE_INFO
{
  ULONG Offset;
  PVOID SearchFilter;
  WCHAR FileName[MAX_PATH];
  WCHAR PathName[MAX_PATH];
  FILE_DIRECTORY_INFORMATION *FileDirectory;
} FIND_FILE_INFO;

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
	WIN32_FIND_DATA FindFileDataW;
	WCHAR FileNameW[MAX_PATH];
	ULONG i;

	i = 0;
   	while ((*lpFileName)!=0 && i < MAX_PATH)
     	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
     	}
   	FileNameW[i] = 0;
	FindFirstFileW(FileNameW,&FindFileDataW);

	// converteer FindFileDataW 

}

HANDLE
STDCALL
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	return FindFirstFileExW(lpFileName,FindExInfoStandard,lpFindFileData,FindExSearchNameMatch,NULL,0);
}

WINBOOL
STDCALL
FindNextFileA(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	WIN32_FIND_DATA FindFileDataW;
	FindNextFileW(hFind,&FindFileDataW);
	// converteer FindFileDataW 
}

WINBOOL
STDCALL
FindNextFileW(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindFileData
    )
{
	int i;
	WCHAR *pNameRead;
  	WCHAR FileName[MAX_PATH];

    FIND_FILE_INFO *FindPtr = hFind;
	FILE_DIRECTORY_INFORMATION *FileDirectory;
 
	if ( FindPtr == NULL )
    		return FALSE;
  	
  	/* Try to find a file */
	FileDirectory = FindPtr->Offset + FindPtr->FileDirectory;
  	while ( FileDirectory->NextEntryOffset != 0 ) {
     
     		pNameRead = FileDirectory->FileName;
     		FindPtr->Offset += FileDirectory->NextEntryOffset;
			for(i=0;i<FileDirectory->FileNameLength;i++)
                                dprintf("%c\n",(char)pNameRead[i]);
     		if (mfs_regexp(pNameRead, FindPtr->FileName))
     		{ 
				/* We found one! */
				if (FindPtr->PathName[0] != L'\0')
				{
					lstrcpyW(lpFindFileData->cFileName, FindPtr->PathName);
					lstrcatW(lpFindFileData->cFileName, L"/");
					lstrcatW(lpFindFileData->cFileName, pNameRead);
				}
				else
        		
          			lstrcpyW(lpFindFileData->cFileName, pNameRead);

	
		
        		lstrcpyW(lpFindFileData->cAlternateFileName, L"");
				lpFindFileData->dwReserved0 = 0;
				lpFindFileData->dwReserved1 = 0;
				return TRUE;
      		}
			FileDirectory = FindPtr->Offset + FindPtr->FileDirectory;
    }
  	return FALSE;
}


HANDLE 
STDCALL
FindFirstFileExA( 
    LPCSTR lpFileName, 
    FINDEX_INFO_LEVELS fInfoLevelId, 
    LPVOID lpFindFileData, 
    FINDEX_SEARCH_OPS fSearchOp, 
    LPVOID lpSearchFilter, 
    DWORD dwAdditionalFlags 
    )
{
	WCHAR FileNameW[MAX_PATH];
	WIN32_FIND_DATAW FindFileDataW;
	FindFirstFileExW(FileNameW,fInfoLevelId,&FindFileDataW,fSearchOp,lpSearchFilter,dwAdditionalFlags);
	// conerteer FindFileDataW	

}
 
HANDLE 
STDCALL 
FindFirstFileExW( 
    LPCWSTR lpFileName, 
    FINDEX_INFO_LEVELS fInfoLevelId, 
    LPVOID lpFindFileData, 
    FINDEX_SEARCH_OPS fSearchOp, 
    LPVOID lpSearchFilter, 
    DWORD dwAdditionalFlags 
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
       		GetCurrentDirectory(MAX_PATH, hFind->PathName);
       		lstrcpyW(hFind->FileName, lpFileName);
    	}
    	else {
       		FilePart[0] = L'\0';
       		lstrcpyW(hFind->FileName, &FilePart[1]);
    	}

  	hFind->Offset = 0;
	

	PathNameString.Length = lstrlenW(hFind->PathName)*sizeof(WCHAR);
   	PathNameString.Buffer = hFind->PathName;
   	PathNameString.MaximumLength = FileNameString.Length;
   

    	FileNameString.Length = lstrlenW(hFind->FileName)*sizeof(WCHAR);
   	FileNameString.Buffer = hFind->FileName;
   	FileNameString.MaximumLength = FileNameString.Length;


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
                NULL,                           // AllocationSize
                FileAttributes,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                NULL,                           // EaBuffer
                0);                                     //

	if ( !NT_SUCCESS(errCode) ) {
		printf("%x\n",errCode);
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
		printf("%x\n",errCode);
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

/************************************************************************/
WINBOOL
mfs_regexp(LPCWSTR lpFileName,LPCWSTR lpFilter)
{
	/*   The following code is provided by Tarang and I trust him...
	*/
        LPWSTR                  lpTempFileName = (LPWSTR)lpFileName;
        LPWSTR                  lpTempFilter   = (LPWSTR)lpFilter;
        WCHAR                   TempToken [ 2 ];
	WCHAR			TempFilter [ 2 ];
        WINBOOL                 Matched = FALSE;

        if ( ( ! (LPWSTR)lpFileName ) || ( ! *(LPWSTR)lpFileName ) ||
             ( ! (LPWSTR)lpFilter ) || ( ! *(LPWSTR)lpFilter ) )
                return 0L;

	if ( ! lstrcmpW ( ( LPSTR )lpFilter, "*.*" ) )
	{
		wsprintf ( TempFilter, "*" );
		lpTempFilter = TempFilter;
		lpFilter     = TempFilter;
	}

        while ( ( lpTempFilter ) && ( *lpTempFilter ) && ( ! Matched ) )
        {
                memset ( TempToken, 0, sizeof ( TempToken ) );
                switch ( *lpTempFilter )
                {
                        default:
                                if ( wcharicmp ( *lpTempFileName, *lpTempFilter ) )
                                {
                                        lpTempFileName = (LPWSTR)lpFileName;
                                        lpTempFilter = wcspbrk ( lpTempFilter, L" ,;" );
                                        if ( lpTempFilter )
                                                lpTempFilter+=sizeof(WCHAR);
                                }
                                else
                                {
                                        lpTempFilter+=sizeof(WCHAR);
                                        lpTempFileName+=sizeof(WCHAR);
                                        switch ( *lpTempFilter )
                                        {
                                                default:
                                                        break;

                                                case L'\0':
                                                case L' ':
                                                case L',':
                                                case L';':
                                                        if ( ! *lpTempFileName )
                                                                Matched = TRUE;
                                                        break;
                                        }
                                }
                                break;

                        case L'?':
                                lpTempFilter+=sizeof(WCHAR);
                                lpTempFileName+=sizeof(WCHAR);
                                break;

                        case L'*':
								lpTempFilter += sizeof(WCHAR);
                                if ( ! ( TempToken [ 0 ] = *( lpTempFilter  ) ) )
                                        Matched = TRUE;
                                else
                                {
                                        lpTempFilter+=sizeof(WCHAR);
                                        while ( ( lpTempFileName = wcspbrk ( lpTempFileName, TempToken ) ) &&
                                                ( ! Matched ) ) {
												lpTempFileName+= sizeof(WCHAR);
                                                Matched = mfs_regexp ( lpTempFileName, lpTempFilter );
										}
                                        if ( ( ! lpTempFileName ) && ( ! Matched ) )
                                        {
                                                lpTempFileName = (LPWSTR)lpFileName;
                                                lpTempFilter = wcspbrk ( lpTempFilter, L" ,;" );
                                                if ( lpTempFilter )
                                                        lpTempFilter+=sizeof(WCHAR);
                                        }
                                }
                                break;

                        case L'\0':
                        case L' ':
                        case L',':
                        case L';':
                                Matched = TRUE;
                                break;
                }
        }

        return (DWORD)Matched;


}

int wcharicmp ( WCHAR char1, WCHAR char2 )
{
	WCHAR	Char1 = ( L'a' <= char1 ) && ( char1 <= L'z' ) ? 
	       		char1 - L'a' + L'A' : char1;
	WCHAR	Char2 = ( L'a' <= char2 ) && ( char2 <= L'z' ) ? 
	                char2 - L'a' + L'A' : char2;
	return ( Char2 - Char1 );
}
