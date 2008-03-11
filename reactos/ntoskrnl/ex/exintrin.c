/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/exinrin.c
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

/* FUNCTIONS ******************************************************************/

LONG
FASTCALL
InterlockedIncrement(IN LONG volatile *Addend)
{
    //
    // Call the intrinsic
    //
    return _InterlockedIncrement(Addend);    
}

LONG
FASTCALL
InterlockedDecrement(IN LONG volatile *Addend)
{
    //
    // Call the intrinsic
    //
    return _InterlockedDecrement(Addend);
}

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
