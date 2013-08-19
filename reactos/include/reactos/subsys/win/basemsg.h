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


typedef struct
{
    ULONG  ExpectedVersion;
    HANDLE DefaultObjectDirectory;
    ULONG  WindowsVersion;
    ULONG  CurrentVersion;
    ULONG  DebugFlags;
    WCHAR  WindowsDirectory[MAX_PATH];
    WCHAR  WindowsSystemDirectory[MAX_PATH];
} BASESRV_API_CONNECTINFO, *PBASESRV_API_CONNECTINFO;

#define BASESRV_VERSION 0x10000


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
    UINT UniqueID;
} BASE_GET_TEMP_FILE, *PBASE_GET_TEMP_FILE;

typedef struct
{
    UINT uExitCode;
} BASE_EXIT_PROCESS, *PBASE_EXIT_PROCESS;

typedef struct
{
    ULONG  iTask;
    HANDLE ConsoleHandle;
    ULONG  BinaryType;
    HANDLE WaitObjectForParent;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    PCHAR  CmdLine;
    PCHAR  appName;
    PCHAR  PifFile;
    PCHAR  CurDirectory;
    PCHAR  Env;
    ULONG  EnvLen;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT CurDrive;
    USHORT VDMState;
} BASE_CHECK_VDM, *PBASE_CHECK_VDM;

typedef struct
{
    ULONG  iTask;
    ULONG  BinaryType;
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    HANDLE WaitObjectForParent;
    USHORT EntryIndex;
    USHORT VDMCreationState;
} BASE_UPDATE_VDM_ENTRY, *PBASE_UPDATE_VDM_ENTRY;

typedef struct
{
    ULONG  iTask;
    HANDLE ConsoleHandle;
    HANDLE WaitObjectForVDM;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    ULONG  ExitCode;
    PCHAR  CmdLine;
    PCHAR  AppName;
    PCHAR  PifFile;
    PCHAR  CurDirectory;
    PCHAR  Env;
    ULONG  EnvLen;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    USHORT CurrentDrive;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT VDMState;
    ULONG  fComingFromBat;
} BASE_GET_NEXT_VDM_COMMAND, *PBASE_GET_NEXT_VDM_COMMAND;

typedef struct
{
    HANDLE ConsoleHandle;
    ULONG  iWowTask;
    HANDLE WaitObjectForVDM;
} BASE_EXIT_VDM, *PBASE_EXIT_VDM;

typedef struct
{
    ULONG FirstVDM;
} BASE_IS_FIRST_VDM, *PBASE_IS_FIRST_VDM;

typedef struct
{
    HANDLE ConsoleHandle;
    HANDLE hParent;
    ULONG  ExitCode;
} BASE_GET_VDM_EXIT_CODE, *PBASE_GET_VDM_EXIT_CODE;

typedef struct
{
    HANDLE ConsoleHandle;
    ULONG  fIncDec;
} BASE_SET_REENTER_COUNT, *PBASE_SET_REENTER_COUNT;

typedef struct
{
    ULONG ShutdownLevel;
    ULONG ShutdownFlags;
} BASE_GETSET_PROCESS_SHUTDOWN_PARAMS, *PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS;

typedef struct
{
    HANDLE ConsoleHandle;
    PCHAR  lpszzCurDirs;
    ULONG  cchCurDirs;
} BASE_GETSET_VDM_CURDIRS, *PBASE_GETSET_VDM_CURDIRS;

typedef struct
{
    HANDLE ConsoleHandle;
    ULONG  fBeginEnd;
} BASE_BAT_NOTIFICATION, *PBASE_BAT_NOTIFICATION;

typedef struct
{
    HANDLE hwndWowExec;
} BASE_REGISTER_WOWEXEC, *PBASE_REGISTER_WOWEXEC;

typedef struct
{
    ULONG VideoMode;
} BASE_SOUND_SENTRY, *PBASE_SOUND_SENTRY;

typedef struct
{
    UNICODE_STRING IniFileName;
} BASE_REFRESH_INIFILE_MAPPING, *PBASE_REFRESH_INIFILE_MAPPING;

typedef struct
{
    ULONG Flags;
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
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
        BASE_GET_TEMP_FILE GetTempFileRequest;
        BASE_EXIT_PROCESS ExitProcessRequest;
        BASE_CHECK_VDM CheckVDMRequest;
        BASE_UPDATE_VDM_ENTRY UpdateVDMEntryRequest;
        BASE_GET_NEXT_VDM_COMMAND GetNextVDMCommandRequest;
        BASE_EXIT_VDM ExitVDMRequest;
        BASE_IS_FIRST_VDM IsFirstVDMRequest;
        BASE_GET_VDM_EXIT_CODE GetVDMExitCodeRequest;
        BASE_SET_REENTER_COUNT SetReenterCountRequest;
        BASE_GETSET_PROCESS_SHUTDOWN_PARAMS ShutdownParametersRequest;
        BASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest;
        BASE_BAT_NOTIFICATION BatNotificationRequest;
        BASE_REGISTER_WOWEXEC RegisterWowExecRequest;
        BASE_SOUND_SENTRY SoundSentryRequest;
        BASE_REFRESH_INIFILE_MAPPING RefreshIniFileMappingRequest;
        BASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest;
    } Data;
} BASE_API_MESSAGE, *PBASE_API_MESSAGE;

// Check that a BASE_API_MESSAGE can hold in a CSR_API_MESSAGE.
CHECK_API_MSG_SIZE(BASE_API_MESSAGE);

#endif // _BASEMSG_H

/* EOF */
