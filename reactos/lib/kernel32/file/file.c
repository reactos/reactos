/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>

#define LPPROGRESS_ROUTINE void*


/* GLOBALS ******************************************************************/

WINBOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem


/* FUNCTIONS ****************************************************************/

VOID STDCALL SetFileApisToOEM(VOID)
{
   bIsFileApiAnsi = FALSE;
}


VOID STDCALL SetFileApisToANSI(VOID)
{
   bIsFileApiAnsi = TRUE;
}


WINBOOL STDCALL AreFileApisANSI(VOID)
{
   return (bIsFileApiAnsi);
}


HFILE STDCALL OpenFile(LPCSTR lpFileName,
		       LPOFSTRUCT lpReOpenBuff,
		       UINT uStyle)
{
   NTSTATUS errCode;
   HANDLE FileHandle = NULL;
   UNICODE_STRING FileNameString;
   WCHAR FileNameW[MAX_PATH];
   WCHAR PathNameW[MAX_PATH];
   ULONG i;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   WCHAR *FilePart;	
   ULONG Len;
   
   if (lpReOpenBuff == NULL) 
     {
	return FALSE;
     }
     
   i = 0;
   while ((*lpFileName)!=0 && i < MAX_PATH)
     {
	FileNameW[i] = *lpFileName;
	lpFileName++;
	i++;
     }
   FileNameW[i] = 0;

   Len = SearchPathW(NULL,FileNameW,NULL,MAX_PATH,PathNameW,&FilePart);
   if ( Len == 0 )
     return (HFILE)NULL;

   if ( Len > MAX_PATH )
     return (HFILE)NULL;
   
   FileNameString.Length = lstrlenW(PathNameW)*sizeof(WCHAR);
   FileNameString.Buffer = PathNameW;
   FileNameString.MaximumLength = FileNameString.Length+sizeof(WCHAR);
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &FileNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   // FILE_SHARE_READ
   // FILE_NO_INTERMEDIATE_BUFFERING
   
   if ((uStyle & OF_PARSE) == OF_PARSE ) 
     return (HFILE)NULL;
   
   errCode = NtOpenFile(&FileHandle,
			GENERIC_READ|SYNCHRONIZE,
			&ObjectAttributes,
			&IoStatusBlock,   
			FILE_SHARE_READ,         
			FILE_NON_DIRECTORY_FILE);
   
   lpReOpenBuff->nErrCode = RtlNtStatusToDosError(errCode);
   
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return (HFILE)INVALID_HANDLE_VALUE;
     }
   
   return (HFILE)FileHandle;
}


WINBOOL STDCALL FlushFileBuffers(HANDLE hFile)
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   
   errCode = NtFlushBuffersFile(hFile,
				&IoStatusBlock);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return(FALSE);
     }
   return(TRUE);
}


DWORD STDCALL SetFilePointer(HANDLE hFile,
			     LONG lDistanceToMove,
			     PLONG lpDistanceToMoveHigh,
			     DWORD dwMoveMethod)
{
   FILE_POSITION_INFORMATION FilePosition;
   FILE_END_OF_FILE_INFORMATION FileEndOfFile;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   LARGE_INTEGER Distance;
   
   DPRINT("SetFilePointer(hFile %x, lDistanceToMove %d, dwMoveMethod %d)\n",
	  hFile,lDistanceToMove,dwMoveMethod);

   Distance.u.LowPart = lDistanceToMove;
   Distance.u.HighPart = (lpDistanceToMoveHigh) ? *lpDistanceToMoveHigh : 0;

   if (dwMoveMethod == FILE_CURRENT) 
     {
	NtQueryInformationFile(hFile,
			       &IoStatusBlock,
			       &FilePosition, 
			       sizeof(FILE_POSITION_INFORMATION),
			       FilePositionInformation);
        FilePosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
     }
   else if (dwMoveMethod == FILE_END) 
     {
	NtQueryInformationFile(hFile,
                               &IoStatusBlock,
                               &FileEndOfFile,
                               sizeof(FILE_END_OF_FILE_INFORMATION),
                               FileEndOfFileInformation);
        FilePosition.CurrentByteOffset.QuadPart =
                FileEndOfFile.EndOfFile.QuadPart - Distance.QuadPart;
     }
   else if ( dwMoveMethod == FILE_BEGIN ) 
     {
        FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;
     }
   
   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FilePosition, 
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return -1;
     }
   
   if (lpDistanceToMoveHigh != NULL) 
     {
        *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
     }
   return FilePosition.CurrentByteOffset.u.LowPart;
}


DWORD STDCALL GetFileType(HANDLE hFile)
{
   return FILE_TYPE_UNKNOWN;
}


DWORD STDCALL GetFileSize(HANDLE hFile,	
			  LPDWORD lpFileSizeHigh)
{
   NTSTATUS errCode;
   FILE_STANDARD_INFORMATION FileStandard;
   IO_STATUS_BLOCK IoStatusBlock;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileStandard, 
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	if ( lpFileSizeHigh == NULL ) 
	  {
	     return -1;
	  }
	else
	  {
	     return 0;
	  }
     }
   if ( lpFileSizeHigh != NULL )
     *lpFileSizeHigh = FileStandard.AllocationSize.u.HighPart;

   return FileStandard.AllocationSize.u.LowPart;
}


DWORD STDCALL GetCompressedFileSizeA(LPCSTR lpFileName,
				     LPDWORD lpFileSizeHigh)
{
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
   
   return GetCompressedFileSizeW(FileNameW,lpFileSizeHigh);
}


DWORD STDCALL GetCompressedFileSizeW(LPCWSTR lpFileName,
				     LPDWORD lpFileSizeHigh)
{
   FILE_COMPRESSION_INFORMATION FileCompression;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   HANDLE hFile;
   
   hFile = CreateFileW(lpFileName,	
		       GENERIC_READ,	
		       FILE_SHARE_READ,	
		       NULL,	
		       OPEN_EXISTING,	
		       FILE_ATTRIBUTE_NORMAL,	
		       NULL);

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileCompression, 
				    sizeof(FILE_COMPRESSION_INFORMATION),
				    FileCompressionInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	CloseHandle(hFile);
	SetLastError(RtlNtStatusToDosError(errCode));
	return 0;
     }
   CloseHandle(hFile);
   return 0;
}


WINBOOL STDCALL GetFileInformationByHandle(HANDLE hFile,
			       LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   FILE_DIRECTORY_INFORMATION FileDirectory;
   FILE_INTERNAL_INFORMATION FileInternal;
   FILE_FS_VOLUME_INFORMATION FileFsVolume;
   FILE_STANDARD_INFORMATION FileStandard;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   
   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileDirectory, 
				    sizeof(FILE_DIRECTORY_INFORMATION),
				    FileDirectoryInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   lpFileInformation->dwFileAttributes = (DWORD)FileDirectory.FileAttributes;
   memcpy(&lpFileInformation->ftCreationTime,&FileDirectory.CreationTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastAccessTime,&FileDirectory.LastAccessTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastWriteTime, &FileDirectory.LastWriteTime,sizeof(LARGE_INTEGER)); 
   lpFileInformation->nFileSizeHigh = FileDirectory.AllocationSize.u.HighPart;
   lpFileInformation->nFileSizeLow = FileDirectory.AllocationSize.u.LowPart;
   
   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileInternal, 
				    sizeof(FILE_INTERNAL_INFORMATION),
				    FileInternalInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   lpFileInformation->nFileIndexHigh = FileInternal.IndexNumber.u.HighPart;
   lpFileInformation->nFileIndexLow = FileInternal.IndexNumber.u.LowPart;
   
   errCode = NtQueryVolumeInformationFile(hFile,
					  &IoStatusBlock,
					  &FileFsVolume, 
					  sizeof(FILE_FS_VOLUME_INFORMATION),
					  FileFsVolumeInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   lpFileInformation->dwVolumeSerialNumber = FileFsVolume.VolumeSerialNumber;
   
   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileStandard, 
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   lpFileInformation->nNumberOfLinks = FileStandard.NumberOfLinks;

   return TRUE;
}


DWORD STDCALL GetFileAttributesA(LPCSTR lpFileName)
{
   ULONG i;
   WCHAR FileNameW[MAX_PATH];
   
   i = 0;
   while ((*lpFileName)!=0 && i < MAX_PATH)
     {
	FileNameW[i] = *lpFileName;
	lpFileName++;
	i++;
     }
   FileNameW[i] = 0;
   return GetFileAttributesW(FileNameW);
}


DWORD STDCALL GetFileAttributesW(LPCWSTR lpFileName)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   HANDLE hFile;
   NTSTATUS errCode;

   hFile = CreateFileW(lpFileName,
		       FILE_READ_ATTRIBUTES,
		       FILE_SHARE_READ,
		       NULL,
		       OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);
   if (hFile == INVALID_HANDLE_VALUE)
     {
	return 0xFFFFFFFF;
     }

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileBasic,
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode))
     {
	CloseHandle(hFile);
	SetLastError(RtlNtStatusToDosError(errCode));
	return 0xFFFFFFFF;
     }
   CloseHandle(hFile);
   return (DWORD)FileBasic.FileAttributes;
}


WINBOOL STDCALL SetFileAttributesA(LPCSTR lpFileName,
				   DWORD dwFileAttributes)
{
	ULONG i;
	WCHAR FileNameW[MAX_PATH];
	i = 0;
	while ((*lpFileName)!=0 && i < MAX_PATH)
	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
	}
	FileNameW[i] = 0;
	return SetFileAttributesW(FileNameW, dwFileAttributes);
}


WINBOOL STDCALL SetFileAttributesW(LPCWSTR lpFileName,
				   DWORD dwFileAttributes)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   HANDLE hFile;
   NTSTATUS errCode;
   
   hFile = CreateFileW(lpFileName,
		       FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
		       FILE_SHARE_READ,	
		       NULL,	
		       OPEN_EXISTING,	
		       FILE_ATTRIBUTE_NORMAL,	
		       NULL);

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileBasic, 
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	CloseHandle(hFile);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   FileBasic.FileAttributes = dwFileAttributes;
   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FileBasic, 
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	CloseHandle(hFile);
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   CloseHandle(hFile);
   return TRUE;		
}


UINT STDCALL GetTempFileNameA(LPCSTR lpPathName,
			      LPCSTR lpPrefixString,
			      UINT uUnique,
			      LPSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
  
   if (lpPathName == NULL)
     return 0;
   
   if (uUnique == 0)
     uUnique = GetCurrentTime();
   /*
    * sprintf(lpTempFileName,"%s\\%c%.3s%4.4x%s",
    *	    lpPathName,'~',lpPrefixString,uUnique,".tmp");
    */
   if (unique)
     return uUnique;
   
   while ((hFile = CreateFileA(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
     {
	//  		wsprintfA(lpTempFileName,"%s\\%c%.3s%4.4x%s",
	//	  	lpPathName,'~',lpPrefixString,++uUnique,".tmp");
     }
   
   CloseHandle((HANDLE)hFile);
   return uUnique;
}


UINT STDCALL GetTempFileNameW(LPCWSTR lpPathName,
			      LPCWSTR lpPrefixString,
			      UINT uUnique,
			      LPWSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
   
   if (lpPathName == NULL)
     return 0;
   
   if (uUnique == 0)
     uUnique = GetCurrentTime();
   
   //	swprintf(lpTempFileName,L"%s\\%c%.3s%4.4x%s",
   //	  lpPathName,'~',lpPrefixString,uUnique,L".tmp");
   
   if (unique)
     return uUnique;
  
   while ((hFile = CreateFileW(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
     {
	//    		wsprintfW(lpTempFileName,L"%s\\%c%.3s%4.4x%s",
	//	  	lpPathName,'~',lpPrefixString,++uUnique,L".tmp");
     }
   
   CloseHandle((HANDLE)hFile);
   return uUnique;
}


WINBOOL STDCALL GetFileTime(HANDLE hFile,
			    LPFILETIME lpCreationTime,
			    LPFILETIME lpLastAccessTime,
			    LPFILETIME lpLastWriteTime)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   NTSTATUS errCode;
	
   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileBasic, 
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   memcpy(lpCreationTime,&FileBasic.CreationTime,sizeof(FILETIME));
   memcpy(lpLastAccessTime,&FileBasic.LastAccessTime,sizeof(FILETIME));
   memcpy(lpLastWriteTime,&FileBasic.LastWriteTime,sizeof(FILETIME));

   return TRUE;
}


WINBOOL STDCALL SetFileTime(HANDLE hFile,
			    CONST FILETIME *lpCreationTime,
			    CONST FILETIME *lpLastAccessTime,
			    CONST FILETIME *lpLastWriteTime)
{
   FILE_BASIC_INFORMATION FileBasic;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS errCode;
   
   memcpy(&FileBasic.CreationTime,lpCreationTime,sizeof(FILETIME));
   memcpy(&FileBasic.LastAccessTime,lpLastAccessTime,sizeof(FILETIME));
   memcpy(&FileBasic.LastWriteTime,lpLastWriteTime,sizeof(FILETIME));
   
   // shoud i initialize changetime ???
   
   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FileBasic, 
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   
   return TRUE;
}


WINBOOL STDCALL SetEndOfFile(HANDLE hFile)
{
   int x = -1;
   DWORD Num;
   return WriteFile(hFile,&x,1,&Num,NULL);
}
