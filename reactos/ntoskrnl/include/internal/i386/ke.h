#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define FRAME_EDITED        0xFFF8

#ifndef __ASM__

#include "intrin_i.h"

#define KeArchFnInit() Ke386FnInit()
#define KeArchHaltProcessor() Ke386HaltProcessor()

extern ULONG Ke386CacheAlignment;

struct _KPCR;
VOID
KiInitializeGdt(struct _KPCR* Pcr);
VOID
Ki386ApplicationProcessorInitializeTSS(VOID);

VOID
FASTCALL
Ki386InitializeTss(
    IN PKTSS Tss,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt
);

VOID
KiGdtPrepareForApplicationProcessorInit(ULONG Id);
VOID
Ki386InitializeLdt(VOID);
VOID
Ki386SetProcessorFeatures(VOID);

VOID
NTAPI
KiSetCR0Bits(VOID);

VOID
NTAPI
KiGetCacheInformation(VOID);

BOOLEAN
NTAPI
KiIsNpxPresent(
    VOID
);

BOOLEAN
NTAPI
KiIsNpxErrataPresent(
    VOID
);

VOID
NTAPI
KiSetProcessorType(VOID);

ULONG
NTAPI
KiGetFeatureBits(VOID);

ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
VOID
KeApplicationProcessorInitDispatcher(VOID);
VOID
KeCreateApplicationProcessorIdleThread(ULONG Id);

typedef
VOID
(NTAPI*PKSYSTEM_ROUTINE)(PKSTART_ROUTINE StartRoutine,
                    PVOID StartContext);

VOID
NTAPI
Ke386InitThreadWithContext(PKTHREAD Thread,
                           PKSYSTEM_ROUTINE SystemRoutine,
                           PKSTART_ROUTINE StartRoutine,
                           PVOID StartContext,
                           PCONTEXT Context);
#define KeArchInitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context) \
  Ke386InitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context)

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

#endif
#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H */

/* EOF */
