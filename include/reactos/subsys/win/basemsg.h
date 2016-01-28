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
    BasepDebugProcess,  // Deprecated
    BasepCheckVDM,
    BasepUpdateVDMEntry,
    BasepGetNextVDMCommand,
    BasepExitVDM,
    BasepIsFirstVDM,
    BasepGetVDMExitCode,
    BasepSetReenterCount,
    BasepSetProcessShutdownParam,
    BasepGetProcessShutdownParam,
    BasepNlsSetUserInfo,
    BasepNlsSetMultipleUserInfo,
    BasepNlsCreateSection,
    BasepSetVDMCurDirs,
    BasepGetVDMCurDirs,
    BasepBatNotification,
    BasepRegisterWowExec,
    BasepSoundSentryNotification,
    BasepRefreshIniFileMapping,
    BasepDefineDosDevice,
    BasepSetTermsrvAppInstallMode,
    BasepNlsUpdateCacheCount,
    BasepSetTermsrvClientTimeZone,
    BasepSxsCreateActivationContext,
    BasepDebugProcessStop, // Alias to BasepDebugProcess, deprecated
    BasepRegisterThread,
    BasepNlsGetUserInfo,

    BasepMaxApiNumber
} BASESRV_API_NUMBER, *PBASESRV_API_NUMBER;

typedef struct _BASESRV_API_CONNECTINFO
{
    ULONG DebugFlags;
} BASESRV_API_CONNECTINFO, *PBASESRV_API_CONNECTINFO;

#if defined(_M_IX86)
C_ASSERT(sizeof(BASESRV_API_CONNECTINFO) == 0x04);
#endif


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

typedef struct _BASE_CREATE_PROCESS
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

typedef struct _BASE_CREATE_THREAD
{
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
} BASE_CREATE_THREAD, *PBASE_CREATE_THREAD;

typedef struct _BASE_GET_TEMP_FILE
{
    UINT UniqueID;
} BASE_GET_TEMP_FILE, *PBASE_GET_TEMP_FILE;

typedef struct _BASE_EXIT_PROCESS
{
    UINT uExitCode;
} BASE_EXIT_PROCESS, *PBASE_EXIT_PROCESS;

typedef struct _BASE_CHECK_VDM
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
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT CurDrive;
    USHORT VDMState;
} BASE_CHECK_VDM, *PBASE_CHECK_VDM;

typedef struct _BASE_UPDATE_VDM_ENTRY
{
    ULONG  iTask;
    ULONG  BinaryType;
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    HANDLE WaitObjectForParent;
    USHORT EntryIndex;
    USHORT VDMCreationState;
} BASE_UPDATE_VDM_ENTRY, *PBASE_UPDATE_VDM_ENTRY;

typedef struct _BASE_GET_NEXT_VDM_COMMAND
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

typedef struct _BASE_EXIT_VDM
{
    HANDLE ConsoleHandle;
    ULONG  iWowTask;
    HANDLE WaitObjectForVDM;
} BASE_EXIT_VDM, *PBASE_EXIT_VDM;

typedef struct _BASE_IS_FIRST_VDM
{
    ULONG FirstVDM;
} BASE_IS_FIRST_VDM, *PBASE_IS_FIRST_VDM;

typedef struct _BASE_GET_VDM_EXIT_CODE
{
    HANDLE ConsoleHandle;
    HANDLE hParent;
    ULONG  ExitCode;
} BASE_GET_VDM_EXIT_CODE, *PBASE_GET_VDM_EXIT_CODE;

typedef struct _BASE_SET_REENTER_COUNT
{
    HANDLE ConsoleHandle;
    ULONG  fIncDec;
} BASE_SET_REENTER_COUNT, *PBASE_SET_REENTER_COUNT;

typedef struct _BASE_GETSET_PROCESS_SHUTDOWN_PARAMS
{
    ULONG ShutdownLevel;
    ULONG ShutdownFlags;
} BASE_GETSET_PROCESS_SHUTDOWN_PARAMS, *PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS;

typedef struct _BASE_GETSET_VDM_CURDIRS
{
    HANDLE ConsoleHandle;
    PCHAR  lpszzCurDirs;
    ULONG  cchCurDirs;
} BASE_GETSET_VDM_CURDIRS, *PBASE_GETSET_VDM_CURDIRS;

typedef struct _BASE_BAT_NOTIFICATION
{
    HANDLE ConsoleHandle;
    ULONG  fBeginEnd;
} BASE_BAT_NOTIFICATION, *PBASE_BAT_NOTIFICATION;

typedef struct _BASE_REGISTER_WOWEXEC
{
    HANDLE hwndWowExec;
} BASE_REGISTER_WOWEXEC, *PBASE_REGISTER_WOWEXEC;

typedef struct _BASE_SOUND_SENTRY
{
    ULONG VideoMode;
} BASE_SOUND_SENTRY, *PBASE_SOUND_SENTRY;

typedef struct _BASE_REFRESH_INIFILE_MAPPING
{
    UNICODE_STRING IniFileName;
} BASE_REFRESH_INIFILE_MAPPING, *PBASE_REFRESH_INIFILE_MAPPING;

typedef struct _BASE_DEFINE_DOS_DEVICE
{
    ULONG Flags;
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
} BASE_DEFINE_DOS_DEVICE, *PBASE_DEFINE_DOS_DEVICE;

typedef struct _BASE_NLS_CREATE_SECTION
{
    HANDLE SectionHandle;
    ULONG Type;
    ULONG LocaleId;
} BASE_NLS_CREATE_SECTION, *PBASE_NLS_CREATE_SECTION;

typedef struct _BASE_NLS_GET_USER_INFO
{
    PVOID /*PNLS_USER_INFO*/ NlsUserInfo;
    ULONG Size;
} BASE_NLS_GET_USER_INFO, *PBASE_NLS_GET_USER_INFO;

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
        BASE_NLS_CREATE_SECTION NlsCreateSection;
        BASE_NLS_GET_USER_INFO NlsGetUserInfo;
    } Data;
} BASE_API_MESSAGE, *PBASE_API_MESSAGE;

// Check that a BASE_API_MESSAGE can hold in a CSR_API_MESSAGE.
CHECK_API_MSG_SIZE(BASE_API_MESSAGE);

#endif // _BASEMSG_H

/* EOF */
