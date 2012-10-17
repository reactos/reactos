/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/subsys/csrss/msg.h
 * PURPOSE:         Public Definitions for communication
 *                  between CSR Clients and Servers.
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSRMSG_H
#define _CSRMSG_H

typedef struct _CSR_CONNECTION_INFO
{
    ULONG Version;
    ULONG Unknown;
    HANDLE ObjectDirectory;
    PVOID SharedSectionBase;
    PVOID SharedSectionHeap;
    PVOID SharedSectionData;
    ULONG DebugFlags;
    ULONG Unknown2[3];
    HANDLE ProcessId;
} CSR_CONNECTION_INFO, *PCSR_CONNECTION_INFO;

typedef struct _CSR_IDENTIFY_ALTERTABLE_THREAD
{
    CLIENT_ID Cid;
} CSR_IDENTIFY_ALTERTABLE_THREAD, *PCSR_IDENTIFY_ALTERTABLE_THREAD;

typedef struct _CSR_SET_PRIORITY_CLASS
{
    HANDLE hProcess;
    ULONG PriorityClass;
} CSR_SET_PRIORITY_CLASS, *PCSR_SET_PRIORITY_CLASS;

typedef struct _CSR_CLIENT_CONNECT
{
    ULONG ServerId;
    PVOID ConnectionInfo;
    ULONG ConnectionInfoSize;
} CSR_CLIENT_CONNECT, *PCSR_CLIENT_CONNECT;

typedef struct _CSR_CAPTURE_BUFFER
{
    ULONG Size;
    struct _CSR_CAPTURE_BUFFER *PreviousCaptureBuffer;
    ULONG PointerCount;
    ULONG_PTR BufferEnd;
    ULONG_PTR PointerArray[1];
} CSR_CAPTURE_BUFFER, *PCSR_CAPTURE_BUFFER;

/*
typedef union _CSR_API_NUMBER
{
    WORD Index;
    WORD Subsystem;
} CSR_API_NUMBER, *PCSR_API_NUMBER;
*/
typedef ULONG CSR_API_NUMBER;

#include "csrss.h" // remove it when the data structures are not used anymore.

/* Keep in sync with definition below. */
// #define CSRSS_HEADER_SIZE (sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(NTSTATUS))

typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    union
    {
        CSR_CONNECTION_INFO ConnectionInfo;
        struct
        {
            PCSR_CAPTURE_BUFFER CsrCaptureData;
            CSR_API_NUMBER ApiNumber;
            ULONG Status;
            ULONG Reserved;
            union
            {
                CSR_CLIENT_CONNECT CsrClientConnect;

                CSR_SET_PRIORITY_CLASS SetPriorityClass;
                CSR_IDENTIFY_ALTERTABLE_THREAD IdentifyAlertableThread;

            /*** Temporary ***/
#if 1
                CSRSS_CREATE_PROCESS CreateProcessRequest;
                CSRSS_CREATE_THREAD CreateThreadRequest;
                CSRSS_TERMINATE_PROCESS TerminateProcessRequest;
                CSRSS_CONNECT_PROCESS ConnectRequest;
                CSRSS_WRITE_CONSOLE WriteConsoleRequest;
                CSRSS_READ_CONSOLE ReadConsoleRequest;
                CSRSS_ALLOC_CONSOLE AllocConsoleRequest;
                CSRSS_SCREEN_BUFFER_INFO ScreenBufferInfoRequest;
                CSRSS_SET_CURSOR SetCursorRequest;
                CSRSS_FILL_OUTPUT FillOutputRequest;
                CSRSS_READ_INPUT ReadInputRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT_CHAR WriteConsoleOutputCharRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB WriteConsoleOutputAttribRequest;
                CSRSS_FILL_OUTPUT_ATTRIB FillOutputAttribRequest;
                CSRSS_SET_CURSOR_INFO SetCursorInfoRequest;
                CSRSS_GET_CURSOR_INFO GetCursorInfoRequest;
                CSRSS_SET_ATTRIB SetAttribRequest;
                CSRSS_SET_CONSOLE_MODE SetConsoleModeRequest;
                CSRSS_GET_CONSOLE_MODE GetConsoleModeRequest;
                CSRSS_CREATE_SCREEN_BUFFER CreateScreenBufferRequest;
                CSRSS_SET_SCREEN_BUFFER SetScreenBufferRequest;
                CSRSS_SET_TITLE SetTitleRequest;
                CSRSS_GET_TITLE GetTitleRequest;
                CSRSS_WRITE_CONSOLE_OUTPUT WriteConsoleOutputRequest;
                CSRSS_FLUSH_INPUT_BUFFER FlushInputBufferRequest;
                CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER ScrollConsoleScreenBufferRequest;
                CSRSS_READ_CONSOLE_OUTPUT_CHAR ReadConsoleOutputCharRequest;
                CSRSS_READ_CONSOLE_OUTPUT_ATTRIB ReadConsoleOutputAttribRequest;
                CSRSS_GET_NUM_INPUT_EVENTS GetNumInputEventsRequest;
                CSRSS_REGISTER_SERVICES_PROCESS RegisterServicesProcessRequest;
                CSRSS_EXIT_REACTOS ExitReactosRequest;
                CSRSS_SET_SHUTDOWN_PARAMETERS SetShutdownParametersRequest;
                CSRSS_GET_SHUTDOWN_PARAMETERS GetShutdownParametersRequest;
                CSRSS_PEEK_CONSOLE_INPUT PeekConsoleInputRequest;
                CSRSS_READ_CONSOLE_OUTPUT ReadConsoleOutputRequest;
                CSRSS_WRITE_CONSOLE_INPUT WriteConsoleInputRequest;
                CSRSS_GET_INPUT_HANDLE GetInputHandleRequest;
                CSRSS_GET_OUTPUT_HANDLE GetOutputHandleRequest;
                CSRSS_CLOSE_HANDLE CloseHandleRequest;
                CSRSS_VERIFY_HANDLE VerifyHandleRequest;
                CSRSS_DUPLICATE_HANDLE DuplicateHandleRequest;
                CSRSS_SETGET_CONSOLE_HW_STATE ConsoleHardwareStateRequest;
                CSRSS_GET_CONSOLE_WINDOW GetConsoleWindowRequest;
                CSRSS_CREATE_DESKTOP CreateDesktopRequest;
                CSRSS_SHOW_DESKTOP ShowDesktopRequest;
                CSRSS_HIDE_DESKTOP HideDesktopRequest;
                CSRSS_SET_CONSOLE_ICON SetConsoleIconRequest;
                CSRSS_SET_LOGON_NOTIFY_WINDOW SetLogonNotifyWindowRequest;
                CSRSS_REGISTER_LOGON_PROCESS RegisterLogonProcessRequest;
                CSRSS_GET_CONSOLE_CP GetConsoleCodePage;
                CSRSS_SET_CONSOLE_CP SetConsoleCodePage;
                CSRSS_GET_CONSOLE_OUTPUT_CP GetConsoleOutputCodePage;
                CSRSS_SET_CONSOLE_OUTPUT_CP SetConsoleOutputCodePage;
                CSRSS_GET_INPUT_WAIT_HANDLE GetConsoleInputWaitHandle;
                CSRSS_GET_PROCESS_LIST GetProcessListRequest;
                CSRSS_ADD_CONSOLE_ALIAS AddConsoleAlias;
                CSRSS_GET_CONSOLE_ALIAS GetConsoleAlias;
                CSRSS_GET_ALL_CONSOLE_ALIASES GetAllConsoleAlias;
                CSRSS_GET_ALL_CONSOLE_ALIASES_LENGTH GetAllConsoleAliasesLength;
                CSRSS_GET_CONSOLE_ALIASES_EXES GetConsoleAliasesExes;
                CSRSS_GET_CONSOLE_ALIASES_EXES_LENGTH GetConsoleAliasesExesLength;
                CSRSS_GENERATE_CTRL_EVENT GenerateCtrlEvent;
                CSRSS_SET_SCREEN_BUFFER_SIZE SetScreenBufferSize;
                CSRSS_GET_CONSOLE_SELECTION_INFO GetConsoleSelectionInfo;
                CSRSS_GET_COMMAND_HISTORY_LENGTH GetCommandHistoryLength;
                CSRSS_GET_COMMAND_HISTORY GetCommandHistory;
                CSRSS_EXPUNGE_COMMAND_HISTORY ExpungeCommandHistory;
                CSRSS_SET_HISTORY_NUMBER_COMMANDS SetHistoryNumberCommands;
                CSRSS_GET_HISTORY_INFO GetHistoryInfo;
                CSRSS_SET_HISTORY_INFO SetHistoryInfo;
                CSRSS_GET_TEMP_FILE GetTempFile;
                CSRSS_DEFINE_DOS_DEVICE DefineDosDeviceRequest;
                CSRSS_SOUND_SENTRY SoundSentryRequest;
                CSRSS_UPDATE_VDM_ENTRY UpdateVdmEntry;
                CSRSS_GET_VDM_EXIT_CODE GetVdmExitCode;
                CSRSS_CHECK_VDM CheckVdm;
#endif
            /*****************/
            } Data;
        };
    };
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

/*** old ***
typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    PVOID CsrCaptureData;
    ULONG Type;
    NTSTATUS Status;
    union
    {
        CSRSS_CREATE_PROCESS CreateProcessRequest;
        CSRSS_CREATE_THREAD CreateThreadRequest;
        CSRSS_TERMINATE_PROCESS TerminateProcessRequest;
        CSRSS_CONNECT_PROCESS ConnectRequest;

    .   .   .   .   .   .   .   .   .   .   .   .   .   .   .

        CSRSS_GET_VDM_EXIT_CODE GetVdmExitCode;
        CSRSS_CHECK_VDM CheckVdm;
    } Data;
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

***/




#define CSR_PORT_NAME L"ApiPort"

/**** move these defines elsewhere ****/

#define CSR_SRV_SERVER 0
#define CSR_SERVER_DLL_MAX 4

/**************************************/



#define CSR_CREATE_API_NUMBER(ServerId, ApiId) \
    (CSR_API_NUMBER)(((ServerId) << 16) | (ApiId))

#define CSR_API_NUMBER_TO_SERVER_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) >> 16)

#define CSR_API_NUMBER_TO_API_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) & 0xFFFF)

#endif // _CSRMSG_H

/* EOF */
