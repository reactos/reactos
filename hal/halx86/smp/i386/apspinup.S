/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for protected mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>
#include <ks386.inc>

PUBLIC _APSpinup
PUBLIC _APSpinupEnd
PUBLIC _APFinalOffset
PUBLIC _APProcessorStateStruct
EXTERN _APEntry:DWORD

.code32
_APSpinup:
    cli

    /* Load segment registers from ProcessorState values */
    mov eax, ([ebx] + [APSegSs - _APSpinup])
    mov ss, eax
    mov eax, ([ebx] + [APSegDs - _APSpinup])
    mov ds, eax
    mov eax, ([ebx] + [APSegEs - _APSpinup])
    mov es, eax
    mov eax, ([ebx] + [APSegGs - _APSpinup])
    mov gs, eax
    mov eax, ([ebx] + [APSegFs - _APSpinup])
    mov fs, eax

    /* Write CR registers with ProcessorState values */
    xor eax, eax
    mov eax, ([ebx] + [APCr2 - _APSpinup])
    mov cr2, eax
    mov eax, ([ebx] + [APCr3 - _APSpinup])
    mov cr3, eax
    mov eax, ([ebx] + [APCr4 - _APSpinup])
    mov cr4, eax

    /* Load AP Stack*/
    mov ecx, ([ebx] + [APEsp - _APSpinup])
    mov esp, ecx

    /* Load LoaderBlock and the location of KiSystemStartup */
    xor eax, eax
    xor ecx, ecx
    mov eax, ([ebx] + [APEip - _APSpinup])
    mov ecx, ([ebx] + [APEcx - _APSpinup])

    /* Push Loaderblock onto the stack as paramter */
    //TODO: Ask about this... It's inside ECX at this stage

    /* make the jump to KiSystemStartup */
    //TODO: Ask about this... It's inside EAX at this stage

    /* APFinal will probably not be impelemented for x86 in final version */
    hlt

    .byte HEX(EA)
_APFinalOffset:
    .long HEX(0)
    .word HEX(08)
    nop
    hlt

/* ProcessorState "Struct" */
_APProcessorStateStruct:
    APCr0:
        .long HEX(0)
    APCr2:
        .long HEX(0)
    APCr3:
        .long HEX(0) 
    APCr4:
        .long HEX(0)   
    APSegCs:
        .long HEX(0)
    APSegSs:
        .long HEX(0)
    APSegDs:
        .long HEX(0)
    APSegEs:
        .long HEX(0)
    APSegGs:
        .long HEX(0)
    APSegFs:
        .long HEX(0)
    APTr:
        .long HEX(0)
    APEip:
        .long HEX(0)
    APEsp:
        .long HEX(0)
    APEcx:
        .long HEX(0)
_APSpinupEnd:
END
