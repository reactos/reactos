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
 * FILE:            base/setup/usetup/filequeue.h
 * PURPOSE:         File queue functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

#define SPFILENOTIFY_STARTQUEUE       0x1
#define SPFILENOTIFY_ENDQUEUE         0x2
#define SPFILENOTIFY_STARTSUBQUEUE    0x3
#define SPFILENOTIFY_ENDSUBQUEUE      0x4

#define SPFILENOTIFY_STARTCOPY        0xb
#define SPFILENOTIFY_ENDCOPY          0xc
#define SPFILENOTIFY_COPYERROR        0xd

#define FILEOP_COPY                   0x0
#define FILEOP_RENAME                 0x1
#define FILEOP_DELETE                 0x2
#define FILEOP_BACKUP                 0x3

#define FILEOP_ABORT                  0x0
#define FILEOP_DOIT                   0x1
#define FILEOP_SKIP                   0x2
#define FILEOP_RETRY                  FILEOP_DOIT
#define FILEOP_NEWPATH                0x4


/* TYPES ********************************************************************/

typedef PVOID HSPFILEQ;

typedef UINT (CALLBACK* PSP_FILE_CALLBACK_W)(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2);

typedef struct _COPYCONTEXT
{
    LPCWSTR DestinationRootPath; /* Not owned by this structure */
    LPCWSTR InstallPath;         /* Not owned by this structure */
    ULONG TotalOperations;
    ULONG CompletedOperations;
    PPROGRESSBAR ProgressBar;
    PPROGRESSBAR MemoryBars[4];
} COPYCONTEXT, *PCOPYCONTEXT;

/* FUNCTIONS ****************************************************************/

NTSTATUS
SetupExtractFile(
    PWCHAR CabinetFileName,
    PWCHAR SourceFileName,
    PWCHAR DestinationFileName);

HSPFILEQ
WINAPI
SetupOpenFileQueue(VOID);

VOID
WINAPI
SetupCloseFileQueue(
    IN HSPFILEQ QueueHandle);

#if 0 // This is the API that is declared in setupapi.h and exported by setupapi.dll
BOOL
WINAPI
SetupQueueCopyWNew(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourceRootPath,
    IN PCWSTR SourcePath,
    IN PCWSTR SourceFileName,
    IN PCWSTR SourceDescription,
    IN PCWSTR SourceTagFile,
    IN PCWSTR TargetDirectory,
    IN PCWSTR TargetFileName,
    IN DWORD CopyStyle);
#endif

BOOL
SetupQueueCopy(
    HSPFILEQ QueueHandle,
    PCWSTR SourceCabinet,
    PCWSTR SourceRootPath,
    PCWSTR SourcePath,
    PCWSTR SourceFilename,
    PCWSTR TargetDirectory,
    PCWSTR TargetFilename);

BOOL
WINAPI
SetupCommitFileQueueW(
    HWND Owner,
    HSPFILEQ QueueHandle,
    PSP_FILE_CALLBACK_W MsgHandler,
    PVOID Context);

/* EOF */
