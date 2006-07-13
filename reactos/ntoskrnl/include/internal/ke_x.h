/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ke_x.h
* PURPOSE:         Internal Inlined Functions for the Kernel
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Guarded Region Routines
//
#define KeEnterGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT((Thread->SpecialApcDisable <= 0) &&      \
           (Thread->SpecialApcDisable != -32768));  \
                                                    \
    /* Disable Special APCs */                      \
    Thread->SpecialApcDisable--;                    \
}

#define KeLeaveGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT(Thread->SpecialApcDisable < 0);          \
                                                    \
    /* Leave region and check if APCs are OK now */ \
    if (!(++Thread->SpecialApcDisable))             \
    {                                               \
        /* Check for Kernel APCs on the list */     \
        if (!IsListEmpty(&Thread->ApcState.         \
                         ApcListHead[KernelMode]))  \
        {                                           \
            /* Check for APC Delivery */            \
            KiCheckForKernelApcDelivery();          \
        }                                           \
    }                                               \
}

//
// TODO: Guarded Mutex Routines
//

//
// TODO: Critical Region Routines
//

//
// TODO: Wait Routines
//

