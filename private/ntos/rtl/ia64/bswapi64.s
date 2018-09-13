//      TITLE("Byte Swap Functions")
//++
//
// Copyright (c) 1998  Intel Corporation
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
//    William Cheung (v-willc) 30-Nov-1998
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//--

#include "ksia64.h"

        SBTTL("RtlUshortByteSwap")
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

         shl        t0 = a0, 48
         ;;
         mux1       v0 = t0, @rev
         br.ret.sptk brp

         LEAF_EXIT(RtlUshortByteSwap)

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

         shl        t0 = a0, 32
         ;;
         mux1       v0 = t0, @rev
         br.ret.sptk brp

         LEAF_EXIT(RtlUlongByteSwap)

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

         LEAF_ENTRY(RtlUlonglongByteSwap)

         mux1       v0 = a0, @rev
         br.ret.sptk brp

         LEAF_EXIT(RtlUlonglongByteSwap)
