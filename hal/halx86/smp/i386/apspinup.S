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
PUBLIC _APCr0
PUBLIC _APCr2
PUBLIC _APCr3
PUBLIC _APCr4
PUBLIC _APSegCs
PUBLIC _APSegSs
PUBLIC _APSegDs
PUBLIC _APSegEs
PUBLIC _APSegGs
PUBLIC _APSegFs
PUBLIC _APTr
PUBLIC _APEip

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
    mov eax, ([ebx] + [_APCr3 - _APSpinup])
    mov cr3, eax

   // hlt 

    /* Enable paging */
    mov eax, cr0
    or  eax, HEX(80000000)
    mov cr0, eax

    /* Prep everything to get to acknowledged by the kernel */
    hlt

/* ProcessorState "Struct" */
_APCr0:
    .long HEX(0)
_APCr2:
    .long HEX(0)
_APCr3:
    .long HEX(0)
_APCr4:
    .long HEX(0)   
_APSegCs:
    .long HEX(0)
_APSegSs:
    .long HEX(0)
_APSegDs:
    .long HEX(0)
_APSegEs:
    .long HEX(0)
_APSegGs:
    .long HEX(0)
_APSegFs:
    .long HEX(0)
_APTr:
    .long HEX(0)
_APEip:
    .long HEX(0)
_APSpinupEnd:
END
 