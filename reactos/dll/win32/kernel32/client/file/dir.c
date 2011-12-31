/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/*
 * NOTES: Changed to using ZwCreateFile
 */

/* INCLUDES ******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryA (
        LPCSTR                  lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
   PWCHAR PathNameW;

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
      return FALSE;

   return CreateDirectoryW (PathNameW,
                            lpSecurityAttributes);
}


/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryExA (
        LPCSTR                  lpTemplateDirectory,
        LPCSTR                  lpNewDirectory,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes)
{
   PWCHAR TemplateDirectoryW;
   PWCHAR NewDirectoryW;
   BOOL ret;

   if (!(TemplateDirectoryW = FilenameA2W(lpTemplateDirectory, TRUE)))
      return FALSE;

   if (!(NewDirectoryW = FilenameA2W(lpNewDirectory, FALSE)))
   {
      RtlFreeHeap (RtlGetProcessHeap (),
                   0,
                   TemplateDirectoryW);
      return FALSE;
   }

   ret = CreateDirectoryExW (TemplateDirectoryW,
                             NewDirectoryW,
                             lpSecurityAttributes);

   RtlFreeHeap (RtlGetProcessHeap (),
                0,
                TemplateDirectoryW);

   return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryW (
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU;
        HANDLE DirectoryHandle = NULL;
        NTSTATUS Status;

        TRACE ("lpPathName %S lpSecurityAttributes %p\n",
                lpPathName, lpSecurityAttributes);

        if (!RtlDosPathNameToNtPathName_U (lpPathName,
                                           &NtPathU,
                                           NULL,
                                           NULL))
        {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

        Status = NtCreateFile (&DirectoryHandle,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_CREATE,
                               FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                               NULL,
                               0);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtPathU.Buffer);

        if (!NT_SUCCESS(Status))
        {
                WARN("NtCreateFile failed with Status %lx\n", Status);
                BaseSetLastNTError(Status);
                return FALSE;
        }

        NtClose (DirectoryHandle);

        return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryExW (
        LPCWSTR                 lpTemplateDirectory,
        LPCWSTR                 lpNewDirectory,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU, NtTemplatePathU;
        HANDLE DirectoryHandle = NULL;
        HANDLE TemplateHandle = NULL;
        FILE_EA_INFORMATION EaInformation;
        FILE_BASIC_INFORMATION FileBasicInfo;
        NTSTATUS Status;
        ULONG OpenOptions, CreateOptions;
        ACCESS_MASK DesiredAccess;
        BOOLEAN ReparsePoint = FALSE;
        PVOID EaBuffer = NULL;
        ULONG EaLength = 0;

        OpenOptions = FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT |
                      FILE_OPEN_FOR_BACKUP_INTENT;
        CreateOptions = FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT;
        DesiredAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES |
                        FILE_READ_ATTRIBUTES;

        TRACE ("lpTemplateDirectory %ws lpNewDirectory %ws lpSecurityAttributes %p\n",
                lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);

        /*
         * Translate the template directory path
         */

        if (!RtlDosPathNameToNtPathName_U (lpTemplateDirectory,
                                           &NtTemplatePathU,
                                           NULL,
                                           NULL))
        {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtTemplatePathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        /*
         * Open the template directory
         */

OpenTemplateDir:
        Status = NtOpenFile (&TemplateHandle,
                             FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | FILE_READ_EA,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             OpenOptions);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_INVALID_PARAMETER &&
                (OpenOptions & FILE_OPEN_REPARSE_POINT))
            {
                /* Some FSs (FAT) don't support reparse points, try opening
                   the directory without FILE_OPEN_REPARSE_POINT */
                OpenOptions &= ~FILE_OPEN_REPARSE_POINT;

                TRACE("Reparse points not supported, try with less options\n");

                /* try again */
                goto OpenTemplateDir;
            }
            else
            {
                WARN("Failed to open the template directory: 0x%x\n", Status);
                goto CleanupNoNtPath;
            }
        }

        /*
         * Translate the new directory path and check if they're the same
         */

        if (!RtlDosPathNameToNtPathName_U (lpNewDirectory,
                                           &NtPathU,
                                           NULL,
                                           NULL))
        {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            goto CleanupNoNtPath;
        }

        if (RtlEqualUnicodeString(&NtPathU,
                                  &NtTemplatePathU,
                                  TRUE))
        {
            WARN("Both directory paths are the same!\n");
            Status = STATUS_OBJECT_NAME_INVALID;
            goto Cleanup;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

        /*
         * Query the basic file attributes from the template directory
         */

        /* Make sure FILE_ATTRIBUTE_NORMAL is used in case the information
           isn't set by the FS */
        FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtQueryInformationFile(TemplateHandle,
                                        &IoStatusBlock,
                                        &FileBasicInfo,
                                        sizeof(FILE_BASIC_INFORMATION),
                                        FileBasicInformation);
        if (!NT_SUCCESS(Status))
        {
            WARN("Failed to query the basic directory attributes\n");
            goto Cleanup;
        }

        /* clear the reparse point attribute if present. We're going to set the
           reparse point later which will cause the attribute to be set */
        if (FileBasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            FileBasicInfo.FileAttributes &= ~FILE_ATTRIBUTE_REPARSE_POINT;

            /* writing the extended attributes requires the FILE_WRITE_DATA
               access right */
            DesiredAccess |= FILE_WRITE_DATA;

            CreateOptions |= FILE_OPEN_REPARSE_POINT;
            ReparsePoint = TRUE;
        }

        /*
         * Read the Extended Attributes if present
         */

        for (;;)
        {
          Status = NtQueryInformationFile(TemplateHandle,
                                          &IoStatusBlock,
                                          &EaInformation,
                                          sizeof(FILE_EA_INFORMATION),
                                          FileEaInformation);
          if (NT_SUCCESS(Status) && (EaInformation.EaSize != 0))
          {
            EaBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       EaInformation.EaSize);
            if (EaBuffer == NULL)
            {
               Status = STATUS_INSUFFICIENT_RESOURCES;
               break;
            }

            Status = NtQueryEaFile(TemplateHandle,
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
               /* we successfully read the extended attributes */
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
            if (Status == STATUS_EAS_NOT_SUPPORTED)
            {
                /* Extended attributes are not supported, so, this is OK */
                /* FIXME: Would deserve a deeper look, comparing with Windows */
                Status = STATUS_SUCCESS;
            }
            /* failure or no extended attributes present, break the loop */
            break;
          }
        }

        if (!NT_SUCCESS(Status))
        {
            WARN("Querying the EA data failed: 0x%x\n", Status);
            goto Cleanup;
        }

        /*
         * Create the new directory
         */

        Status = NtCreateFile (&DirectoryHandle,
                               DesiredAccess,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FileBasicInfo.FileAttributes,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_CREATE,
                               CreateOptions,
                               EaBuffer,
                               EaLength);
        if (!NT_SUCCESS(Status))
        {
            if (ReparsePoint &&
                (Status == STATUS_INVALID_PARAMETER || Status == STATUS_ACCESS_DENIED))
            {
                /* The FS doesn't seem to support reparse points... */
                WARN("Cannot copy the hardlink, destination doesn\'t support reparse points!\n");
            }

            goto Cleanup;
        }

        if (ReparsePoint)
        {
            /*
             * Copy the reparse point
             */

            PREPARSE_GUID_DATA_BUFFER ReparseDataBuffer =
                (PREPARSE_GUID_DATA_BUFFER)RtlAllocateHeap(RtlGetProcessHeap(),
                                                           0,
                                                           MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

            if (ReparseDataBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            /* query the size of the reparse data buffer structure */
            Status = NtFsControlFile(TemplateHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     FSCTL_GET_REPARSE_POINT,
                                     NULL,
                                     0,
                                     ReparseDataBuffer,
                                     MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
            if (NT_SUCCESS(Status))
            {
                /* write the reparse point */
                Status = NtFsControlFile(DirectoryHandle,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &IoStatusBlock,
                                         FSCTL_SET_REPARSE_POINT,
                                         ReparseDataBuffer,
                                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                                         NULL,
                                         0);
            }

            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        ReparseDataBuffer);

            if (!NT_SUCCESS(Status))
            {
                /* fail, we were unable to read the reparse point data! */
                WARN("Querying or setting the reparse point failed: 0x%x\n", Status);
                goto Cleanup;
            }
        }
        else
        {
            /*
             * Copy alternate file streams, if existing
             */

            /* FIXME - enumerate and copy the file streams */
        }

        /*
         * We successfully created the directory and copied all information
         * from the template directory
         */
        Status = STATUS_SUCCESS;

Cleanup:
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtPathU.Buffer);

CleanupNoNtPath:
        if (TemplateHandle != NULL)
        {
                NtClose(TemplateHandle);
        }

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtTemplatePathU.Buffer);

        /* free the he extended attributes buffer */
        if (EaBuffer != NULL)
        {
                RtlFreeHeap (RtlGetProcessHeap (),
                             0,
                             EaBuffer);
        }

        if (DirectoryHandle != NULL)
        {
                NtClose(DirectoryHandle);
        }

        if (!NT_SUCCESS(Status))
        {
                BaseSetLastNTError(Status);
                return FALSE;
        }

        return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveDirectoryA (
        LPCSTR  lpPathName
        )
{
   PWCHAR PathNameW;

   TRACE("RemoveDirectoryA(%s)\n",lpPathName);

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
       return FALSE;

   return RemoveDirectoryW (PathNameW);
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveDirectoryW (
        LPCWSTR lpPathName
        )
{
        FILE_DISPOSITION_INFORMATION FileDispInfo;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU;
        HANDLE DirectoryHandle = NULL;
        NTSTATUS Status;

        TRACE("lpPathName %S\n", lpPathName);

        if (!RtlDosPathNameToNtPathName_U (lpPathName,
                                           &NtPathU,
                                           NULL,
                                           NULL))
        {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        TRACE("NtPathU '%S'\n", NtPathU.Buffer);

        Status = NtOpenFile(&DirectoryHandle,
                            DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

        RtlFreeUnicodeString(&NtPathU);

        if (!NT_SUCCESS(Status))
        {
                WARN("Status 0x%08x\n", Status);
                BaseSetLastNTError (Status);
                return FALSE;
        }

        FileDispInfo.DeleteFile = TRUE;

        Status = NtSetInformationFile (DirectoryHandle,
                                       &IoStatusBlock,
                                       &FileDispInfo,
                                       sizeof(FILE_DISPOSITION_INFORMATION),
                                       FileDispositionInformation);
        NtClose(DirectoryHandle);

        if (!NT_SUCCESS(Status))
        {
                BaseSetLastNTError (Status);
                return FALSE;
        }

        return TRUE;
}

/* EOF */
