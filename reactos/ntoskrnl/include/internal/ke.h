/*
 * Various useful prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_H
#define __INCLUDE_INTERNAL_KERNEL_H

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <stdarg.h>

/* INTERNAL KERNEL FUNCTIONS ************************************************/

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait);
VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait);
BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr);

VOID KiInterruptDispatch(ULONG irq);
VOID KiDispatchInterrupt(ULONG irq);
VOID KeDrainDpcQueue(VOID);
VOID KeExpireTimers(VOID);
NTSTATUS KeAddThreadTimeout(PKTHREAD Thread, PLARGE_INTEGER Interval);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);

VOID KeDumpStackFrames(PVOID Stack, ULONG NrFrames);
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
BOOLEAN KiTestAlert(PKTHREAD Thread, PCONTEXT UserContext);
VOID KeCallApcsThread(VOID);
VOID KeRemoveAllWaitsThread(PETHREAD Thread, NTSTATUS WaitStatus);
PULONG KeGetStackTopThread(PETHREAD Thread);

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(VOID);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KeInitializeTimerImpl(VOID);
VOID KeInitializeBugCheck(VOID);

#endif
