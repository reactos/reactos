//  TITLE("Byte and Short Emulation")
//++
//
//
// Copyright (c) 1995  Digital Equipment Corporation
//
// Module Name:
//
//    byteme.s
//
// Abstract:
//
//    This module implements the code to perform atomic store operations
//    on bytes and shorts (words).
//
//    Atomic byte and short opcodes have been added into the Alpha
//    architecture as of Alpha 21164A or EV56. In chips 21164 and prior,
//    an illegal instruction will occur and an exception will lead them here.
//
// Author:
//
//    Wim Colgate, May 18th, 1995
//    Tom Van Baak, May 18th, 1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// VOID
// KiInterlockedStoreByte(
//    IN PUCHAR Address,
//    IN UCHAR  Data
//    )
//
// Routine Description:
//
//    This routine stores the data byte specified by Data to the location
//    specified by Address. The architecture requires byte granularity,
//    so locking is necessary.
//
// Arguments:
//
//    Address(a0) - Supplies a pointer to byte data value.
//    Data(a1) - Supplied the byte data value to store.
//
// Return Value:
//
//    None
//
//--

        LEAF_ENTRY(KiInterlockedStoreByte)

        bic     a0, 3, t0               // clear low 2 bits
        and     a0, 3, t1               // mask of three low bits
10:     ldl_l   t2, 0(t0)               // load locked full longword
        insbl   a1, t1, t3              // insert byte low
        mskbl   t2, t1, t2              // mask byte low
        bis     t2, t3, t2              // merge data
        stl_c   t2, 0(t0)               // store conditional
        beq     t2, 20f                 // failed to store, retry, forward
        ret     zero, (ra), 1           // return

20:     br      zero, 10b               // try again

        .end    KiInterlockedStoreByte

//++
//
// VOID
// KiInterlockedStoreWord(
//    IN PUSHORT Address,
//    IN USHORT Data
//    )
//
// Routine Description:
//
//    This routine stores the short data specified by Data to the aligned
//    location specified by Address. The architecture requires word granularity,
//    so locking is necessary.
//
// Arguments:
//
//    Address(a0) - Supplies a pointer to an aligned short data value.
//    Data(a1) - Supplied the short data value to store.
//
// Return Value:
//
//    None
//
//--

        LEAF_ENTRY(KiInterlockedStoreWord)

        bic     a0, 2, t0               // clear low short address bit
        and     a0, 2, t1               // mask of low short address bit
10:     ldl_l   t2, 0(t0)               // load locked full longword
        inswl   a1, t1, t3              // insert word low
        mskwl   t2, t1, t2              // mask word low
        bis     t2, t3, t2              // merge data
        stl_c   t2, 0(t0)               // store conditional
        beq     t2, 20f                 // failed to store, retry, forward
        ret     zero, (ra), 1           // return

20:     br      zero, 10b               // try again

        .end    KiInterlockedStoreWord
