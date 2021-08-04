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
PUBLIC _APProcessorStateStruct
EXTERN _APEntry:DWORD

.code32
_APSpinup:

    /* At this point the value in ebx is the location of the bootstub + the size of the APEntry segment */
    mov ecx, ebx
    add ecx, [_APSpinupEnd - _APSpinup]
    /* Load a 4kb stack for AP */
    mov esp, [ecx + HEX(4096)] 

    /* Load the GDT based on BSP */

    /* TODO: replace this with something that's going to actually work */
    xor eax, eax
    mov eax, ([ebx] + [APCr3 - _APSpinup])
    mov cr3, eax

    /* Load some ProcessorState values into proper area */
    xor eax, eax

    /* Ss */
    mov eax, ([ebx] + [APSegSs - _APSpinup])
    mov ss, eax

    hlt

    /* Enable paging */
    mov eax, cr0
    or  eax, HEX(80000000)
    mov cr0, eax

    /* Prep everything to get to acknowledged by the kernel */
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
_APSpinupEnd:
END
 