/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Removed use of SearchPath (not used by Windows)
 *                  18/08/2002: CreateFileW mess cleaned up (KJK::Hyperion)
 *                  24/08/2002: removed superfluous DPRINTs (KJK::Hyperion)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

#if DBG
DEBUG_CHANNEL(kernel32file);
#endif

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE WINAPI CreateFileA (LPCSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   PWCHAR FileNameW;
   HANDLE FileHandle;

   TRACE("CreateFileA(lpFileName %s)\n",lpFileName);

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return INVALID_HANDLE_VALUE;

   FileHandle = CreateFileW (FileNameW,
			     dwDesiredAccess,
			     dwShareMode,
			     lpSecurityAttributes,
			     dwCreationDisposition,
			     dwFlagsAndAttributes,
			     hTemplateFile);

   return FileHandle;
}


/*
 * @implemented
 */
HANDLE WINAPI CreateFileW (LPCWSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING NtPathU;
   LPCWSTR pszConsoleFileName;
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG FileAttributes, Flags = 0;
   PVOID EaBuffer = NULL;
   ULONG EaLength = 0;
   BOOLEAN TrailingBackslash;

   if (!lpFileName || !lpFileName[0])
   {
       SetLastError( ERROR_PATH_NOT_FOUND );
       return INVALID_HANDLE_VALUE;
   }

   TRACE("CreateFileW(lpFileName %S)\n",lpFileName);

   /* validate & translate the creation disposition */
   switch (dwCreationDisposition)
     {
      case CREATE_NEW:
	dwCreationDisposition = FILE_CREATE;
	break;

      case CREATE_ALWAYS:
	dwCreationDisposition = FILE_OVERWRITE_IF;
	break;

      case OPEN_EXISTING:
	dwCreationDisposition = FILE_OPEN;
	break;

      case OPEN_ALWAYS:
	dwCreationDisposition = FILE_OPEN_IF;
	break;

      case TRUNCATE_EXISTING:
	dwCreationDisposition = FILE_OVERWRITE;
        break;

      default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return (INVALID_HANDLE_VALUE);
     }

   /* check for console input/output */
   pszConsoleFileName = IntCheckForConsoleFileName(lpFileName, dwDesiredAccess);
   if (pszConsoleFileName)
   {
      return OpenConsoleW(pszConsoleFileName,
                          dwDesiredAccess,
                          lpSecurityAttributes ? lpSecurityAttributes->bInheritHandle : FALSE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE);
   }

  /* validate & translate the flags */

   /* translate the flags that need no validation */
   if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
   {
      /* yes, nonalert is correct! apc's are not delivered
      while waiting for file io to complete */
      Flags |= FILE_SYNCHRONOUS_IO_NONALERT;
   }

   if(dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH)
      Flags |= FILE_WRITE_THROUGH;

   if(dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
      Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

   if(dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS)
      Flags |= FILE_RANDOM_ACCESS;

   if(dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN)
      Flags |= FILE_SEQUENTIAL_ONLY;

   if(dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
   {
      Flags |= FILE_DELETE_ON_CLOSE;
      dwDesiredAccess |= DELETE;
   }

   if(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
   {
      if(dwDesiredAccess & GENERIC_ALL)
         Flags |= FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REMOTE_INSTANCE;
      else
      {
         if(dwDesiredAccess & GENERIC_READ)
            Flags |= FILE_OPEN_FOR_BACKUP_INTENT;

         if(dwDesiredAccess & GENERIC_WRITE)
            Flags |= FILE_OPEN_REMOTE_INSTANCE;
      }
   }
   else
      Flags |= FILE_NON_DIRECTORY_FILE;

   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
      Flags |= FILE_OPEN_REPARSE_POINT;

   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
      Flags |= FILE_OPEN_NO_RECALL;

   FileAttributes = (dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY));

   /* handle may always be waited on and querying attributes are always allowed */
   dwDesiredAccess |= SYNCHRONIZE | FILE_READ_ATTRIBUTES;

   /* FILE_FLAG_POSIX_SEMANTICS is handled later */

   /* validate & translate the filename */
   if (!RtlDosPathNameToNtPathName_U (lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
   {
     WARN("Invalid path\n");
     SetLastError(ERROR_FILE_NOT_FOUND);
     return INVALID_HANDLE_VALUE;
   }

   TRACE("NtPathU \'%wZ\'\n", &NtPathU);

   TrailingBackslash = FALSE;
   if (NtPathU.Length >= sizeof(WCHAR) &&
       NtPathU.Buffer[NtPathU.Length / sizeof(WCHAR) - 1])
   {
      TrailingBackslash = TRUE;
   }

   if (hTemplateFile != NULL)
   {
      FILE_EA_INFORMATION EaInformation;

      for (;;)
      {
         /* try to get the size of the extended attributes, if we fail just continue
            creating the file without copying the attributes! */
         Status = NtQueryInformationFile(hTemplateFile,
                                         &IoStatusBlock,
                                         &EaInformation,
                                         sizeof(FILE_EA_INFORMATION),
                                         FileEaInformation);
         if (NT_SUCCESS(Status) && (EaInformation.EaSize != 0))
         {
            /* there's extended attributes to read, let's give it a try */
            EaBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       EaInformation.EaSize);
            if (EaBuffer == NULL)
            {
               RtlFreeHeap(RtlGetProcessHeap(),
                           0,
                           NtPathU.Buffer);

               /* the template file handle is valid and has extended attributes,
                  however we seem to lack some memory here. We should fail here! */
               SetLastError(ERROR_NOT_ENOUGH_MEMORY);
               return INVALID_HANDLE_VALUE;
            }

            Status = NtQueryEaFile(hTemplateFile,
                                   &IoStatusBlock,
                                   EaBuffer,
                                   EaInformation.EaSize,
                                   FALSE,
                                   NULL,
                                   0,
                                   NULL,
                                   TRUE);

            if (NT_SUCCESS(Status))
            {
               /* we successfully read the extended attributes, break the loop
                  and continue */
               EaLength = EaInformation.EaSize;
               break;
            }
            else
            {
               RtlFreeHeap(RtlGetProcessHeap(),
                           0,
                           EaBuffer);
               EaBuffer = NULL;

               if (Status != STATUS_BUFFER_TOO_SMALL)
               {
                  /* unless we just allocated not enough memory, break the loop
                     and just continue without copying extended attributes */
                  break;
               }
            }
         }
         else
         {
            /* we either failed to get the size of the extended attributes or
               they're empty, just continue as there's no need to copy
               attributes */
            break;
         }
      }
   }

   /* build the object attributes */
   InitializeObjectAttributes(&ObjectAttributes,
                              &NtPathU,
                              0,
                              NULL,
                              NULL);

   if (lpSecurityAttributes)
   {
      if(lpSecurityAttributes->bInheritHandle)
         ObjectAttributes.Attributes |= OBJ_INHERIT;

      ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
   }

   if(!(dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS))
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

   /* perform the call */
   Status = NtCreateFile (&FileHandle,
			  dwDesiredAccess,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
			  FileAttributes,
			  dwShareMode,
			  dwCreationDisposition,
			  Flags,
			  EaBuffer,
			  EaLength);

   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               NtPathU.Buffer);

   /* free the extended attributes buffer if allocated */
   if (EaBuffer != NULL)
   {
      RtlFreeHeap(RtlGetProcessHeap(),
                  0,
                  EaBuffer);
   }

   /* error */
   if (!NT_SUCCESS(Status))
   {
      /* In the case file creation was rejected due to CREATE_NEW flag
       * was specified and file with that name already exists, correct
       * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
       * Note: RtlNtStatusToDosError is not the subject to blame here.
       */
      if (Status == STATUS_OBJECT_NAME_COLLISION &&
          dwCreationDisposition == FILE_CREATE)
      {
         SetLastError( ERROR_FILE_EXISTS );
      }
      else if (Status == STATUS_FILE_IS_A_DIRECTORY &&
               TrailingBackslash)
      {
         SetLastError(ERROR_PATH_NOT_FOUND);
      }
      else
      {
         BaseSetLastNTError (Status);
      }

      return INVALID_HANDLE_VALUE;
   }

  /*
  create with OPEN_ALWAYS (FILE_OPEN_IF) returns info = FILE_OPENED or FILE_CREATED
  create with CREATE_ALWAYS (FILE_OVERWRITE_IF) returns info = FILE_OVERWRITTEN or FILE_CREATED
  */
  if (dwCreationDisposition == FILE_OPEN_IF)
  {
    SetLastError(IoStatusBlock.Information == FILE_OPENED ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
  }
  else if (dwCreationDisposition == FILE_OVERWRITE_IF)
  {
    SetLastError(IoStatusBlock.Information == FILE_OVERWRITTEN ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
  }
  else
  {
    SetLastError(ERROR_SUCCESS);
  }

  return FileHandle;
}

/*
 * @implemented
 */
HFILE WINAPI
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

	TRACE("OpenFile('%s', lpReOpenBuff %p, uStyle %x)\n", lpFileName, lpReOpenBuff, uStyle);

	if (lpReOpenBuff == NULL)
	{
		return HFILE_ERROR;
	}

    lpReOpenBuff->nErrCode = 0;

	if (uStyle & OF_REOPEN) lpFileName = lpReOpenBuff->szPathName;

	if (!lpFileName)
	{
		return HFILE_ERROR;
	}

	if (!GetFullPathNameA(lpFileName,
						  sizeof(lpReOpenBuff->szPathName),
						  lpReOpenBuff->szPathName,
						  NULL))
	{
	    lpReOpenBuff->nErrCode = (WORD)GetLastError();
		return HFILE_ERROR;
	}

    if (uStyle & OF_PARSE)
    {
        lpReOpenBuff->fFixedDisk = (GetDriveTypeA(lpReOpenBuff->szPathName) != DRIVE_REMOVABLE);
        TRACE("(%s): OF_PARSE, res = '%s'\n", lpFileName, lpReOpenBuff->szPathName);
        return 0;
    }

    if ((uStyle & OF_EXIST) && !(uStyle & OF_CREATE))
    {
        DWORD dwAttributes = GetFileAttributesA(lpReOpenBuff->szPathName);

        switch (dwAttributes)
        {
            case INVALID_FILE_ATTRIBUTES: /* File does not exist */
                SetLastError(ERROR_FILE_NOT_FOUND);
                lpReOpenBuff->nErrCode = (WORD) ERROR_FILE_NOT_FOUND;
                return -1;

            case FILE_ATTRIBUTE_DIRECTORY:
                SetLastError(ERROR_ACCESS_DENIED);
                lpReOpenBuff->nErrCode = (WORD) ERROR_ACCESS_DENIED;
                return -1;

            default:
                lpReOpenBuff->cBytes = sizeof(OFSTRUCT);
                return 1;
        }
    }
    lpReOpenBuff->cBytes = sizeof(OFSTRUCT);
	if ((uStyle & OF_CREATE) == OF_CREATE)
	{
		DWORD Sharing;
		switch (uStyle & 0x70)
		{
			case OF_SHARE_EXCLUSIVE: Sharing = 0; break;
			case OF_SHARE_DENY_WRITE: Sharing = FILE_SHARE_READ; break;
			case OF_SHARE_DENY_READ: Sharing = FILE_SHARE_WRITE; break;
			case OF_SHARE_DENY_NONE:
			case OF_SHARE_COMPAT:
			default:
				Sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		return (HFILE)(ULONG_PTR) CreateFileA (lpFileName,
		                            GENERIC_READ | GENERIC_WRITE,
		                            Sharing,
		                            NULL,
		                            CREATE_ALWAYS,
		                            FILE_ATTRIBUTE_NORMAL,
		                            0);
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
		lpReOpenBuff->nErrCode = (WORD)GetLastError();
		return HFILE_ERROR;
	}

    if (uStyle & OF_DELETE)
    {
        if (!DeleteFileW(PathNameW))
		{
			lpReOpenBuff->nErrCode = (WORD)GetLastError();
			return HFILE_ERROR;
		}
        TRACE("(%s): OF_DELETE return = OK\n", lpFileName);
        return TRUE;
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

	if (!RtlDosPathNameToNtPathName_U (PathNameW,
					   &FileNameString,
					   NULL,
					   NULL))
	{
		return HFILE_ERROR;
	}

	// FILE_SHARE_READ
	// FILE_NO_INTERMEDIATE_BUFFERING

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &FileNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	errCode = NtOpenFile (&FileHandle,
	                      GENERIC_READ | SYNCHRONIZE,
	                      &ObjectAttributes,
	                      &IoStatusBlock,
	                      FILE_SHARE_READ,
	                      FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	RtlFreeHeap(RtlGetProcessHeap(), 0, FileNameString.Buffer);

	lpReOpenBuff->nErrCode = (WORD)RtlNtStatusToDosError(errCode);

	if (!NT_SUCCESS(errCode))
	{
		BaseSetLastNTError (errCode);
		return HFILE_ERROR;
	}

	if (uStyle & OF_EXIST)
	{
		NtClose(FileHandle);
		return (HFILE)1;
	}

	return (HFILE)(ULONG_PTR)FileHandle;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
OpenDataFile(HANDLE hFile, DWORD dwUnused)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
ReOpenFile(IN HANDLE hOriginalFile,
           IN DWORD dwDesiredAccess,
           IN DWORD dwShareMode,
           IN DWORD dwFlags)
{
   STUB;
   return INVALID_HANDLE_VALUE;
}

/* EOF */
