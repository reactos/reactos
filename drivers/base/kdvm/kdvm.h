/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdvm/kdvm.h
 * PURPOSE:         Base definitions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef _KDDLL_H_
#define _KDDLL_H_

#define NOEXTAPI
#include <ntifs.h>
#include <windbgkd.h>
#include <arc/arc.h>

#undef RtlEqualMemory
#define RtlEqualMemory(a, b, c) (RtlCompareMemory(a, b, c) != c)

//#define KDDEBUG /* uncomment to enable debugging this dll */

typedef ULONG (*PFNDBGPRNT)(const char *Format, ...);
extern PFNDBGPRNT KdpDbgPrint;

#ifndef KDDEBUG
#define KDDBGPRINT(...)
#else
#define KDDBGPRINT KdpDbgPrint
#endif

#define KDRPC_PROTOCOL_VERSION 0x101
#define CONNECTION_TEST_ROUNDS 2 /*100*/
#define KDVM_BUFFER_SIZE (131072 + 1024)
#define KDRPC_TEST_BUFFER_SIZE 512

extern UCHAR KdVmDataBuffer[KDVM_BUFFER_SIZE];
extern PHYSICAL_ADDRESS KdVmBufferPhysicalAddress;
extern ULONG KdVmBufferPos;

typedef enum
{
    KDP_PACKET_RECEIVED = 0,
    KDP_PACKET_TIMEOUT = 1,
    KDP_PACKET_RESEND = 2
} KDP_STATUS;

typedef struct _KDVM_MARSHAL_STRING
{
    USHORT Length;
    USHORT MaximumLength;
} KDVM_MARSHAL_STRING;

#pragma pack(push,1)
typedef struct
{
    CHAR Magic[8];
    UCHAR Command;
} KDVM_CMD_HEADER;

typedef struct
{
    USHORT Id;
    CHAR Magic[9];
} KDVM_RECEIVE_HEADER, *PKDVM_RECEIVE_HEADER;

typedef struct _KDVM_CONTEXT
{
    ULONG   RetryCount;
    BOOLEAN BreakInRequested;
    UCHAR align;
} KDVM_CONTEXT, *PKDVM_CONTEXT;

typedef struct
{
    struct
    {
        UCHAR KdDebuggerNotPresent : 1;
        UCHAR RetryKdSendPacket : 1;
        UCHAR KdDebuggerEnabledAvailable : 1;
    };
    BOOLEAN KdDebuggerEnabled;
    USHORT Unused;
} KDVM_SENDPACKET_INFO;

typedef struct _KDVM_SEND_PKT_REQUEST
{
    KDVM_MARSHAL_STRING MessageHeader;
    KDVM_MARSHAL_STRING MessageData;
    KDVM_CONTEXT KdContext;
    ULONG PacketType;
    ULONG HeaderSize;
    ULONG DataSize;
    KDVM_SENDPACKET_INFO Info;
} KDVM_SEND_PKT_REQUEST, *PKDVM_SEND_PKT_REQUEST;

typedef struct _KDVM_SEND_PKT_RESULT
{
    UCHAR CommandType;
    KDVM_CONTEXT KdContext;
    KDVM_SENDPACKET_INFO Info;
} KDVM_SEND_PKT_RESULT, *PKDVM_SEND_PKT_RESULT;

typedef struct
{
    ULONG PacketType;
    KDVM_SENDPACKET_INFO Info;
    KDVM_MARSHAL_STRING MessageHeader;
    KDVM_MARSHAL_STRING MessageData;
    KDVM_CONTEXT KdContext;
} KDVM_RECV_PKT_REQUEST;

typedef struct
{
    UCHAR CommandType;
    KDVM_MARSHAL_STRING MessageHeader;
    KDVM_MARSHAL_STRING MessageData;
    KDVM_CONTEXT KdContext;
    KDP_STATUS KdStatus;
    ULONG FullSize;
    ULONG HeaderSize;
    ULONG DataSize;
    KDVM_SENDPACKET_INFO Info;
} KDVM_RECV_PKT_RESULT, *PKDVM_RECV_PKT_RESULT;
#pragma pack(pop)

VOID
NTAPI
KdVmDbgDumpBuffer(
    _In_ PVOID Buffer,
    _In_ ULONG Size);

VOID
FASTCALL
KdVmExchange(
    _In_ ULONG_PTR PhysicalAddress,
    _In_ SIZE_T BufferSize);

VOID
NTAPI
KdVmPrepareBuffer(
    VOID);

VOID
NTAPI
KdVmKdVmExchangeData(
    _Out_ PVOID* ReceiveData,
    _Out_ PULONG ReceiveDataSize);


#endif /* _KDDLL_H_ */
