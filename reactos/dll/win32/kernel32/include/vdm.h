/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/include/vdm.h
 * PURPOSE:         Virtual DOS Machines (VDM) Support Definitions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#pragma once

/* CONSTANTS ******************************************************************/

typedef enum _VDM_ENTRY_CODE
{
    VdmEntryUndo,
    VdmEntryUpdateProcess,
    VdmEntryUpdateControlCHandler
} VDM_ENTRY_CODE;

//
// Undo States
//
#define VDM_UNDO_PARTIAL    0x01
#define VDM_UNDO_FULL       0x02
#define VDM_UNDO_REUSE      0x04
#define VDM_UNDO_COMPLETED  0x08

//
// Binary Types to share with VDM
//
#define BINARY_TYPE_EXE     0x01
#define BINARY_TYPE_COM     0x02
#define BINARY_TYPE_PIF     0x03
#define BINARY_TYPE_DOS     0x10
#define BINARY_TYPE_SEPARATE_WOW 0x20
#define BINARY_TYPE_WOW     0x40
#define BINARY_TYPE_WOW_EX  0x80

//
// VDM States
//
#define VDM_NOT_LOADED      0x01
#define VDM_NOT_READY       0x02
#define VDM_READY           0x04


/* FUNCTION PROTOTYPES ********************************************************/

BOOL
NTAPI
BaseDestroyVDMEnvironment(
    IN PANSI_STRING AnsiEnv,
    IN PUNICODE_STRING UnicodeEnv
);

BOOL
WINAPI
BaseGetVdmConfigInfo(
    IN LPCWSTR CommandLineReserved,
    IN ULONG DosSeqId,
    IN ULONG BinaryType,
    IN PUNICODE_STRING CmdLineString,
    OUT PULONG VdmSize
);

BOOL
NTAPI
BaseCreateVDMEnvironment(
    IN PWCHAR lpEnvironment,
    IN PANSI_STRING AnsiEnv,
    IN PUNICODE_STRING UnicodeEnv
);

BOOL
WINAPI
BaseUpdateVDMEntry(
    IN ULONG UpdateIndex,
    IN OUT PHANDLE WaitHandle,
    IN ULONG IndexInfo,
    IN ULONG BinaryType
);

BOOL
WINAPI
BaseCheckForVDM(
    IN HANDLE ProcessHandle,
    OUT LPDWORD ExitCode
);

BOOL
WINAPI
BaseCheckVDM(
    IN ULONG BinaryType,
    IN PCWCH ApplicationName,
    IN PCWCH CommandLine,
    IN PCWCH CurrentDirectory,
    IN PANSI_STRING AnsiEnvironment,
    IN PCSR_API_MESSAGE ApiMessage,
    IN OUT PULONG iTask,
    IN DWORD CreationFlags,
    IN LPSTARTUPINFOW StartupInfo,
    IN HANDLE hUserToken OPTIONAL
);

/* EOF */
