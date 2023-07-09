//++
//
// Module Name:
//
//    fillmem.s
//
// Abstract:
//
//    This module implements functions to move, zero, and fill blocks
//    of memory. If the memory is aligned, then these functions are
//    very efficient.
//
// Author:
//
//
// Environment:
//
//    User or Kernel mode.
//
//--

#include "ksia64.h"

//++
//
// VOID
// RtlFillMemory (
//    IN PVOID destination,
//    IN SIZE_T length,
//    IN UCHAR fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a dword boundary, and then filling 4-byte blocks, followed by any 
//    remaining bytes.
//
// Arguments:
//
//    destination (a0) - Supplies a pointer to the memory to fill.
//
//    length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.  It also returns the Destination pointer
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemory)

        ARGPTR(a0)                        // sign extend the address
        
        ALTERNATE_ENTRY(RtlFillMemory64)

#ifndef _WIN64
        zxt4        a1 = a1
#endif
        mov         t0 = a0
        cmp.eq      pt0 = zero, a1        // length == 0 ?
        add         t1 = -1, a0

        cmp.ge      pt1 = 7, a1
        mov         v0 = a0
 (pt0)  br.ret.spnt brp                   // return if length is zero
        ;;

//
// Align address on dword boundary by determining the number of bytes
// before the next dword boundary by performing an AND operation on
// the 2's complement of the address with a mask value of 0x3.
//

        nop.m       0
        andcm       t1 = 7, t1            // t1 = # bytes before dword boundary
 (pt1)  br.cond.spnt TailSet              // 1 <= length <= 3, br to TailSet
        ;;

        cmp.eq      pt2 = zero, t1        // skip HeadSet if t1 is zero
        dep         t2 = a2, a2, 8, 8
        ;;
        dep         t2 = t2, t2, 16, 16   // t2 = all lower 4 bytes = [fill]
        ;;

        sub         a1 = a1, t1           // a1 = adjusted length
        shl         t3 = t2, 32
        zxt4        t2 = t2
        ;;

        nop.m       0
        or          t2 = t3, t2           // t2 = all 8 bytes = [fill]
 (pt2)  br.cond.sptk SkipHeadSet 

//
// Copy the leading bytes until t1 is equal to zero
//

HeadSet:
        st1         [t0] = a2, 1
        add         t1 = -1, t1
        ;;
        cmp.ne      pt0 = zero, t1

(pt0)   br.cond.spnt HeadSet
        nop.b       0
        nop.b       0

//
// now the address is dword aligned;
// fall into the DwordSet loop if remaining length is greater than 4;
// else skip the DwordSet loop
//

SkipHeadSet:
        cmp.ge      pt1 = 7, a1
        nop.m       0
 (pt1)  br.cond.spnt SkipDwordSet

//
// fill 4 bytes at a time until the remaining length is less than 4
//

DwordSet:
        st8         [t0] = t2, 8
        add         a1 = -8, a1
        ;;
        cmp.le      pt0 = 8, a1

(pt0)   br.cond.spnt DwordSet
        nop.b       0
        nop.b       0

SkipDwordSet:
        cmp.eq      pt3 = zero, a1        // return now if length equals 0
        nop.m       0
(pt3)   br.ret.sptk brp

//
// copy the remaining bytes one at a time
//

TailSet:
        st1         [t0] = a2, 1
        add         a1 = -1, a1
        nop.i       0
        ;;

        cmp.ne      pt0, pt3 = 0, a1
(pt3)   br.ret.sptk brp
(pt0)   br.cond.spnt TailSet
        ;;

        LEAF_EXIT(RtlFillMemory)

//++
//
// VOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern 
//    4 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 2, 30
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ARGPTR(a0)
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmu10:
        st4         [a0] = a2, 4
        br.cloop.dptk.few Rfmu10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.spnt brp

        LEAF_EXIT(RtlFillMemoryUlong)

//++
//
// VOID
// RtlFillMemoryUlonglong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONGLONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified pattern 
//    8 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2,a3) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlonglong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 3, 29
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ARGPTR(a0)
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmul10:
        st8         [a0] = a2, 8
        br.cloop.dptk.few Rfmul10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.spnt brp
        ;;

        LEAF_EXIT(RtlFillMemoryUlonglong)


//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function simply sets up the fill value (out2) and branches
//    directly to RtlFillMemory
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(RtlZeroMemory)

        ARGPTR(a0)

        ALTERNATE_ENTRY(RtlZeroMemory64)
        
        alloc       t22 = ar.pfs, 0, 0, 3, 0
        mov         out2 = 0
        br          RtlFillMemory64

        LEAF_EXIT(RtlZeroMemory)


//++
//
// VOID
// RtlMoveMemory (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned.
//
//    Algorithm:
//        1) Length equals zero, return immediately
//        2) Source & Destination don't overlap, copy from low to high
//           else copy from high to low address one byte at a time
//        3) if Source & Destination are both 8-byte aligned, copy 8 bytes
//           at a time and the remaining bytes are copied one at a time.
//        4) if Source & Destination are both 4-byte aligned, copy 4 bytes
//           at a time and the remaining bytes are copied one at a time.
//        5) else copy one byte at a time from low to high address.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination address of
//       the move operation.
//
//    Source (a1) - Supplies a pointer to the source address of the move
//       operation.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be moved.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(memcpy)
        ALTERNATE_ENTRY(memmove)

        zxt4        a2 = a2
        ;;

        ALTERNATE_ENTRY(RtlMoveMemory)
        ALTERNATE_ENTRY(RtlCopyMemory)

        mov         v0 = a0
        and         t0 = 7, a0
        and         t1 = 7, a1
        ;;

        cmp.le      pt0 = a0, a1
        add         t21 = a1, a2
(pt0)   br.cond.sptk CopyUp
        ;;

        nop.m       0
        cmp.lt      pt1 = a0, t21
(pt1)   br.cond.spnt CopyDown

CopyUp:
        cmp.gt      pt4 = 8, a2
        or          t2 = t0, t1
(pt4)   br.cond.sptk ByteMoveUp
        ;;

        cmp.eq      pt2 = 0, t2
        cmp.eq      pt3 = 4, t2
        cmp.eq      pt5 = t0, t1

(pt2)   br.cond.sptk QwordMoveUp
(pt3)   br.cond.sptk DwordMoveUp
(pt5)   br.cond.sptk AlignedMove

ByteMoveUp:
        cmp.eq      pt0 = zero, a2
        cmp.lt      pt1 = 32, a2
        nop.i       0

 (pt1)  br.cond.sptk UnalignedMove
 (pt0)  br.ret.spnt brp
        nop.b       0

ByteMoveUpLoop:

        ld1         t10 = [a1], 1
        add         a2 = -1, a2
        ;;
        cmp.eq      pt0, pt1 = zero, a2

        st1         [a0] = t10, 1
 (pt1)  br.cond.sptk ByteMoveUpLoop
 (pt0)  br.ret.spnt brp


UnalignedMove:
        nop.m       0
        add         t20 = -1, a1
        ;;
        andcm       t20 = 7, t20
        ;;

        cmp.eq      pt0 = zero, t20
 (pt0)  br.cond.spnt SkipUnalignedMoveByteLoop
        nop.b       0

UnalignedMoveByteLoop:

        ld1         t10 = [a1], 1
        add         t20 = -1, t20
        add         a2 = -1, a2
        ;;

        cmp.eq      pt0, pt1 = zero, t20
        st1         [a0] = t10, 1
 (pt1)  br.cond.sptk UnalignedMoveByteLoop
        ;;


SkipUnalignedMoveByteLoop:

        ld8         t12 = [a1], 8
        and         t20 = 7, a0
        mov         t18 = 1
        ;;

        sub         t15 = a0, t20
        shl         t21 = t20, 3
        ;;
        sub         t22 = 64, t21
        ;;

        sub         t16 = 8, t20
        shl         t18 = t18, t22
        ;;
        add         t18 = -1, t18
        ;;

        ld8         t13 = [t15]
        andcm       t19 = -1, t18
        ;;
        shr.u       t19 = t19, t22
        ;;

SpecialLoop:

        add         a2 = -8, a2
        shl         t10 = t12, t21
        and         t11 = t13, t19
        ;;

        cmp.lt      pt0 = 8, a2
        shr         t13 = t12, t22
        or          t10 = t10, t11
        ;;

        st8         [t15] = t10, 8
 (pt0)  ld8         t12 = [a1], 8
 (pt0)  br.cond.dptk SpecialLoop
        ;;

        add         a2 = t20, a2
        sub         a1 = a1, t20
        mov         a0 = t15

        br          ByteMoveUpLoop
        nop.b       0
        nop.b       0

AlignedMove:

        nop.m       0
        sub         t22 = 8, t0
        ;;
        sub         a2 = a2, t22

AlignedMoveByteLoop:

        ld1         t10 = [a1], 1
        add         t22 = -1, t22
        ;;
        cmp.ne      pt1 = zero, t22

        st1         [a0] = t10, 1
 (pt1)  br.cond.sptk AlignedMoveByteLoop
        nop.b       0

        cmp.gt      pt2 = 8, a2
 (pt2)  br.cond.spnt ByteMoveUp
        nop.b       0

//
// both src & dest are now 8-byte aligned
//

QwordMoveUp:
        ld8         t10 = [a1], 8
        add         a2 = -8, a2
        ;;
        cmp.gt      pt0, pt1 = 8, a2
        
        st8         [a0] = t10, 8
 (pt1)  br.cond.sptk QwordMoveUp
 (pt0)  br.cond.spnt ByteMoveUp

DwordMoveUp:
        ld4         t10 = [a1], 4
        add         a2 = -4, a2
        ;;
        cmp.gt      pt0, pt1 = 4, a2
        
        st4         [a0] = t10, 4
 (pt1)  br.cond.sptk DwordMoveUp
 (pt0)  br.cond.spnt ByteMoveUp
        ;;

CopyDown:
        cmp.eq      pt0 = zero, a2
        cmp.ne      pt6 = t0, t1
(pt0)   br.ret.spnt brp                       // return if length is zero

        cmp.gt      pt4 = 8, a2
        add         t20 = a2, a0
        add         t21 = a2, a1

        nop.m       0
(pt6)   br.cond.spnt ByteMoveDown             // incompatible alignment
(pt4)   br.cond.sptk ByteMoveDown             // less than 8 bytes to copy
        ;;

        nop.m       0
        nop.m       0
        and         t22 = 0x7, t21
        ;;

        add         t20 = -1, t20
        add         t21 = -1, t21
        sub         a2 = a2, t22
        ;;

TailMove:
        cmp.eq      pt0, pt1 = zero, t22
        ;;

 (pt1)  ld1         t10 = [t21], -1
 (pt1)  add         t22 = -1, t22
        ;;

 (pt1)  st1         [t20] = t10, -1
 (pt1)  br.cond.sptk TailMove
        nop.b       0


Block8Move:
        nop.m       0
        add         t20 = -7, t20
        add         t21 = -7, t21
        ;;

Block8MoveLoop:
        cmp.gt      pt5, pt6 = 8, a2
        ;;

(pt6)   ld8         t10 = [t21], -8
(pt6)   add         a2 = -8, a2
        ;;

(pt6)   st8         [t20] = t10, -8
(pt6)   br.cond.sptk Block8MoveLoop
        nop.b       0

        nop.m       0        
        add         t20 = 8, t20             // adjust source
        add         t21 = 8, t21             // adjust destination
        ;;

ByteMoveDown:

        nop.m       0
        add         t20 = -1, t20             // adjust source
        add         t21 = -1, t21             // adjust destination
        ;;

ByteMoveDownLoop:

        cmp.eq      pt0, pt1 = zero, a2
        ;;

 (pt1)  ld1         t10 = [t21], -1
 (pt1)  add         a2 = -1, a2
        ;;

 (pt1)  st1         [t20] = t10, -1
 (pt1)  br.cond.sptk ByteMoveDownLoop
 (pt0)  br.ret.spnt brp
        ;;

        LEAF_EXIT(RtlMoveMemory)


        LEAF_ENTRY(RtlCompareMemory)

        cmp.eq      pt0 = 0, a2
        mov         v0 = 0
 (pt0)  br.ret.sptk.many brp
        ;;

        add         t2 = -1, a2

Rcmp10:
        ld1         t0 = [a0], 1
        ld1         t1 = [a1], 1
        ;;
        cmp4.eq     pt2 = t0, t1
        ;;

 (pt2)  cmp.ne.unc  pt1 = v0, t2
 (pt2)  add         v0 = 1, v0
 (pt1)  br.cond.dptk.few Rcmp10

        br.ret.sptk.many brp

        LEAF_EXIT(RtlCompareMemory)


//++
//
// VOID
// RtlCopyIa64FloatRegisterContext (
//     PFLOAT128 Destination, 
//     PFLOAT128 Source, 
//     ULONGLONG Length
//     )
//
// Routine Description:
//
//    This routine copies floating point context from one place to
//    another.  It assumes both the source and the destination are
//    16-byte aligned and the buffer contains only memory image of
//    floating point registers.  Note that Length must be greater
//    than 0 and a multiple of 16.
//
// Arguments:
//
//    a0 - Destination
//    a1 - Source
//    a2 - Length
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(RtlCopyIa64FloatRegisterContext)

        .prologue
        .save       ar.lc, t22

        ARGPTR(a0)
        ARGPTR(a1)

        mov         t22 = ar.lc
        shr         t0 = a2, 4
        ;;

        cmp.gtu     pt0, pt1 = 16, a2
        add         t0 = -1, t0
        ;;

        PROLOGUE_END

(pt1)   mov         ar.lc = t0
(pt0)   br.ret.spnt brp

Rcf10:

        ldf.fill    ft0 = [a1], 16
        nop.m       0
        nop.i       0
        ;;

        stf.spill   [a0] = ft0, 16
        nop.i       0
        br.cloop.dptk Rcf10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        NESTED_EXIT(RtlCopyIa64FloatRegisterContext)



        NESTED_ENTRY(RtlpCopyContextSubSet)

        .prologue
        .save       ar.lc, t22

        ARGPTR(a0)
        ARGPTR(a1)
        mov         t22 = ar.lc
        mov         t0 = a0
        mov         t1 = a1
        ;;

        PROLOGUE_END

        ld8         t3 = [t1], CxFltS0
        ;;
        st8         [t0] = t3, CxFltS0
        mov         t2 = 3

        add         t10 = CxFltS4, a0
        add         t11 = CxFltS4, a1
        ;;
        mov         ar.lc = t2

Rcc10:
        ldf.fill    ft0 = [t1], 16
        ;;
        stf.spill   [t0] = ft0, 16
        mov         t2 = 15
        br.cloop.dptk.few Rcc10
        ;;

        mov         t0 = CxStIFS
        mov         t1 = CxStFPSR
        mov         ar.lc = t2

Rcc20:
        ldf.fill    ft0 = [t11], 16
        ;;
        stf.spill   [t10] = ft0, 16
        sub         t2 = t0, t1
        br.cloop.dptk.few Rcc20
        ;;

        add         t11 = CxStFPSR, a1
        add         t10 = CxStFPSR, a0
        shr         t2 = t2, 3
        ;;

        mov         ar.lc = t2
        ;;

Rcc30:
        ld8         t0 = [t11], 8
        ;;
        st8         [t10] = t0, 8
        nop.i       0

        br.cloop.dptk.few Rcc30
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp

        NESTED_EXIT(RtlpCopyContextSubSet)
