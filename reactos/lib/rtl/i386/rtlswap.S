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

FUNC @RtlUshortByteSwap@4
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Swap high and low bits */
    mov ah, cl
    mov al, ch
    ret
ENDFUNC

FUNC @RtlUlongByteSwap@4
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Swap high and low bits */
    mov eax, ecx
    bswap eax
    ret
ENDFUNC

FUNC @RtlUlonglongByteSwap@8
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
ENDFUNC

END
