/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kddll.h
 * PURPOSE:         Base definitions for the kernel debugger.
 */

#ifndef _KDGDB_H_
#define _KDGDB_H_

#define NOEXTAPI
#include <ntifs.h>
#include <halfuncs.h>
#include <stdio.h>
#include <arc/arc.h>
#include <windbgkd.h>
#include <kddll.h>

#define KDDEBUG /* uncomment to enable debugging this dll */

#ifndef KDDEBUG
#define KDDBGPRINT(...)
#else
extern ULONG KdpDbgPrint(const char* Format, ...);
#define KDDBGPRINT KdpDbgPrint
#endif

FORCEINLINE
VOID
InitManipulateFromStateChange(
    _In_ ULONG ApiNumber,
    _In_ const DBGKD_ANY_WAIT_STATE_CHANGE* StateChange,
    _Out_ DBGKD_MANIPULATE_STATE64* Manipulate)
{
    Manipulate->ApiNumber = ApiNumber;
    Manipulate->Processor = StateChange->Processor;
    Manipulate->ProcessorLevel = StateChange->ProcessorLevel;
}

/* Callbacks to simulate a KdReceive <-> KdSend loop without GDB being aware of it */
typedef VOID (*KDP_SEND_HANDLER)(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData
);
typedef KDSTATUS (*KDP_MANIPULATESTATE_HANDLER)(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext
);

/* gdb_input.c */
extern HANDLE gdb_dbg_thread;
extern HANDLE gdb_dbg_process;
extern KDSTATUS gdb_interpret_input(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern KDSTATUS gdb_receive_and_interpret_packet(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);

/* gdb_receive.c */
extern CHAR gdb_input[];
KDSTATUS NTAPI gdb_receive_packet(_Inout_ PKD_CONTEXT KdContext);
char hex_value(char ch);

/* gdb_send.c */
void send_gdb_packet(_In_ CHAR* Buffer);
void send_gdb_memory(_In_ VOID* Buffer, size_t Length);
void gdb_send_debug_io(_In_ PSTRING String);
void gdb_send_exception(void);
void send_gdb_ntstatus(_In_ NTSTATUS Status);

/* kdcom.c */
KDSTATUS NTAPI KdpPollBreakIn(VOID);
VOID NTAPI KdpSendByte(_In_ UCHAR Byte);
KDSTATUS NTAPI KdpReceiveByte(_Out_ PUCHAR OutByte);

/* kdpacket.c */
extern DBGKD_ANY_WAIT_STATE_CHANGE CurrentStateChange;
extern CONTEXT CurrentContext;
extern DBGKD_GET_VERSION64 KdVersion;
extern KDDEBUGGER_DATA64* KdDebuggerDataBlock;
extern KDP_SEND_HANDLER KdpSendPacketHandler;
extern KDP_MANIPULATESTATE_HANDLER KdpManipulateStateHandler;
/* Commone ManipulateState handlers */
extern KDSTATUS ContinueManipulateStateHandler(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern KDSTATUS SetContextManipulateHandler(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);

/* arch_sup.c */
extern KDSTATUS gdb_send_register(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern KDSTATUS gdb_send_registers(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);

/* Architecture specific defines. See ntoskrnl/include/internal/arch/ke.h */
#ifdef _M_IX86
#  define KdpGetContextPc(Context) \
    ((Context)->Eip)
#  define KdpSetContextPc(Context, ProgramCounter) \
    ((Context)->Eip = (ProgramCounter))
#  define KD_BREAKPOINT_SIZE        sizeof(UCHAR)
#else
#  error "Please define relevant macros for your architecture"
#endif

#endif /* _KDGDB_H_ */
