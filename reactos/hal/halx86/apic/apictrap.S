/*
 * FILE:            hal/halx86/apic/apictrap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <internal/i386/asmmacro.S>

#ifdef _M_AMD64
#include <ksamd64.inc>
PUBLIC ApicSpuriousService
#else
#include <ks386.inc>
PUBLIC _ApicSpuriousService
#endif

.code

TRAP_ENTRY HalpTrap0D, 0
TRAP_ENTRY HalpApcInterrupt, KI_SOFTWARE_TRAP
TRAP_ENTRY HalpDispatchInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpClockInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpProfileInterrupt, KI_PUSH_FAKE_ERROR_CODE

FUNC ApicSpuriousService
    int 3
    iret
ENDFUNC ApicSpuriousService


END
