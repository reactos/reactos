/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            include/reactos/subsys/win/basemsg.h
 * PURPOSE:         Public definitions for communication
 *                  between Base API Clients and Servers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _BASEMSG_H
#define _BASEMSG_H

#pragma once

#define BASESRV_SERVERDLL_INDEX     1
#define BASESRV_FIRST_API_NUMBER    0

// Windows Server 2003 table from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
typedef enum _BASESRV_API_NUMBER
{
    BasepCreateProcess = BASESRV_FIRST_API_NUMBER,
    BasepCreateThread,
    BasepGetTempFile,
    BasepExitProcess,
    // BasepDebugProcess,
    // BasepCheckVDM,
    // BasepUpdateVDMEntry,
    // BasepGetNextVDMCommand,
    // BasepExitVDM,
    // BasepIsFirstVDM,
    // BasepGetVDMExitCode,
    // BasepSetReenterCount,
    BasepSetProcessShutdownParam,
    BasepGetProcessShutdownParam,
    // BasepNlsSetUserInfo,
    // BasepNlsSetMultipleUserInfo,
    // BasepNlsCreateSection,
    // BasepSetVDMCurDirs,
    // BasepGetVDMCurDirs,
    // BasepBatNotification,
    // BasepRegisterWowExec,
    BasepSoundSentryNotification,
    // BasepRefreshIniFileMapping,
    BasepDefineDosDevice,
    // BasepSetTermsrvAppInstallMode,
    // BasepNlsUpdateCacheCount,
    // BasepSetTermsrvClientTimeZone,
    // BasepSxsCreateActivationContext,
    // BasepRegisterThread,
    // BasepNlsGetUserInfo,

    BasepMaxApiNumber
} BASESRV_API_NUMBER, *PBASESRV_API_NUMBER;

typedef struct _BASE_SXS_CREATEPROCESS_MSG
{
    ULONG Flags;
    ULONG ProcessParameterFlags;
    HANDLE FileHandle;    
    UNICODE_STRING SxsWin32ExePath;
    UNICODE_STRING SxsNtExePath;
    SIZE_T OverrideManifestOffset;
    ULONG OverrideManifestSize;
    SIZE_T OverridePolicyOffset;
    ULONG OverridePolicySize;
    PVOID PEManifestAddress;
    ULONG PEManifestSize;
    UNICODE_STRING CultureFallbacks;
    ULONG Unknown[7];
    UNICODE_STRING AssemblyName;
} BASE_SXS_CREATEPROCESS_MSG, *PBASE_SXS_CREATEPROCESS_MSG;

typedef struct
{
    //
    // NT-type structure (BASE_CREATEPROCESS_MSG)
    //
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    ULONG CreationFlags;
    ULONG VdmBinaryType;
    ULONG VdmTask;
    HANDLE hVDM;
    BASE_SXS_CREATEPROCESS_MSG Sxs;
    PVOID PebAddressNative;
    ULONG PebAddressWow64;
    USHORT ProcessorArchitecture;
} BASE_CREATE_PROCESS, *PBASE_CREATE_PROCESS;

typedef struct
{
    CLIENT_ID ClientId;
    HANDLE ThreadHandle;
} BASE_CREATE_THREAD, *PBASE_CREATE_THREAD;

typedef struct
{
    UINT uExitCode;
} BASE_EXIT_PROCESS, *PBASE_EXIT_PROCESS;

typedef struct
{
    UINT UniqueID;
} BASE_GET_TEMP_FILE, *PBASE_GET_TEMP_FILE;

typedef struct
{
    ULONG iTask;
    HANDLE ConsoleHandle;
    ULONG BinaryType;
    HANDLE WaitObjectForParent;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG CodePage;
    ULONG dwCreationFlags;
    PCHAR CmdLine;
    PCHAR appName;
    PCHAR PifFile;
    PCHAR CurDirectory;
    PCHAR Env;
    ULONG EnvLen;
    PVOID StartupInfo;
    PCHAR Desktop;
    ULONG DesktopLen;
    PCHAR Title;
    ULONG TitleLen;
    PCHAR Reserved;
    ULONG ReservedLen;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT CurDrive;
    USHORT VDMState;
} BASE_CHECK_VDM, *PBASE_CHECK_VDM;

typedef struct
{
    ULONG iTask;
    ULONG BinaryType;
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    HANDLE WaitObjectForParent;
    USHORT EntryIndex;
    USHORT VDMCreationState;
} BASE_UPDATE_VDM_ENTRY, *PBASE_UPDATE_VDM_ENTRY;

typedef struct
{
    HANDLE ConsoleHandle;
    HANDLE hParent;
    ULONG ExitCode;
} BASE_GET_VDM_EXIT_CODE, *PBASE_GET_VDM_EXIT_CODE;

typedef struct
{
    DWORD Level;
    DWORD Flags;
} BASE_SET_PROCESS_SHUTDOWN_PARAMS, *PBASE_SET_PROCESS_SHUTDOWN_PARAMS;

typedef struct
{
    DWORD Level;
    DWORD Flags;
} BASE_GET_PROCESS_SHUTDOWN_PARAMS, *PBASE_GET_PROCESS_SHUTDOWN_PARAMS;

typedef struct
{
    ULONG VideoMode;
} BASE_SOUND_SENTRY, *PBASE_SOUND_SENTRY;

typedef struct
{
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetName;
    DWORD dwFlags;
} BASE_DEFINE_DOS_DEVICE, *PBASE_DEFINE_DOS_DEVICE;

typedef struct _BASE_API_MESSAGE
{
    PORT_MESSAGE Header;

    PCSR_CAPTURE_BUFFER CsrCaptureData;
    CSR_API_NUMBER ApiNumber;
    NTSTATUS Status;
    ULONG Reserved;
    union
    {
        BASE_CREATE_PROCESS CreateProcessRequest;
        BASE_CREATE_THREAD CreateThreadRequest;
        BASE_EXIT_PROCESS ExitProcessRequest;
        BASE_GET_TEMP_FILE GetTempFile;
        BASE_CHECK_VDM CheckVdm;
        BASE_UPDATE_VDM_ENTRY UpdateVdmEntry;
        BASE_GET_VDM_EXIT_CODE GetVdmExitCode;
        BASE_SET_PROCESS_SHUTDOWN_PARAMS SetShutdownParametersRequest;
        BASE_GET_PROCESS_SHUTDOWN_PARAMS GetShutdownParametersRequest;
        BASE_SOUND_SENTRY SoundSentryRequest;
        BASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest;
    } Data;
} BASE_API_MESSAGE, *PBASE_API_MESSAGE;

// Check that a BASE_API_MESSAGE can hold in a CSR_API_MESSAGE.
CHECK_API_MSG_SIZE(BASE_API_MESSAGE);

#endif // _BASEMSG_H

/* EOF */
