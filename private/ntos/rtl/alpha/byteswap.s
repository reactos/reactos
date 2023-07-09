//      TITLE("Byte Swap Functions")
//++
//
// Copyright (c) 1997  Microsoft Corporation
//
// Module Name:
//
//    byteswap.s
//
// Abstract:
//
//    This module implements functions to perform byte swapping operations.
//
// Author:
//
//    Forrest Foltz (forrestf) 29-Dec-1997
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("RtlUlongByteSwap")
//++
//
// USHORT
// RtlUshortByteSwap(
//    IN USHORT Source
//    )
//
// Routine Description:
//
//    The RtlUshortByteSwap function exchanges bytes 0 and 1 of Source
//    and returns the resulting USHORT.
//
// Arguments:
//
//    Source (a0[0..15]) - 16-bit value to byteswap.
//
// Return Value:
//
//    The 16-bit integer result is returned as the function value
//    in v0[0..15].
//
//--

        LEAF_ENTRY(RtlUshortByteSwap)
										          // a0 = ......AB
        sll     a0, 8, t0               // t0 = .....AB.
        srl     a0, 8, t1               // t1 = .......A
        zapnot  t0, 2, t0               // t0 = ......B.
        bis     t0, t1, v0              // v0 = ......BA
        ret     zero, (ra)              // return

        .end    RtlUshortByteSwap

        SBTTL("RtlUlongByteSwap")
//++
//
// ULONG
// RtlUlongByteSwap(												
//    IN ULONG Source
//    )
//
// Routine Description:
//
//    The RtlUlongByteSwap function exchanges byte pairs 0:3 and 1:2 of
//    Source and returns the the resulting ULONG.
//
// Arguments:
//
//    Source (a0[0..31]) - 32-bit value to byteswap.
//
// Return Value:
//
//    The 32-bit integer result is returned as the function value
//    in v0[0..31].
//
//--

        LEAF_ENTRY(RtlUlongByteSwap)
													 // a0 = ....ABCD
        sll     a0, 24, t0              // t0 = .ABCD000
        sll     a0, 8, t1               // t1 = ...ABCD0
        srl     a0, 8, t2               // t2 = 0....ABC
        extbl   a0, 3, t3               // t3 = 0000000A
        zapnot  t2, 2, t2               // t2 = 000000B0
        zapnot  t1, 4, t1               // t1 = 00000C00
        zapnot  t0, 8, v0               // v0 = 0000D000
        bis     v0, t1, v0              // v0 = 0000DC00
        bis     v0, t2, v0              // v0 = 0000DCB0
        addl    v0, t3, v0              // v0 = ....DCBA
        ret     zero, (ra)

        .end    RtlUlongByteSwap

        SBTTL("RtlUlonglongByteSwap")
//++
//
// ULONG
// RtlUlonglongByteSwap(
//    IN ULONGLONG Source
//    )
//
// /*++
//
// Routine Description:
//
//    The RtlUlonglongByteSwap function exchanges byte pairs 0:7, 1:6, 2:5,
//    and 3:4 of Source and returns the resulting ULONGLONG.
//
// Arguments:
//
//    Source (a0[0..63]) - 64-bit value to byteswap.
//
// Return Value:
//
//    Swapped 64-bit value.
//
//--

        .struct 0
BsF0:   .space  8                       // preserved F0
BsTmp:  .space  8                       // temp for floating load/stores
ByteSwapFrameLength:                    // length of frame

        NESTED_ENTRY(RtlUlonglongByteSwap, ByteSwapFrameLength, zero)

        lda     sp, -ByteSwapFrameLength(sp) // allocate stack frame
        stt     f0, BsF0(sp)            // callable at interrupt time,
                                        // so we must preserve f0
        PROLOGUE_END

//
// The floating-point LDG instruction followed by STT does half of the work
// for us, in that it performs a 64-bit "word" swap.
//
// See page A-12 of "Alpha AXP Architecture", 2nd edition for an
// description.
//

        stq     a0, BsTmp(sp)           // BsTmp = ABCD EFGH
        ldg     f0, BsTmp(sp)           // f0    = GHEF CDAB
        stt     f0, BsTmp(sp)           // BsTmp = GHEF CDAB
        ldq     t0, BsTmp(sp)           // t0    = GHEF CDAB

        sll     t0, 8, t1               // t1    = HEFC DAB.
        srl     t0, 8, t0               // t0    = .GHE FCDA
        zap     t1, 0x55, t1            // t1    = H.F. D.B.
        zap     t0, 0xAA, t0            // t0    = .G.E .C.A
        bis     t0, t1, v0              // v0    = HGFE DCBA

        ldt     f0, BsF0(sp)            // restore f.p. register f0
        lda     sp, ByteSwapFrameLength(sp) // clean stack
        ret     zero, (ra)              // return

        .end    RtlUlonglongByteSwap

