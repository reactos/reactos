/* $Id: file.c,v 1.34 2002/06/07 16:54:04 hbirr Exp $
 *
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
#include <kernel32/error.h>

#define LPPROGRESS_ROUTINE void*


/* GLOBALS ******************************************************************/

WINBOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem


/* FUNCTIONS ****************************************************************/

VOID STDCALL
SetFileApisToOEM(VOID)
{
   bIsFileApiAnsi = FALSE;
}


VOID STDCALL
SetFileApisToANSI(VOID)
{
   bIsFileApiAnsi = TRUE;
}


WINBOOL STDCALL
AreFileApisANSI(VOID)
{
   return bIsFileApiAnsi;
}


HFILE STDCALL
OpenFile(LPCSTR lpFileName,
	 LPOFSTRUCT lpReOpenBuff,
	 UINT uStyle)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING FileNameString;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	WCHAR PathNameW[MAX_PATH];
	HANDLE FileHandle = NULL;
	NTSTATUS errCode;
	PWCHAR FilePart;
	ULONG Len;

	DPRINT("OpenFile('%s', lpReOpenBuff %x, uStyle %x)\n", lpFileName, lpReOpenBuff, uStyle);

	if (lpReOpenBuff == NULL)
	{
		return FALSE;
	}

	RtlInitAnsiString (&FileName, (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU, &FileName, TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU, &FileName, TRUE);

	Len = SearchPathW (NULL,
	                   FileNameU.Buffer,
	                   NULL,
	                   OFS_MAXPATHNAME,
	                   PathNameW,
	                   &FilePart);

	RtlFreeUnicodeString(&FileNameU);

	if (Len == 0 || Len > OFS_MAXPATHNAME)
	{
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	FileName.Buffer = lpReOpenBuff->szPathName;
	FileName.Length = 0;
	FileName.MaximumLength = OFS_MAXPATHNAME;

	RtlInitUnicodeString(&FileNameU, PathNameW);

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName, &FileNameU, FALSE);
	else
		RtlUnicodeStringToOemString (&FileName, &FileNameU, FALSE);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)PathNameW,
					   &FileNameString,
					   NULL,
					   NULL))
	{
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &FileNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	// FILE_SHARE_READ
	// FILE_NO_INTERMEDIATE_BUFFERING

	if ((uStyle & OF_PARSE) == OF_PARSE)
	{
		RtlFreeUnicodeString(&FileNameString);
		return (HFILE)NULL;
	}

	errCode = NtOpenFile (&FileHandle,
	                      GENERIC_READ|SYNCHRONIZE,
	                      &ObjectAttributes,
	                      &IoStatusBlock,
	                      FILE_SHARE_READ,
	                      FILE_NON_DIRECTORY_FILE);

	RtlFreeUnicodeString(&FileNameString);

	lpReOpenBuff->nErrCode = RtlNtStatusToDosError(errCode);

	if (!NT_SUCCESS(errCode))
	{
		SetLastErrorByStatus (errCode);
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	return (HFILE)FileHandle;
}


WINBOOL STDCALL
FlushFileBuffers(HANDLE hFile)
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;

   if (IsConsoleHandle(hFile))
   {
      return FALSE;
   }

   errCode = NtFlushBuffersFile(hFile,
				&IoStatusBlock);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return(FALSE);
     }
   return(TRUE);
}


DWORD STDCALL
SetFilePointer(HANDLE hFile,
	       LONG lDistanceToMove,
	       PLONG lpDistanceToMoveHigh,
	       DWORD dwMoveMethod)
{
   FILE_POSITION_INFORMATION FilePosition;
   FILE_STANDARD_INFORMATION FileStandart;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   LARGE_INTEGER Distance;
   
   DPRINT("SetFilePointer(hFile %x, lDistanceToMove %d, dwMoveMethod %d)\n",
	  hFile,lDistanceToMove,dwMoveMethod);

   Distance.u.LowPart = lDistanceToMove;
   if (lpDistanceToMoveHigh)
   {
      Distance.u.HighPart = *lpDistanceToMoveHigh;
   }
   else if (lDistanceToMove >= 0)
   {
      Distance.u.HighPart = 0;
   }
   else
   {
      Distance.u.HighPart = -1;
   }

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
                               &FileStandart,
                               sizeof(FILE_STANDARD_INFORMATION),
                               FileStandardInformation);
        FilePosition.CurrentByteOffset.QuadPart =
                  FileStandart.EndOfFile.QuadPart + Distance.QuadPart;
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
	SetLastErrorByStatus(errCode);
	return -1;
     }
   
   if (lpDistanceToMoveHigh != NULL)
     {
        *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
     }
   return FilePosition.CurrentByteOffset.u.LowPart;
}


DWORD STDCALL
GetFileType(HANDLE hFile)
{
   FILE_FS_DEVICE_INFORMATION DeviceInfo;
   IO_STATUS_BLOCK StatusBlock;
   NTSTATUS Status;

   /* get real handle */
   switch ((ULONG)hFile)
     {
	case STD_INPUT_HANDLE:
	  hFile = NtCurrentPeb()->ProcessParameters->InputHandle;
	  break;

	case STD_OUTPUT_HANDLE:
	  hFile = NtCurrentPeb()->ProcessParameters->OutputHandle;
	  break;

	case STD_ERROR_HANDLE:
	  hFile = NtCurrentPeb()->ProcessParameters->ErrorHandle;
	  break;
     }

   /* check console handles */
   if (IsConsoleHandle(hFile))
     {
//	if (VerifyConsoleHandle(hFile))
	  return FILE_TYPE_CHAR;
     }

   Status = NtQueryVolumeInformationFile(hFile,
					 &StatusBlock,
					 &DeviceInfo,
					 sizeof(FILE_FS_DEVICE_INFORMATION),
					 FileFsDeviceInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FILE_TYPE_UNKNOWN;
     }

   switch (DeviceInfo.DeviceType)
     {
	case FILE_DEVICE_CD_ROM:
	case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
	case FILE_DEVICE_CONTROLLER:
	case FILE_DEVICE_DATALINK:
	case FILE_DEVICE_DFS:
	case FILE_DEVICE_DISK:
	case FILE_DEVICE_DISK_FILE_SYSTEM:
	case FILE_DEVICE_VIRTUAL_DISK:
	  return FILE_TYPE_DISK;

	case FILE_DEVICE_KEYBOARD:
	case FILE_DEVICE_MOUSE:
	case FILE_DEVICE_NULL:
	case FILE_DEVICE_PARALLEL_PORT:
	case FILE_DEVICE_PRINTER:
	case FILE_DEVICE_SERIAL_PORT:
	case FILE_DEVICE_SCREEN:
	case FILE_DEVICE_SOUND:
	case FILE_DEVICE_MODEM:
	  return FILE_TYPE_CHAR;

	case FILE_DEVICE_NAMED_PIPE:
	  return FILE_TYPE_PIPE;
     }

   return FILE_TYPE_UNKNOWN;
}


DWORD STDCALL
GetFileSize(HANDLE hFile,
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
	SetLastErrorByStatus(errCode);
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
     *lpFileSizeHigh = FileStandard.EndOfFile.u.HighPart;

   return FileStandard.EndOfFile.u.LowPart;
}


DWORD STDCALL
GetCompressedFileSizeA(LPCSTR lpFileName,
		       LPDWORD lpFileSizeHigh)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   DWORD Size;

   RtlInitAnsiString(&FileName,
		     (LPSTR)lpFileName);

   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString(&FileNameU,
				  &FileName,
				  TRUE);
   else
     RtlOemStringToUnicodeString(&FileNameU,
				 &FileName,
				 TRUE);

   Size = GetCompressedFileSizeW(FileNameU.Buffer,
				 lpFileSizeHigh);

   RtlFreeUnicodeString (&FileNameU);

   return Size;
}


DWORD STDCALL
GetCompressedFileSizeW(LPCWSTR lpFileName,
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
	SetLastErrorByStatus(errCode);
	return 0;
     }
   CloseHandle(hFile);
   return 0;
}


WINBOOL STDCALL
GetFileInformationByHandle(HANDLE hFile,
			   LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   struct
   {
        FILE_FS_VOLUME_INFORMATION FileFsVolume;
        WCHAR Name[255];
   }
   FileFsVolume;

   FILE_BASIC_INFORMATION FileBasic;
   FILE_INTERNAL_INFORMATION FileInternal;
   FILE_STANDARD_INFORMATION FileStandard;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileBasic,
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->dwFileAttributes = (DWORD)FileBasic.FileAttributes;
   memcpy(&lpFileInformation->ftCreationTime,&FileBasic.CreationTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastAccessTime,&FileBasic.LastAccessTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastWriteTime, &FileBasic.LastWriteTime,sizeof(LARGE_INTEGER));

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileInternal,
				    sizeof(FILE_INTERNAL_INFORMATION),
				    FileInternalInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->nFileIndexHigh = FileInternal.IndexNumber.u.HighPart;
   lpFileInformation->nFileIndexLow = FileInternal.IndexNumber.u.LowPart;

   errCode = NtQueryVolumeInformationFile(hFile,
					  &IoStatusBlock,
					  &FileFsVolume,
					  sizeof(FileFsVolume),
					  FileFsVolumeInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->dwVolumeSerialNumber = FileFsVolume.FileFsVolume.VolumeSerialNumber;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileStandard,
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->nNumberOfLinks = FileStandard.NumberOfLinks;
   lpFileInformation->nFileSizeHigh = FileStandard.EndOfFile.u.HighPart;
   lpFileInformation->nFileSizeLow = FileStandard.EndOfFile.u.LowPart;

   return TRUE;
}


DWORD STDCALL
GetFileAttributesA(LPCSTR lpFileName)
{
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	WINBOOL Result;

	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);

	Result = GetFileAttributesW (FileNameU.Buffer);

	RtlFreeUnicodeString (&FileNameU);

	return Result;
}


DWORD STDCALL
GetFileAttributesW(LPCWSTR lpFileName)
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
	SetLastErrorByStatus(errCode);
	return 0xFFFFFFFF;
     }
   CloseHandle(hFile);
   return (DWORD)FileBasic.FileAttributes;
}


WINBOOL STDCALL
SetFileAttributesA(LPCSTR lpFileName,
		   DWORD dwFileAttributes)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   WINBOOL Result;

   RtlInitAnsiString(&FileName,
		     (LPSTR)lpFileName);

   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString(&FileNameU,
				  &FileName,
				  TRUE);
   else
     RtlOemStringToUnicodeString(&FileNameU,
				 &FileName,
				 TRUE);

   Result = SetFileAttributesW(FileNameU.Buffer,
			       dwFileAttributes);

   RtlFreeUnicodeString(&FileNameU);

   return Result;
}


WINBOOL STDCALL
SetFileAttributesW(LPCWSTR lpFileName,
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
	SetLastErrorByStatus(errCode);
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
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   CloseHandle(hFile);
   return TRUE;
}


UINT STDCALL
GetTempFileNameA(LPCSTR lpPathName,
		 LPCSTR lpPrefixString,
		 UINT uUnique,
		 LPSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
   UINT len;
   const char *format = "%.*s\\~%.3s%4.4x.TMP";

   DPRINT("GetTempFileNameA(lpPathName %s, lpPrefixString %.*s, "
	  "uUnique %x, lpTempFileName %x)\n", lpPathName, 4, 
	  lpPrefixString, uUnique, lpTempFileName);
  
   if (lpPathName == NULL)
     return 0;

   len = strlen(lpPathName);
   if (len > 0 && (lpPathName[len-1] == '\\' || lpPathName[len-1] == '/'))
     len--;

   if (uUnique == 0)
     uUnique = GetCurrentTime();
   
   sprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,uUnique);
   
   if (unique)
     return uUnique;
   
   while ((hFile = CreateFileA(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return 0;
      }
      sprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,++uUnique);
   }
   CloseHandle(hFile);
   return uUnique;
}


UINT STDCALL
GetTempFileNameW(LPCWSTR lpPathName,
		 LPCWSTR lpPrefixString,
		 UINT uUnique,
		 LPWSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
   UINT len;
   const WCHAR *format = L"%.*S\\~%.3S%4.4x.TMP";
   
   DPRINT("GetTempFileNameW(lpPathName %S, lpPrefixString %.*S, "
	  "uUnique %x, lpTempFileName %x)\n", lpPathName, 4, 
	  lpPrefixString, uUnique, lpTempFileName);

   if (lpPathName == NULL)
     return 0;

   len = wcslen(lpPathName);
   if (len > 0 && (lpPathName[len-1] == L'\\' || lpPathName[len-1] == L'/'))
     len--;
   
   if (uUnique == 0)
     uUnique = GetCurrentTime();
   
   swprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,uUnique);
   
   if (unique)
     return uUnique;
  
   while ((hFile = CreateFileW(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return 0;
      }
      swprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,++uUnique);
   }
   CloseHandle(hFile);
   return uUnique;
}


WINBOOL STDCALL
GetFileTime(HANDLE hFile,
	    LPFILETIME lpCreationTime,
	    LPFILETIME lpLastAccessTime,
	    LPFILETIME lpLastWriteTime)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hFile,
				   &IoStatusBlock,
				   &FileBasic,
				   sizeof(FILE_BASIC_INFORMATION),
				   FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   if (lpCreationTime)
     memcpy(lpCreationTime, &FileBasic.CreationTime, sizeof(FILETIME));
   if (lpLastAccessTime)
     memcpy(lpLastAccessTime, &FileBasic.LastAccessTime, sizeof(FILETIME));
   if (lpLastWriteTime)
     memcpy(lpLastWriteTime, &FileBasic.LastWriteTime, sizeof(FILETIME));

   return TRUE;
}


WINBOOL STDCALL
SetFileTime(HANDLE hFile,
	    CONST FILETIME *lpCreationTime,
	    CONST FILETIME *lpLastAccessTime,
	    CONST FILETIME *lpLastWriteTime)
{
   FILE_BASIC_INFORMATION FileBasic;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hFile,
				   &IoStatusBlock,
				   &FileBasic,
				   sizeof(FILE_BASIC_INFORMATION),
				   FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   if (lpCreationTime)
     memcpy(&FileBasic.CreationTime, lpCreationTime, sizeof(FILETIME));
   if (lpLastAccessTime)
     memcpy(&FileBasic.LastAccessTime, lpLastAccessTime, sizeof(FILETIME));
   if (lpLastWriteTime)
     memcpy(&FileBasic.LastWriteTime, lpLastWriteTime, sizeof(FILETIME));

   // should i initialize changetime ???

   Status = NtSetInformationFile(hFile,
				 &IoStatusBlock,
				 &FileBasic,
				 sizeof(FILE_BASIC_INFORMATION),
				 FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   
   return TRUE;
}


WINBOOL STDCALL
SetEndOfFile(HANDLE hFile)
{
   int x = -1;
   DWORD Num;
   return WriteFile(hFile,&x,1,&Num,NULL);
}

/* EOF */
