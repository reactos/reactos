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

TRAP_ENTRY HalpClockInterrupt, (TF_VOLATILES OR TF_SEND_EOI)
TRAP_ENTRY HalpProfileInterrupt, (TF_VOLATILES OR TF_SEND_EOI)

PUBLIC ApicSpuriousService
ApicSpuriousService:
    iretq

PUBLIC HackEoi
HackEoi:
    xor rax, rax
    mov ax, ss
    push rax
    push rsp
    pushfq
    mov ax, cs
    push rax
    lea rax, HackEoiReturn[rip]
    push rax
    mov dword ptr [HEX(0FFFFFFFFFFFE00B0)], 0
    iretq
HackEoiReturn:
    add rsp, 8 // esp was changed by the iret to the pushed value
    ret

#else
#include <ks386.inc>
#include <internal/i386/asmmacro.S>
.code

TRAP_ENTRY HalpClockInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpProfileInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpTrap0D, 0
TRAP_ENTRY HalpApcInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpDispatchInterrupt, KI_PUSH_FAKE_ERROR_CODE

PUBLIC _ApicSpuriousService
_ApicSpuriousService:
    iret

// VBox APIC needs an iret more or less directly following the EOI
PUBLIC _HackEoi
_HackEoi:
    pushfd
    push cs
    push offset OnlyOnePersonKnowsHowToHackAroundVBoxBugsAndThatIsNotYou // !!
    mov dword ptr ds:[HEX(0FFFE00B0)], 0
    iretd
OnlyOnePersonKnowsHowToHackAroundVBoxBugsAndThatIsNotYou:
    ret

#endif





END
