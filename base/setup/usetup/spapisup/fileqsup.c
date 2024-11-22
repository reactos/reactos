/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/fileqsup.c
 * PURPOSE:         Interfacing with Setup* API File Queue support functions
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#ifndef _USETUP_PCH_
#include "usetup.h"
#endif

#define NDEBUG
#include <debug.h>

/* DEFINITIONS **************************************************************/

typedef struct _QUEUEENTRY
{
    LIST_ENTRY ListEntry;
    PWSTR SourceCabinet;    /* May be NULL if the file is not in a cabinet */
    PWSTR SourceRootPath;
    PWSTR SourcePath;
    PWSTR SourceFileName;
    PWSTR TargetDirectory;
    PWSTR TargetFileName;
} QUEUEENTRY, *PQUEUEENTRY;

typedef struct _FILEQUEUEHEADER
{
    LIST_ENTRY DeleteQueue; // PQUEUEENTRY entries
    ULONG DeleteCount;

    LIST_ENTRY RenameQueue; // PQUEUEENTRY entries
    ULONG RenameCount;

    LIST_ENTRY CopyQueue;   // PQUEUEENTRY entries
    ULONG CopyCount;

    BOOLEAN HasCurrentCabinet;
    CABINET_CONTEXT CabinetContext;
    CAB_SEARCH Search;
    WCHAR CurrentCabinetName[MAX_PATH];
} FILEQUEUEHEADER, *PFILEQUEUEHEADER;


/* SETUP* API COMPATIBILITY FUNCTIONS ****************************************/

static NTSTATUS
SetupExtractFile(
    IN OUT PFILEQUEUEHEADER QueueHeader,
    IN PCWSTR CabinetFileName,
    IN PCWSTR SourceFileName,
    IN PCWSTR DestinationPathName)
{
    ULONG CabStatus;

    DPRINT("SetupExtractFile(CabinetFileName: '%S', SourceFileName: '%S', DestinationPathName: '%S')\n",
           CabinetFileName, SourceFileName, DestinationPathName);

    if (QueueHeader->HasCurrentCabinet)
    {
        DPRINT("CurrentCabinetName: '%S'\n", QueueHeader->CurrentCabinetName);
    }

    if (QueueHeader->HasCurrentCabinet &&
        (wcscmp(CabinetFileName, QueueHeader->CurrentCabinetName) == 0))
    {
        DPRINT("Using same cabinet as last time\n");

        /* Use our last location because the files should be sequential */
        CabStatus = CabinetFindNextFileSequential(&QueueHeader->CabinetContext,
                                                  SourceFileName,
                                                  &QueueHeader->Search);
        if (CabStatus != CAB_STATUS_SUCCESS)
        {
            DPRINT("Sequential miss on file: %S\n", SourceFileName);

            /* Looks like we got unlucky */
            CabStatus = CabinetFindFirst(&QueueHeader->CabinetContext,
                                         SourceFileName,
                                         &QueueHeader->Search);
        }
    }
    else
    {
        DPRINT("Using new cabinet\n");

        if (QueueHeader->HasCurrentCabinet)
        {
            QueueHeader->HasCurrentCabinet = FALSE;
            CabinetCleanup(&QueueHeader->CabinetContext);
        }

        RtlStringCchCopyW(QueueHeader->CurrentCabinetName,
                          ARRAYSIZE(QueueHeader->CurrentCabinetName),
                          CabinetFileName);

        CabinetInitialize(&QueueHeader->CabinetContext);
        CabinetSetEventHandlers(&QueueHeader->CabinetContext,
                                NULL, NULL, NULL, NULL);
        CabinetSetCabinetName(&QueueHeader->CabinetContext, CabinetFileName);

        CabStatus = CabinetOpen(&QueueHeader->CabinetContext);
        if (CabStatus == CAB_STATUS_SUCCESS)
        {
            DPRINT("Opened cabinet %S\n", CabinetFileName /*CabinetGetCabinetName(&QueueHeader->CabinetContext)*/);
            QueueHeader->HasCurrentCabinet = TRUE;
        }
        else
        {
            DPRINT("Cannot open cabinet (%d)\n", CabStatus);
            return STATUS_UNSUCCESSFUL;
        }

        /* We have to start at the beginning here */
        CabStatus = CabinetFindFirst(&QueueHeader->CabinetContext,
                                     SourceFileName,
                                     &QueueHeader->Search);
    }

    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT1("Unable to find '%S' in cabinet '%S'\n",
                SourceFileName, CabinetGetCabinetName(&QueueHeader->CabinetContext));
        return STATUS_UNSUCCESSFUL;
    }

    CabinetSetDestinationPath(&QueueHeader->CabinetContext, DestinationPathName);
    CabStatus = CabinetExtractFile(&QueueHeader->CabinetContext, &QueueHeader->Search);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("Cannot extract file %S (%d)\n", SourceFileName, CabStatus);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

HSPFILEQ
WINAPI
SetupOpenFileQueue(VOID)
{
    PFILEQUEUEHEADER QueueHeader;

    /* Allocate the queue header */
    QueueHeader = RtlAllocateHeap(ProcessHeap, 0, sizeof(FILEQUEUEHEADER));
    if (QueueHeader == NULL)
        return NULL;

    RtlZeroMemory(QueueHeader, sizeof(FILEQUEUEHEADER));

    /* Initialize the file queues */
    InitializeListHead(&QueueHeader->DeleteQueue);
    QueueHeader->DeleteCount = 0;
    InitializeListHead(&QueueHeader->RenameQueue);
    QueueHeader->RenameCount = 0;
    InitializeListHead(&QueueHeader->CopyQueue);
    QueueHeader->CopyCount = 0;

    QueueHeader->HasCurrentCabinet = FALSE;

    return (HSPFILEQ)QueueHeader;
}

static VOID
SetupDeleteQueueEntry(
    IN PQUEUEENTRY Entry)
{
    if (Entry == NULL)
        return;

    /* Delete all strings */
    if (Entry->SourceCabinet != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

    if (Entry->SourceRootPath != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);

    if (Entry->SourcePath != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);

    if (Entry->SourceFileName != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->SourceFileName);

    if (Entry->TargetDirectory != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);

    if (Entry->TargetFileName != NULL)
        RtlFreeHeap(ProcessHeap, 0, Entry->TargetFileName);

    /* Delete queue entry */
    RtlFreeHeap(ProcessHeap, 0, Entry);
}

BOOL
WINAPI
SetupCloseFileQueue(
    IN HSPFILEQ QueueHandle)
{
    PFILEQUEUEHEADER QueueHeader;
    PLIST_ENTRY ListEntry;
    PQUEUEENTRY Entry;

    if (QueueHandle == NULL)
        return FALSE;

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    /* Delete the delete queue */
    while (!IsListEmpty(&QueueHeader->DeleteQueue))
    {
        ListEntry = RemoveHeadList(&QueueHeader->DeleteQueue);
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);
        SetupDeleteQueueEntry(Entry);
    }

    /* Delete the rename queue */
    while (!IsListEmpty(&QueueHeader->RenameQueue))
    {
        ListEntry = RemoveHeadList(&QueueHeader->RenameQueue);
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);
        SetupDeleteQueueEntry(Entry);
    }

    /* Delete the copy queue */
    while (!IsListEmpty(&QueueHeader->CopyQueue))
    {
        ListEntry = RemoveHeadList(&QueueHeader->CopyQueue);
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);
        SetupDeleteQueueEntry(Entry);
    }

    /* Delete queue header */
    RtlFreeHeap(ProcessHeap, 0, QueueHeader);

    return TRUE;
}

/* A simplified version of SetupQueueCopyW that wraps Cabinet support around */
BOOL
WINAPI
SetupQueueCopyWithCab(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourceRootPath,
    IN PCWSTR SourcePath OPTIONAL,
    IN PCWSTR SourceFileName,
    IN PCWSTR SourceDescription OPTIONAL,
    IN PCWSTR SourceCabinet OPTIONAL,
    IN PCWSTR SourceTagFile OPTIONAL,
    IN PCWSTR TargetDirectory,
    IN PCWSTR TargetFileName OPTIONAL,
    IN ULONG CopyStyle)
{
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;
    ULONG Length;

    if (QueueHandle == NULL ||
        SourceRootPath == NULL ||
        SourceFileName == NULL ||
        TargetDirectory == NULL)
    {
        return FALSE;
    }

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    DPRINT("SetupQueueCopy(Cab '%S', SrcRootPath '%S', SrcPath '%S', SrcFN '%S' --> DstPath '%S', DstFN '%S')\n",
           SourceCabinet ? SourceCabinet : L"n/a",
           SourceRootPath, SourcePath, SourceFileName,
           TargetDirectory, TargetFileName);

    /* Allocate new queue entry */
    Entry = RtlAllocateHeap(ProcessHeap, 0, sizeof(QUEUEENTRY));
    if (Entry == NULL)
        return FALSE;

    RtlZeroMemory(Entry, sizeof(QUEUEENTRY));

    /* Copy source cabinet if available */
    Entry->SourceCabinet = NULL;
    if (SourceCabinet != NULL)
    {
        Length = wcslen(SourceCabinet);
        Entry->SourceCabinet = RtlAllocateHeap(ProcessHeap,
                                               0,
                                               (Length + 1) * sizeof(WCHAR));
        if (Entry->SourceCabinet == NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->SourceCabinet, Length + 1, SourceCabinet);
    }

    /* Copy source root path */
    Length = wcslen(SourceRootPath);
    Entry->SourceRootPath = RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (Length + 1) * sizeof(WCHAR));
    if (Entry->SourceRootPath == NULL)
    {
        if (Entry->SourceCabinet != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->SourceRootPath, Length + 1, SourceRootPath);

    /* Copy source path */
    Entry->SourcePath = NULL;
    if (SourcePath != NULL)
    {
        Length = wcslen(SourcePath);
        if ((Length > 0) && (SourcePath[Length - 1] == L'\\'))
            Length--;
        Entry->SourcePath = RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (Length + 1) * sizeof(WCHAR));
        if (Entry->SourcePath == NULL)
        {
            if (Entry->SourceCabinet != NULL)
                RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

            RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->SourcePath, Length + 1, SourcePath);
    }

    /* Copy source file name */
    Length = wcslen(SourceFileName);
    Entry->SourceFileName = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                    0,
                                                    (Length + 1) * sizeof(WCHAR));
    if (Entry->SourceFileName == NULL)
    {
        if (Entry->SourcePath != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);

        RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);

        if (Entry->SourceCabinet != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->SourceFileName, Length + 1, SourceFileName);

    /* Copy target directory */
    Length = wcslen(TargetDirectory);
    if ((Length > 0) && (TargetDirectory[Length - 1] == L'\\'))
        Length--;
    Entry->TargetDirectory = RtlAllocateHeap(ProcessHeap,
                                             0,
                                             (Length + 1) * sizeof(WCHAR));
    if (Entry->TargetDirectory == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Entry->SourceFileName);

        if (Entry->SourcePath != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);

        RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);

        if (Entry->SourceCabinet != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->TargetDirectory, Length + 1, TargetDirectory);

    /* Copy optional target filename */
    Entry->TargetFileName = NULL;
    if (TargetFileName != NULL)
    {
        Length = wcslen(TargetFileName);
        Entry->TargetFileName = RtlAllocateHeap(ProcessHeap,
                                                0,
                                                (Length + 1) * sizeof(WCHAR));
        if (Entry->TargetFileName == NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceFileName);

            if (Entry->SourcePath != NULL)
                RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);

            RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);

            if (Entry->SourceCabinet != NULL)
                RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->TargetFileName, Length + 1, TargetFileName);
    }

    /* Append queue entry */
    InsertTailList(&QueueHeader->CopyQueue, &Entry->ListEntry);
    ++QueueHeader->CopyCount;

    return TRUE;
}

BOOL
WINAPI
SetupQueueDeleteW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR PathPart1,
    IN PCWSTR PathPart2 OPTIONAL)
{
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;
    ULONG Length;

    if (QueueHandle == NULL || PathPart1 == NULL)
    {
        return FALSE;
    }

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    DPRINT1("SetupQueueDeleteW(PathPart1 '%S', PathPart2 '%S')\n",
           PathPart1, PathPart2);

    /* Allocate new queue entry */
    Entry = RtlAllocateHeap(ProcessHeap, 0, sizeof(QUEUEENTRY));
    if (Entry == NULL)
        return FALSE;

    RtlZeroMemory(Entry, sizeof(QUEUEENTRY));

    Entry->SourceCabinet = NULL;
    Entry->SourceRootPath = NULL;
    Entry->SourcePath = NULL;
    Entry->SourceFileName = NULL;

    /* Copy first part of path */
    Length = wcslen(PathPart1);
    // if ((Length > 0) && (SourcePath[Length - 1] == L'\\'))
        // Length--;
    Entry->TargetDirectory = RtlAllocateHeap(ProcessHeap,
                                             0,
                                             (Length + 1) * sizeof(WCHAR));
    if (Entry->TargetDirectory == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->TargetDirectory, Length + 1, PathPart1);

    /* Copy optional second part of path */
    if (PathPart2 != NULL)
    {
        Length = wcslen(PathPart2);
        Entry->TargetFileName = RtlAllocateHeap(ProcessHeap,
                                                0,
                                                (Length + 1) * sizeof(WCHAR));
        if (Entry->TargetFileName == NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->TargetFileName, Length + 1, PathPart2);
    }

    /* Append the queue entry */
    InsertTailList(&QueueHeader->DeleteQueue, &Entry->ListEntry);
    ++QueueHeader->DeleteCount;

    return TRUE;
}

BOOL
WINAPI
SetupQueueRenameW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourcePath,
    IN PCWSTR SourceFileName OPTIONAL,
    IN PCWSTR TargetPath OPTIONAL,
    IN PCWSTR TargetFileName)
{
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;
    ULONG Length;

    if (QueueHandle == NULL ||
        SourcePath  == NULL ||
        TargetFileName == NULL)
    {
        return FALSE;
    }

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    DPRINT1("SetupQueueRenameW(SrcPath '%S', SrcFN '%S' --> DstPath '%S', DstFN '%S')\n",
           SourcePath, SourceFileName, TargetPath, TargetFileName);

    /* Allocate a new queue entry */
    Entry = RtlAllocateHeap(ProcessHeap, 0, sizeof(QUEUEENTRY));
    if (Entry == NULL)
        return FALSE;

    RtlZeroMemory(Entry, sizeof(QUEUEENTRY));

    Entry->SourceCabinet  = NULL;
    Entry->SourceRootPath = NULL;

    /* Copy source path */
    Length = wcslen(SourcePath);
    if ((Length > 0) && (SourcePath[Length - 1] == L'\\'))
        Length--;
    Entry->SourcePath = RtlAllocateHeap(ProcessHeap,
                                        0,
                                        (Length + 1) * sizeof(WCHAR));
    if (Entry->SourcePath == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->SourcePath, Length + 1, SourcePath);

    /* Copy optional source file name */
    Entry->SourceFileName = NULL;
    if (SourceFileName != NULL)
    {
        Length = wcslen(SourceFileName);
        Entry->SourceFileName = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                        0,
                                                        (Length + 1) * sizeof(WCHAR));
        if (Entry->SourceFileName == NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->SourceFileName, Length + 1, SourceFileName);
    }

    /* Copy optional target directory */
    Entry->TargetDirectory = NULL;
    if (TargetPath != NULL)
    {
        Length = wcslen(TargetPath);
        if ((Length > 0) && (TargetPath[Length - 1] == L'\\'))
            Length--;
        Entry->TargetDirectory = RtlAllocateHeap(ProcessHeap,
                                                 0,
                                                 (Length + 1) * sizeof(WCHAR));
        if (Entry->TargetDirectory == NULL)
        {
            if (Entry->SourceFileName != NULL)
                RtlFreeHeap(ProcessHeap, 0, Entry->SourceFileName);

            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }
        RtlStringCchCopyW(Entry->TargetDirectory, Length + 1, TargetPath);
    }

    /* Copy target filename */
    Length = wcslen(TargetFileName);
    Entry->TargetFileName = RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (Length + 1) * sizeof(WCHAR));
    if (Entry->TargetFileName == NULL)
    {
        if (Entry->TargetDirectory != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);

        if (Entry->SourceFileName != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceFileName);

        RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }
    RtlStringCchCopyW(Entry->TargetFileName, Length + 1, TargetFileName);

    /* Append the queue entry */
    InsertTailList(&QueueHeader->RenameQueue, &Entry->ListEntry);
    ++QueueHeader->RenameCount;

    return TRUE;
}

BOOL
WINAPI
SetupCommitFileQueueW(
    IN HWND Owner,
    IN HSPFILEQ QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID Context OPTIONAL)
{
    BOOL Success = TRUE; // Suppose success
    UINT Result;
    NTSTATUS Status;
    PFILEQUEUEHEADER QueueHeader;
    PLIST_ENTRY ListEntry;
    PQUEUEENTRY Entry;
    FILEPATHS_W FilePathInfo;
    WCHAR FileSrcPath[MAX_PATH];
    WCHAR FileDstPath[MAX_PATH];

    if (QueueHandle == NULL)
        return FALSE;

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    Result = MsgHandler(Context,
                        SPFILENOTIFY_STARTQUEUE,
                        (UINT_PTR)Owner,
                        0);
    if (Result == FILEOP_ABORT)
        return FALSE;


    /*
     * Commit the delete queue
     */

    if (!IsListEmpty(&QueueHeader->DeleteQueue))
    {
        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTSUBQUEUE,
                            FILEOP_DELETE,
                            QueueHeader->DeleteCount);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto Quit;
        }
    }

    for (ListEntry = QueueHeader->DeleteQueue.Flink;
         ListEntry != &QueueHeader->DeleteQueue;
         ListEntry = ListEntry->Flink)
    {
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);

        /* Build the full target path */
        CombinePaths(FileDstPath, ARRAYSIZE(FileDstPath), 2,
                     Entry->TargetDirectory, Entry->TargetFileName);

        DPRINT1(" -----> " "Delete: '%S'\n", FileDstPath);

        FilePathInfo.Target = FileDstPath;
        FilePathInfo.Source = NULL;
        FilePathInfo.Win32Error = STATUS_SUCCESS;
        FilePathInfo.Flags = 0; // FIXME: Unused yet...

        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTDELETE,
                            (UINT_PTR)&FilePathInfo,
                            FILEOP_DELETE);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto EndDelete;
        }
        else if (Result == FILEOP_SKIP)
            goto EndDelete;
        // else (Result == FILEOP_DOIT)

RetryDelete:
        /* Force-delete the file */
        Status = SetupDeleteFile(FileDstPath, TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* An error happened */
            FilePathInfo.Win32Error = (UINT)Status;
            Result = MsgHandler(Context,
                                SPFILENOTIFY_DELETEERROR,
                                (UINT_PTR)&FilePathInfo,
                                0);
            if (Result == FILEOP_ABORT)
            {
                Success = FALSE;
                goto EndDelete;
            }
            else if (Result == FILEOP_SKIP)
                goto EndDelete;
            else if (Result == FILEOP_RETRY)
                goto RetryDelete;

            Success = FALSE;
        }

EndDelete:
        /* This notification is always sent, even in case of error */
        FilePathInfo.Win32Error = (UINT)Status;
        MsgHandler(Context,
                   SPFILENOTIFY_ENDDELETE,
                   (UINT_PTR)&FilePathInfo,
                   0);
        if (Success == FALSE /* && Result == FILEOP_ABORT */)
            goto Quit;
    }

    if (!IsListEmpty(&QueueHeader->DeleteQueue))
    {
        MsgHandler(Context,
                   SPFILENOTIFY_ENDSUBQUEUE,
                   FILEOP_DELETE,
                   0);
    }


    /*
     * Commit the rename queue
     */

    if (!IsListEmpty(&QueueHeader->RenameQueue))
    {
        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTSUBQUEUE,
                            FILEOP_RENAME,
                            QueueHeader->RenameCount);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto Quit;
        }
    }

    for (ListEntry = QueueHeader->RenameQueue.Flink;
         ListEntry != &QueueHeader->RenameQueue;
         ListEntry = ListEntry->Flink)
    {
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);

        /* Build the full source path */
        CombinePaths(FileSrcPath, ARRAYSIZE(FileSrcPath), 2,
                     Entry->SourcePath, Entry->SourceFileName);

        /* Build the full target path */
        CombinePaths(FileDstPath, ARRAYSIZE(FileDstPath), 2,
                     Entry->TargetDirectory, Entry->TargetFileName);

        DPRINT1(" -----> " "Rename: '%S' ==> '%S'\n", FileSrcPath, FileDstPath);

        FilePathInfo.Target = FileDstPath;
        FilePathInfo.Source = FileSrcPath;
        FilePathInfo.Win32Error = STATUS_SUCCESS;
        FilePathInfo.Flags = 0; // FIXME: Unused yet...

        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTRENAME,
                            (UINT_PTR)&FilePathInfo,
                            FILEOP_RENAME);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto EndRename;
        }
        else if (Result == FILEOP_SKIP)
            goto EndRename;
        // else (Result == FILEOP_DOIT)

RetryRename:
        /* Move or rename the file */
        Status = SetupMoveFile(FileSrcPath, FileDstPath,
                               MOVEFILE_REPLACE_EXISTING
                                    | MOVEFILE_COPY_ALLOWED
                                    | MOVEFILE_WRITE_THROUGH);
        if (!NT_SUCCESS(Status))
        {
            /* An error happened */
            FilePathInfo.Win32Error = (UINT)Status;
            Result = MsgHandler(Context,
                                SPFILENOTIFY_RENAMEERROR,
                                (UINT_PTR)&FilePathInfo,
                                0);
            if (Result == FILEOP_ABORT)
            {
                Success = FALSE;
                goto EndRename;
            }
            else if (Result == FILEOP_SKIP)
                goto EndRename;
            else if (Result == FILEOP_RETRY)
                goto RetryRename;

            Success = FALSE;
        }

EndRename:
        /* This notification is always sent, even in case of error */
        FilePathInfo.Win32Error = (UINT)Status;
        MsgHandler(Context,
                   SPFILENOTIFY_ENDRENAME,
                   (UINT_PTR)&FilePathInfo,
                   0);
        if (Success == FALSE /* && Result == FILEOP_ABORT */)
            goto Quit;
    }

    if (!IsListEmpty(&QueueHeader->RenameQueue))
    {
        MsgHandler(Context,
                   SPFILENOTIFY_ENDSUBQUEUE,
                   FILEOP_RENAME,
                   0);
    }


    /*
     * Commit the copy queue
     */

    if (!IsListEmpty(&QueueHeader->CopyQueue))
    {
        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTSUBQUEUE,
                            FILEOP_COPY,
                            QueueHeader->CopyCount);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto Quit;
        }
    }

    for (ListEntry = QueueHeader->CopyQueue.Flink;
         ListEntry != &QueueHeader->CopyQueue;
         ListEntry = ListEntry->Flink)
    {
        Entry = CONTAINING_RECORD(ListEntry, QUEUEENTRY, ListEntry);

        //
        // TODO: Send a SPFILENOTIFY_NEEDMEDIA notification
        // when we switch to a new installation media.
        // Param1 = (UINT_PTR)(PSOURCE_MEDIA)SourceMediaInfo;
        // Param2 = (UINT_PTR)(TCHAR[MAX_PATH])NewPathInfo;
        //

        /* Build the full source path */
        if (Entry->SourceCabinet == NULL)
        {
            CombinePaths(FileSrcPath, ARRAYSIZE(FileSrcPath), 3,
                         Entry->SourceRootPath, Entry->SourcePath,
                         Entry->SourceFileName);
        }
        else
        {
            /*
             * The cabinet must be in Entry->SourceRootPath only!
             * (Should we ignore Entry->SourcePath?)
             */
            CombinePaths(FileSrcPath, ARRAYSIZE(FileSrcPath), 3,
                         Entry->SourceRootPath, Entry->SourcePath,
                         Entry->SourceCabinet);
        }

        /* Build the full target path */
        RtlStringCchCopyW(FileDstPath, ARRAYSIZE(FileDstPath), Entry->TargetDirectory);
        if (Entry->SourceCabinet == NULL)
        {
            /* If the file is not in a cabinet, possibly use a different target name */
            if (Entry->TargetFileName != NULL)
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->TargetFileName);
            else
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->SourceFileName);
        }
        else
        {
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->SourceFileName);
        }

        DPRINT(" -----> " "Copy: '%S' ==> '%S'\n", FileSrcPath, FileDstPath);

        //
        // Technically, here we should create the target directory,
        // if it does not already exist... before calling the handler!
        //

        FilePathInfo.Target = FileDstPath;
        FilePathInfo.Source = FileSrcPath;
        FilePathInfo.Win32Error = STATUS_SUCCESS;
        FilePathInfo.Flags = 0; // FIXME: Unused yet...

        Result = MsgHandler(Context,
                            SPFILENOTIFY_STARTCOPY,
                            (UINT_PTR)&FilePathInfo,
                            FILEOP_COPY);
        if (Result == FILEOP_ABORT)
        {
            Success = FALSE;
            goto EndCopy;
        }
        else if (Result == FILEOP_SKIP)
            goto EndCopy;
        // else (Result == FILEOP_DOIT)

RetryCopy:
        if (Entry->SourceCabinet != NULL)
        {
            /*
             * The file is in a cabinet, use only the destination path
             * and keep the source name as the target name.
             */
            /* Extract the file from the cabinet */
            Status = SetupExtractFile(QueueHeader,
                                      FileSrcPath, // Specifies the cabinet path
                                      Entry->SourceFileName,
                                      Entry->TargetDirectory);
        }
        else
        {
            /* Copy the file */
            Status = SetupCopyFile(FileSrcPath, FileDstPath, FALSE);
        }

        if (!NT_SUCCESS(Status))
        {
            /* An error happened */
            FilePathInfo.Win32Error = (UINT)Status;
            Result = MsgHandler(Context,
                                SPFILENOTIFY_COPYERROR,
                                (UINT_PTR)&FilePathInfo,
                                (UINT_PTR)NULL); // FIXME: Unused yet...
            if (Result == FILEOP_ABORT)
            {
                Success = FALSE;
                goto EndCopy;
            }
            else if (Result == FILEOP_SKIP)
                goto EndCopy;
            else if (Result == FILEOP_RETRY)
                goto RetryCopy;
            else if (Result == FILEOP_NEWPATH)
                goto RetryCopy; // TODO!

            Success = FALSE;
        }

EndCopy:
        /* This notification is always sent, even in case of error */
        FilePathInfo.Win32Error = (UINT)Status;
        MsgHandler(Context,
                   SPFILENOTIFY_ENDCOPY,
                   (UINT_PTR)&FilePathInfo,
                   0);
        if (Success == FALSE /* && Result == FILEOP_ABORT */)
            goto Quit;
    }

    if (!IsListEmpty(&QueueHeader->CopyQueue))
    {
        MsgHandler(Context,
                   SPFILENOTIFY_ENDSUBQUEUE,
                   FILEOP_COPY,
                   0);
    }


Quit:
    /* All the queues have been committed */
    MsgHandler(Context,
               SPFILENOTIFY_ENDQUEUE,
               (UINT_PTR)Success,
               0);

    return Success;
}


/* GLOBALS *******************************************************************/

SPFILE_EXPORTS SpFileExports =
{
    SetupOpenFileQueue,
    SetupCloseFileQueue,
    SetupQueueCopyWithCab,
    SetupQueueDeleteW,
    SetupQueueRenameW,
    SetupCommitFileQueueW
};

/* EOF */
