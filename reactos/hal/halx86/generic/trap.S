/*
 * FILE:            ntoskrnl/ke/i386/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/i386/asm.h>
#include <internal/i386/asmmacro.S>

.code32
.text

TRAP_ENTRY HalpTrap0D, 0
TRAP_ENTRY HalpApcInterrupt, KI_SOFTWARE_TRAP
TRAP_ENTRY HalpClockInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpProfileInterrupt, KI_PUSH_FAKE_ERROR_CODE


