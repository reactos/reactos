/*
 * Various useful prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_H
#define __INCLUDE_INTERNAL_KERNEL_H

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <stdarg.h>

/* INTERNAL KERNEL FUNCTIONS ************************************************/

struct _KTHREAD;

typedef struct _KTRAP_FRAME
{
   PVOID DebugEbp;
   PVOID DebugEip;
   PVOID DebugArgMark;
   PVOID TempCs;
   PVOID TempEip;
   PVOID Dr0;
   PVOID Dr1;
   PVOID Dr2;
   PVOID Dr3;
   PVOID Dr6;
   PVOID Dr7;
   USHORT Gs;
   USHORT Reserved1;
   USHORT Es;
   USHORT Reserved2;
   USHORT Ds;
   USHORT Reserved3;
   ULONG Edx;
   ULONG Ecx;
   ULONG Eax;
   ULONG PreviousMode;
   PVOID ExceptionList;
   USHORT Fs;
   USHORT Reserved4;
   ULONG Edi;
   ULONG Esi;
   ULONG Ebx;
   ULONG Ebp;
   ULONG ErrorCode;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
   ULONG Esp;
   USHORT Ss;
   USHORT Reserved5;
   USHORT V86_Es;
   USHORT Reserved6;
   USHORT V86_Ds;
   USHORT Reserved7;
   USHORT V86_Fs;
   USHORT Reserved8;
   USHORT V86_Gs;
   USHORT Reserved9;
} KTRAP_FRAME;

VOID KiUpdateSystemTime (VOID);

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait);
VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait);
BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr);

VOID KiInterruptDispatch(ULONG irq);
VOID KeExpireTimers(VOID);
NTSTATUS KeAddThreadTimeout(struct _KTHREAD* Thread, 
			    PLARGE_INTEGER Interval);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);

VOID KeDumpStackFrames(PVOID Stack, ULONG NrFrames);
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
BOOLEAN KiTestAlert(struct _KTHREAD* Thread, PCONTEXT UserContext);
VOID KeRemoveAllWaitsThread(struct _ETHREAD* Thread, NTSTATUS WaitStatus);
PULONG KeGetStackTopThread(struct _ETHREAD* Thread);

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(VOID);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KeInitializeTimerImpl(VOID);
VOID KeInitializeBugCheck(VOID);

VOID KeInit1(VOID);
VOID KeInit2(VOID);

#endif
