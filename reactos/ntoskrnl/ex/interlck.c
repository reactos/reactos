/* $Id: interlck.c,v 1.4 2000/06/07 13:04:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/interlck.c
 * PURPOSE:         Implements interlocked functions 
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>


/* FUNCTIONS *****************************************************************/

INTERLOCKED_RESULT
STDCALL
ExInterlockedDecrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	)
{
        KIRQL oldlvl;
        LONG  oldval;

        KeAcquireSpinLock (Lock, &oldlvl);

        oldval = *Addend;
        (*Addend)--;

        KeReleaseSpinLock (Lock, oldlvl);

        return oldval;
}


ULONG
STDCALL
ExInterlockedExchangeUlong (
	PULONG		Target,
	ULONG		Value,
	PKSPIN_LOCK	Lock
	)
{
        KIRQL oldlvl;
        LONG  oldval;

        KeAcquireSpinLock (Lock, &oldlvl);

        oldval = *Target;
        *Target = Value;

        KeReleaseSpinLock (Lock, oldlvl);

        return oldval;
}


ULONG
STDCALL
ExInterlockedAddUlong (
	PULONG		Addend,
	ULONG		Increment,
	PKSPIN_LOCK	Lock
	)
{
        KIRQL oldlvl;
        ULONG oldval;

        KeAcquireSpinLock (Lock, &oldlvl);

        oldval = *Addend;
        *Addend += Increment;

        KeReleaseSpinLock (Lock, oldlvl);

        return oldval;
}

LARGE_INTEGER
STDCALL
ExInterlockedAddLargeInteger (
        PLARGE_INTEGER Addend,
        LARGE_INTEGER Increment,
        PKSPIN_LOCK Lock
        )
{
        KIRQL oldlvl;
        LARGE_INTEGER oldval;


        KeAcquireSpinLock (Lock, &oldlvl);


        oldval.QuadPart = Addend->QuadPart;
        Addend->QuadPart += Increment.QuadPart;

        KeReleaseSpinLock (Lock, oldlvl);

        return oldval;
}

INTERLOCKED_RESULT
STDCALL
ExInterlockedIncrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	)
{
        KIRQL oldlvl;
        LONG  oldval;

        KeAcquireSpinLock (Lock, &oldlvl);

        oldval = *Addend;
        (*Addend)++;

        KeReleaseSpinLock (Lock, oldlvl);

        return oldval;
}

/* EOF */
