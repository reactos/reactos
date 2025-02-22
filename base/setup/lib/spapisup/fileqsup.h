/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/fileqsup.h
 * PURPOSE:         Interfacing with Setup* API File Queue support functions
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#include "spapisup.h"

// FIXME: Temporary measure until all the users of this header
// (usetup...) use or define SetupAPI-conforming APIs.
#if defined(_SETUPAPI_H_) || defined(_INC_SETUPAPI)

#include <setupapi.h>

#else

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


/* TYPES *********************************************************************/

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

#endif


/* FUNCTIONS *****************************************************************/

// #define SetupOpenFileQueue
typedef HSPFILEQ
(WINAPI* pSpFileQueueOpen)(VOID);

// #define SetupCloseFileQueue
typedef BOOL
(WINAPI* pSpFileQueueClose)(
    IN HSPFILEQ QueueHandle);

// #define SetupQueueCopyW
typedef BOOL
(WINAPI* pSpFileQueueCopy)(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourceRootPath,
    IN PCWSTR SourcePath OPTIONAL,
    IN PCWSTR SourceFileName,
    IN PCWSTR SourceDescription OPTIONAL,
    IN PCWSTR SourceCabinet OPTIONAL,
    IN PCWSTR SourceTagFile OPTIONAL,
    IN PCWSTR TargetDirectory,
    IN PCWSTR TargetFileName OPTIONAL,
    IN ULONG CopyStyle);

// #define SetupQueueDeleteW
typedef BOOL
(WINAPI* pSpFileQueueDelete)(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR PathPart1,
    IN PCWSTR PathPart2 OPTIONAL);

// #define SetupQueueRenameW
typedef BOOL
(WINAPI* pSpFileQueueRename)(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourcePath,
    IN PCWSTR SourceFileName OPTIONAL,
    IN PCWSTR TargetPath OPTIONAL,
    IN PCWSTR TargetFileName);

// #define SetupCommitFileQueueW
typedef BOOL
(WINAPI* pSpFileQueueCommit)(
    IN HWND Owner,
    IN HSPFILEQ QueueHandle,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID Context OPTIONAL);

typedef struct _SPFILE_EXPORTS
{
    pSpFileQueueOpen   SpFileQueueOpen;
    pSpFileQueueClose  SpFileQueueClose;
    pSpFileQueueCopy   SpFileQueueCopy;
    pSpFileQueueDelete SpFileQueueDelete;
    pSpFileQueueRename SpFileQueueRename;
    pSpFileQueueCommit SpFileQueueCommit;
} SPFILE_EXPORTS, *PSPFILE_EXPORTS;

extern /*SPLIBAPI*/ SPFILE_EXPORTS SpFileExports;

#define SpFileQueueOpen     (SpFileExports.SpFileQueueOpen)
#define SpFileQueueClose    (SpFileExports.SpFileQueueClose)
#define SpFileQueueCopy     (SpFileExports.SpFileQueueCopy)
#define SpFileQueueDelete   (SpFileExports.SpFileQueueDelete)
#define SpFileQueueRename   (SpFileExports.SpFileQueueRename)
#define SpFileQueueCommit   (SpFileExports.SpFileQueueCommit)

/* EOF */
