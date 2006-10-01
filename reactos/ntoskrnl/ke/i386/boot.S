/*
 * FILE:            ntoskrnl/ke/i386/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         FreeLDR Wrapper Bootstrap Code
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

.bss
.align 16

/* Kernel Boot Stack */
.globl _P0BootStack
.space KERNEL_STACK_SIZE
_P0BootStack:

/* Kernel Double-Fault and Temporary DPC Stack */
.globl _KiDoubleFaultStack
.space KERNEL_STACK_SIZE
_KiDoubleFaultStack:

/* FUNCTIONS *****************************************************************/

.text
.func NtProcessStartup
_NtProcessStartup:

    /* NTLDR Boot: Call the main kernel initialization */
    test dword ptr [esp+4], 0x80000000
    jnz _KiSystemStartup@4

    /* FREELDR Boot: Cal the FreeLDR wrapper */
    jmp @KiRosPrepareForSystemStartup@8
.endfunc
