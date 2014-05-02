/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Memory functions for amd64
 * FILE:              lib/rtl/amd64/rtlmem.S
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* FUNCTIONS *****************************************************************/

.code64

/* SIZE_T
 * RtlCompareMemory(
 *   IN CONST VOID *Source1, <rcx>
 *   IN CONST VOID *Source2, <rdx>
 *   IN SIZE_T  Length <r8>
 * );
 */
.proc RtlCompareMemory

    /* Save registers */
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    .ENDPROLOG

    /* Setup registers for compare */
    mov rsi, rcx
    mov rdi, rdx

    /* Clear direction flag */
    cli

    /* Get number of qwords */
    mov rcx, r8
    shr rcx, 3
    jz RtlCompareMemory2

    /* Compare qwords */
    repe cmpsq
    jnz RtlCompareMemory4

RtlCompareMemory2:
    /* Compare rest */
    mov rcx, r8
    and rcx, 7
    jz RtlCompareMemory3

    repe cmpsb
    jnz RtlCompareMemory5

RtlCompareMemory3:
    /* All equal */
    /* Return the full count */
    mov rax, rcx
    jmp RtlCompareMemory6

RtlCompareMemory4:
    /* Not equal after comparing qwords */
    /* Compare the last qword */
    sub rsi, 8
    sub rdi, 8
    mov rcx, 8
    repe cmpsb

RtlCompareMemory5:
    /* Not equal after comparing bytes */
    /* Return difference */
    sub rdi, rdx
    dec rdi
    mov rax, rdi

RtlCompareMemory6:
   /* Cleanup and return */
    pop rdi
    pop rsi
    ret
.endp

END

