/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/internal/hdl.h
 * PURPOSE:         Internal header for the Configuration Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */
#define _HDL_
#include <cportlib/cportlib.h>
#include <ndk/extypes.h>

//
// Define this if you want debugging support
//
#define _HDL_DEBUG_     0x00

//
// These define the Debug Masks Supported
//
#define HDL_XXX_DEBUG   0x01

//
// Debug/Tracing support
//
#if _HDL_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define HDLTRACE DbgPrintEx
#else
#define HDLTRACE(x, ...)                                 \
    if (x & HdlpTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define HDLTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Well-known messages that Io and Pnp post to the kernel log
//
typedef enum _HEADLESS_LOG_MESSAGE
{
    HeadlessLogDriverLoad = 1,
    HeadlessLogDriverSuccess,
    HeadlessLogDriverFailed,
    HeadlessLogEventFailed,
    HeadlessLogObjectFailed,
    HeadlessLogDirectoryFailed,
    HeadlessLogPnpFailed,
    HeadlessLogPnpFailed2,
    HeadlessLogBootDriversFailed,
    HeadlessLogNtdllFailed,
    HeadlessLogSystemDriversFailed,
    HeadlessLogReassignSystemRootFailed,
    HeadlessLogProtectSystemRootFailed,
    HeadlessLogConvertSystemRootFailed,
    HeadlessLogConvertDeviceNameFailed,
    HeadlessLogGroupOrderListFailed,
    HeadlessLogGroupTableFailed
    //
    // There are more, but not applicable to ReactOS, I believe
    //
} HEADLESS_LOG_MESSAGE;

//
// Headless Log Entry
//
typedef struct _HEADLESS_LOG_ENTRY
{
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfEntry;
    PWCHAR String;
} HEADLESS_LOG_ENTRY, *PHEADLESS_LOG_ENTRY;

//
// Headless Bugcheck Information
//
typedef struct _HEADLESS_CMD_SET_BLUE_SCREEN_DATA
{
    ULONG ValueIndex;
    UCHAR Data[ANYSIZE_ARRAY];
} HEADLESS_CMD_SET_BLUE_SCREEN_DATA, *PHEADLESS_CMD_SET_BLUE_SCREEN_DATA;

//
// Headless Control Structure, mostly for !SAC
//
typedef struct _HEADLESS_GLOBALS
{
    KSPIN_LOCK SpinLock;
    HANDLE PageLockHandle;
    PHEADLESS_LOG_ENTRY LogEntries;
    PUCHAR TmpBuffer;
    PUCHAR InputBuffer;
    PHEADLESS_CMD_SET_BLUE_SCREEN_DATA BlueScreenData;
    union
    {
        struct
        {
            ULONG TerminalEnabled:1;
            ULONG InBugCheck:1;
            ULONG NewLogEntryAdded:1;
            ULONG UsedBiosSettings:1;
            ULONG InputProcessing:1;
            ULONG InputLineDone:1;
            ULONG ProcessingCmd:1;
            ULONG TerminalParity:1;
            ULONG TerminalStopBits:1;
            ULONG TerminalPortNumber:3;
            ULONG IsNonLegacyDevice:1;
        };
        ULONG AllFlags;
    };
    ULONG TerminalBaudRate;
    ULONG TerminalPort;
    PUCHAR TerminalPortAddress;
    LARGE_INTEGER DelayTime;
    ULONG MicroSecondsDelayTime;
    UCHAR TerminalType;
    SIZE_T InputBufferIndex;
    USHORT LogEntryLast;
    USHORT LogEntryStart;
    GUID SystemGUID;
    BOOLEAN IsMMIODevice;
    BOOLEAN IsLastCharCR;
} HEADLESS_GLOBALS, *PHEADLESS_GLOBALS;

//
// FIXME: A public header in the NDK? Ask Alex
//
typedef enum _HEADLESS_CMD
{
    HeadlessCmdEnableTerminal = 1,
    HeadlessCmdCheckForReboot,
    HeadlessCmdPutString,
    HeadlessCmdClearDisplay,
    HeadlessCmdClearToEndOfDisplay,
    HeadlessCmdClearToEndOfLine,
    HeadlessCmdDisplayAttributesOff,
    HeadlessCmdDisplayInverseVideo,
    HeadlessCmdSetColor,
    HeadlessCmdPositionCursor,
    HeadlessCmdTerminalPoll,
    HeadlessCmdGetByte,
    HeadlessCmdGetLine,
    HeadlessCmdStartBugCheck,
    HeadlessCmdDoBugCheckProcessing,
    HeadlessCmdQueryInformation,
    HeadlessCmdAddLogEntry,
    HeadlessCmdDisplayLog,
    HeadlessCmdSetBlueScreenData,
    HeadlessCmdSendBlueScreenData,
    HeadlessCmdQueryGUID,
    HeadlessCmdPutData
} HEADLESS_CMD, *PHEADLESS_CMD;

typedef enum _HEADLESS_TERM_PORT_TYPE
{
    HeadlessUndefinedPortType = 0,
    HeadlessSerialPort
} HEADLESS_TERM_PORT_TYPE, *PHEADLESS_TERM_PORT_TYPE;

typedef enum _HEADLESS_TERM_SERIAL_PORT
{
    SerialPortUndefined = 0,
    ComPort1,
    ComPort2,
    ComPort3,
    ComPort4
} HEADLESS_TERM_SERIAL_PORT, *PHEADLESS_TERM_SERIAL_PORT;

typedef struct _HEADLESS_RSP_QUERY_INFO
{
    HEADLESS_TERM_PORT_TYPE PortType;
    union
    {
        struct
        {
            BOOLEAN TerminalAttached;
            BOOLEAN UsedBiosSettings;
            HEADLESS_TERM_SERIAL_PORT TerminalPort;
            PUCHAR TerminalPortBaseAddress;
            ULONG TerminalBaudRate;
            UCHAR TerminalType;
        } Serial;
    };
} HEADLESS_RSP_QUERY_INFO, *PHEADLESS_RSP_QUERY_INFO;

typedef struct _HEADLESS_CMD_ENABLE_TERMINAL
{
    BOOLEAN Enable;
} HEADLESS_CMD_ENABLE_TERMINAL, *PHEADLESS_CMD_ENABLE_TERMINAL;

typedef struct _HEADLESS_CMD_PUT_STRING
{
    UCHAR String[1];
} HEADLESS_CMD_PUT_STRING, *PHEADLESS_CMD_PUT_STRING;

typedef struct _HEADLESS_CMD_SET_COLOR
{
    ULONG TextColor;
    ULONG BkgdColor;
} HEADLESS_CMD_SET_COLOR, *PHEADLESS_CMD_SET_COLOR;

typedef struct _HEADLESS_CMD_CURSOR_POS
{
    ULONG CursorCol;
    ULONG CursorRow;
} HEADLESS_CMD_CURSOR_POS, *PHEADLESS_CMD_CURSOR_POS;

typedef struct _HEADLESS_RSP_GET_BYTE
{
    UCHAR Value;
} HEADLESS_RSP_GET_BYTE, *PHEADLESS_RSP_GET_BYTE;

NTSTATUS
NTAPI
HeadlessDispatch(
    IN HEADLESS_CMD Command,
    IN PVOID InputBuffer,
    IN SIZE_T InputBufferSize,
    OUT PVOID OutputBuffer,
    OUT PSIZE_T OutputBufferSize
);

//
// Global variables accessible from all of Hdl
//
extern PHEADLESS_GLOBALS HeadlessGlobals;

//
// Inlined functions
//
//#include "hdl_x.h"
