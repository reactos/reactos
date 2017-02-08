#ifndef _WINDBGKD_
#define _WINDBGKD_

//
// Dependencies
//
#include "wdbgexts.h"

//
// Conversion Macros
//
#define COPYSE(p64, p32, f)                 \
    p64->f = (ULONG64)(LONG64)(LONG)p32->f

//
// Packet Size and Control Stream Size
//
#define PACKET_MAX_SIZE                     4000
#define DBGKD_MAXSTREAM                     16

//
// Magic Packet IDs
//
#define INITIAL_PACKET_ID                   0x80800000
#define SYNC_PACKET_ID                      0x00000800

//
// Magic Packet bytes
//
#define BREAKIN_PACKET                      0x62626262
#define BREAKIN_PACKET_BYTE                 0x62
#define PACKET_LEADER                       0x30303030
#define PACKET_LEADER_BYTE                  0x30
#define CONTROL_PACKET_LEADER               0x69696969
#define CONTROL_PACKET_LEADER_BYTE          0x69
#define PACKET_TRAILING_BYTE                0xAA

//
// Packet Types
//
#define PACKET_TYPE_UNUSED                  0
#define PACKET_TYPE_KD_STATE_CHANGE32       1
#define PACKET_TYPE_KD_STATE_MANIPULATE     2
#define PACKET_TYPE_KD_DEBUG_IO             3
#define PACKET_TYPE_KD_ACKNOWLEDGE          4
#define PACKET_TYPE_KD_RESEND               5
#define PACKET_TYPE_KD_RESET                6
#define PACKET_TYPE_KD_STATE_CHANGE64       7
#define PACKET_TYPE_KD_POLL_BREAKIN         8
#define PACKET_TYPE_KD_TRACE_IO             9
#define PACKET_TYPE_KD_CONTROL_REQUEST      10
#define PACKET_TYPE_KD_FILE_IO              11
#define PACKET_TYPE_MAX                     12

//
// Wait State Change Types
//
#define DbgKdMinimumStateChange             0x00003030
#define DbgKdExceptionStateChange           0x00003030
#define DbgKdLoadSymbolsStateChange         0x00003031
#define DbgKdCommandStringStateChange       0x00003032
#define DbgKdMaximumStateChange             0x00003033

//
// This is combined with the basic state change code
// if the state is from an alternate source
//
#define DbgKdAlternateStateChange           0x00010000

//
// Manipulate Types
//
#define DbgKdMinimumManipulate              0x00003130
#define DbgKdReadVirtualMemoryApi           0x00003130
#define DbgKdWriteVirtualMemoryApi          0x00003131
#define DbgKdGetContextApi                  0x00003132
#define DbgKdSetContextApi                  0x00003133
#define DbgKdWriteBreakPointApi             0x00003134
#define DbgKdRestoreBreakPointApi           0x00003135
#define DbgKdContinueApi                    0x00003136
#define DbgKdReadControlSpaceApi            0x00003137
#define DbgKdWriteControlSpaceApi           0x00003138
#define DbgKdReadIoSpaceApi                 0x00003139
#define DbgKdWriteIoSpaceApi                0x0000313A
#define DbgKdRebootApi                      0x0000313B
#define DbgKdContinueApi2                   0x0000313C
#define DbgKdReadPhysicalMemoryApi          0x0000313D
#define DbgKdWritePhysicalMemoryApi         0x0000313E
#define DbgKdQuerySpecialCallsApi           0x0000313F
#define DbgKdSetSpecialCallApi              0x00003140
#define DbgKdClearSpecialCallsApi           0x00003141
#define DbgKdSetInternalBreakPointApi       0x00003142
#define DbgKdGetInternalBreakPointApi       0x00003143
#define DbgKdReadIoSpaceExtendedApi         0x00003144
#define DbgKdWriteIoSpaceExtendedApi        0x00003145
#define DbgKdGetVersionApi                  0x00003146
#define DbgKdWriteBreakPointExApi           0x00003147
#define DbgKdRestoreBreakPointExApi         0x00003148
#define DbgKdCauseBugCheckApi               0x00003149
#define DbgKdSwitchProcessor                0x00003150
#define DbgKdPageInApi                      0x00003151
#define DbgKdReadMachineSpecificRegister    0x00003152
#define DbgKdWriteMachineSpecificRegister   0x00003153
#define OldVlm1                             0x00003154
#define OldVlm2                             0x00003155
#define DbgKdSearchMemoryApi                0x00003156
#define DbgKdGetBusDataApi                  0x00003157
#define DbgKdSetBusDataApi                  0x00003158
#define DbgKdCheckLowMemoryApi              0x00003159
#define DbgKdClearAllInternalBreakpointsApi 0x0000315A
#define DbgKdFillMemoryApi                  0x0000315B
#define DbgKdQueryMemoryApi                 0x0000315C
#define DbgKdSwitchPartition                0x0000315D
#define DbgKdWriteCustomBreakpointApi       0x0000315E
#define DbgKdGetContextExApi                0x0000315F
#define DbgKdSetContextExApi                0x00003160
#define DbgKdMaximumManipulate              0x00003161

//
// Debug I/O Types
//
#define DbgKdPrintStringApi                 0x00003230
#define DbgKdGetStringApi                   0x00003231

//
// Trace I/O Types
//
#define DbgKdPrintTraceApi                  0x00003330

//
// Control Request Types
//
#define DbgKdRequestHardwareBp              0x00004300
#define DbgKdReleaseHardwareBp              0x00004301

//
// File I/O Types
//
#define DbgKdCreateFileApi                 0x00003430
#define DbgKdReadFileApi                   0x00003431
#define DbgKdWriteFileApi                  0x00003432
#define DbgKdCloseFileApi                  0x00003433

//
// Control Report Flags
//
#define REPORT_INCLUDES_SEGS                0x0001
#define REPORT_STANDARD_CS                  0x0002

//
// Protocol Versions
//
#define DBGKD_64BIT_PROTOCOL_VERSION1       5
#define DBGKD_64BIT_PROTOCOL_VERSION2       6

//
// Query Memory Address Spaces
//
#define DBGKD_QUERY_MEMORY_VIRTUAL          0
#define DBGKD_QUERY_MEMORY_PROCESS          0
#define DBGKD_QUERY_MEMORY_SESSION          1
#define DBGKD_QUERY_MEMORY_KERNEL           2

//
// Query Memory Flags
//
#define DBGKD_QUERY_MEMORY_READ             0x01
#define DBGKD_QUERY_MEMORY_WRITE            0x02
#define DBGKD_QUERY_MEMORY_EXECUTE          0x04
#define DBGKD_QUERY_MEMORY_FIXED            0x08

//
// Internal Breakpoint Flags
//
#define DBGKD_INTERNAL_BP_FLAG_COUNTONLY    0x01
#define DBGKD_INTERNAL_BP_FLAG_INVALID      0x02 
#define DBGKD_INTERNAL_BP_FLAG_SUSPENDED    0x04
#define DBGKD_INTERNAL_BP_FLAG_DYING        0x08

//
// Fill Memory Flags
//
#define DBGKD_FILL_MEMORY_VIRTUAL           0x01
#define DBGKD_FILL_MEMORY_PHYSICAL          0x02

//
// Physical Memory Caching Flags
//
#define DBGKD_CACHING_DEFAULT               0
#define DBGKD_CACHING_CACHED                1
#define DBGKD_CACHING_UNCACHED              2
#define DBGKD_CACHING_WRITE_COMBINED        3

//
// Partition Switch Flags
//
#define DBGKD_PARTITION_DEFAULT             0x00
#define DBGKD_PARTITION_ALTERNATE           0x01

//
// AMD64 Control Space types
//
#define AMD64_DEBUG_CONTROL_SPACE_KPCR 0
#define AMD64_DEBUG_CONTROL_SPACE_KPRCB 1
#define AMD64_DEBUG_CONTROL_SPACE_KSPECIAL 2
#define AMD64_DEBUG_CONTROL_SPACE_KTHREAD 3


//
// KD Packet Structure
//
typedef struct _KD_PACKET
{
    ULONG PacketLeader;
    USHORT PacketType;
    USHORT ByteCount;
    ULONG PacketId;
    ULONG Checksum;
} KD_PACKET, *PKD_PACKET;

//
// KD Context
//
typedef struct _KD_CONTEXT
{
    ULONG KdpDefaultRetries;
    BOOLEAN KdpControlCPending;
} KD_CONTEXT, *PKD_CONTEXT;

//
// Control Sets for Supported Architectures
//
#include <pshpack4.h>
typedef struct _X86_DBGKD_CONTROL_SET
{
    ULONG TraceFlag;
    ULONG Dr7;
    ULONG CurrentSymbolStart;
    ULONG CurrentSymbolEnd;
} X86_DBGKD_CONTROL_SET, *PX86_DBGKD_CONTROL_SET;

typedef struct _ALPHA_DBGKD_CONTROL_SET
{
    ULONG __padding;
} ALPHA_DBGKD_CONTROL_SET, *PALPHA_DBGKD_CONTROL_SET;

typedef struct _IA64_DBGKD_CONTROL_SET
{
    ULONG Continue;
    ULONG64 CurrentSymbolStart;
    ULONG64 CurrentSymbolEnd;
} IA64_DBGKD_CONTROL_SET, *PIA64_DBGKD_CONTROL_SET;

typedef struct _AMD64_DBGKD_CONTROL_SET
{
    ULONG TraceFlag;
    ULONG64 Dr7;
    ULONG64 CurrentSymbolStart;
    ULONG64 CurrentSymbolEnd;
} AMD64_DBGKD_CONTROL_SET, *PAMD64_DBGKD_CONTROL_SET;

typedef struct _ARM_DBGKD_CONTROL_SET
{
    ULONG Continue;
    ULONG CurrentSymbolStart;
    ULONG CurrentSymbolEnd;
} ARM_DBGKD_CONTROL_SET, *PARM_DBGKD_CONTROL_SET;

typedef struct _DBGKD_ANY_CONTROL_SET
{
    union
    {
        X86_DBGKD_CONTROL_SET X86ControlSet;
        ALPHA_DBGKD_CONTROL_SET AlphaControlSet;
        IA64_DBGKD_CONTROL_SET IA64ControlSet;
        AMD64_DBGKD_CONTROL_SET Amd64ControlSet;
        ARM_DBGKD_CONTROL_SET ARMControlSet;
    };
} DBGKD_ANY_CONTROL_SET, *PDBGKD_ANY_CONTROL_SET;
#include <poppack.h>

#if defined(_M_IX86)
typedef X86_DBGKD_CONTROL_SET DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;
#elif defined(_M_AMD64)
typedef AMD64_DBGKD_CONTROL_SET DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;
#elif defined(_M_ARM)
typedef ARM_DBGKD_CONTROL_SET DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;
#else
#error Unsupported Architecture
#endif

//
// DBGKM Structure for Exceptions
//
typedef struct _DBGKM_EXCEPTION32
{
    EXCEPTION_RECORD32 ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION32, *PDBGKM_EXCEPTION32;

typedef struct _DBGKM_EXCEPTION64
{
    EXCEPTION_RECORD64 ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION64, *PDBGKM_EXCEPTION64;

//
// DBGKD Structure for State Change
//
typedef struct _X86_DBGKD_CONTROL_REPORT
{
    ULONG   Dr6;
    ULONG   Dr7;
    USHORT  InstructionCount;
    USHORT  ReportFlags;
    UCHAR   InstructionStream[DBGKD_MAXSTREAM];
    USHORT  SegCs;
    USHORT  SegDs;
    USHORT  SegEs;
    USHORT  SegFs;
    ULONG   EFlags;
} X86_DBGKD_CONTROL_REPORT, *PX86_DBGKD_CONTROL_REPORT;

typedef struct _ALPHA_DBGKD_CONTROL_REPORT
{
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} ALPHA_DBGKD_CONTROL_REPORT, *PALPHA_DBGKD_CONTROL_REPORT;

typedef struct _IA64_DBGKD_CONTROL_REPORT
{
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} IA64_DBGKD_CONTROL_REPORT, *PIA64_DBGKD_CONTROL_REPORT;

typedef struct _AMD64_DBGKD_CONTROL_REPORT
{
    ULONG64 Dr6;
    ULONG64 Dr7;
    ULONG EFlags;
    USHORT InstructionCount;
    USHORT ReportFlags;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
} AMD64_DBGKD_CONTROL_REPORT, *PAMD64_DBGKD_CONTROL_REPORT;

typedef struct _ARM_DBGKD_CONTROL_REPORT
{
    ULONG Cpsr;
    ULONG InstructionCount;
    UCHAR InstructionStream[DBGKD_MAXSTREAM];
} ARM_DBGKD_CONTROL_REPORT, *PARM_DBGKD_CONTROL_REPORT;

typedef struct _DBGKD_ANY_CONTROL_REPORT
{
    union
    {
        X86_DBGKD_CONTROL_REPORT X86ControlReport;
        ALPHA_DBGKD_CONTROL_REPORT AlphaControlReport;
        IA64_DBGKD_CONTROL_REPORT IA64ControlReport;
        AMD64_DBGKD_CONTROL_REPORT Amd64ControlReport;
        ARM_DBGKD_CONTROL_REPORT ARMControlReport;
    };
} DBGKD_ANY_CONTROL_REPORT, *PDBGKD_ANY_CONTROL_REPORT;

#if defined(_M_IX86)
typedef X86_DBGKD_CONTROL_REPORT DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;
#elif defined(_M_AMD64)
typedef AMD64_DBGKD_CONTROL_REPORT DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;
#elif defined(_M_ARM)
typedef ARM_DBGKD_CONTROL_REPORT DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;
#else
#error Unsupported Architecture
#endif

//
// DBGKD Structure for Debug I/O Type Print String
//
typedef struct _DBGKD_PRINT_STRING
{
    ULONG LengthOfString;
} DBGKD_PRINT_STRING, *PDBGKD_PRINT_STRING;

//
// DBGKD Structure for Debug I/O Type Get String
//
typedef struct _DBGKD_GET_STRING
{
    ULONG LengthOfPromptString;
    ULONG LengthOfStringRead;
} DBGKD_GET_STRING, *PDBGKD_GET_STRING;

//
// DBGKD Structure for Debug I/O
//
typedef struct _DBGKD_DEBUG_IO
{
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    union
    {
        DBGKD_PRINT_STRING PrintString;
        DBGKD_GET_STRING GetString;
    } u;
} DBGKD_DEBUG_IO, *PDBGKD_DEBUG_IO;

//
// DBGkD Structure for Command String
//
typedef struct _DBGKD_COMMAND_STRING
{
    ULONG Flags;
    ULONG Reserved1;
    ULONG64 Reserved2[7];
} DBGKD_COMMAND_STRING, *PDBGKD_COMMAND_STRING;

//
// DBGKD Structure for Load Symbols
//
typedef struct _DBGKD_LOAD_SYMBOLS32
{
    ULONG PathNameLength;
    ULONG BaseOfDll;
    ULONG ProcessId;
    ULONG CheckSum;
    ULONG SizeOfImage;
    BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS32, *PDBGKD_LOAD_SYMBOLS32;

typedef struct _DBGKD_LOAD_SYMBOLS64
{
    ULONG PathNameLength;
    ULONG64 BaseOfDll;
    ULONG64 ProcessId;
    ULONG CheckSum;
    ULONG SizeOfImage;
    BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS64, *PDBGKD_LOAD_SYMBOLS64;

//
// DBGKD Structure for Wait State Change
//

typedef struct _DBGKD_WAIT_STATE_CHANGE32
{
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG Thread;
    ULONG ProgramCounter;
    union
    {
        DBGKM_EXCEPTION32 Exception;
        DBGKD_LOAD_SYMBOLS32 LoadSymbols;
    } u;
} DBGKD_WAIT_STATE_CHANGE32, *PDBGKD_WAIT_STATE_CHANGE32;

typedef struct _DBGKD_WAIT_STATE_CHANGE64
{
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG64 Thread;
    ULONG64 ProgramCounter;
    union
    {
        DBGKM_EXCEPTION64 Exception;
        DBGKD_LOAD_SYMBOLS64 LoadSymbols;
    } u;
} DBGKD_WAIT_STATE_CHANGE64, *PDBGKD_WAIT_STATE_CHANGE64;

typedef struct _DBGKD_ANY_WAIT_STATE_CHANGE
{
    ULONG NewState;
    USHORT ProcessorLevel;
    USHORT Processor;
    ULONG NumberProcessors;
    ULONG64 Thread;
    ULONG64 ProgramCounter;
    union
    {
        DBGKM_EXCEPTION64 Exception;
        DBGKD_LOAD_SYMBOLS64 LoadSymbols;
        DBGKD_COMMAND_STRING CommandString;
    } u;
    union
    {
        DBGKD_CONTROL_REPORT ControlReport;
        DBGKD_ANY_CONTROL_REPORT AnyControlReport;
    };
} DBGKD_ANY_WAIT_STATE_CHANGE, *PDBGKD_ANY_WAIT_STATE_CHANGE;

//
// DBGKD Manipulate Structures
//
typedef struct _DBGKD_READ_MEMORY32
{
    ULONG TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesRead;
} DBGKD_READ_MEMORY32, *PDBGKD_READ_MEMORY32;

typedef struct _DBGKD_READ_MEMORY64
{
    ULONG64 TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesRead;
} DBGKD_READ_MEMORY64, *PDBGKD_READ_MEMORY64;

typedef struct _DBGKD_WRITE_MEMORY32
{
    ULONG TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY32, *PDBGKD_WRITE_MEMORY32;

typedef struct _DBGKD_WRITE_MEMORY64
{
    ULONG64 TargetBaseAddress;
    ULONG TransferCount;
    ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY64, *PDBGKD_WRITE_MEMORY64;

typedef struct _DBGKD_GET_CONTEXT
{
    ULONG Unused;
} DBGKD_GET_CONTEXT, *PDBGKD_GET_CONTEXT;

typedef struct _DBGKD_SET_CONTEXT
{
    ULONG ContextFlags;
} DBGKD_SET_CONTEXT, *PDBGKD_SET_CONTEXT;

typedef struct _DBGKD_WRITE_BREAKPOINT32
{
    ULONG BreakPointAddress;
    ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT32, *PDBGKD_WRITE_BREAKPOINT32;

typedef struct _DBGKD_WRITE_BREAKPOINT64
{
    ULONG64 BreakPointAddress;
    ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT64, *PDBGKD_WRITE_BREAKPOINT64;

typedef struct _DBGKD_RESTORE_BREAKPOINT
{
    ULONG BreakPointHandle;
} DBGKD_RESTORE_BREAKPOINT, *PDBGKD_RESTORE_BREAKPOINT;

typedef struct _DBGKD_CONTINUE
{
    NTSTATUS ContinueStatus;
} DBGKD_CONTINUE, *PDBGKD_CONTINUE;

#include <pshpack4.h>
typedef struct _DBGKD_CONTINUE2
{
    NTSTATUS ContinueStatus;
    union
    {
        DBGKD_CONTROL_SET ControlSet;
        DBGKD_ANY_CONTROL_SET AnyControlSet;
    };
} DBGKD_CONTINUE2, *PDBGKD_CONTINUE2;
#include <poppack.h>

typedef struct _DBGKD_READ_WRITE_IO32
{
    ULONG IoAddress;
    ULONG DataSize;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO32, *PDBGKD_READ_WRITE_IO32;

typedef struct _DBGKD_READ_WRITE_IO64
{
    ULONG64 IoAddress;
    ULONG DataSize;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO64, *PDBGKD_READ_WRITE_IO64;

typedef struct _DBGKD_READ_WRITE_IO_EXTENDED32
{
    ULONG DataSize;
    ULONG InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
    ULONG IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO_EXTENDED32, *PDBGKD_READ_WRITE_IO_EXTENDED32;

typedef struct _DBGKD_READ_WRITE_IO_EXTENDED64
{
    ULONG DataSize;
    ULONG InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
    ULONG64 IoAddress;
    ULONG DataValue;
} DBGKD_READ_WRITE_IO_EXTENDED64, *PDBGKD_READ_WRITE_IO_EXTENDED64;

typedef struct _DBGKD_READ_WRITE_MSR
{
    ULONG Msr;
    ULONG DataValueLow;
    ULONG DataValueHigh;
} DBGKD_READ_WRITE_MSR, *PDBGKD_READ_WRITE_MSR;

typedef struct _DBGKD_QUERY_SPECIAL_CALLS
{
    ULONG NumberOfSpecialCalls;
} DBGKD_QUERY_SPECIAL_CALLS, *PDBGKD_QUERY_SPECIAL_CALLS;

typedef struct _DBGKD_SET_SPECIAL_CALL32
{
    ULONG SpecialCall;
} DBGKD_SET_SPECIAL_CALL32, *PDBGKD_SET_SPECIAL_CALL32;

typedef struct _DBGKD_SET_SPECIAL_CALL64
{
    ULONG64 SpecialCall;
} DBGKD_SET_SPECIAL_CALL64, *PDBGKD_SET_SPECIAL_CALL64;

typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT32
{
    ULONG BreakpointAddress;
    ULONG Flags;
} DBGKD_SET_INTERNAL_BREAKPOINT32, *PDBGKD_SET_INTERNAL_BREAKPOINT32;

typedef struct _DBGKD_SET_INTERNAL_BREAKPOINT64
{
    ULONG64 BreakpointAddress;
    ULONG Flags;
} DBGKD_SET_INTERNAL_BREAKPOINT64, *PDBGKD_SET_INTERNAL_BREAKPOINT64;

typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT32
{
    ULONG BreakpointAddress;
    ULONG Flags;
    ULONG Calls;
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;
    ULONG MaxInstructions;
    ULONG TotalInstructions;
} DBGKD_GET_INTERNAL_BREAKPOINT32, *PDBGKD_GET_INTERNAL_BREAKPOINT32;

typedef struct _DBGKD_GET_INTERNAL_BREAKPOINT64
{
    ULONG64 BreakpointAddress;
    ULONG Flags;
    ULONG Calls;
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;
    ULONG MaxInstructions;
    ULONG TotalInstructions;
} DBGKD_GET_INTERNAL_BREAKPOINT64, *PDBGKD_GET_INTERNAL_BREAKPOINT64;

typedef struct _DBGKD_BREAKPOINTEX
{
    ULONG BreakPointCount;
    NTSTATUS ContinueStatus;
} DBGKD_BREAKPOINTEX, *PDBGKD_BREAKPOINTEX;

typedef struct _DBGKD_SEARCH_MEMORY
{
    union
    {
        ULONG64 SearchAddress;
        ULONG64 FoundAddress;
    };
    ULONG64 SearchLength;
    ULONG PatternLength;
} DBGKD_SEARCH_MEMORY, *PDBGKD_SEARCH_MEMORY;

typedef struct _DBGKD_GET_SET_BUS_DATA
{
    ULONG BusDataType;
    ULONG BusNumber;
    ULONG SlotNumber;
    ULONG Offset;
    ULONG Length;
} DBGKD_GET_SET_BUS_DATA, *PDBGKD_GET_SET_BUS_DATA;

typedef struct _DBGKD_FILL_MEMORY
{
    ULONG64 Address;
    ULONG Length;
    USHORT Flags;
    USHORT PatternLength;
} DBGKD_FILL_MEMORY, *PDBGKD_FILL_MEMORY;

typedef struct _DBGKD_QUERY_MEMORY
{
    ULONG64 Address;
    ULONG64 Reserved;
    ULONG AddressSpace;
    ULONG Flags;
} DBGKD_QUERY_MEMORY, *PDBGKD_QUERY_MEMORY;

typedef struct _DBGKD_SWITCH_PARTITION
{
    ULONG Partition;
} DBGKD_SWITCH_PARTITION;

typedef struct _DBGKD_CONTEXT_EX
{
   ULONG Offset;
   ULONG ByteCount;
   ULONG BytesCopied;
} DBGKD_CONTEXT_EX, *PDBGKD_CONTEXT_EX;

typedef struct _DBGKD_WRITE_CUSTOM_BREAKPOINT
{
   ULONG64 BreakPointAddress;
   ULONG64 BreakPointInstruction;
   ULONG BreakPointHandle;
   UCHAR BreakPointInstructionSize;
   UCHAR BreakPointInstructionAlignment;
} DBGKD_WRITE_CUSTOM_BREAKPOINT, *PDBGKD_WRITE_CUSTOM_BREAKPOINT;

//
// DBGKD Structure for Manipulate
//
typedef struct _DBGKD_MANIPULATE_STATE32
{
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    NTSTATUS ReturnStatus;
    union
    {
        DBGKD_READ_MEMORY32 ReadMemory;
        DBGKD_WRITE_MEMORY32 WriteMemory;
        DBGKD_READ_MEMORY64 ReadMemory64;
        DBGKD_WRITE_MEMORY64 WriteMemory64;
        DBGKD_GET_CONTEXT GetContext;
        DBGKD_SET_CONTEXT SetContext;
        DBGKD_WRITE_BREAKPOINT32 WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
        DBGKD_CONTINUE Continue;
        DBGKD_CONTINUE2 Continue2;
        DBGKD_READ_WRITE_IO32 ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED32 ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL32 SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT32 SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT32 GetInternalBreakpoint;
        DBGKD_GET_VERSION32 GetVersion32;
        DBGKD_BREAKPOINTEX BreakPointEx;
        DBGKD_READ_WRITE_MSR ReadWriteMsr;
        DBGKD_SEARCH_MEMORY SearchMemory;
        DBGKD_GET_SET_BUS_DATA GetSetBusData;
        DBGKD_FILL_MEMORY FillMemory;
        DBGKD_QUERY_MEMORY QueryMemory;
        DBGKD_SWITCH_PARTITION SwitchPartition;
    } u;
} DBGKD_MANIPULATE_STATE32, *PDBGKD_MANIPULATE_STATE32;

typedef struct _DBGKD_MANIPULATE_STATE64
{
    ULONG ApiNumber;
    USHORT ProcessorLevel;
    USHORT Processor;
    NTSTATUS ReturnStatus;
    union
    {
        DBGKD_READ_MEMORY64 ReadMemory;
        DBGKD_WRITE_MEMORY64 WriteMemory;
        DBGKD_GET_CONTEXT GetContext;
        DBGKD_SET_CONTEXT SetContext;
        DBGKD_WRITE_BREAKPOINT64 WriteBreakPoint;
        DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
        DBGKD_CONTINUE Continue;
        DBGKD_CONTINUE2 Continue2;
        DBGKD_READ_WRITE_IO64 ReadWriteIo;
        DBGKD_READ_WRITE_IO_EXTENDED64 ReadWriteIoExtended;
        DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
        DBGKD_SET_SPECIAL_CALL64 SetSpecialCall;
        DBGKD_SET_INTERNAL_BREAKPOINT64 SetInternalBreakpoint;
        DBGKD_GET_INTERNAL_BREAKPOINT64 GetInternalBreakpoint;
        DBGKD_GET_VERSION64 GetVersion64;
        DBGKD_BREAKPOINTEX BreakPointEx;
        DBGKD_READ_WRITE_MSR ReadWriteMsr;
        DBGKD_SEARCH_MEMORY SearchMemory;
        DBGKD_GET_SET_BUS_DATA GetSetBusData;
        DBGKD_FILL_MEMORY FillMemory;
        DBGKD_QUERY_MEMORY QueryMemory;
        DBGKD_SWITCH_PARTITION SwitchPartition;
        DBGKD_WRITE_CUSTOM_BREAKPOINT WriteCustomBreakpoint;
        DBGKD_CONTEXT_EX ContextEx;
    } u;
} DBGKD_MANIPULATE_STATE64, *PDBGKD_MANIPULATE_STATE64;

//
// File I/O Structure
//
typedef struct _DBGKD_CREATE_FILE
{
    ULONG DesiredAccess;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
    ULONG64 Handle;
    ULONG64 Length;
} DBGKD_CREATE_FILE, *PDBGKD_CREATE_FILE;

typedef struct _DBGKD_READ_FILE
{
    ULONG64 Handle;
    ULONG64 Offset;
    ULONG Length;
} DBGKD_READ_FILE, *PDBGKD_READ_FILE;

typedef struct _DBGKD_WRITE_FILE
{
    ULONG64 Handle;
    ULONG64 Offset;
    ULONG Length;
} DBGKD_WRITE_FILE, *PDBGKD_WRITE_FILE;

typedef struct _DBGKD_CLOSE_FILE
{
    ULONG64 Handle;
} DBGKD_CLOSE_FILE, *PDBGKD_CLOSE_FILE;

typedef struct _DBGKD_FILE_IO
{
    ULONG ApiNumber;
    ULONG Status;
    union
    {
        ULONG64 ReserveSpace[7];
        DBGKD_CREATE_FILE CreateFile;
        DBGKD_READ_FILE ReadFile;
        DBGKD_WRITE_FILE WriteFile;
        DBGKD_CLOSE_FILE CloseFile;
    } u;
} DBGKD_FILE_IO, *PDBGKD_FILE_IO;


//
// Control Request Structure
//
typedef struct _DBGKD_REQUEST_BREAKPOINT
{
    ULONG HardwareBreakPointNumber;
    ULONG Available;
} DBGKD_REQUEST_BREAKPOINT, *PDBGKD_REQUEST_BREAKPOINT;

typedef struct _DBGKD_RELEASE_BREAKPOINT
{
    ULONG HardwareBreakPointNumber;
    ULONG Released;
} DBGKD_RELEASE_BREAKPOINT, *PDBGKD_RELEASE_BREAKPOINT;

typedef struct _DBGKD_CONTROL_REQUEST
{
    ULONG ApiNumber;
    union
    {
        DBGKD_REQUEST_BREAKPOINT RequestBreakpoint;
        DBGKD_RELEASE_BREAKPOINT ReleaseBreakpoint;
    } u;
} DBGKD_CONTROL_REQUEST, *PDBGKD_CONTROL_REQUEST;

//
// Trace I/O Structure
//
typedef struct _DBGKD_PRINT_TRACE
{
    ULONG LengthOfData;
} DBGKD_PRINT_TRACE, *PDBGKD_PRINT_TRACE;

typedef struct _DBGKD_TRACE_IO
{
   ULONG ApiNumber;
   USHORT ProcessorLevel;
   USHORT Processor;
   union
   {
       ULONG64 ReserveSpace[7];
       DBGKD_PRINT_TRACE PrintTrace;
   } u;
} DBGKD_TRACE_IO, *PDBGKD_TRACE_IO;

static
__inline
VOID
NTAPI
ExceptionRecord32To64(IN PEXCEPTION_RECORD32 Ex32,
                      OUT PEXCEPTION_RECORD64 Ex64)
{
    ULONG i;

    Ex64->ExceptionCode = Ex32->ExceptionCode;
    Ex64->ExceptionFlags = Ex32->ExceptionFlags;
    Ex64->ExceptionRecord = Ex32->ExceptionRecord;
    COPYSE(Ex64,Ex32,ExceptionAddress);
    Ex64->NumberParameters = Ex32->NumberParameters;

    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
    {
        COPYSE(Ex64,Ex32,ExceptionInformation[i]);
    }
}

#endif
