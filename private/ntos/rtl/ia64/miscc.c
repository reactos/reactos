/*++

Copyright (c) 1996  Intel Corporation
Copyright (c) 1990  Microsoft Corporation

Module Name:

    miscc.c

Abstract:

    This module implements the functions that get the memory stack
    and backing store limits.

Author:

    William K. Cheung (wcheung) 09-Aug-1996

Environment:

    Any mode.

Revision History:

--*/

#include "ntrtlp.h"

VOID
RtlpFlushRSE (
    OUT PULONGLONG BackingStore,
    OUT PULONGLONG RNat
    );


VOID
RtlpCaptureRnats (
   IN OUT PCONTEXT ContextRecord
   )

/*++

Routine Description:

    This function captures Nat bits of all the stacked registers in
    the RSE frame specified in the context record.
--*/
{
    SHORT BsFrameSize;                  // in 8-byte units
    SHORT TempFrameSize;                // in 8-byte units
    SHORT RNatSaveIndex;
    ULONGLONG Rnat;
    ULONGLONG Bsp;
    ULONGLONG TopRnatAddress;

    RtlpFlushRSE(&Bsp, &Rnat);

    BsFrameSize = (SHORT)ContextRecord->StIFS & PFS_SIZE_MASK;
    RNatSaveIndex = (SHORT)(ContextRecord->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG;
    TempFrameSize = RNatSaveIndex + BsFrameSize - NAT_BITS_PER_RNAT_REG;
    while (TempFrameSize >= 0) {
        BsFrameSize++;
        TempFrameSize -= NAT_BITS_PER_RNAT_REG;
    }
    TopRnatAddress = (ContextRecord->RsBSP + BsFrameSize * 8) | RNAT_ALIGNMENT;
    if (TopRnatAddress < Bsp) {
        ContextRecord->RsRNAT = *(PULONGLONG)TopRnatAddress;
    } else {
        ContextRecord->RsRNAT = Rnat;
    }
}


VOID
Rtlp64GetBStoreLimits (
    OUT PULONGLONG LowBStoreLimit,
    OUT PULONGLONG HighBStoreLimit
    )

/*++

Routine Description:

    This function returns the current backing store limits based on the 
    current processor mode.

Arguments:

    LowBStoreLimit - Supplies a pointer to a variable that is to receive
        the low limit of the backing store.

    HighBStoreLimit - Supplies a pointer to a variable that is to receive
        the high limit of the backing store.

Return Value:

    None.

--*/

{
#if defined(NTOS_KERNEL_RUNTIME)

    //
    // Kernel Mode
    //

    *LowBStoreLimit = (ULONGLONG)(PCR->InitialBStore);
    *HighBStoreLimit = (ULONGLONG)(PCR->BStoreLimit);

#else

    //
    // User Mode
    //

    PTEB CurrentTeb = NtCurrentTeb();

    *HighBStoreLimit = (ULONGLONG)CurrentTeb->BStoreLimit;
    *LowBStoreLimit = (ULONGLONG)CurrentTeb->NtTib.StackBase;

#endif // defined(NTOS_KERNEL_RUNTIME)
}
    
VOID
RtlpGetStackLimits (
    OUT PULONG_PTR LowStackLimit,
    OUT PULONG_PTR HighStackLimit
    )

/*++

Routine Description:

    This function returns the current memory stack limits based on the 
    current processor mode.

Arguments:

    LowStackLimit - Supplies a pointer to a variable that is to receive
        the low limit of the memory stack.

    HighStackLimit - Supplies a pointer to a variable that is to receive
        the high limit of the memory stack.

Return Value:

    None.

--*/

{

#if defined(NTOS_KERNEL_RUNTIME)

    //
    // Kernel Mode
    //

    *HighStackLimit = (ULONG_PTR)PCR->InitialStack;
    *LowStackLimit = (ULONG_PTR)PCR->StackLimit;

#else

    //
    // User Mode
    //

    PTEB CurrentTeb = NtCurrentTeb();

    *HighStackLimit = (ULONG_PTR)CurrentTeb->NtTib.StackBase;
    *LowStackLimit = (ULONG_PTR)CurrentTeb->NtTib.StackLimit;

#endif // defined(NTOS_KERNEL_RUNTIME)
}

VOID
Rtlp64GetStackLimits (
    OUT PULONGLONG LowStackLimit,
    OUT PULONGLONG HighStackLimit
    )

/*++

Routine Description:

    This function returns the current memory stack limits based on the 
    current processor mode.

Arguments:

    LowStackLimit - Supplies a pointer to a variable that is to receive
        the low limit of the memory stack.

    HighStackLimit - Supplies a pointer to a variable that is to receive
        the high limit of the memory stack.

Return Value:

    None.

--*/

{

#if defined(NTOS_KERNEL_RUNTIME)

    //
    // Kernel Mode
    //

    *HighStackLimit = (ULONG_PTR)PCR->InitialStack;
    *LowStackLimit = (ULONG_PTR)PCR->StackLimit;

#else

    //
    // User Mode
    //

    PTEB CurrentTeb = NtCurrentTeb();

    *HighStackLimit = (ULONGLONG)CurrentTeb->NtTib.StackBase;
    *LowStackLimit = (ULONGLONG)CurrentTeb->NtTib.StackLimit;

#endif // defined(NTOS_KERNEL_RUNTIME)
}
