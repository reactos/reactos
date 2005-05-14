/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Removed use of SearchPath (not used by Windows)
 *                  18/08/2002: CreateFileW mess cleaned up (KJK::Hyperion)
 *                  24/08/2002: removed superfluous DPRINTs (KJK::Hyperion)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL CreateFileA (LPCSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   PWCHAR FileNameW;
   HANDLE FileHandle;

   DPRINT("CreateFileA(lpFileName %s)\n",lpFileName);

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
HANDLE STDCALL CreateFileW (LPCWSTR			lpFileName,
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
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   PVOID EaBuffer = NULL;
   ULONG EaLength = 0;

   DPRINT("CreateFileW(lpFileName %S)\n",lpFileName);

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

   /* validate & translate the filename */
   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
   {
     DPRINT("Invalid path\n");
     SetLastError(ERROR_PATH_NOT_FOUND);
     return INVALID_HANDLE_VALUE;
   }

   DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);

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

   /* check for console output */
   if (0 == _wcsicmp(L"CONOUT$", lpFileName))
   {
      /* FIXME: Send required access rights to Csrss */
      Request.Type = CSRSS_GET_OUTPUT_HANDLE;
      Status = CsrClientCallServer(&Request,
			           &Reply,
			           sizeof(CSRSS_API_REQUEST),
			           sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
      {
         SetLastErrorByStatus(Status);
	 return INVALID_HANDLE_VALUE;
      }
      else
      {
         return Reply.Data.GetOutputHandleReply.OutputHandle;
      }
   }

   /* check for console input */
   if (0 == _wcsicmp(L"CONIN$", lpFileName))
   {
      /* FIXME: Send required access rights to Csrss */
      Request.Type = CSRSS_GET_INPUT_HANDLE;
      Status = CsrClientCallServer(&Request,
			           &Reply,
			           sizeof(CSRSS_API_REQUEST),
			           sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
      {
         SetLastErrorByStatus(Status);
	 return INVALID_HANDLE_VALUE;
      }
      else
      {
         return Reply.Data.GetInputHandleReply.InputHandle;
      }
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

   RtlFreeUnicodeString(&NtPathU);

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

/* EOF */
