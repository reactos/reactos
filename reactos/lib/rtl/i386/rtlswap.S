/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Byte swap functions
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 */

.intel_syntax noprefix

.globl @RtlUshortByteSwap@4
.globl @RtlUlongByteSwap@4
.globl @RtlUlonglongByteSwap@8

/* FUNCTIONS ***************************************************************/

.func @RtlUshortByteSwap@4, @RtlUshortByteSwap@4
@RtlUshortByteSwap@4:

    /* Swap high and low bits */
    mov ah, cl
    mov al, ch
    ret
.endfunc

.func @RtlUlongByteSwap@4, @RtlUlongByteSwap@4
@RtlUlongByteSwap@4:

    /* Swap high and low bits */
    mov eax, ecx
    bswap eax
    ret
.endfunc

.func @RtlUlonglongByteSwap@8, @RtlUlonglongByteSwap@8
@RtlUlonglongByteSwap@8:

    /* Get 64-bit integer */
    mov edx, [esp+8]
    mov eax, [esp+4]

    /* Swap it */
    bswap edx
    bswap eax

    /* Return it */
    mov ecx, eax
    mov eax, edx
    mov edx, ecx
    ret
.endfunc


