#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

#ifndef __ASM__

#include "intrin_i.h"
#include "v86m.h"

extern ULONG Ke386CacheAlignment;

#define IMAGE_FILE_MACHINE_ARCHITECTURE IMAGE_FILE_MACHINE_I386

//
// INT3 is 1 byte long
//
#define KD_BREAKPOINT_SIZE 1

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Eip)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Eip = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Eip)

#define KeGetContextReturnRegister(Context) \
    ((Context)->Eax)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->Eax = (ReturnValue))

VOID
FASTCALL
Ki386InitializeTss(
    IN PKTSS Tss,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt
);

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
