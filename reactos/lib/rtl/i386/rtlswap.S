/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Byte swap functions
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 *                    Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>

PUBLIC @RtlUshortByteSwap@4
PUBLIC @RtlUlongByteSwap@4
PUBLIC @RtlUlonglongByteSwap@8

/* FUNCTIONS ***************************************************************/
.code

@RtlUshortByteSwap@4:
FUNC RtlUshortByteSwap
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Swap high and low bits */
    mov ah, cl
    mov al, ch
    ret
ENDFUNC RtlUshortByteSwap

@RtlUlongByteSwap@4:
FUNC RtlUlongByteSwap
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Swap high and low bits */
    mov eax, ecx
    bswap eax
    ret
ENDFUNC RtlUlongByteSwap

@RtlUlonglongByteSwap@8:
FUNC RtlUlonglongByteSwap
    FPO 0, 2, 0, 0, 0, FRAME_FPO

    /* Get 64-bit integer */
    mov eax, [esp+8]
    mov edx, [esp+4]

    /* Swap it */
    bswap edx
    bswap eax

    /* Return it (NOTE: this might look wrong, since fastcall functions
       should clean up the stack, even if the first parameter is an ULONGLONG,
       and therefore put on tthe stack instead of in ecx and edx,
       but thats exactly how the function behaves on Windows! */
    ret
ENDFUNC RtlUlonglongByteSwap

END
