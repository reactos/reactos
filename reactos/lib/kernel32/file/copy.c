/* $Id: copy.c,v 1.12 2002/12/27 23:50:21 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/copy.c
 * PURPOSE:         Copying files
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  01/11/98 Created
 *                  07/02/99 Moved to seperate file 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* FUNCTIONS ****************************************************************/


static NTSTATUS
CopyLoop (
	HANDLE			FileHandleSource,
	HANDLE			FileHandleDest,
	LARGE_INTEGER		SourceFileSize,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	WINBOOL			*pbCancel,
	WINBOOL                 *KeepDest
	)
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   UCHAR *lpBuffer = NULL;
   ULONG RegionSize = 0x10000;
   LARGE_INTEGER BytesCopied;
   DWORD CallbackReason;
   DWORD ProgressResult;
   WINBOOL EndOfFileFound;

   *KeepDest = FALSE;
   errCode = NtAllocateVirtualMemory(NtCurrentProcess(),
				     (PVOID *)&lpBuffer,
				     2,
				     &RegionSize,
				     MEM_RESERVE | MEM_COMMIT,
				     PAGE_READWRITE);

   if (NT_SUCCESS(errCode))
     {
	BytesCopied.QuadPart = 0;
	EndOfFileFound = FALSE;
	CallbackReason = CALLBACK_STREAM_SWITCH; 
	while (! EndOfFileFound &&
	       NT_SUCCESS(errCode) &&
	       (NULL == pbCancel || ! *pbCancel))
	  {
	     if (NULL != lpProgressRoutine)
	       {
		   ProgressResult = (*lpProgressRoutine)(SourceFileSize,
							 BytesCopied,
							 SourceFileSize,
							 BytesCopied,
							 0,
							 CallbackReason,
							 FileHandleSource,
							 FileHandleDest,
							 lpData);
		   switch (ProgressResult)
		     {
		     case PROGRESS_CANCEL:
			DPRINT("Progress callback requested cancel\n");
			errCode = STATUS_REQUEST_ABORTED;
			break;
		     case PROGRESS_STOP:
			DPRINT("Progress callback requested stop\n");
			errCode = STATUS_REQUEST_ABORTED;
			*KeepDest = TRUE;
			break;
		     case PROGRESS_QUIET:
			lpProgressRoutine = NULL;
			break;
		     case PROGRESS_CONTINUE:
		     default:
			break;
		     }
		   CallbackReason = CALLBACK_CHUNK_FINISHED;
	       }
	     if (NT_SUCCESS(errCode))
	       {
		  errCode = NtReadFile(FileHandleSource,
				       NULL,
				       NULL,
				       NULL,
				       (PIO_STATUS_BLOCK)&IoStatusBlock,
				       lpBuffer,
				       RegionSize,
				       NULL,
				       NULL);
		  if (NT_SUCCESS(errCode) && (NULL == pbCancel || ! *pbCancel))
		    {
		       errCode = NtWriteFile(FileHandleDest,
					     NULL,
					     NULL,
					     NULL,
					     (PIO_STATUS_BLOCK)&IoStatusBlock,
					     lpBuffer,
					     IoStatusBlock.Information,
					     NULL,
					     NULL);
		       if (NT_SUCCESS(errCode))
			 {
			    BytesCopied.QuadPart += IoStatusBlock.Information;
			 }
		       else
			 {
			    DPRINT("Error 0x%08x reading writing to dest\n", errCode);
			 }
		    }
		  else if (!NT_SUCCESS(errCode))
		    {
		       if (STATUS_END_OF_FILE == errCode)
			 {
			    EndOfFileFound = TRUE;
			    errCode = STATUS_SUCCESS;
			 }
		       else
			 {
			    DPRINT("Error 0x%08x reading from source\n", errCode);
			 }
		    }
	       }
	  }

	if (! EndOfFileFound && (NULL != pbCancel && *pbCancel))
	  {
	  DPRINT("User requested cancel\n");
	  errCode = STATUS_REQUEST_ABORTED;
	  }

	NtFreeVirtualMemory(NtCurrentProcess(),
			    (PVOID *)&lpBuffer,
			    &RegionSize,
			    MEM_RELEASE);
     }
   else
     {
	DPRINT("Error 0x%08x allocating buffer of %d bytes\n", errCode, RegionSize);
     }

   return errCode;
}

static NTSTATUS
SetLastWriteTime(
	HANDLE FileHandle,
	TIME LastWriteTime
	)
{
   NTSTATUS errCode = STATUS_SUCCESS;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;

   errCode = NtQueryInformationFile (FileHandle,
				     &IoStatusBlock,
				     &FileBasic,
				     sizeof(FILE_BASIC_INFORMATION),
				     FileBasicInformation);
   if (!NT_SUCCESS(errCode))
     {
	DPRINT("Error 0x%08x obtaining FileBasicInformation\n", errCode);
     }
   else
     {
	FileBasic.LastWriteTime = LastWriteTime;
	errCode = NtSetInformationFile (FileHandle,
					&IoStatusBlock,
					&FileBasic,
					sizeof(FILE_BASIC_INFORMATION),
					FileBasicInformation);
	if (!NT_SUCCESS(errCode))
	  {
	     DPRINT("Error 0x%0x setting LastWriteTime\n", errCode);
	  }
     }

   return errCode;
}

WINBOOL
STDCALL
CopyFileExW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	WINBOOL			*pbCancel,
	DWORD			dwCopyFlags
	)
{
   NTSTATUS errCode;
   HANDLE FileHandleSource, FileHandleDest;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_STANDARD_INFORMATION FileStandard;
   FILE_BASIC_INFORMATION FileBasic;
   FILE_DISPOSITION_INFORMATION FileDispInfo;
   WINBOOL RC = FALSE;
   WINBOOL KeepDestOnError = FALSE;
   DWORD SystemError;

   FileHandleSource = CreateFileW(lpExistingFileName,
				  GENERIC_READ,
				  FILE_SHARE_READ,
				  NULL,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
				  NULL);
   if (INVALID_HANDLE_VALUE != FileHandleSource)
     {
	errCode = NtQueryInformationFile(FileHandleSource,
					 &IoStatusBlock,
					 &FileStandard,
					 sizeof(FILE_STANDARD_INFORMATION),
					 FileStandardInformation);
	if (!NT_SUCCESS(errCode))
	  {
	     DPRINT("Status 0x%08x obtaining FileStandardInformation for source\n", errCode);
	     SetLastErrorByStatus(errCode);
	  }
	else
	  {
	     errCode = NtQueryInformationFile(FileHandleSource,
	  				      &IoStatusBlock,&FileBasic,
					      sizeof(FILE_BASIC_INFORMATION),
					      FileBasicInformation);
	     if (!NT_SUCCESS(errCode))
	       {
		  DPRINT("Status 0x%08x obtaining FileBasicInformation for source\n", errCode);
		  SetLastErrorByStatus(errCode);
	       }
	     else
	       {
		  FileHandleDest = CreateFileW(lpNewFileName,
					       GENERIC_WRITE,
					       FILE_SHARE_WRITE,
					       NULL,
					       dwCopyFlags ? CREATE_NEW : CREATE_ALWAYS,
                                               FileBasic.FileAttributes,
					       NULL);
		  if (INVALID_HANDLE_VALUE != FileHandleDest)
		    {
		       errCode = CopyLoop(FileHandleSource,
					  FileHandleDest,
					  FileStandard.EndOfFile,
					  lpProgressRoutine,
					  lpData,
					  pbCancel,
					  &KeepDestOnError);
		       if (!NT_SUCCESS(errCode))
			 {
			    SetLastErrorByStatus(errCode);
			 }
		       else
			 {
			    errCode = SetLastWriteTime(FileHandleDest,
						       FileBasic.LastWriteTime);
			    if (!NT_SUCCESS(errCode))
			      {
				 SetLastErrorByStatus(errCode);
			      }
			    else
			      {
				 RC = TRUE;
			      }
			 }
		       NtClose(FileHandleDest);
		       if (! RC && ! KeepDestOnError)
			 {
			    SystemError = GetLastError();
			    SetFileAttributesW(lpNewFileName, FILE_ATTRIBUTE_NORMAL);
			    DeleteFileW(lpNewFileName);
			    SetLastError(SystemError);
			 }
		    }
		  else
		    {
		    DPRINT("Error %d during opening of dest file\n", GetLastError());
		    }
	       }
	  }
	NtClose(FileHandleSource);
     }
   else
     {
     DPRINT("Error %d during opening of source file\n", GetLastError());
     }

   return RC;
}


WINBOOL
STDCALL
CopyFileExA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	WINBOOL			*pbCancel,
	DWORD			dwCopyFlags
	)
{
	UNICODE_STRING ExistingFileNameU;
	UNICODE_STRING NewFileNameU;
	ANSI_STRING ExistingFileName;
	ANSI_STRING NewFileName;
	WINBOOL Result;

	RtlInitAnsiString (&ExistingFileName,
	                   (LPSTR)lpExistingFileName);

	RtlInitAnsiString (&NewFileName,
	                   (LPSTR)lpNewFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&ExistingFileNameU,
		                              &ExistingFileName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&NewFileNameU,
		                              &NewFileName,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&ExistingFileNameU,
		                             &ExistingFileName,
		                             TRUE);
		RtlOemStringToUnicodeString (&NewFileNameU,
		                             &NewFileName,
		                             TRUE);
	}

	Result = CopyFileExW (ExistingFileNameU.Buffer,
	                      NewFileNameU.Buffer,
	                      lpProgressRoutine,
	                      lpData,
	                      pbCancel,
	                      dwCopyFlags);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             ExistingFileNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             NewFileNameU.Buffer);

	return Result;
}


WINBOOL
STDCALL
CopyFileA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName,
	WINBOOL	bFailIfExists
	)
{
	return CopyFileExA (lpExistingFileName,
	                    lpNewFileName,
	                    NULL,
	                    NULL,
	                    NULL,
	                    bFailIfExists);
}


WINBOOL
STDCALL
CopyFileW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName,
	WINBOOL	bFailIfExists
	)
{
	return CopyFileExW (lpExistingFileName,
	                    lpNewFileName,
	                    NULL,
	                    NULL,
	                    NULL,
	                    bFailIfExists);
}

/* EOF */
