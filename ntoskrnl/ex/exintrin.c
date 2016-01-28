/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ex/exintrin.c
 * PURPOSE:         Exported kernel functions which are now intrinsics
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedCompareExchange
#undef InterlockedExchangeAdd
#undef InterlockedExchange

//
// HAL Port to Inlined Port
//
#define H2I(Port) PtrToUshort(Port)

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
LONG
FASTCALL
InterlockedIncrement(IN LONG volatile *Addend)
{
    //
    // Call the intrinsic
    //
    return _InterlockedIncrement(Addend);    
}

/*
 * @implemented
 */
LONG
FASTCALL
InterlockedDecrement(IN LONG volatile *Addend)
{
    //
    // Call the intrinsic
    //
    return _InterlockedDecrement(Addend);
}

/*
 * @implemented
 */
LONG
FASTCALL
InterlockedCompareExchange(IN OUT LONG volatile *Destination,
                           IN LONG Exchange,
                           IN LONG Comperand)
{
    //
    // Call the intrinsic
    //
    return _InterlockedCompareExchange(Destination, Exchange, Comperand);
}

/*
 * @implemented
 */
LONG
FASTCALL
InterlockedExchange(IN OUT LONG volatile *Destination,
                    IN LONG Value)
{
    //
    // Call the intrinsic
    //
    return _InterlockedExchange(Destination, Value);
}

/*
 * @implemented
 */
LONG
FASTCALL
InterlockedExchangeAdd(IN OUT LONG volatile *Addend,
                       IN LONG Increment)
{
    //
    // Call the intrinsic
    //
    return _InterlockedExchangeAdd(Addend, Increment);
}

/*
 * @implemented
 */
VOID
NTAPI
ProbeForRead(IN CONST VOID *Address,
             IN SIZE_T Length,
             IN ULONG Alignment)
{
    ULONG_PTR Last, Current = (ULONG_PTR)Address;
    PAGED_CODE();

    /* Only probe if we have a valid length */
    if (Length != 0)
    {
        /* Sanity check */
        ASSERT((Alignment == 1) ||
               (Alignment == 2) ||
               (Alignment == 4) ||
               (Alignment == 8) ||
               (Alignment == 16));

        /* Check the alignment */
        if ((Current & (Alignment - 1)) != 0)
        {
            /* Incorrect alignment */
            ExRaiseDatatypeMisalignment();
        }
        
        /* Get the end address */
        Last = Current + Length - 1;
        if ((Last < Current) || (Last >= (ULONG_PTR)MmUserProbeAddress))
        {
            /* Raise an access violation */
            ExRaiseAccessViolation();
        }

        /* ProbeForRead doesn't check if memory pages are readable! */
    }
}

/*
 * @implemented
 */
VOID
NTAPI
ProbeForWrite(IN PVOID Address,
              IN SIZE_T Length,
              IN ULONG Alignment)
{
    ULONG_PTR Last, Current = (ULONG_PTR)Address;
    PAGED_CODE();

    /* Only probe if we have a valid length */
    if (Length != 0)
    {
        /* Sanity check */
        ASSERT((Alignment == 1) ||
               (Alignment == 2) ||
               (Alignment == 4) ||
               (Alignment == 8) ||
               (Alignment == 16));

        /* Check the alignment */
        if ((Current & (Alignment - 1)) != 0)
        {
            /* Incorrect alignment */
            ExRaiseDatatypeMisalignment();
        }

        /* Get the end address */
        Last = Current + Length - 1;
        if ((Last < Current) || (Last >= (ULONG_PTR)MmUserProbeAddress))
        {
            /* Raise an access violation */
            ExRaiseAccessViolation();
        }

        /* Round down to the last page */
        Last = PAGE_ROUND_DOWN(Last) + PAGE_SIZE;
        do
        {
            /* Attempt a write */
            *(volatile CHAR*)Current = *(volatile CHAR*)Current;

            /* Go to the next address */
            Current = PAGE_ROUND_DOWN(Current) + PAGE_SIZE;
        } while (Current != Last);
    }
}
