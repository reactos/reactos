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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filequeue.c
 * PURPOSE:         File queue functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* INCLUDES *****************************************************************/

typedef struct _QUEUEENTRY
{
    struct _QUEUEENTRY *Prev;
    struct _QUEUEENTRY *Next;

    PWSTR SourceCabinet;    /* May be NULL if the file is not in a cabinet */
    PWSTR SourceRootPath;
    PWSTR SourcePath;
    PWSTR SourceFilename;
    PWSTR TargetDirectory;
    PWSTR TargetFilename;
} QUEUEENTRY, *PQUEUEENTRY;


typedef struct _FILEQUEUEHEADER
{
    PQUEUEENTRY CopyHead;
    PQUEUEENTRY CopyTail;
    ULONG CopyCount;
} FILEQUEUEHEADER, *PFILEQUEUEHEADER;


/* FUNCTIONS ****************************************************************/

HSPFILEQ
WINAPI
SetupOpenFileQueue(VOID)
{
    PFILEQUEUEHEADER QueueHeader;

    /* Allocate queue header */
    QueueHeader = (PFILEQUEUEHEADER)RtlAllocateHeap(ProcessHeap,
                                                    0,
                                                    sizeof(FILEQUEUEHEADER));
    if (QueueHeader == NULL)
        return NULL;

    /* Initialize queue header */
    RtlZeroMemory(QueueHeader,
                  sizeof(FILEQUEUEHEADER));

    return (HSPFILEQ)QueueHeader;
}


VOID
WINAPI
SetupCloseFileQueue(
    HSPFILEQ QueueHandle)
{
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;

    if (QueueHandle == NULL)
        return;

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    /* Delete copy queue */
    Entry = QueueHeader->CopyHead;
    while (Entry != NULL)
    {
        /* Delete all strings */
        if (Entry->SourceCabinet != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);

        if (Entry->SourceRootPath != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);

        if (Entry->SourcePath != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);

        if (Entry->SourceFilename != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);

        if (Entry->TargetDirectory != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);

        if (Entry->TargetFilename != NULL)
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetFilename);

        /* Unlink current queue entry */
        if (Entry->Next != NULL)
        {
            QueueHeader->CopyHead = Entry->Next;
            QueueHeader->CopyHead->Prev = NULL;
        }
        else
        {
            QueueHeader->CopyHead = NULL;
            QueueHeader->CopyTail = NULL;
        }

        /* Delete queue entry */
        RtlFreeHeap(ProcessHeap, 0, Entry);

        /* Get next queue entry */
        Entry = QueueHeader->CopyHead;
    }

    /* Delete queue header */
    RtlFreeHeap(ProcessHeap, 0, QueueHeader);
}


BOOL
SetupQueueCopy(
    HSPFILEQ QueueHandle,
    PCWSTR SourceCabinet,
    PCWSTR SourceRootPath,
    PCWSTR SourcePath,
    PCWSTR SourceFilename,
    PCWSTR TargetDirectory,
    PCWSTR TargetFilename)
{
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;
    ULONG Length;

    /* SourceCabinet may be NULL */
    if (QueueHandle == NULL ||
        SourceRootPath == NULL ||
        SourceFilename == NULL ||
        TargetDirectory == NULL)
        return FALSE;

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    /* Allocate new queue entry */
    Entry = (PQUEUEENTRY)RtlAllocateHeap(ProcessHeap,
                                         0,
                                         sizeof(QUEUEENTRY));
    if (Entry == NULL)
        return FALSE;

    RtlZeroMemory(Entry,
                  sizeof(QUEUEENTRY));

    /* Copy source cabinet if available */
    if (SourceCabinet != NULL)
    {
        Length = wcslen(SourceCabinet);
        Entry->SourceCabinet = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                       0,
                                                       (Length + 1) * sizeof(WCHAR));
        if (Entry->SourceCabinet == NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }

        wcsncpy(Entry->SourceCabinet, SourceCabinet, Length);
        Entry->SourceCabinet[Length] = (WCHAR)0;
    }
    else
    {
        Entry->SourceCabinet = NULL;
    }

    /* Copy source root path */
    Length = wcslen(SourceRootPath);
    Entry->SourceRootPath = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                    0,
                                                    (Length + 1) * sizeof(WCHAR));
    if (Entry->SourceRootPath == NULL)
    {
        if (Entry->SourceCabinet != NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);
        }

        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }

    wcsncpy(Entry->SourceRootPath, SourceRootPath, Length);
    Entry->SourceRootPath[Length] = (WCHAR)0;

    /* Copy source path */
    if (SourcePath != NULL)
    {
        Length = wcslen(SourcePath);
        if ((Length > 0) && (SourcePath[Length - 1] == L'\\'))
            Length--;
        Entry->SourcePath = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                    0,
                                                    (Length + 1) * sizeof(WCHAR));
        if (Entry->SourcePath == NULL)
        {
            if (Entry->SourceCabinet != NULL)
            {
                RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);
            }

            RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }

        wcsncpy(Entry->SourcePath, SourcePath, Length);
        Entry->SourcePath[Length] = (WCHAR)0;
    }

    /* Copy source file name */
    Length = wcslen(SourceFilename);
    Entry->SourceFilename = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                    0,
                                                    (Length + 1) * sizeof(WCHAR));
    if (Entry->SourceFilename == NULL)
    {
        if (Entry->SourceCabinet != NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);
        }

        RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
        RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }

    wcsncpy(Entry->SourceFilename, SourceFilename, Length);
    Entry->SourceFilename[Length] = (WCHAR)0;

    /* Copy target directory */
    Length = wcslen(TargetDirectory);
    if ((Length > 0) && (TargetDirectory[Length - 1] == L'\\'))
        Length--;
    Entry->TargetDirectory = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                     0,
                                                     (Length + 1) * sizeof(WCHAR));
    if (Entry->TargetDirectory == NULL)
    {
        if (Entry->SourceCabinet != NULL)
        {
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);
        }

        RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
        RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
        RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);
        RtlFreeHeap(ProcessHeap, 0, Entry);
        return FALSE;
    }

    wcsncpy(Entry->TargetDirectory, TargetDirectory, Length);
    Entry->TargetDirectory[Length] = (WCHAR)0;

    /* Copy optional target filename */
    if (TargetFilename != NULL)
    {
        Length = wcslen(TargetFilename);
        Entry->TargetFilename = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                        0,
                                                        (Length + 1) * sizeof(WCHAR));
        if (Entry->TargetFilename == NULL)
        {
            if (Entry->SourceCabinet != NULL)
            {
                RtlFreeHeap(ProcessHeap, 0, Entry->SourceCabinet);
            }

            RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
            RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
            RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);
            RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);
            RtlFreeHeap(ProcessHeap, 0, Entry);
            return FALSE;
        }

        wcsncpy(Entry->TargetFilename, TargetFilename, Length);
        Entry->TargetFilename[Length] = (WCHAR)0;
    }

    /* Append queue entry */
    if (QueueHeader->CopyHead == NULL) // && QueueHeader->CopyTail == NULL)
    {
        Entry->Prev = NULL;
        Entry->Next = NULL;
        QueueHeader->CopyHead = Entry;
        QueueHeader->CopyTail = Entry;
    }
    else
    {
        Entry->Prev = QueueHeader->CopyTail;
        Entry->Next = NULL;
        QueueHeader->CopyTail->Next = Entry;
        QueueHeader->CopyTail = Entry;
    }

    QueueHeader->CopyCount++;

    return TRUE;
}


BOOL
WINAPI
SetupCommitFileQueueW(
    HWND Owner,
    HSPFILEQ QueueHandle,
    PSP_FILE_CALLBACK_W MsgHandler,
    PVOID Context)
{
    WCHAR CabinetName[MAX_PATH];
    PFILEQUEUEHEADER QueueHeader;
    PQUEUEENTRY Entry;
    NTSTATUS Status;
    PCWSTR TargetRootPath, TargetPath;

    WCHAR FileSrcPath[MAX_PATH];
    WCHAR FileDstPath[MAX_PATH];

    TargetRootPath = ((PCOPYCONTEXT)Context)->DestinationRootPath;
    TargetPath = ((PCOPYCONTEXT)Context)->InstallPath;

    if (QueueHandle == NULL)
        return FALSE;

    QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

    MsgHandler(Context,
               SPFILENOTIFY_STARTQUEUE,
               0,
               0);

    MsgHandler(Context,
               SPFILENOTIFY_STARTSUBQUEUE,
               FILEOP_COPY,
               QueueHeader->CopyCount);

    /* Commit copy queue */
    Entry = QueueHeader->CopyHead;
    while (Entry != NULL)
    {
        /* Build the full source path */
        CombinePaths(FileSrcPath, ARRAYSIZE(FileSrcPath), 3,
                     Entry->SourceRootPath, Entry->SourcePath,
                     Entry->SourceFilename);

        /* Build the full target path */
        wcscpy(FileDstPath, TargetRootPath);
        if (Entry->TargetDirectory[0] == UNICODE_NULL)
        {
            /* Installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, TargetPath);
        }
        else if (Entry->TargetDirectory[0] == L'\\')
        {
            /* Absolute path */
            if (Entry->TargetDirectory[1] != UNICODE_NULL)
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->TargetDirectory);
        }
        else // if (Entry->TargetDirectory[0] != L'\\')
        {
            /* Path relative to the installation path */

            /* Add the installation path */
            ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 2,
                        TargetPath, Entry->TargetDirectory);
        }

        /*
         * If the file is in a cabinet, use only the destination path.
         * Otherwise possibly use a different target name.
         */
        if (Entry->SourceCabinet == NULL)
        {
            if (Entry->TargetFilename != NULL)
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->TargetFilename);
            else
                ConcatPaths(FileDstPath, ARRAYSIZE(FileDstPath), 1, Entry->SourceFilename);
        }

        /* FIXME: Do it! */
        DPRINT("Copy: '%S' ==> '%S'\n", FileSrcPath, FileDstPath);

        MsgHandler(Context,
                   SPFILENOTIFY_STARTCOPY,
                   (UINT_PTR)Entry->SourceFilename,
                   FILEOP_COPY);

        if (Entry->SourceCabinet != NULL)
        {
            /* Extract the file */
            CombinePaths(CabinetName, ARRAYSIZE(CabinetName), 3,
                         Entry->SourceRootPath, Entry->SourcePath,
                         Entry->SourceCabinet);
            Status = SetupExtractFile(CabinetName, Entry->SourceFilename, FileDstPath);
        }
        else
        {
            /* Copy the file */
            Status = SetupCopyFile(FileSrcPath, FileDstPath);
        }

        if (!NT_SUCCESS(Status))
        {
            MsgHandler(Context,
                       SPFILENOTIFY_COPYERROR,
                       (UINT_PTR)Entry->SourceFilename,
                       FILEOP_COPY);
        }
        else
        {
            MsgHandler(Context,
                       SPFILENOTIFY_ENDCOPY,
                       (UINT_PTR)Entry->SourceFilename,
                       FILEOP_COPY);
        }

        Entry = Entry->Next;
    }

    MsgHandler(Context,
               SPFILENOTIFY_ENDSUBQUEUE,
               FILEOP_COPY,
               0);

    MsgHandler(Context,
               SPFILENOTIFY_ENDQUEUE,
               0,
               0);

    return TRUE;
}

/* EOF */
