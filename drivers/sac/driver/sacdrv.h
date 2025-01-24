/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/sacdrv.h
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

#ifndef _SACDRV_H_
#define _SACDRV_H_

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <stdio.h>
#include <ntoskrnl/include/internal/hdl.h>
#include <sacmsg.h>

/* DEFINES ********************************************************************/

//
// SAC Heap Allocator Macros
//
#define SacAllocatePool(Length, Tag)    \
    MyAllocatePool(Length, Tag, __FILE__, __LINE__)
#define SacFreePool(Pointer)            \
    MyFreePool((PVOID*)(&Pointer))

//
// SAC Debugging Macro and Constants
//
#define SAC_DBG_ENTRY_EXIT                  0x01
#define SAC_DBG_UTIL                        0x02
#define SAC_DBG_INIT                        0x04
#define SAC_DBG_MM                          0x1000
#define SAC_DBG_MACHINE                     0x2000
#define SAC_DBG(x, ...)                     \
    if (SACDebug & x)                       \
    {                                       \
        DbgPrint("SAC %s: ", __FUNCTION__); \
        DbgPrint(__VA_ARGS__);              \
    }

//
// SAC Parameter Checking Macros
//
#define CHECK_PARAMETER_WITH_STATUS(Condition, Status)  \
{                                                       \
    if (!NT_VERIFY(Condition))                          \
    {                                                   \
        return Status;                                  \
    }                                                   \
}
#define CHECK_PARAMETER(x)      \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER)
#define CHECK_PARAMETER1(x)     \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_1)
#define CHECK_PARAMETER2(x)     \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_2)
#define CHECK_PARAMETER3(x)     \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_3)
#define CHECK_PARAMETER4(x)     \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_INVALID_PARAMETER_4)
#define CHECK_ALLOCATION(x)     \
    CHECK_PARAMETER_WITH_STATUS(x, STATUS_NO_MEMORY)

//
// SAC Channel Event Macros
//
#define ChannelInitializeEvent(Channel, Attributes, x)                  \
{                                                                       \
    PVOID Object, WaitObject;                                           \
    if (Attributes->x)                                                  \
    {                                                                   \
        if (!VerifyEventWaitable(Attributes->x, &Object, &WaitObject))  \
        {                                                               \
            Status = STATUS_INVALID_HANDLE;                             \
            goto FailChannel;                                           \
        }                                                               \
        Channel->x = Attributes->x;                                     \
        Channel->x##ObjectBody = Object;                                \
        Channel->x##WaitObjectBody = WaitObject;                        \
    }                                                                   \
}
#define ChannelUninitializeEvent(Channel, x, f)         \
{                                                       \
    ASSERT(ChannelGetFlags(Channel) & (f));             \
    ASSERT(Channel->x##ObjectBody);                     \
    ASSERT(Channel->x##WaitObjectBody);                 \
    if (Channel->x##ObjectBody)                         \
    {                                                   \
        ObDereferenceObject(Channel->x##ObjectBody);    \
        Channel->Flags &= ~(f);                         \
        Channel->x = NULL;                              \
        Channel->x##ObjectBody = NULL;                  \
        Channel->x##WaitObjectBody = NULL;              \
    }                                                   \
}
#define ChannelSetEvent(Channel, x)                                     \
{                                                                       \
    ASSERT(Channel->x);                                                 \
    ASSERT(Channel->x##ObjectBody);                                     \
    ASSERT(Channel->x##WaitObjectBody);                                 \
    if (Channel->x##WaitObjectBody)                                     \
    {                                                                   \
        KeSetEvent(Channel->x##WaitObjectBody, EVENT_INCREMENT, FALSE); \
        Status = STATUS_SUCCESS;                                        \
    }                                                                   \
    else                                                                \
    {                                                                   \
        Status = STATUS_UNSUCCESSFUL;                                   \
    }                                                                   \
}
#define ChannelClearEvent(Channel, x)               \
{                                                   \
    ASSERT(Channel->x);                             \
    ASSERT(Channel->x##ObjectBody);                 \
    ASSERT(Channel->x##WaitObjectBody);             \
    if (Channel->x##WaitObjectBody)                 \
    {                                               \
        KeClearEvent(Channel->x##WaitObjectBody);   \
        Status = STATUS_SUCCESS;                    \
    }                                               \
    else                                            \
    {                                               \
        Status = STATUS_UNSUCCESSFUL;               \
    }                                               \
}

//
// SAC Pool Tags, taken from pooltag.txt:
//
//  Rcp? - sacdrv.sys - SAC Driver (Headless)
//  RcpA - sacdrv.sys -     Internal memory mgr alloc block
//  RcpI - sacdrv.sys -     Internal memory mgr initial heap block
//  RcpS - sacdrv.sys -     Security related block
#define GENERIC_TAG                         '?pcR'
#define ALLOC_BLOCK_TAG                     'ApcR'
#define INITIAL_BLOCK_TAG                   'IpcR'
#define SECURITY_BLOCK_TAG                  'SpcR'
#define FREE_POOL_TAG                       'FpcR'
#define GLOBAL_BLOCK_TAG                    'GpcR'
#define CHANNEL_BLOCK_TAG                   'CpcR'
#define LOCAL_MEMORY_SIGNATURE              'SSEL'
#define GLOBAL_MEMORY_SIGNATURE             'DAEH'

//
// Size Definitions
//
#define SAC_MEMORY_LIST_SIZE                (1 * 1024 * 1024)   // 1MB
#define SAC_OBUFFER_SIZE                    (2 * 1024)          // 2KB
#define SAC_CHANNEL_NAME_SIZE               64
#define SAC_CHANNEL_DESCRIPTION_SIZE        256
#define SAC_MAX_CHANNELS                    10
#define SAC_SERIAL_PORT_BUFFER_SIZE         1024                // 1KB
#define SAC_MAX_MESSAGES                    200
#define SAC_VTUTF8_COL_WIDTH                80
#define SAC_VTUTF8_COL_HEIGHT               25
#define SAC_VTUTF8_ROW_HEIGHT               24
#define MAX_UTF8_ENCODE_BLOCK_LENGTH        (Utf8ConversionBufferSize / 3 - 1)
#define SAC_VTUTF8_OBUFFER_SIZE             0x2D00
#define SAC_VTUTF8_IBUFFER_SIZE             0x2000
#define SAC_RAW_OBUFFER_SIZE                0x2000
#define SAC_RAW_IBUFFER_SIZE                0x2000

//
// Channel flags
//
#define SAC_CHANNEL_FLAG_INTERNAL           0x1
#define SAC_CHANNEL_FLAG_CLOSE_EVENT        0x2
#define SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT 0x4
#define SAC_CHANNEL_FLAG_LOCK_EVENT         0x8
#define SAC_CHANNEL_FLAG_REDRAW_EVENT       0x10
#define SAC_CHANNEL_FLAG_APPLICATION        0x20

//
// Cell Flags
//
#define SAC_CELL_FLAG_BLINK                 1
#define SAC_CELL_FLAG_BOLD                  2
#define SAC_CELL_FLAG_INVERTED              4

//
// Forward definitions
//
struct _SAC_CHANNEL;

//
// Structures used by the SAC Heap Allocator
//
typedef struct _SAC_MEMORY_ENTRY
{
    ULONG Signature;
    ULONG Tag;
    ULONG Size;
} SAC_MEMORY_ENTRY, *PSAC_MEMORY_ENTRY;
typedef struct _SAC_MEMORY_LIST
{
    ULONG Signature;
    PSAC_MEMORY_ENTRY LocalDescriptor;
    ULONG Size;
    struct _SAC_MEMORY_LIST* Next;
} SAC_MEMORY_LIST, *PSAC_MEMORY_LIST;

typedef struct _SAC_MESSAGE_ENTRY
{
    ULONG Index;
    PWCHAR Buffer;
} SAC_MESSAGE_ENTRY, *PSAC_MESSAGE_ENTRY;

//
// These are the VT-100/220/ANSI Escape Codes supported by SAC as input
//
typedef enum _SAC_ANSI_COMMANDS
{
    SacCursorUp,
    SacCursorDown,
    SacCursorRight,
    SacCursorLeft,
    SacFontNormal,
    SacFontBlink,
    SacFontBlinkOff,
    SacFontBold,
    SacFontBoldOff,
    SacFontInverse,
    SacFontInverseOff,
    SacBackTab,
    SacEraseEndOfLine,
    SacEraseStartOfLine,
    SacEraseLine,
    SacEraseEndOfScreen,
    SacEraseStartOfScreen,
    SacEraseScreen,
    SacSetCursorPosition,
    SacSetScrollRegion,
    SacSetColors,
    SacSetBackgroundColor,
    SacSetFontColor,
    SacSetColorsAndAttributes
} SAC_ANSI_COMMANDS;

//
// These are the VT-100/220/ANSI Escape Codes send by SAC as output
//
typedef enum _SAC_ANSI_DISPATCH
{
    SacAnsiClearScreen,
    SacAnsiClearEndOfScreen,
    SacAnsiClearEndOfLine,
    SacAnsiSetColors,
    SacAnsiSetPosition,
    SacAnsiClearAttributes,
    SacAnsiSetInverseAttribute,
    SacAnsiClearInverseAttribute,
    SacAnsiSetBlinkAttribute,
    SacAnsiClearBlinkAttribute,
    SacAnsiSetBoldAttribute,
    SacAnsiClearBoldAttribute
} SAC_ANSI_DISPATCH;

//
// Commands that the consumer and producer share
//
typedef enum _SAC_POST_COMMANDS
{
    Nothing,
    Shutdown,
    Close,
    Restart
} SAC_POST_COMMANDS;

//
// SAC supports 3 different channel output types
//
typedef enum _SAC_CHANNEL_TYPE
{
    VtUtf8,
    Cmd,
    Raw
} SAC_CHANNEL_TYPE;

//
// A SAC channel can be active or inactive
//
typedef enum _SAC_CHANNEL_STATUS
{
    Inactive,
    Active
} SAC_CHANNEL_STATUS, *PSAC_CHANNEL_STATUS;

//
// A SAC channel identifier
//
typedef struct _SAC_CHANNEL_ID
{
    GUID ChannelGuid;
    ULONG ChannelId;
} SAC_CHANNEL_ID, *PSAC_CHANNEL_ID;

//
// Reference-counted SAC channel semaphore lock
//
typedef struct _SAC_CHANNEL_LOCK
{
    LONG RefCount;
    KSEMAPHORE Lock;
} SAC_CHANNEL_LOCK, *PSAC_CHANNEL_LOCK;

//
// Structure of the cell-buffer when in VT-UTF8 Mode
//
typedef struct _SAC_CELL_DATA
{
    UCHAR CellBackColor;
    UCHAR CellForeColor;
    UCHAR CellFlags;
    WCHAR Char;
} SAC_CELL_DATA, *PSAC_CELL_DATA;
C_ASSERT(sizeof(SAC_CELL_DATA) == 6);

//
// Screen buffer when in VT-UTF8 Mode
//
typedef struct _SAC_VTUTF8_SCREEN
{
    SAC_CELL_DATA Cell[SAC_VTUTF8_ROW_HEIGHT][SAC_VTUTF8_COL_WIDTH];
} SAC_VTUTF8_SCREEN, *PSAC_VTUTF8_SCREEN;

//
// Small optimization to easily recognize the most common VT-100/ANSI codes
//
typedef struct _SAC_STATIC_ESCAPE_STRING
{
    WCHAR Sequence[10];
    ULONG Size;
    ULONG Action;
} SAC_STATIC_ESCAPE_STRING, *PSAC_STATIC_ESCAPE_STRING;

//
// Channel callbacks
//
typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_CREATE)(
    IN struct _SAC_CHANNEL* Channel
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_DESTROY)(
    IN struct _SAC_CHANNEL* Channel
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_OREAD)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG ByteCount
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_OECHO)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCHAR String,
    IN ULONG Length
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_OFLUSH)(
    IN struct _SAC_CHANNEL* Channel
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_OWRITE)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCHAR String,
    IN ULONG Length
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_IREAD)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    IN PULONG ReturnBufferSize
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_IBUFFER_FULL)(
    IN struct _SAC_CHANNEL* Channel,
    OUT PBOOLEAN BufferStatus
);

typedef
ULONG
(NTAPI *PSAC_CHANNEL_IBUFFER_LENGTH)(
    IN struct _SAC_CHANNEL* Channel
);

typedef
WCHAR
(NTAPI *PSAC_CHANNEL_IREAD_LAST)(
    IN struct _SAC_CHANNEL* Channel
);

typedef
NTSTATUS
(NTAPI *PSAC_CHANNEL_IWRITE)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize
);

//
// A channel and its attributes
//
typedef struct _SAC_CHANNEL
{
    LONG Index;
    SAC_CHANNEL_ID ChannelId;
    HANDLE CloseEvent;
    PVOID CloseEventObjectBody;
    PKEVENT CloseEventWaitObjectBody;
    HANDLE HasNewDataEvent;
    PVOID HasNewDataEventObjectBody;
    PKEVENT HasNewDataEventWaitObjectBody;
    HANDLE LockEvent;
    PVOID LockEventObjectBody;
    PKEVENT LockEventWaitObjectBody;
    HANDLE RedrawEvent;
    PVOID RedrawEventObjectBody;
    PKEVENT RedrawEventWaitObjectBody;
    PFILE_OBJECT FileObject;
    SAC_CHANNEL_TYPE ChannelType;
    SAC_CHANNEL_STATUS ChannelStatus;
    WCHAR NameBuffer[SAC_CHANNEL_NAME_SIZE + 1];
    WCHAR DescriptionBuffer[SAC_CHANNEL_DESCRIPTION_SIZE + 1];
    ULONG Flags;
    GUID ApplicationType;
    LONG WriteEnabled;
    ULONG IBufferIndex;
    PCHAR IBuffer;
    LONG ChannelHasNewIBufferData;
    UCHAR CursorRow;
    UCHAR CursorCol;
    UCHAR CellForeColor;
    UCHAR CellBackColor;
    UCHAR CellFlags;
    PCHAR OBuffer;
    ULONG OBufferIndex;
    ULONG OBufferFirstGoodIndex;
    LONG ChannelHasNewOBufferData;
    PSAC_CHANNEL_CREATE ChannelCreate;
    PSAC_CHANNEL_DESTROY ChannelDestroy;
    PSAC_CHANNEL_OFLUSH ChannelOutputFlush;
    PSAC_CHANNEL_OECHO ChannelOutputEcho;
    PSAC_CHANNEL_OWRITE ChannelOutputWrite;
    PSAC_CHANNEL_OREAD ChannelOutputRead;
    PSAC_CHANNEL_IWRITE ChannelInputWrite;
    PSAC_CHANNEL_IREAD ChannelInputRead;
    PSAC_CHANNEL_IREAD_LAST ChannelInputReadLast;
    PSAC_CHANNEL_IBUFFER_FULL ChannelInputBufferIsFull;
    PSAC_CHANNEL_IBUFFER_LENGTH ChannelInputBufferLength;
    SAC_CHANNEL_LOCK ChannelAttributeLock;
    SAC_CHANNEL_LOCK ChannelOBufferLock;
    SAC_CHANNEL_LOCK ChannelIBufferLock;
} SAC_CHANNEL, *PSAC_CHANNEL;

typedef struct _SAC_CHANNEL_ATTRIBUTES
{
    SAC_CHANNEL_TYPE ChannelType;
    WCHAR NameBuffer[SAC_CHANNEL_NAME_SIZE + 1];
    WCHAR DescriptionBuffer[SAC_CHANNEL_DESCRIPTION_SIZE + 1];
    ULONG Flag;
    HANDLE CloseEvent;
    HANDLE HasNewDataEvent;
    HANDLE LockEvent;
    HANDLE RedrawEvent;
    GUID ChannelId;
} SAC_CHANNEL_ATTRIBUTES, *PSAC_CHANNEL_ATTRIBUTES;

//
// Cached Machine Information
//
typedef struct _SAC_MACHINE_INFO
{
    PWCHAR MachineName;
    PWCHAR MachineGuid;
    PWCHAR ProcessorArchitecture;
    PWCHAR MajorVersion;
    PWCHAR BuildNumber;
    PWCHAR ProductType;
    PWCHAR ServicePack;
} SAC_MACHINE_INFO, *PSAC_MACHINE_INFO;

//
// The device extension for the SAC
//
typedef struct _SAC_DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    BOOLEAN Initialized;
    BOOLEAN Rundown;
    BOOLEAN PriorityFail;
    BOOLEAN RundownInProgress;
    KPRIORITY PriorityBoost;
    PEPROCESS Process;
    KSPIN_LOCK Lock;
    KEVENT RundownEvent;
    KEVENT Event;
    HANDLE WorkerThreadHandle;
    KEVENT WorkerThreadEvent;
    KTIMER Timer;
    KDPC Dpc;
    LIST_ENTRY List;
} SAC_DEVICE_EXTENSION, *PSAC_DEVICE_EXTENSION;

//
// Dispatch Routines
//
NTSTATUS
NTAPI
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
DispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
DispatchShutdownControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
NTAPI
UnloadHandler(
    IN PDRIVER_OBJECT DriverObject
);

//
// Initialization and shutdown routines
//
VOID
NTAPI
FreeGlobalData(
    VOID
);

VOID
NTAPI
FreeDeviceData(
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
NTAPI
InitializeDeviceData(
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
NTAPI
InitializeGlobalData(
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_OBJECT DriverObject
);

BOOLEAN
NTAPI
InitializeMemoryManagement(
    VOID
);

VOID
NTAPI
FreeMemoryManagement(
    VOID
);

VOID
NTAPI
InitializeCmdEventInfo(
    VOID
);

VOID
NTAPI
InitializeMachineInformation(
    VOID
);

NTSTATUS
NTAPI
PreloadGlobalMessageTable(
    IN PVOID ImageBase
);

NTSTATUS
NTAPI
TearDownGlobalMessageTable(
    VOID
);

NTSTATUS
NTAPI
GetCommandConsoleLaunchingPermission(
    OUT PBOOLEAN Permission
);

NTSTATUS
NTAPI
ImposeSacCmdServiceStartTypePolicy(
    VOID
);

NTSTATUS
NTAPI
RegisterBlueScreenMachineInformation(
    VOID
);

VOID
NTAPI
FreeMachineInformation(
    VOID
);

//
// DPC, Timer, Thread Callbacks
//
VOID
NTAPI
TimerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

//
// Custom SAC Heap Allocator Routines
//
PVOID
NTAPI
MyAllocatePool(
    IN SIZE_T PoolSize,
    IN ULONG Tag,
    IN PCHAR File,
    IN ULONG Line
);

VOID
NTAPI
MyFreePool(
    IN PVOID *Block
);

//
// Connection Manager Routines
//
NTSTATUS
NTAPI
ConMgrInitialize(
    VOID
);

VOID
NTAPI
ConMgrWorkerProcessEvents(
    IN PSAC_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
ConMgrShutdown(
    VOID
);

BOOLEAN
NTAPI
ConMgrSimpleEventMessage(
    IN ULONG MessageIndex,
    IN BOOLEAN LockHeld
);

BOOLEAN
NTAPI
SacPutSimpleMessage(
    IN ULONG MessageIndex
);

VOID
NTAPI
SacPutString(
    IN PWCHAR String
);

NTSTATUS
NTAPI
ConMgrWriteData(
    IN PSAC_CHANNEL Channel,
    IN PVOID Buffer,
    IN ULONG BufferLength
);

NTSTATUS
NTAPI
ConMgrFlushData(
    IN PSAC_CHANNEL Channel
);

BOOLEAN
NTAPI
ConMgrIsWriteEnabled(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ConMgrHandleEvent(
    IN ULONG EventCode,
    IN PSAC_CHANNEL Channel,
    OUT PVOID Data
);

//
// Channel Manager Routines
//
NTSTATUS
NTAPI
ChanMgrInitialize(
    VOID
);

NTSTATUS
NTAPI
ChanMgrShutdown(
    VOID
);

NTSTATUS
NTAPI
ChanMgrCreateChannel(
    OUT PSAC_CHANNEL *Channel,
    IN PSAC_CHANNEL_ATTRIBUTES Attributes
);

NTSTATUS
NTAPI
ChanMgrGetByHandle(
    IN SAC_CHANNEL_ID ChannelId,
    OUT PSAC_CHANNEL* TargetChannel
);

NTSTATUS
NTAPI
ChanMgrReleaseChannel(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChanMgrGetNextActiveChannel(
    IN PSAC_CHANNEL CurrentChannel,
    IN PULONG TargetIndex,
    OUT PSAC_CHANNEL *TargetChannel
);

NTSTATUS
NTAPI
ChanMgrCloseChannel(
    IN PSAC_CHANNEL Channel
);

//
// Channel Routines
//
NTSTATUS
NTAPI
ChannelClose(
    IN PSAC_CHANNEL Channel
);

BOOLEAN
NTAPI
ChannelIsEqual(
    IN PSAC_CHANNEL Channel,
    IN PSAC_CHANNEL_ID ChannelId
);

NTSTATUS
NTAPI
ChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize
);

NTSTATUS
NTAPI
ChannelOFlush(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelSetRedrawEvent(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelClearRedrawEvent(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelHasRedrawEvent(
    IN PSAC_CHANNEL Channel,
    OUT PBOOLEAN Present
);

BOOLEAN
NTAPI
ChannelIsActive(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelGetName(
    IN PSAC_CHANNEL Channel,
    OUT PWCHAR *Name
);

BOOLEAN
NTAPI
ChannelIsEqual(
    IN PSAC_CHANNEL Channel,
    IN PSAC_CHANNEL_ID ChannelId
);

NTSTATUS
NTAPI
ChannelCreate(
    IN PSAC_CHANNEL Channel,
    IN PSAC_CHANNEL_ATTRIBUTES Attributes,
    IN SAC_CHANNEL_ID ChannelId
);

NTSTATUS
NTAPI
ChannelDestroy(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize
);

WCHAR
NTAPI
ChannelIReadLast(
    IN PSAC_CHANNEL Channel
);

ULONG
NTAPI
ChannelIBufferLength(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
ChannelIRead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    IN OUT PULONG ResultBufferSize
);

//
// RAW Channel Table
//
NTSTATUS
NTAPI
RawChannelCreate(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
RawChannelDestroy(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
RawChannelORead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG ByteCount
);

NTSTATUS
NTAPI
RawChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCHAR String,
    IN ULONG Length
);

NTSTATUS
NTAPI
RawChannelOFlush(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
RawChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR String,
    IN ULONG Length
);

NTSTATUS
NTAPI
RawChannelIRead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    IN PULONG ReturnBufferSize
);

NTSTATUS
NTAPI
RawChannelIBufferIsFull(
    IN PSAC_CHANNEL Channel,
    OUT PBOOLEAN BufferStatus
);

ULONG
NTAPI
RawChannelIBufferLength(
    IN PSAC_CHANNEL Channel
);

WCHAR
NTAPI
RawChannelIReadLast(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
RawChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize
);

//
// VT-UTF8 Channel Table
//
NTSTATUS
NTAPI
VTUTF8ChannelCreate(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
VTUTF8ChannelDestroy(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
VTUTF8ChannelORead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG ByteCount
);

NTSTATUS
NTAPI
VTUTF8ChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCHAR String,
    IN ULONG Length
);

NTSTATUS
NTAPI
VTUTF8ChannelOFlush(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
VTUTF8ChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR String,
    IN ULONG Length
);

NTSTATUS
NTAPI
VTUTF8ChannelIRead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    IN PULONG ReturnBufferSize
);

NTSTATUS
NTAPI
VTUTF8ChannelIBufferIsFull(
    IN PSAC_CHANNEL Channel,
    OUT PBOOLEAN BufferStatus
);

ULONG
NTAPI
VTUTF8ChannelIBufferLength(
    IN PSAC_CHANNEL Channel
);

WCHAR
NTAPI
VTUTF8ChannelIReadLast(
    IN PSAC_CHANNEL Channel
);

NTSTATUS
NTAPI
VTUTF8ChannelIWrite(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize
);


//
// Helper Routines
//
BOOLEAN
NTAPI
SacTranslateUtf8ToUnicode(
    IN CHAR Utf8Char,
    IN PCHAR Utf8Buffer,
    OUT PWCHAR Utf8Value
);

ULONG
NTAPI
GetMessageLineCount(
    IN ULONG MessageIndex
);

NTSTATUS
NTAPI
SerialBufferGetChar(
    OUT PCHAR Char
);

NTSTATUS
NTAPI
UTF8EncodeAndSend(
    IN PWCHAR String
);

NTSTATUS
NTAPI
TranslateMachineInformationXML(
    IN PWCHAR *Buffer,
    IN PWCHAR ExtraData
);

PWCHAR
NTAPI
GetMessage(
    IN ULONG MessageIndex
);

BOOLEAN
NTAPI
VerifyEventWaitable(
    IN HANDLE Handle,
    OUT PVOID *WaitObject,
    OUT PVOID *ActualWaitObject
);

BOOLEAN
NTAPI
SacTranslateUnicodeToUtf8(
    IN PWCHAR SourceBuffer,
    IN ULONG SourceBufferLength,
    OUT PCHAR DestinationBuffer,
    IN ULONG DestinationBufferSize,
    OUT PULONG UTF8Count,
    OUT PULONG ProcessedCount
);

//
// SAC Command Functions
//
VOID
NTAPI
DoRebootCommand(
    IN BOOLEAN Reboot
);

VOID
NTAPI
DoFullInfoCommand(
    VOID
);

VOID
NTAPI
DoPagingCommand(
    VOID
);

VOID
NTAPI
DoSetTimeCommand(
    IN PCHAR InputTime
);

VOID
NTAPI
DoKillCommand(
    IN PCHAR KillString
);

VOID
NTAPI
DoLowerPriorityCommand(
    IN PCHAR PrioString
);

VOID
NTAPI
DoRaisePriorityCommand(
    IN PCHAR PrioString
);

VOID
NTAPI
DoLimitMemoryCommand(
    IN PCHAR LimitString
);

VOID
NTAPI
DoCrashCommand(
    VOID
);

VOID
NTAPI
DoMachineInformationCommand(
    VOID
);

VOID
NTAPI
DoChannelCommand(
    IN PCHAR ChannelString
);

VOID
NTAPI
DoCmdCommand(
    IN PCHAR InputString
);

VOID
NTAPI
DoLockCommand(
    VOID
);

VOID
NTAPI
DoHelpCommand(
    VOID
);

VOID
NTAPI
DoGetNetInfo(
    IN BOOLEAN DoPrint
);

VOID
NTAPI
DoSetIpAddressCommand(
    IN PCHAR IpString
);

VOID
NTAPI
DoTlistCommand(
    VOID
);

//
// External data
//
extern ULONG SACDebug;
extern PSAC_MESSAGE_ENTRY GlobalMessageTable;
extern KMUTEX CurrentChannelLock;
extern LONG CurrentChannelRefCount;
extern PCHAR SerialPortBuffer;
extern LONG SerialPortConsumerIndex, SerialPortProducerIndex;
extern PCHAR Utf8ConversionBuffer;
extern BOOLEAN GlobalPagingNeeded, GlobalDoThreads;
extern ULONG Utf8ConversionBufferSize;
extern BOOLEAN CommandConsoleLaunchingEnabled;

//
// Function to initialize a SAC Semaphore Lock
//
FORCEINLINE
VOID
SacInitializeLock(IN PSAC_CHANNEL_LOCK Lock)
{
    KeInitializeSemaphore(&Lock->Lock, 1, 1);
}

//
// Function to acquire a SAC Semaphore Lock
//
FORCEINLINE
VOID
SacAcquireLock(IN PSAC_CHANNEL_LOCK Lock)
{
    KeWaitForSingleObject(&Lock->Lock, Executive, KernelMode, FALSE, NULL);
    ASSERT(Lock->RefCount == 0);
    _InterlockedIncrement(&Lock->RefCount);
}

//
// Function to release a SAC Semaphore Lock
//
FORCEINLINE
VOID
SacReleaseLock(IN PSAC_CHANNEL_LOCK Lock)
{
    ASSERT(Lock->RefCount == 1);
    _InterlockedDecrement(&Lock->RefCount);
    KeReleaseSemaphore(&Lock->Lock, SEMAPHORE_INCREMENT, 1, FALSE);
}

//
// Function to check if the SAC Mutex Lock is held
//
FORCEINLINE
VOID
SacAssertMutexLockHeld(VOID)
{
    ASSERT(CurrentChannelRefCount == 1);
    ASSERT(KeReadStateMutex(&CurrentChannelLock) == 0);
}

//
// Function to check if the SAC Mutex Lock is held
//
FORCEINLINE
VOID
SacInitializeMutexLock(VOID)
{
    KeInitializeMutex(&CurrentChannelLock, 0);
    CurrentChannelRefCount = 0;
}

//
// Function to acquire the SAC Mutex Lock
//
FORCEINLINE
VOID
SacAcquireMutexLock(VOID)
{
    KeWaitForSingleObject(&CurrentChannelLock, Executive, KernelMode, FALSE, NULL);
    ASSERT(CurrentChannelRefCount == 0);
    _InterlockedIncrement(&CurrentChannelRefCount);
}

//
// Function to release the SAC Mutex Lock
//
FORCEINLINE
VOID
SacReleaseMutexLock(VOID)
{
    ASSERT(CurrentChannelRefCount == 1);
    _InterlockedDecrement(&CurrentChannelRefCount);
    KeReleaseMutex(&CurrentChannelLock, FALSE);
}

//
// Locking Macros
//
#define ChannelLockCreates()            SacAcquireLock(&ChannelCreateLock);
#define ChannelUnlockCreates()          SacReleaseLock(&ChannelCreateLock);
#define ChannelLockOBuffer(x)           SacAcquireLock(&x->ChannelOBufferLock);
#define ChannelUnlockOBuffer(x)         SacReleaseLock(&x->ChannelOBufferLock);
#define ChannelLockIBuffer(x)           SacAcquireLock(&x->ChannelIBufferLock);
#define ChannelUnlockIBuffer(x)         SacReleaseLock(&x->ChannelIBufferLock);
#define ChannelLockAttributes(x)        SacAcquireLock(&x->ChannelAttributeLock);
#define ChannelUnlockAttributes(x)      SacReleaseLock(&x->ChannelAttributeLock);
#define ChannelSlotLock(x)              SacAcquireLock(&ChannelSlotLock[x]);
#define ChannelSlotUnlock(x)            SacReleaseLock(&ChannelSlotLock[x]);

//
// Channel Accessors
//
FORCEINLINE
ULONG
ChannelGetFlags(IN PSAC_CHANNEL Channel)
{
    return Channel->Flags;
}

FORCEINLINE
LONG
ChannelGetIndex(IN PSAC_CHANNEL Channel)
{
    /* Return the index of the channel */
    return Channel->Index;
}

FORCEINLINE
BOOLEAN
ChannelHasNewIBufferData(IN PSAC_CHANNEL Channel)
{
    /* Return if there's any new data in the input buffer */
    return Channel->ChannelHasNewIBufferData;
}

//
// FIXME: ANSI.H
//
//
// Source: http://en.wikipedia.org/wiki/ANSI_escape_code
//
typedef enum _VT_ANSI_ATTRIBUTES
{
    //
    // Attribute modifiers (mostly supported)
    //
    Normal,
    Bold,
    Faint,
    Italic,
    Underline,
    SlowBlink,
    FastBlink,
    Inverse,
    Conceal,
    Strikethrough,

    //
    // Font selectors (not supported)
    //
    PrimaryFont,
    AlternateFont1,
    AlternateFont2,
    AlternateFont3,
    Alternatefont4,
    AlternateFont5,
    AlternateFont6,
    AlternateFont7,
    AlternateFont8,
    AlternateFont9,

    //
    // Additional attributes (not supported)
    //
    Fraktur,
    DoubleUnderline,

    //
    // Attribute Un-modifiers (mostly supported)
    //
    BoldOff,
    ItalicOff,
    UnderlineOff,
    BlinkOff,
    Reserved,
    InverseOff,
    ConcealOff,
    StrikethroughOff,

    //
    // Standard Text Color
    //
    SetColorStart,
    SetColorBlack = SetColorStart,
    SetColorRed,
    SetColorGreen,
    SetColorYellow,
    SetColorBlue,
    SetcolorMAgent,
    SetColorCyan,
    SetColorWhite,
    SetColorMax = SetColorWhite,

    //
    // Extended Text Color (not supported)
    //
    SetColor256,
    SeTextColorDefault,

    //
    // Standard Background Color
    //
    SetBackColorStart,
    SetBackColorBlack = SetBackColorStart,
    SetBackColorRed,
    SetBackColorGreen,
    SetBackColorYellow,
    SetBackColorBlue,
    SetBackcolorMAgent,
    SetBackColorCyan,
    SetBackColorWhite,
    SetBackColorMax = SetBackColorWhite,

    //
    // Extended Background Color (not supported)
    //
    SetBackColor256,
    SetBackColorDefault,

    //
    // Extra Attributes (not supported)
    //
    Reserved1,
    Framed,
    Encircled,
    Overlined,
    FramedOff,
    OverlinedOff,
    Reserved2,
    Reserved3,
    Reserved4,
    Reserved5

    //
    // Ideograms (not supported)
    //
} VT_ANSI_ATTRIBUTES;

//
// The following site is a good reference on VT100/ANSI escape codes
// https://web.archive.org/web/20190503084310/http://www.termsys.demon.co.uk/vtansi.htm
//
#define VT_ANSI_ESCAPE              L'\x1B'
#define VT_ANSI_COMMAND             L'['

#define VT_ANSI_CURSOR_UP_CHAR      L'A'
#define VT_ANSI_CURSOR_UP           L"[A"

#define VT_ANSI_CURSOR_DOWN_CHAR    L'B'
#define VT_ANSI_CURSOR_DOWN         L"[B"

#define VT_ANSI_CURSOR_RIGHT_CHAR   L'C'
#define VT_ANSI_CURSOR_RIGHT        L"[C"

#define VT_ANSI_CURSOR_LEFT_CHAR    L'D'
#define VT_ANSI_CURSOR_LEFT         L"[D"

#define VT_ANSI_ERASE_LINE_CHAR     L'K'
#define VT_ANSI_ERASE_END_LINE      L"[K"
#define VT_ANSI_ERASE_START_LINE    L"[1K"
#define VT_ANSI_ERASE_ENTIRE_LINE   L"[2K"

#define VT_ANSI_ERASE_SCREEN_CHAR   L'J'
#define VT_ANSI_ERASE_DOWN_SCREEN   L"[J"
#define VT_ANSI_ERASE_UP_SCREEN     L"[1J"
#define VT_ANSI_ERASE_ENTIRE_SCREEN L"[2J"

#define VT_ANSI_BACKTAB_CHAR        L'Z'
#define VT_220_BACKTAB              L"[0Z"

#define VT_ANSI_SET_ATTRIBUTE_CHAR  L'm'
#define VT_ANSI_SEPARATOR_CHAR      L';'
#define VT_ANSI_HVP_CURSOR_CHAR     L'f'
#define VT_ANSI_CUP_CURSOR_CHAR     L'H'
#define VT_ANSI_SCROLL_CHAR         L'r'

#endif /* _SACDRV_H_ */
