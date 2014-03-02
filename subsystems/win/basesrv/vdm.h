/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/vdm.h
 * PURPOSE:         VDM Definitions
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef __VDM_H__
#define __VDM_H__

#include <win/vdm.h>

/* DEFINITIONS ****************************************************************/

#define VDM_POLICY_KEY_NAME L"Software\\Policies\\Microsoft\\Windows\\AppCompat"
#define VDM_DISALLOWED_VALUE_NAME L"VDMDisallowed"

typedef struct _VDM_CONSOLE_RECORD
{
    LIST_ENTRY Entry;
    HANDLE ConsoleHandle;
    PCHAR CurrentDirs;
    ULONG CurDirsLength;
    ULONG SessionId;
    LIST_ENTRY DosListHead;
    // TODO: Structure incomplete!!!
} VDM_CONSOLE_RECORD, *PVDM_CONSOLE_RECORD;

typedef struct _VDM_DOS_RECORD
{
    LIST_ENTRY Entry;
    USHORT State;
    ULONG ExitCode;
    HANDLE ServerEvent;
    HANDLE ClientEvent;
    PVDM_COMMAND_INFO CommandInfo;
    // TODO: Structure incomplete!!!
} VDM_DOS_RECORD, *PVDM_DOS_RECORD;

/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI BaseSrvGetConsoleRecord(HANDLE ConsoleHandle, PVDM_CONSOLE_RECORD *Record);
VOID NTAPI BaseInitializeVDM(VOID);

#endif // __VDM_H__
