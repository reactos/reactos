/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgnt.h

Abstract:

    This module contains prototypes and data structures that
    are needed by the NT specific portion of DmKd.

Author:

    Wesley Witt (wesw) 2-Aug-1993

Environment:

Revision History:

--*/

#include "crash.h"

#ifndef _KDH_
#define _KDH_

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer) ((CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((DWORD)0x00000000L)
#define STATUS_UNSUCCESSFUL              ((DWORD)0xC0000001L)
#define STATUS_BUFFER_OVERFLOW           ((DWORD)0x80000005L)
#define STATUS_INVALID_PARAMETER         ((DWORD)0xC000000DL)
#define STATUS_WAIT_RETURN               ((DWORD)0xF0000001L)
#endif


extern DBGKD_WAIT_STATE_CHANGE64  sc;

extern DBGKD_GET_VERSION64 vs;

#define KD_PROCESSID      1
#define KD_THREADID       (sc.Processor + 1)


extern DWORD DmKdState;
//
// DmKdState defines
//
#define S_UNINITIALIZED  0
#define S_REBOOTED       1
#define S_INITIALIZED    2
#define S_READY          3

//---------------------------------------------------------------------------
// prototypes for:  SUPPORT.C
//---------------------------------------------------------------------------

VOID
ClearBps(
    VOID
    );

DWORD
DmKdReadPhysicalMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

DWORD
DmKdWritePhysicalMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

DWORD
DmKdReboot(
    VOID
    );

DWORD
DmKdCrash(
    DWORD BugCheckCode
    );

DWORD
DmKdGetContext(
    IN USHORT Processor,
    IN OUT PCONTEXT Context
    );

DWORD
DmKdSetContext(
    IN USHORT Processor,
    IN CONST CONTEXT *Context
    );

DWORD
DmKdWriteBreakPoint(
    IN ULONG64 BreakPointAddress,
    OUT PULONG BreakPointHandle
    );

DWORD
DmKdRestoreBreakPoint(
    IN ULONG BreakPointHandle
    );

DWORD
DmKdReadIoSpace(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize
    );

DWORD
DmKdWriteIoSpace(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize
    );

DWORD
DmKdReadIoSpaceEx(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize,
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

DWORD
DmKdWriteIoSpaceEx(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize,
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

DWORD
DmKdReadMemoryWrapper(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

DWORD
DmKdReadVirtualMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

DWORD
DmKdReadVirtualMemoryNow(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

DWORD
DmKdWriteVirtualMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

DWORD
DmKdReadControlSpace(
    IN USHORT Processor,
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

DWORD
DmKdWriteControlSpace(
    IN USHORT Processor,
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

DWORD
DmKdContinue (
    IN DWORD ContinueStatus
    );

DWORD
DmKdContinue2 (
    IN DWORD ContinueStatus,
    IN PDBGKD_CONTROL_SET ControlSet
    );

DWORD
DmKdSetSpecialCalls (
    IN ULONG NumSpecialCalls,
    IN PULONG64 Calls
    );

DWORD
DmKdSetInternalBp (
    ULONG64 addr,
    ULONG flags
    );

DWORD
DmKdGetInternalBp (
    ULONG64 addr,
    PULONG flags,
    PULONG calls,
    PULONG minInstr,
    PULONG maxInstr,
    PULONG totInstr,
    PULONG maxCPS
    );

DWORD
DmKdGetVersion (
    PDBGKD_GET_VERSION64 GetVersion
    );

DWORD
DmKdWriteBreakPointEx(
    IN ULONG  BreakPointCount,
    IN OUT PDBGKD_WRITE_BREAKPOINT64 BreakPoints,
    IN DWORD ContinueStatus
    );

DWORD
DmKdRestoreBreakPointEx(
    IN ULONG  BreakPointCount,
    IN PDBGKD_RESTORE_BREAKPOINT BreakPointHandles
    );


//---------------------------------------------------------------------------
// prototypes for:  COM.C
//---------------------------------------------------------------------------

BOOL
DmKdInitComPort(
    BOOL KdModemControl
    );

BOOL
DmKdWriteComPort(
    IN PUCHAR   Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesWritten
    );

BOOL
DmKdReadComPort(
    IN PUCHAR   Buffer,
    IN ULONG    SizeOfBuffer,
    IN PULONG   BytesRead
    );

VOID
DmKdCheckComStatus (
    VOID
    );


//---------------------------------------------------------------------------
// prototypes for:  PACKET.C
//---------------------------------------------------------------------------

VOID
DmKdWriteControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
    );

ULONG
DmKdComputeChecksum (
    IN PUCHAR Buffer,
    IN ULONG Length
    );

BOOL
DmKdSynchronizeTarget (
    VOID
    );

VOID
DmKdSendBreakin(
    VOID
    );

BOOL
DmKdWritePacket(
    IN PVOID PacketData,
    IN size_t PacketDataLength,
    IN USHORT PacketType,
    IN PVOID MorePacketData OPTIONAL,
    IN USHORT MorePacketDataLength OPTIONAL
    );

BOOL
DmKdReadPacketLeader(
    IN  ULONG  PacketType,
    OUT PULONG PacketLeader
    );

BOOL
DmKdWaitForPacket(
    IN  USHORT PacketType,
    OUT PVOID  Packet
    );

DWORD
DmKdWaitStateChange(
    OUT PDBGKD_WAIT_STATE_CHANGE64 StateChange,
    OUT PVOID Buffer,
    IN  ULONG BufferLength
    );



//---------------------------------------------------------------------------
// prototypes for:  CACHE.C
//---------------------------------------------------------------------------

ULONG
DmKdReadCachedVirtualMemory (
    IN ULONG64 BaseAddress,
    IN ULONG TransferCount,
    IN PUCHAR UserBuffer,
    IN PULONG BytesRead,
    IN ULONG NonDiscardable
    );

VOID
DmKdInitVirtualCacheEntry (
    IN ULONG64  BaseAddress,
    IN ULONG  Length,
    IN PUCHAR UserBuffer,
    IN ULONG  NonDiscardable
    );

VOID
DmKdWriteCachedVirtualMemory (
    IN ULONG64 BaseAddress,
    IN ULONG TransferCount,
    IN PUCHAR UserBuffer
    );

VOID
DmKdPurgeCachedVirtualMemory (
    BOOL fPurgeNonDiscardable
    );

VOID
DmKdSetMaxCacheSize(
    IN ULONG MaxCacheSize
    );

typedef struct tagKDOPTIONS {
    LPSTR    keyword;  // data keyword
    USHORT   id;       // data identifier
    USHORT   typ;      // data type
    UINT_PTR value;    // data value, beware usage depends on typ field
} KDOPTIONS, *PKDOPTIONS;

#define KDT_DWORD             0
#define KDT_STRING            1

#define KDO_BAUDRATE          0   // these constants must be consecutive because
#define KDO_PORT              1   // they are used as indexes into the kdoptions
#define KDO_CACHE             2   // array of structures.
#define KDO_VERBOSE           3
#define KDO_INITIALBP         4
#define KDO_DEFER             5
#define KDO_USEMODEM          6
#define KDO_LOGFILEAPPEND     7
#define KDO_GOEXIT            8
#define KDO_SYMBOLPATH        9
#define KDO_LOGFILENAME      10
#define KDO_CRASHDUMP        11

#define MAXKDOPTIONS (sizeof(KdOptions) / sizeof(KDOPTIONS))

extern KDOPTIONS KdOptions[];

#endif
