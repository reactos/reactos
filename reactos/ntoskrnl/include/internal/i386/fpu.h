#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H

#include <internal/i386/ke.h>

extern ULONG HardwareMathSupport;

VOID
KiCheckFPU(VOID);

NTSTATUS
KiHandleFpuFault(PKTRAP_FRAME Tf, ULONG ExceptionNr);

VOID
KiFloatingSaveAreaToFxSaveArea(PFX_SAVE_AREA FxSaveArea, CONST FLOATING_SAVE_AREA *FloatingSaveArea);

BOOL
KiContextToFxSaveArea(PFX_SAVE_AREA FxSaveArea, PCONTEXT Context);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_FPU_H */

