/*
 * FILE:            hal/halx86/apic/apictrap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#ifdef _M_AMD64
#include <ksamd64.inc>
#include <trapamd64.inc>
.code

TRAP_ENTRY HalpClockInterrupt, TF_SEND_EOI
TRAP_ENTRY HalpClockIpi, TF_SEND_EOI
TRAP_ENTRY HalpProfileInterrupt, TF_SEND_EOI

PUBLIC ApicSpuriousService
ApicSpuriousService:
    iretq

APIC_ESR = HEX(0FFFFFFFFFFFE0280)

/* Rearm the error status register, then end the interrupt */
PUBLIC ApicErrorService
ApicErrorService:
    mov dword ptr [APIC_ESR], 0
    mov dword ptr [APIC_EOI], 0
    iretq

#else
#include <ks386.inc>
#include <internal/i386/asmmacro.S>
.code

TRAP_ENTRY HalpClockInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpClockIpi, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpProfileInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpTrap0D, 0
TRAP_ENTRY HalpApcInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpDispatchInterrupt, KI_PUSH_FAKE_ERROR_CODE

PUBLIC _ApicSpuriousService
_ApicSpuriousService:
    iret

APIC_ESR = HEX(0FFFE0280)
APIC_EOI = HEX(0FFFE00B0)

/* Rearm the error status register, then end the interrupt. SS is used
   because DS is not valid when the interrupt arrives in V86 mode */
PUBLIC _ApicErrorService
_ApicErrorService:
    mov dword ptr ss:[APIC_ESR], 0
    mov dword ptr ss:[APIC_EOI], 0
    iret

#endif





END
