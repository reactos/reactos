#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H

#include <internal/i386/ke.h>

extern ULONG HardwareMathSupport;

VOID
KiCheckFPU(VOID);

NTSTATUS
KiHandleFpuFault(PKTRAP_FRAME Tf, ULONG ExceptionNr);

VOID
KiFxSaveAreaToFloatingSaveArea(FLOATING_SAVE_AREA *FloatingSaveArea, CONST PFX_SAVE_AREA FxSaveArea);

BOOL
KiContextToFxSaveArea(PFX_SAVE_AREA FxSaveArea, PCONTEXT Context);

PFX_SAVE_AREA
KiGetFpuState(PKTHREAD Thread);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H */

