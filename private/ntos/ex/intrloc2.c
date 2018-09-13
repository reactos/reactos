/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    intrloc2.c

Abstract:

   This module implements the *portable*  (i.e. SLOW) versions of
   the executive's simple atomic increment/decrement procedures.
   Real implementation should be in assembler.

Author:

    Bryan Willman  (bryanwi)  2-Aug-90

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

INTERLOCKED_RESULT
ExInterlockedIncrementLong (
    IN PLONG Addend,
    IN PKSPIN_LOCK Lock
    )

/*++

Routine Description:

    This function atomically increments Addend, returning an ennumerated
    type which indicates what interesting transitions in the value of
    Addend occurred due the operation.

Arguments:

    Addend - Pointer to variable to increment.

    Lock - Spinlock used to implement atomicity.

Return Value:

    An ennumerated type:

    ResultNegative if Addend is < 0 after increment.
    ResultZero     if Addend is = 0 after increment.
    ResultPositive if Addend is > 0 after increment.

--*/

{
    LONG    OldValue;

    OldValue = (LONG)ExInterlockedAddUlong((PULONG)Addend, 1, Lock);

    if (OldValue < -1)
        return ResultNegative;

    if (OldValue == -1)
        return ResultZero;

    if (OldValue > -1)
        return ResultPositive;
}

INTERLOCKED_RESULT
ExInterlockedDecrementLong (
    IN PLONG Addend,
    IN PKSPIN_LOCK Lock
    )

/*++

Routine Description:

    This function atomically decrements Addend, returning an ennumerated
    type which indicates what interesting transitions in the value of
    Addend occurred due the operation.

Arguments:

    Addend - Pointer to variable to decrement.

    Lock - Spinlock used to implement atomicity.

Return Value:

    An ennumerated type:

    ResultNegative if Addend is < 0 after decrement.
    ResultZero     if Addend is = 0 after decrement.
    ResultPositive if Addend is > 0 after decrement.

--*/

{
    LONG    OldValue;

    OldValue = (LONG)ExInterlockedAddUlong((PULONG)Addend, -1, Lock);

    if (OldValue > 1)
        return ResultPositive;

    if (OldValue == 1)
        return ResultZero;

    if (OldValue < 1)
        return ResultNegative;
}
