/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/include/vdm.h
 * PURPOSE:         Virtual DOS Machines (VDM) Support Definitions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#pragma once

/* FUNCTION PROTOTYPES ********************************************************/

BOOL
NTAPI
BaseCreateVDMEnvironment(
    IN PWCHAR lpEnvironment,
    OUT PANSI_STRING AnsiEnv,
    OUT PUNICODE_STRING UnicodeEnv
);

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

NTSTATUS
WINAPI
BaseCheckVDM(
    IN ULONG BinaryType,
    IN PCWCH ApplicationName,
    IN PCWCH CommandLine,
    IN PCWCH CurrentDirectory,
    IN PANSI_STRING AnsiEnvironment,
    IN PBASE_API_MESSAGE ApiMessage,
    IN OUT PULONG iTask,
    IN DWORD CreationFlags,
    IN LPSTARTUPINFOW StartupInfo,
    IN HANDLE hUserToken OPTIONAL
);

/* EOF */
