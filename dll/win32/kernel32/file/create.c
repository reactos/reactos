/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
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
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

#define SYMLINK_FLAG_RELATIVE   1

typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

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
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG FileAttributes, Flags = 0;
   PVOID EaBuffer = NULL;
   ULONG EaLength = 0;

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
   if (0 == _wcsicmp(L"CONOUT$", lpFileName)
       || 0 == _wcsicmp(L"CONIN$", lpFileName))
   {
      return OpenConsoleW(lpFileName,
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
      Flags |= FILE_DELETE_ON_CLOSE;

   if(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
   {
      if(dwDesiredAccess & GENERIC_ALL)
         Flags |= FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_FOR_RECOVERY;
      else
      {
         if(dwDesiredAccess & GENERIC_READ)
            Flags |= FILE_OPEN_FOR_BACKUP_INTENT;

         if(dwDesiredAccess & GENERIC_WRITE)
            Flags |= FILE_OPEN_FOR_RECOVERY;
      }
   }
   else
      Flags |= FILE_NON_DIRECTORY_FILE;

   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
      Flags |= FILE_OPEN_REPARSE_POINT;

   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
      Flags |= FILE_OPEN_NO_RECALL;

   FileAttributes = (dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY));

   /* handle may allways be waited on and querying attributes are allways allowed */
   dwDesiredAccess |= SYNCHRONIZE | FILE_READ_ATTRIBUTES;

   /* FILE_FLAG_POSIX_SEMANTICS is handled later */

   /* validate & translate the filename */
   if (!RtlDosPathNameToNtPathName_U (lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
   {
     WARN("Invalid path\n");
     SetLastError(ERROR_PATH_NOT_FOUND);
     return INVALID_HANDLE_VALUE;
   }

   TRACE("NtPathU \'%wZ\'\n", &NtPathU);

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
      else
      {
         SetLastErrorByStatus (Status);
      }

      return INVALID_HANDLE_VALUE;
   }

  /*
  create with OPEN_ALWAYS (FILE_OPEN_IF) returns info = FILE_OPENED or FILE_CREATED
  create with CREATE_ALWAYS (FILE_OVERWRITE_IF) returns info = FILE_OVERWRITTEN or FILE_CREATED
  */
  if (dwCreationDisposition == FILE_OPEN_IF)
  {
    SetLastError(IoStatusBlock.Information == FILE_OPENED ? ERROR_ALREADY_EXISTS : 0);
  }
  else if (dwCreationDisposition == FILE_OVERWRITE_IF)
  {
    SetLastError(IoStatusBlock.Information == FILE_OVERWRITTEN ? ERROR_ALREADY_EXISTS : 0);
  }

  return FileHandle;
}


/*
 * @implemented
 */
BOOLEAN
WINAPI
CreateSymbolicLinkW(IN LPCWSTR lpSymlinkFileName,
                    IN LPCWSTR lpTargetFileName,
                    IN DWORD dwFlags)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hSymlink = NULL;
    UNICODE_STRING SymlinkFileName = { 0, 0, NULL };
    UNICODE_STRING TargetFileName = { 0, 0, NULL };
    BOOLEAN bAllocatedTarget = FALSE, bRelativePath = FALSE;
    LPWSTR lpTargetFullFileName = NULL;
    SIZE_T cbPrintName;
    SIZE_T cbReparseData;
    PREPARSE_DATA_BUFFER pReparseData = NULL;
    PBYTE pBufTail;
    NTSTATUS Status;
    ULONG dwCreateOptions;
    DWORD dwErr;

    if(!lpSymlinkFileName || !lpTargetFileName || (dwFlags | SYMBOLIC_LINK_FLAG_DIRECTORY) != SYMBOLIC_LINK_FLAG_DIRECTORY)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(dwFlags & SYMBOLIC_LINK_FLAG_DIRECTORY)
        dwCreateOptions = FILE_DIRECTORY_FILE;
    else
        dwCreateOptions = FILE_NON_DIRECTORY_FILE;

    switch(RtlDetermineDosPathNameType_U(lpTargetFileName))
    {
    case RtlPathTypeUnknown:
    case RtlPathTypeRooted:
    case RtlPathTypeRelative:
        bRelativePath = TRUE;
        RtlInitUnicodeString(&TargetFileName, lpTargetFileName);
        break;

    case RtlPathTypeDriveRelative:
        {
            LPWSTR FilePart;
            SIZE_T cchTargetFullFileName;

            cchTargetFullFileName = GetFullPathNameW(lpTargetFileName, 0, NULL, &FilePart);

            if(cchTargetFullFileName == 0)
            {
                dwErr = GetLastError();
                goto Cleanup;
            }

            lpTargetFullFileName = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchTargetFullFileName * sizeof(WCHAR));

            if(lpTargetFullFileName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if(GetFullPathNameW(lpTargetFileName, cchTargetFullFileName, lpTargetFullFileName, &FilePart) == 0)
            {
                dwErr = GetLastError();
                goto Cleanup;
            }
        }

        lpTargetFileName = lpTargetFullFileName;

        // fallthrough

    case RtlPathTypeUncAbsolute:
    case RtlPathTypeDriveAbsolute:
    case RtlPathTypeLocalDevice:
    case RtlPathTypeRootLocalDevice:
    default:
        if(!RtlDosPathNameToNtPathName_U(lpTargetFileName, &TargetFileName, NULL, NULL))
        {
            bAllocatedTarget = TRUE;
            dwErr = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    cbPrintName = wcslen(lpTargetFileName) * sizeof(WCHAR);
    cbReparseData = FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + TargetFileName.Length + cbPrintName;
    pReparseData = RtlAllocateHeap(RtlGetProcessHeap(), 0, cbReparseData);

    if(pReparseData == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    pBufTail = (PBYTE)(pReparseData->SymbolicLinkReparseBuffer.PathBuffer);

    pReparseData->ReparseTag = (ULONG)IO_REPARSE_TAG_SYMLINK;
    pReparseData->ReparseDataLength = (USHORT)cbReparseData - REPARSE_DATA_BUFFER_HEADER_SIZE;
    pReparseData->Reserved = 0;

    pReparseData->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    pReparseData->SymbolicLinkReparseBuffer.SubstituteNameLength = TargetFileName.Length;
    pBufTail += pReparseData->SymbolicLinkReparseBuffer.SubstituteNameOffset;
    RtlCopyMemory(pBufTail, TargetFileName.Buffer, TargetFileName.Length);

    pReparseData->SymbolicLinkReparseBuffer.PrintNameOffset = pReparseData->SymbolicLinkReparseBuffer.SubstituteNameLength;
    pReparseData->SymbolicLinkReparseBuffer.PrintNameLength = (USHORT)cbPrintName;
    pBufTail += pReparseData->SymbolicLinkReparseBuffer.PrintNameOffset;
    RtlCopyMemory(pBufTail, lpTargetFileName, cbPrintName);

    pReparseData->SymbolicLinkReparseBuffer.Flags = 0;

    if(bRelativePath)
        pReparseData->SymbolicLinkReparseBuffer.Flags |= 1; // TODO! give this lone flag a name

    if(!RtlDosPathNameToNtPathName_U(lpSymlinkFileName, &SymlinkFileName, NULL, NULL))
    {
        dwErr = ERROR_PATH_NOT_FOUND;
        goto Cleanup;
    }

    InitializeObjectAttributes(&ObjectAttributes, &SymlinkFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile
    (
        &hSymlink,
        FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT | dwCreateOptions,
        NULL,
        0
    );

    if(!NT_SUCCESS(Status))
    {
        dwErr = RtlNtStatusToDosError(Status);
        goto Cleanup;
    }

    Status = NtFsControlFile
    (
        hSymlink,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        FSCTL_SET_REPARSE_POINT,
        pReparseData,
        cbReparseData,
        NULL,
        0
    );

    if(!NT_SUCCESS(Status))
    {
        FILE_DISPOSITION_INFORMATION DispInfo;
        DispInfo.DeleteFile = TRUE;
        NtSetInformationFile(hSymlink, &IoStatusBlock, &DispInfo, sizeof(DispInfo), FileDispositionInformation);

        dwErr = RtlNtStatusToDosError(Status);
        goto Cleanup;
    }

    dwErr = NO_ERROR;

Cleanup:
    if(hSymlink)
        NtClose(hSymlink);

    RtlFreeUnicodeString(&SymlinkFileName);
    if (bAllocatedTarget)
    {
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    TargetFileName.Buffer);
    }

    if(lpTargetFullFileName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpTargetFullFileName);

    if(pReparseData)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pReparseData);

    if(dwErr)
    {
        SetLastError(dwErr);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
CreateSymbolicLinkA(IN LPCSTR lpSymlinkFileName,
                    IN LPCSTR lpTargetFileName,
                    IN DWORD dwFlags)
{
    PWCHAR SymlinkW, TargetW;
    BOOLEAN Ret;

    if(!lpSymlinkFileName || !lpTargetFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(SymlinkW = FilenameA2W(lpSymlinkFileName, FALSE)))
        return FALSE;

    if (!(TargetW = FilenameA2W(lpTargetFileName, TRUE)))
        return FALSE;

    Ret = CreateSymbolicLinkW(SymlinkW,
                              TargetW,
                              dwFlags);

    RtlFreeHeap(RtlGetProcessHeap(), 0, SymlinkW);
    RtlFreeHeap(RtlGetProcessHeap(), 0, TargetW);

    return Ret;
}


/* EOF */
