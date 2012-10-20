/*** Private header for BASESRV ***/

#pragma once

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* CSRSS Header */
#include <csrss/server.h>


#include <win/base.h>



#define BASESRV_SERVERDLL_INDEX         1
#define BASESRV_FIRST_API_NUMBER        0


// Windows NT 4 table, adapted from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_NT
// It is for testing purposes. After that I will update it to 2k3 version and add stubs.
typedef enum _BASESRV_API_NUMBER
{
    BasepCreateProcess = BASESRV_FIRST_API_NUMBER,
    BasepCreateThread,
    BasepGetTempFile,
    BasepExitProcess,
    // BasepDebugProcess,
    BasepCheckVDM,
    BasepUpdateVDMEntry,
    // BasepGetNextVDMCommand,
    // BasepExitVDM,
    // BasepIsFirstVDM,
    BasepGetVDMExitCode,
    // BasepSetReenterCount,
    BasepSetProcessShutdownParam,
    BasepGetProcessShutdownParam,
    // BasepNlsSetUserInfo,
    // BasepNlsSetMultipleUserInfo,
    // BasepNlsCreateSortSection,
    // BasepNlsPreserveSection,
    // BasepSetVDMCurDirs,
    // BasepGetVDMCurDirs,
    // BasepBatNotification,
    // BasepRegisterWowExec,
    BasepSoundSentryNotification,
    // BasepRefreshIniFileMapping,
    BasepDefineDosDevice,

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

    //
    // ReactOS Data
    //
    BOOL bInheritHandles;
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
    ULONG Status;
    ULONG Reserved;
    union
    {
        BASE_CREATE_PROCESS CreateProcessRequest;
        BASE_CREATE_THREAD CreateThreadRequest;
        BASE_EXIT_PROCESS ExitProcessRequest; // CSRSS_TERMINATE_PROCESS TerminateProcessRequest;
        BASE_GET_TEMP_FILE GetTempFile;
        BASE_CHECK_VDM CheckVdm;
        BASE_UPDATE_VDM_ENTRY UpdateVdmEntry;
        BASE_GET_VDM_EXIT_CODE GetVdmExitCode;
        BASE_SET_PROCESS_SHUTDOWN_PARAMS SetShutdownParametersRequest; // CSRSS_SET_SHUTDOWN_PARAMETERS
        BASE_GET_PROCESS_SHUTDOWN_PARAMS GetShutdownParametersRequest; // CSRSS_GET_SHUTDOWN_PARAMETERS
        BASE_SOUND_SENTRY SoundSentryRequest;
        BASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest;
    } Data;
} BASE_API_MESSAGE, *PBASE_API_MESSAGE;








typedef struct _NLS_USER_INFO
{
    WCHAR iCountry[80];
    WCHAR sCountry[80];
    WCHAR sList[80];
    WCHAR iMeasure[80];
    WCHAR iPaperSize[80];
    WCHAR sDecimal[80];
    WCHAR sThousand[80];
    WCHAR sGrouping[80];
    WCHAR iDigits[80];
    WCHAR iLZero[80];
    WCHAR iNegNumber[80];
    WCHAR sNativeDigits[80];
    WCHAR iDigitSubstitution[80];
    WCHAR sCurrency[80];
    WCHAR sMonDecSep[80];
    WCHAR sMonThouSep[80];
    WCHAR sMonGrouping[80];
    WCHAR iCurrDigits[80];
    WCHAR iCurrency[80];
    WCHAR iNegCurr[80];
    WCHAR sPosSign[80];
    WCHAR sNegSign[80];
    WCHAR sTimeFormat[80];
    WCHAR s1159[80];
    WCHAR s2359[80];
    WCHAR sShortDate[80];
    WCHAR sYearMonth[80];
    WCHAR sLongDate[80];
    WCHAR iCalType[80];
    WCHAR iFirstDay[80];
    WCHAR iFirstWeek[80];
    WCHAR sLocale[80];
    WCHAR sLocaleName[85];
    LCID UserLocaleId;
    LUID InteractiveUserLuid;
    CHAR InteractiveUserSid[68]; // SECURITY_MAX_SID_SIZE to make ROS happy
    ULONG ulCacheUpdateCount;
} NLS_USER_INFO, *PNLS_USER_INFO;


typedef struct _BASE_STATIC_SERVER_DATA
{
    UNICODE_STRING WindowsDirectory;
    UNICODE_STRING WindowsSystemDirectory;
    UNICODE_STRING NamedObjectDirectory;
    USHORT WindowsMajorVersion;
    USHORT WindowsMinorVersion;
    USHORT BuildNumber;
    USHORT CSDNumber;
    USHORT RCNumber;
    WCHAR CSDVersion[128];
    SYSTEM_BASIC_INFORMATION SysInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;
    PVOID IniFileMapping;
    NLS_USER_INFO NlsUserInfo;
    BOOLEAN DefaultSeparateVDM;
    BOOLEAN IsWowTaskReady;
    UNICODE_STRING WindowsSys32x86Directory;
    BOOLEAN fTermsrvAppInstallMode;
    TIME_ZONE_INFORMATION tziTermsrvClientTimeZone;
    KSYSTEM_TIME ktTermsrvClientBias;
    ULONG TermsrvClientTimeZoneId;
    BOOLEAN LUIDDeviceMapsEnabled;
    ULONG TermsrvClientTimeZoneChangeNum;
} BASE_STATIC_SERVER_DATA, *PBASE_STATIC_SERVER_DATA;


