/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtlp.h
 * PURPOSE:         Run-Time Libary Internal Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

/* PAGED_CODE equivalent for user-mode RTL */
#ifdef DBG
extern VOID FASTCALL CHECK_PAGED_CODE_RTL(char *file, int line);
#define PAGED_CODE_RTL() CHECK_PAGED_CODE_RTL(__FILE__, __LINE__)
#else
#define PAGED_CODE_RTL()
#endif

#ifdef _PPC_
#define SWAPD(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
#define SWAPW(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
#else
#define SWAPD(x) x
#define SWAPW(x) x
#endif

VOID
NTAPI
RtlpGetStackLimits(PULONG_PTR StackBase,
                   PULONG_PTR StackLimit);

PEXCEPTION_REGISTRATION_RECORD
NTAPI
RtlpGetExceptionList(VOID);

VOID
NTAPI
RtlpSetExceptionList(PEXCEPTION_REGISTRATION_RECORD NewExceptionList);

typedef struct _DISPATCHER_CONTEXT
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

/* These provide support for sharing code between User and Kernel RTL */
PVOID
NTAPI
RtlpAllocateMemory(
    ULONG Bytes,
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
RtlDeleteHeapLock(PRTL_CRITICAL_SECTION CriticalSection);

NTSTATUS
NTAPI
RtlEnterHeapLock(PRTL_CRITICAL_SECTION CriticalSection);

NTSTATUS
NTAPI
RtlInitializeHeapLock(PRTL_CRITICAL_SECTION CriticalSection);

NTSTATUS
NTAPI
RtlLeaveHeapLock(PRTL_CRITICAL_SECTION CriticalSection);

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(BOOLEAN Type);

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh);

#define RtlpAllocateStringMemory RtlpAllocateMemory
#define RtlpFreeStringMemory     RtlpFreeMemory

BOOLEAN
NTAPI
RtlpSetInDbgPrint(IN BOOLEAN NewValue);

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

PVOID
NTAPI
RtlpGetExceptionAddress(VOID);

VOID
NTAPI
RtlpCaptureContext(OUT PCONTEXT ContextRecord);

/* i386/debug.S */
NTSTATUS
NTAPI
DebugService(IN ULONG Service,
             IN const void* Buffer,
             IN ULONG Length,
             IN PVOID Argument1,
             IN PVOID Argument2);

NTSTATUS
NTAPI
DebugService2(IN PVOID Argument1,
              IN PVOID Argument2,
              IN ULONG Service);

/* Tags for the String Allocators */
#define TAG_USTR        TAG('U', 'S', 'T', 'R')
#define TAG_ASTR        TAG('A', 'S', 'T', 'R')
#define TAG_OSTR        TAG('O', 'S', 'T', 'R')

/* Timer Queue */

extern HANDLE TimerThreadHandle;

NTSTATUS
RtlpInitializeTimerThread(VOID);

/* EOF */
