/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Application processor startup code for i386
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

#define ZERO_OFFSET(f) (f - _APEntry16)
#define PS(f) (f - _APEntryCpuState)

PUBLIC _APEntry16
PUBLIC _APEntry16End
PUBLIC _APEntryJump32Offset
PUBLIC _APEntryJump32Segment
PUBLIC _TempPageTableAddr
PUBLIC _APEntryCpuState
PUBLIC _APEntry32

.code16
_APEntry16:
    cli

    /* Load final descriptor tables */
#ifdef _USE_ML
    data32 lgdt fword ptr cs:[ZERO_OFFSET(PS_Gdtr)]
    data32 lidt fword ptr cs:[ZERO_OFFSET(PS_Idtr)]
#else
    data32 lgdt cs:[ZERO_OFFSET(PS_Gdtr)]
    data32 lidt cs:[ZERO_OFFSET(PS_Idtr)]
#endif

    /* Processor state base address */
    mov ebx, cs:[ZERO_OFFSET(PS_SelfPtr)]
    mov esi, cs:[ZERO_OFFSET(PS_SegDs)]

    /* Load temp page table */
    mov eax, cs:[ZERO_OFFSET(_TempPageTableAddr)]
    mov cr3, eax

    /* Enable paging and protected mode */
    mov eax, cs:[ZERO_OFFSET(PS_Cr0)]
    mov cr0, eax

    /* Long jump, 32bit address */
    .byte HEX(66)
    .byte HEX(EA)
_APEntryJump32Offset:
    .long 0
_APEntryJump32Segment:
    .word 0

_APEntry16End:

.align 4
_TempPageTableAddr:
    .long 0
// Processor state
_APEntryCpuState:
PS_SelfPtr:
    .long 0 // self pointer (for use in protected mode)
PS_Esp:
    .long 0
PS_Eip:
    .long 0
PS_Eflags:
    .long 0
PS_SegCs:
    .long 0
PS_SegDs:
    .long 0
PS_SegEs:
    .long 0
PS_SegSs:
    .long 0
PS_SegFs:
    .long 0
PS_SegGs:
    .long 0
// KSPECIAL_REGISTERS
PS_Cr0:
    .long 0
PS_Cr2:
    .long 0
PS_Cr3:
    .long 0
PS_Cr4:
    .long 0
PS_KernelDr0:
    .long 0
PS_KernelDr1:
    .long 0
PS_KernelDr2:
    .long 0
PS_KernelDr3:
    .long 0
PS_KernelDr6:
    .long 0
PS_KernelDr7:
    .long 0
    .space 2
PS_Gdtr:
    .word 0
    .long 0
    .space 2
PS_Idtr:
    .word 0
    .long 0
PS_Tr:
    .word 0
PS_Ldtr:
    .word 0
    .space 4*6 // reserved
.endcode16

.code32
_APEntry32:
    cli

    /* Load segment registers from ProcessorState values */
    /* DS should be the first one */
    mov ds, esi

    mov eax, [ebx + PS(PS_SegEs)]
    mov es, eax
    mov eax, [ebx + PS(PS_SegSs)]
    mov ss, eax
    mov eax, [ebx + PS(PS_SegFs)]
    mov fs, eax
    mov eax, [ebx + PS(PS_SegGs)]
    mov gs, eax

    /* Write CR registers with ProcessorState values */
    mov eax, [ebx + PS(PS_Cr3)]
    mov cr3, eax
    mov eax, [ebx + PS(PS_Cr4)]
    mov cr4, eax

    /* Load debug registers */
    mov eax, [ebx + PS(PS_KernelDr0)]
    mov dr0, eax
    mov eax, [ebx + PS(PS_KernelDr1)]
    mov dr1, eax
    mov eax, [ebx + PS(PS_KernelDr2)]
    mov dr2, eax
    mov eax, [ebx + PS(PS_KernelDr3)]
    mov dr3, eax
    mov eax, [ebx + PS(PS_KernelDr6)]
    mov dr6, eax
    mov eax, [ebx + PS(PS_KernelDr7)]
    mov dr7, eax

    /* Load TSS */
    ltr word ptr [ebx + PS(PS_Tr)]

    /* Load AP Stack*/
    mov esp, [ebx + PS(PS_Esp)]

    /* Load Eip and push it as a "return" address */
    mov eax, [ebx + PS(PS_Eip)]
    push eax

    /* Load flags */
    mov eax, [ebx + PS(PS_Eflags)]
    sahf

    /* Set up all GP registers */
    xor edi, edi
    xor esi, esi
    xor ebp, ebp
    xor ebx, ebx
    xor edx, edx
    xor ecx, ecx
    xor eax, eax

    /* Jump into the kernel */
    ret
END
