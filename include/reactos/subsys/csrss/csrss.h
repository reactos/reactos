/*****************************  CSRSS Data  ***********************************/

#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

#define CSR_NATIVE     0x0000
#define CSR_CONSOLE    0x0001
#define CSR_GUI        0x0002
#define CONSOLE_INPUT_MODE_VALID  (0x0f)
#define CONSOLE_OUTPUT_MODE_VALID (0x03)


#define CSR_CSRSS_SECTION_SIZE          (65536)

typedef VOID (CALLBACK *PCONTROLDISPATCHER)(DWORD);

typedef struct
{
    ULONG Dummy;
} CSRSS_CONNECT_PROCESS, *PCSRSS_CONNECT_PROCESS;

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
} CSRSS_CREATE_PROCESS, *PCSRSS_CREATE_PROCESS;

typedef struct
{
    CLIENT_ID ClientId;
    HANDLE ThreadHandle;
} CSRSS_CREATE_THREAD, *PCSRSS_CREATE_THREAD;

typedef struct
{
    UINT uExitCode;
} CSRSS_TERMINATE_PROCESS, *PCSRSS_TERMINATE_PROCESS;

typedef struct
{
    USHORT nMaxIds;
    PDWORD ProcessId;
    ULONG nProcessIdsTotal;
} CSRSS_GET_PROCESS_LIST, *PCSRSS_GET_PROCESS_LIST;

#include "csrcons.h"

typedef struct
{
    HANDLE  UniqueThread;
    CLIENT_ID Cid;
} CSRSS_IDENTIFY_ALERTABLE_THREAD, *PCSRSS_IDENTIFY_ALERTABLE_THREAD;

typedef struct
{
    HANDLE ProcessId;
} CSRSS_REGISTER_SERVICES_PROCESS, *PCSRSS_REGISTER_SERVICES_PROCESS;

typedef struct
{
    UINT Flags;
    DWORD Reserved;
} CSRSS_EXIT_REACTOS, *PCSRSS_EXIT_REACTOS;

typedef struct
{
    DWORD Level;
    DWORD Flags;
} CSRSS_SET_SHUTDOWN_PARAMETERS, *PCSRSS_SET_SHUTDOWN_PARAMETERS;

typedef struct
{
    DWORD Level;
    DWORD Flags;
} CSRSS_GET_SHUTDOWN_PARAMETERS, *PCSRSS_GET_SHUTDOWN_PARAMETERS;

typedef struct
{
    HANDLE Handle;
} CSRSS_CLOSE_HANDLE, *PCSRSS_CLOSE_HANDLE;

typedef struct
{
    HANDLE Handle;
} CSRSS_VERIFY_HANDLE, *PCSRSS_VERIFY_HANDLE;

typedef struct
{
    HANDLE Handle;
    DWORD Access;
    BOOL Inheritable;
    DWORD Options;
} CSRSS_DUPLICATE_HANDLE, *PCSRSS_DUPLICATE_HANDLE;

typedef struct
{
    HDESK DesktopHandle;
} CSRSS_CREATE_DESKTOP, *PCSRSS_CREATE_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
    ULONG Width;
    ULONG Height;
} CSRSS_SHOW_DESKTOP, *PCSRSS_SHOW_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
} CSRSS_HIDE_DESKTOP, *PCSRSS_HIDE_DESKTOP;

typedef struct
{
    HWND LogonNotifyWindow;
} CSRSS_SET_LOGON_NOTIFY_WINDOW, *PCSRSS_SET_LOGON_NOTIFY_WINDOW;

typedef struct
{
    HANDLE ProcessId;
    BOOL Register;
} CSRSS_REGISTER_LOGON_PROCESS, *PCSRSS_REGISTER_LOGON_PROCESS;

typedef struct
{
    HANDLE InputWaitHandle;
} CSRSS_GET_INPUT_WAIT_HANDLE, *PCSRSS_GET_INPUT_WAIT_HANDLE;

typedef struct
{
    UINT UniqueID;
} CSRSS_GET_TEMP_FILE, *PCSRSS_GET_TEMP_FILE;

typedef struct
{
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetName;
    DWORD dwFlags;
} CSRSS_DEFINE_DOS_DEVICE, *PCSRSS_DEFINE_DOS_DEVICE;

typedef struct
{
    ULONG VideoMode;
} CSRSS_SOUND_SENTRY, *PCSRSS_SOUND_SENTRY;

typedef struct
{
    ULONG iTask;
    ULONG BinaryType;
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    HANDLE WaitObjectForParent;
    USHORT EntryIndex;
    USHORT VDMCreationState;
} CSRSS_UPDATE_VDM_ENTRY, *PCSRSS_UPDATE_VDM_ENTRY;

typedef struct
{
    HANDLE ConsoleHandle;
    HANDLE hParent;
    ULONG ExitCode;
} CSRSS_GET_VDM_EXIT_CODE, *PCSRSS_GET_VDM_EXIT_CODE;

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
} CSRSS_CHECK_VDM, *PCSRSS_CHECK_VDM;

#define CSR_API_MESSAGE_HEADER_SIZE(Type)       (FIELD_OFFSET(CSR_API_MESSAGE, Data) + sizeof(Type))
#define CSRSS_MAX_WRITE_CONSOLE                 (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR     (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB   (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR      (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR))
#define CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB    (LPC_MAX_DATA_LENGTH - CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB))

#define CREATE_PROCESS                  (0x0)
#define TERMINATE_PROCESS               (0x1)
#define WRITE_CONSOLE                   (0x2)
#define READ_CONSOLE                    (0x3)
#define ALLOC_CONSOLE                   (0x4)
#define FREE_CONSOLE                    (0x5)
#define CONNECT_PROCESS                 (0x6)
#define SCREEN_BUFFER_INFO              (0x7)
#define SET_CURSOR                      (0x8)
#define FILL_OUTPUT                     (0x9)
#define READ_INPUT                      (0xA)
#define WRITE_CONSOLE_OUTPUT_CHAR       (0xB)
#define WRITE_CONSOLE_OUTPUT_ATTRIB     (0xC)
#define FILL_OUTPUT_ATTRIB              (0xD)
#define GET_CURSOR_INFO                 (0xE)
#define SET_CURSOR_INFO                 (0xF)
#define SET_ATTRIB                      (0x10)
#define GET_CONSOLE_MODE                (0x11)
#define SET_CONSOLE_MODE                (0x12)
#define CREATE_SCREEN_BUFFER            (0x13)
#define SET_SCREEN_BUFFER               (0x14)
#define SET_TITLE                       (0x15)
#define GET_TITLE                       (0x16)
#define WRITE_CONSOLE_OUTPUT            (0x17)
#define FLUSH_INPUT_BUFFER              (0x18)
#define SCROLL_CONSOLE_SCREEN_BUFFER    (0x19)
#define READ_CONSOLE_OUTPUT_CHAR        (0x1A)
#define READ_CONSOLE_OUTPUT_ATTRIB      (0x1B)
#define GET_NUM_INPUT_EVENTS            (0x1C)
#define REGISTER_SERVICES_PROCESS       (0x1D)
#define EXIT_REACTOS                    (0x1E)
#define GET_SHUTDOWN_PARAMETERS         (0x1F)
#define SET_SHUTDOWN_PARAMETERS         (0x20)
#define PEEK_CONSOLE_INPUT              (0x21)
#define READ_CONSOLE_OUTPUT             (0x22)
#define WRITE_CONSOLE_INPUT             (0x23)
#define GET_INPUT_HANDLE                (0x24)
#define GET_OUTPUT_HANDLE               (0x25)
#define CLOSE_HANDLE                    (0x26)
#define VERIFY_HANDLE                   (0x27)
#define DUPLICATE_HANDLE                (0x28)
#define SETGET_CONSOLE_HW_STATE         (0x29)
#define GET_CONSOLE_WINDOW              (0x2A)
#define CREATE_DESKTOP                  (0x2B)
#define SHOW_DESKTOP                    (0x2C)
#define HIDE_DESKTOP                    (0x2D)
#define SET_CONSOLE_ICON                (0x2E)
#define SET_LOGON_NOTIFY_WINDOW         (0x2F)
#define REGISTER_LOGON_PROCESS          (0x30)
#define GET_CONSOLE_CP                  (0x31)
#define SET_CONSOLE_CP                  (0x32)
#define GET_CONSOLE_OUTPUT_CP           (0x33)
#define SET_CONSOLE_OUTPUT_CP           (0x34)
#define GET_INPUT_WAIT_HANDLE           (0x35)
#define GET_PROCESS_LIST                (0x36)
#define START_SCREEN_SAVER              (0x37)
#define ADD_CONSOLE_ALIAS               (0x38)
#define GET_CONSOLE_ALIAS               (0x39)
#define GET_ALL_CONSOLE_ALIASES         (0x3A)
#define GET_ALL_CONSOLE_ALIASES_LENGTH  (0x3B)
#define GET_CONSOLE_ALIASES_EXES        (0x3C)
#define GET_CONSOLE_ALIASES_EXES_LENGTH (0x3D)
#define GENERATE_CTRL_EVENT             (0x3E)
#define CREATE_THREAD                   (0x3F)
#define SET_SCREEN_BUFFER_SIZE          (0x40)
#define GET_CONSOLE_SELECTION_INFO      (0x41)
#define GET_COMMAND_HISTORY_LENGTH      (0x42)
#define GET_COMMAND_HISTORY             (0x43)
#define EXPUNGE_COMMAND_HISTORY         (0x44)
#define SET_HISTORY_NUMBER_COMMANDS     (0x45)
#define GET_HISTORY_INFO                (0x46)
#define SET_HISTORY_INFO                (0x47)
#define GET_TEMP_FILE                   (0x48)
#define DEFINE_DOS_DEVICE               (0x49)
#define SOUND_SENTRY                    (0x50)
#define UPDATE_VDM_ENTRY                (0x51)
#define GET_VDM_EXIT_CODE               (0x52)
#define CHECK_VDM                       (0x53)


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


/* Types used in the new CSR. Temporarly here for proper compile of NTDLL */
// Not used at the moment...
typedef enum _CSR_SRV_API_NUMBER
{
    CsrpClientConnect,
    CsrpThreadConnect,
    CsrpProfileControl,
    CsrpIdentifyAlertable,
    CsrpSetPriorityClass,
    CsrpMaxApiNumber
} CSR_SRV_API_NUMBER, *PCSR_SRV_API_NUMBER;

#endif /* __INCLUDE_CSRSS_CSRSS_H */
