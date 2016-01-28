/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtlp.h
 * PURPOSE:         Run-Time Libary Internal Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

/* PAGED_CODE equivalent for user-mode RTL */
#if DBG
extern VOID FASTCALL CHECK_PAGED_CODE_RTL(char *file, int line);
#define PAGED_CODE_RTL() CHECK_PAGED_CODE_RTL(__FILE__, __LINE__)
#else
#define PAGED_CODE_RTL()
#endif

#ifdef _PPC_
#define SWAPD(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
#define SWAPW(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
#define SWAPQ(x) ((SWAPD((x)&0xffffffff) << 32) | (SWAPD((x)>>32)))
#else
#define SWAPD(x) (x)
#define SWAPW(x) (x)
#define SWAPQ(x) (x)
#endif

#define ROUND_DOWN(n, align) \
    (((ULONG)(n)) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)(n)) + (align) - 1, (align))

#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

extern PVOID MmHighestUserAddress;

NTSTATUS
NTAPI
RtlpSafeCopyMemory(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length);

VOID
NTAPI
RtlpGetStackLimits(PULONG_PTR LowLimit,
                   PULONG_PTR HighLimit);

PEXCEPTION_REGISTRATION_RECORD
NTAPI
RtlpGetExceptionList(VOID);

VOID
NTAPI
RtlpSetHeapParameters(IN PRTL_HEAP_PARAMETERS Parameters);

VOID
NTAPI
RtlpSetExceptionList(PEXCEPTION_REGISTRATION_RECORD NewExceptionList);

BOOLEAN
NTAPI
RtlCallVectoredExceptionHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context
);

VOID
NTAPI
RtlCallVectoredContinueHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context
);

typedef struct _DISPATCHER_CONTEXT
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

/* These provide support for sharing code between User and Kernel RTL */
PVOID
NTAPI
RtlpAllocateMemory(
    SIZE_T Bytes,
    ULONG Tag);

VOID
NTAPI
RtlpFreeMemory(
    PVOID Mem,
    ULONG Tag);

KPROCESSOR_MODE
NTAPI
RtlpGetMode(VOID);

BOOLEAN
NTAPI
RtlpCaptureStackLimits(
    IN ULONG_PTR Ebp,
    IN ULONG_PTR *StackBegin,
    IN ULONG_PTR *StackEnd
);

NTSTATUS
NTAPI
RtlDeleteHeapLock(IN OUT PHEAP_LOCK Lock);

NTSTATUS
NTAPI
RtlEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive);

BOOLEAN
NTAPI
RtlTryEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive);

NTSTATUS
NTAPI
RtlInitializeHeapLock(IN OUT PHEAP_LOCK *Lock);

NTSTATUS
NTAPI
RtlLeaveHeapLock(IN OUT PHEAP_LOCK Lock);

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(VOID);

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh);

#define RtlpAllocateStringMemory RtlpAllocateMemory
#define RtlpFreeStringMemory     RtlpFreeMemory

ULONG
NTAPI
RtlGetTickCount(VOID);
#define NtGetTickCount RtlGetTickCount

BOOLEAN
NTAPI
RtlpSetInDbgPrint(
    VOID
);

VOID
NTAPI
RtlpClearInDbgPrint(
    VOID
);

/* i386/except.S */

EXCEPTION_DISPOSITION
NTAPI
RtlpExecuteHandlerForException(PEXCEPTION_RECORD ExceptionRecord,
                               PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                               PCONTEXT Context,
                               PVOID DispatcherContext,
                               PEXCEPTION_ROUTINE ExceptionHandler);

EXCEPTION_DISPOSITION
NTAPI
RtlpExecuteHandlerForUnwind(PEXCEPTION_RECORD ExceptionRecord,
                            PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            PCONTEXT Context,
                            PVOID DispatcherContext,
                            PEXCEPTION_ROUTINE ExceptionHandler);

VOID
NTAPI
RtlpCheckLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                      IN PCONTEXT ContextRecord,
                      IN PVOID ContextData,
                      IN ULONG Size);

VOID
NTAPI
RtlpCaptureContext(OUT PCONTEXT ContextRecord);

//
// Debug Service calls
//
ULONG
NTAPI
DebugService(
    IN ULONG Service,
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PVOID Argument3,
    IN PVOID Argument4
);

VOID
NTAPI
DebugService2(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN ULONG Service
);

/* Tags for the String Allocators */
#define TAG_USTR        'RTSU'
#define TAG_ASTR        'RTSA'
#define TAG_OSTR        'RTSO'

/* Timer Queue */

extern HANDLE TimerThreadHandle;

NTSTATUS
RtlpInitializeTimerThread(VOID);

/* bitmap64.c */
typedef struct _RTL_BITMAP64
{
    ULONG64 SizeOfBitMap;
    PULONG64 Buffer;
} RTL_BITMAP64, *PRTL_BITMAP64;

typedef struct _RTL_BITMAP_RUN64
{
    ULONG64 StartingIndex;
    ULONG64 NumberOfBits;
} RTL_BITMAP_RUN64, *PRTL_BITMAP_RUN64;

/* EOF */
