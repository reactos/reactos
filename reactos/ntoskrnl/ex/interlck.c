/* $Id: interlck.c,v 1.5 2000/07/04 01:27:58 ekohl Exp $
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

VOID
FASTCALL
ExInterlockedAddLargeStatistic (
	IN	PLARGE_INTEGER	Addend,
	IN	ULONG		Increment
	)
{
	Addend->QuadPart += Increment;
}

LONGLONG
FASTCALL
ExInterlockedCompareExchange64 (
	IN OUT	PLONGLONG	Destination,
	IN	PLONGLONG	Exchange,
	IN	PLONGLONG	Comparand,
	IN	PKSPIN_LOCK	Lock
	)
{
	KIRQL oldlvl;
	LONGLONG oldval;

	KeAcquireSpinLock (Lock, &oldlvl);

	oldval = *Destination;
	if (*Destination == *Comparand)
	{
		*Destination = *Exchange;
	}

	KeReleaseSpinLock (Lock, oldlvl);

	return oldval;
}


/* EOF */
