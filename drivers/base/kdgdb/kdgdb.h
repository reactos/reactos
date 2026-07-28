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
#include <inttypes.h>
#include <windbgkd.h>
#include <kddll.h>

#include <pstypes.h>

// #define KDDEBUG /* uncomment to enable debugging this dll */

/* To undefine once https://sourceware.org/bugzilla/show_bug.cgi?id=17397 is resolved */
#define MONOPROCESS 1

#define GDB_PACKET_MAX_SIZE 0x1000
/* Each byte takes two characters once hex-encoded */
#define GDB_MEMORY_MAX_SIZE (GDB_PACKET_MAX_SIZE / 2)
/* qXfer replies additionally carry a one-character 'm'/'l' continuation flag */
#define GDB_XFER_MAX_SIZE (GDB_MEMORY_MAX_SIZE - 1)
/* Size of the largest register any supported architecture exposes to GDB */
#define GDB_MAX_REGISTER_SIZE 16

#ifndef KDDEBUG
#define KDDBGPRINT(...)
#else
extern ULONG KdpDbgPrint(const char* Format, ...);
#define KDDBGPRINT KdpDbgPrint
#endif

/* GDB doesn't like pid - tid 0, so +1 them */
FORCEINLINE HANDLE gdb_tid_to_handle(UINT_PTR Tid)
{
    return (HANDLE)(Tid - 1);
}
#define gdb_pid_to_handle gdb_tid_to_handle

FORCEINLINE UINT_PTR handle_to_gdb_tid(HANDLE Handle)
{
    return (UINT_PTR)Handle + 1;
}
#define handle_to_gdb_pid handle_to_gdb_tid

/* Format a thread as GDB expects it, which depends on multiprocess support */
FORCEINLINE
LONG
format_gdb_tid(
    _Out_writes_(BufferSize) char* Buffer,
    _In_ SIZE_T BufferSize,
    _In_ HANDLE ProcessId,
    _In_ UINT_PTR Tid)
{
#if MONOPROCESS
    UNREFERENCED_PARAMETER(ProcessId);
    return _snprintf(Buffer, BufferSize, "%" PRIxPTR, Tid);
#else
    return _snprintf(Buffer, BufferSize, "p%" PRIxPTR ".%" PRIxPTR,
                     handle_to_gdb_pid(ProcessId), Tid);
#endif
}

FORCEINLINE ULONG_PTR KdpGetDirectoryTableBase(PKPROCESS Process)
{
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    return Process->DirectoryTableBase;
#else
    return Process->DirectoryTableBase[0];
#endif
}

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
typedef BOOLEAN (*KDP_SEND_HANDLER)(
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
extern UINT_PTR gdb_dbg_tid;
extern UINT_PTR gdb_dbg_pid;
extern KDSTATUS gdb_receive_and_interpret_packet(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern ULONG64 gdb_hardware_breakpoint_dr7(VOID);
extern BOOLEAN gdb_get_watchpoint_stop(_Out_ const CHAR** Reason, _Out_ PULONG64 Address);

/* gdb_receive.c */
extern CHAR gdb_input[GDB_PACKET_MAX_SIZE + 1];
extern ULONG gdb_input_length;
extern BOOLEAN gdb_no_ack_mode;
KDSTATUS NTAPI gdb_receive_packet(_Inout_ PKD_CONTEXT KdContext);
char hex_value(char ch);

/* gdb_send.c */
KDSTATUS send_gdb_packet(_In_ const CHAR* Buffer);
void start_gdb_packet(void);
void send_gdb_partial_packet(_In_ const CHAR* Buffer);
KDSTATUS finish_gdb_packet(void);
KDSTATUS send_gdb_memory(_In_ const VOID* Buffer, size_t Length);
void send_gdb_partial_memory(_In_ const VOID* Buffer, _In_ size_t Length);
ULONG send_gdb_partial_binary(_In_ const VOID* Buffer, _In_ size_t Length);
KDSTATUS gdb_send_debug_io(_In_ PSTRING String, _In_ BOOLEAN WithPrefix);
KDSTATUS gdb_send_exception(void);
void send_gdb_ntstatus(_In_ NTSTATUS Status);
extern const char hex_chars[];

/* kdcom.c */
KDSTATUS NTAPI KdpPollBreakIn(VOID);
VOID NTAPI KdpSendByte(_In_ UCHAR Byte);
KDSTATUS NTAPI KdpReceiveByte(_Out_ PUCHAR OutByte);
KDSTATUS NTAPI KdpPollByte(OUT PUCHAR OutByte);

/* kdpacket.c */
extern DBGKD_ANY_WAIT_STATE_CHANGE CurrentStateChange;
extern CONTEXT CurrentContext;
extern DBGKD_GET_VERSION64 KdVersion;
extern KDDEBUGGER_DATA64* KdDebuggerDataBlock;
extern LIST_ENTRY* ProcessListHead;
extern LIST_ENTRY* ModuleListHead;
/* The lists are only linked once Ps/the loader have gotten far enough */
FORCEINLINE BOOLEAN ps_initialized(VOID)
{
    return (ProcessListHead != NULL) && (ProcessListHead->Flink != NULL);
}
FORCEINLINE BOOLEAN modules_initialized(VOID)
{
    return (ModuleListHead != NULL) && (ModuleListHead->Flink != NULL);
}
extern KDP_SEND_HANDLER KdpSendPacketHandler;
extern KDP_MANIPULATESTATE_HANDLER KdpManipulateStateHandler;
/* Common ManipulateState handlers */
extern KDSTATUS ContinueManipulateStateHandler(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern KDSTATUS SetContextManipulateHandler(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern KDSTATUS SetContextManipulateHandlerWithReply(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext);
extern PEPROCESS TheIdleProcess;
extern PETHREAD TheIdleThread;

/* utils.c */
extern PEPROCESS find_process( _In_ UINT_PTR Pid);
extern PETHREAD find_thread(_In_ UINT_PTR Pid, _In_ UINT_PTR Tid);
extern BOOLEAN gdb_decode_hex(
    _In_reads_(InputLength) const CHAR* Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_(OutputLength) VOID* Output,
    _In_ SIZE_T OutputLength);
extern BOOLEAN parse_hex_value(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _Out_ PULONG64 Value,
    _Out_opt_ const char** Next);
extern BOOLEAN parse_hex_fields(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _In_z_ const char* Delimiters,
    _Out_writes_(Count) PULONG64 Values,
    _In_ ULONG Count,
    _Out_opt_ const char** Rest);
extern BOOLEAN parse_gdb_thread_id(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _Out_ PUINT_PTR Pid,
    _Out_ PUINT_PTR Tid);

/* gdb_regs.c */
extern KDSTATUS gdb_send_register(void);
extern KDSTATUS gdb_send_registers(void);
extern BOOLEAN gdb_write_register(_In_ ULONG Register, _In_reads_(Length) const CHAR* Value, _In_ ULONG Length);
extern BOOLEAN gdb_write_registers(_In_reads_(Length) const CHAR* Value, _In_ ULONG Length);

/* arch_sup.c: the architecture only describes its register file */
extern const UCHAR gdb_reg_size[];
extern const ULONG gdb_reg_count;
extern const void* gdb_ctx_to_reg(_In_ CONTEXT* Context, _In_ ULONG Register, _Out_ PULONG ScalarValue);
extern BOOLEAN gdb_set_ctx_reg(_Inout_ CONTEXT* Context, _In_ ULONG Register, _In_reads_bytes_(Size) const UCHAR* Value, _In_ SIZE_T Size);
extern const void* gdb_thread_to_reg(_In_ PETHREAD Thread, _In_ ULONG Register);
extern const CHAR gdb_target_xml[];
extern const SIZE_T gdb_target_xml_length;

/* Architecture specific defines. See ntoskrnl/include/internal/arch/ke.h */
#ifdef _M_IX86
/* Handling passing over the breakpoint instruction */
#  define KdpGetContextPc(Context) \
    ((Context)->Eip)
#  define KdpSetContextPc(Context, ProgramCounter) \
    ((Context)->Eip = (ProgramCounter))
#  define KD_BREAKPOINT_TYPE        UCHAR
#  define KD_BREAKPOINT_SIZE        sizeof(UCHAR)
#  define KD_BREAKPOINT_VALUE       0xCC
/* Single step mode */
#  define KdpSetSingleStep(Context) \
    ((Context)->EFlags |= EFLAGS_TF)
#  define KdpClearSingleStep(Context) \
    ((Context)->EFlags &= ~EFLAGS_TF)
#elif defined(_M_AMD64)
#  define KdpGetContextPc(Context) \
    ((Context)->Rip)
#  define KdpSetContextPc(Context, ProgramCounter) \
    ((Context)->Rip = (ProgramCounter))
#  define KD_BREAKPOINT_TYPE        UCHAR
#  define KD_BREAKPOINT_SIZE        sizeof(UCHAR)
#  define KD_BREAKPOINT_VALUE       0xCC
/* Single step mode */
#  define KdpSetSingleStep(Context) \
    ((Context)->EFlags |= EFLAGS_TF)
#  define KdpClearSingleStep(Context) \
    ((Context)->EFlags &= ~EFLAGS_TF)
#else
#  error "Please define relevant macros for your architecture"
#endif

#endif /* _KDGDB_H_ */
