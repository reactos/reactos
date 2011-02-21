/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Byte swap functions
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 */

.globl RtlUshortByteSwap
.globl RtlUlongByteSwap
.globl RtlUlonglongByteSwap

/* FUNCTIONS ***************************************************************/

RtlUshortByteSwap:
    /* Swap high and low bits */
    rlwinm 4,3,24,0xff
    rlwinm 5,3,8,0xff00
    or 3,4,5
    blr

RtlUlongByteSwap:
    rlwinm 4,3,8,0xff
    rlwinm 5,3,24,0xff000000
    or 4,4,5
    rlwinm 5,3,8,0xff0000
    rlwinm 3,3,24,0xff00
    or 3,4,5
    or 3,3,6
    blr

RtlUlonglongByteSwap:
    stwu 1,16(1)
    stw 4,4(1)
    bl RtlUlongByteSwap
    stw 3,4(1)
    lwz 3,4(1)
    bl RtlUlongByteSwap
    lwz 4,4(1)
    subi 1,1,16
    blr
