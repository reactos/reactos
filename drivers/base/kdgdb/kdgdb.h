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

// #define KDDEBUG /* uncomment to enable debugging this dll */

#ifndef KDDEBUG
#define KDDBGPRINT(...)
#else
extern ULONG KdpDbgPrint(const char* Format, ...);
#define KDDBGPRINT KdpDbgPrint
#endif

/* gdb_input.c */
extern HANDLE gdb_dbg_thread;
KDSTATUS gdb_interpret_input(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);

/* gdb_receive.c */
extern CHAR gdb_input[];
KDSTATUS NTAPI gdb_receive_packet(_Inout_ PKD_CONTEXT KdContext);
char hex_value(char ch);

/* gdb_send.c */
void send_gdb_packet(_In_ CHAR* Buffer);
void send_gdb_memory(_In_ VOID* Buffer, size_t Length);
void gdb_send_debug_io(_In_ PSTRING String);
void gdb_send_exception(void);

/* kdcom.c */
KDSTATUS NTAPI KdpPollBreakIn(VOID);
VOID NTAPI KdpSendByte(_In_ UCHAR Byte);
KDSTATUS NTAPI KdpReceiveByte(_Out_ PUCHAR OutByte);

/* kdpacket.c */
extern DBGKD_ANY_WAIT_STATE_CHANGE CurrentStateChange;
extern DBGKD_GET_VERSION64 KdVersion;
extern KDDEBUGGER_DATA64* KdDebuggerDataBlock;

/* arch_sup.c */
void gdb_send_registers(void);

#endif /* _KDGDB_H_ */
