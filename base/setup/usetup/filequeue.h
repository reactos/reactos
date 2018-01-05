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
 * PROGRAMMER:
 */

#pragma once

#define SPFILENOTIFY_STARTQUEUE         0x00000001
#define SPFILENOTIFY_ENDQUEUE           0x00000002
#define SPFILENOTIFY_STARTSUBQUEUE      0x00000003
#define SPFILENOTIFY_ENDSUBQUEUE        0x00000004

#define SPFILENOTIFY_STARTDELETE        0x00000005
#define SPFILENOTIFY_ENDDELETE          0x00000006
#define SPFILENOTIFY_DELETEERROR        0x00000007

#define SPFILENOTIFY_STARTRENAME        0x00000008
#define SPFILENOTIFY_ENDRENAME          0x00000009
#define SPFILENOTIFY_RENAMEERROR        0x0000000a

#define SPFILENOTIFY_STARTCOPY          0x0000000b
#define SPFILENOTIFY_ENDCOPY            0x0000000c
#define SPFILENOTIFY_COPYERROR          0x0000000d

#define SPFILENOTIFY_NEEDMEDIA          0x0000000e
#define SPFILENOTIFY_QUEUESCAN          0x0000000f

#define FILEOP_COPY                     0
#define FILEOP_RENAME                   1
#define FILEOP_DELETE                   2
#define FILEOP_BACKUP                   3

#define FILEOP_ABORT                    0
#define FILEOP_DOIT                     1
#define FILEOP_SKIP                     2
#define FILEOP_RETRY                    FILEOP_DOIT
#define FILEOP_NEWPATH                  4


/* TYPES ********************************************************************/

typedef PVOID HSPFILEQ;

typedef struct _FILEPATHS_W
{
    PCWSTR Target;
    PCWSTR Source;
    UINT   Win32Error;
    ULONG  Flags;
} FILEPATHS_W, *PFILEPATHS_W;

typedef UINT (CALLBACK* PSP_FILE_CALLBACK_W)(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2);


/* FUNCTIONS ****************************************************************/

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

/* A simplified version of SetupQueueCopyW that wraps Cabinet support around */
BOOL
WINAPI
SetupQueueCopyWithCab(          // SetupQueueCopyW
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourceCabinet OPTIONAL,
    IN PCWSTR SourceRootPath,
    IN PCWSTR SourcePath OPTIONAL,
    IN PCWSTR SourceFileName,
    IN PCWSTR TargetDirectory,
    IN PCWSTR TargetFileName OPTIONAL);

BOOL
WINAPI
SetupQueueDeleteW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR PathPart1,
    IN PCWSTR PathPart2 OPTIONAL);

BOOL
WINAPI
SetupQueueRenameW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourcePath,
    IN PCWSTR SourceFileName OPTIONAL,
    IN PCWSTR TargetPath OPTIONAL,
    IN PCWSTR TargetFileName);

BOOL
WINAPI
SetupCommitFileQueueW(
    IN HWND Owner,
    IN HSPFILEQ QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID Context OPTIONAL);

/* EOF */
