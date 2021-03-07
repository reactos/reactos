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

MACRO(HAL_GENERATE_INT_ENTRY, Vector)
    int &Vector
    ret
    nop
ENDM

.code

TRAP_ENTRY HalpClockInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpClockInterruptStub, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpProfileInterrupt, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpLocalApicErrorService, KI_PUSH_FAKE_ERROR_CODE
TRAP_ENTRY HalpTrap0D, 0
//TRAP_ENTRY HalpApcInterrupt, KI_PUSH_FAKE_ERROR_CODE
//TRAP_ENTRY HalpDispatchInterrupt, KI_PUSH_FAKE_ERROR_CODE

PUBLIC _ApicSpuriousService
_ApicSpuriousService:
    iret

PUBLIC _PicSpuriousService37
_PicSpuriousService37:
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

PUBLIC _HwInterruptTable
_HwInterruptTable:
HAL_GENERATE_INT_ENTRY 0
HAL_GENERATE_INT_ENTRY 1
HAL_GENERATE_INT_ENTRY 2
.long HEX(90C303CD) // HACK hardcode due compilator - {int 3, ret, nop}
i = HEX(4)
REPEAT 251
    HAL_GENERATE_INT_ENTRY %i
    i = i + 1
ENDR
.long HEX(CCCCCCCC) // int [255] -> {int 3} (Debugger Trap)
.long HEX(CCCCCCCC) // for HotPatching next function

#endif

END
