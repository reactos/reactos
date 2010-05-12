/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Memory functions for amd64
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asmmacro.S>

/* FUNCTIONS *****************************************************************/
.intel_syntax noprefix

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

    /* Setup registers for compare */
    mov rsi, rcx
    mov rdi, rdx

    /* Clear direction flag */
    cli

    /* Get number of qwords */
    mov rcx, r8
    shr rcx, 3
    jz 2f

    /* Compare qwords */
    repe cmpsq
    jnz 4f

2:  /* Compare rest */
    mov rcx, r8
    and rcx, 7
    jz 3f

    repe cmpsb
    jnz 5f

3:  /* All equal */
    /* Return the full count */
    mov rax, rcx
    jmp 6f

4:  /* Not equal after comparing qwords */
    /* Compare the last qword */
    sub rsi, 8
    sub rdi, 8
    mov rcx, 8
    repe cmpsb

5:  /* Not equal after comparing bytes */
    /* Return difference */
    sub rdi, rdx
    dec rdi
    mov rax, rdi

6:  /* Cleanup and return */
    pop rdi
    pop rsi
    ret
.endproc

