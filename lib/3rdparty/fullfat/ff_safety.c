/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/

/**
 *	@file		ff_safety.c
 *	@author		James Walmsley
 *	@ingroup	SAFETY
 *
 *	@defgroup	SAFETY	Process Safety for FullFAT
 *	@brief		Provides semaphores, and thread-safety for FullFAT.
 *
 *	This module aims to be as portable as possible. It is necessary to modify
 *	the functions FF_CreateSemaphore, FF_PendSemaphore, FF_ReleaseSemaphore,
 *  and FF_DestroySemaphore, as appropriate for your platform.
 *
 *	If your application has no OS and is therefore single threaded, simply
 *	have:
 *
 *	FF_CreateSemaphore() return NULL.
 *
 *	FF_PendSemaphore() should do nothing.
 *
 *	FF_ReleaseSemaphore() should do nothing.
 *
 *	FF_DestroySemaphore() should do nothing.
 *
 **/

#include "ff_safety.h"	// Íncludes ff_types.h
#include <ntifs.h>

#define TAG_FULLFAT 'FLUF'

// Call your OS's CreateSemaphore function
//
void *FF_CreateSemaphore(void) {
    PKSEMAPHORE ProcessSemaphore;

    /* Allocate some memory to store the semaphore */
    ProcessSemaphore = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(KSEMAPHORE),
                                             TAG_FULLFAT);
    if (ProcessSemaphore)
    {
        /* Initialize it */
        KeInitializeSemaphore(ProcessSemaphore,
                              0,
                              MAXLONG);
    }

    return ProcessSemaphore;
}

// Call your OS's PendSemaphore with the provided pSemaphore pointer.
//
// This should block indefinitely until the Semaphore
// becomes available. (No timeout!)
// If your OS doesn't do it for you, you should sleep
// this thread until the Semaphore is available.
void FF_PendSemaphore(void *pSemaphore) {
    NTSTATUS Status;

    /* Sanity check */
    if (pSemaphore)
    {
        /* Wait for the sempaphore to become signaled */
        Status = KeWaitForSingleObject(pSemaphore,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        if (NT_SUCCESS(Status))
        {
            if (Status != STATUS_SUCCESS)
            {
                // log an error?
            }
        }
        else
        {
            // log an error?
        }
    }
}

// Call your OS's ReleaseSemaphore with the provided pSemaphore pointer.
//
void FF_ReleaseSemaphore(void *pSemaphore) {

    /* Sanity check */
    if (pSemaphore)
    {
        /* Increment the semaphore */
        KeReleaseSemaphore(pSemaphore,
                           0,
                           1,
                           FALSE);
    }
}

// Call your OS's DestroySemaphore with the provided pSemaphore pointer.
//
void FF_DestroySemaphore(void *pSemaphore) {

    /* Sanity check */
    if (pSemaphore)
    {
        /* Free the semaphore memory */
        ExFreePoolWithTag(pSemaphore,
                          TAG_FULLFAT);
    }
}

// FIXME: what do we do with this?
void FF_Yield(void) {
	// Call your OS's thread Yield function.
	// If this doesn't work, then a deadlock will occur
}

// Call your OS's thread sleep function,
// Sleep for TimeMs milliseconds
void FF_Sleep(FF_T_UINT32 TimeMs) {
    LARGE_INTEGER Interval;
    NTSTATUS Status;

    /* Calculate the interval */
    Interval.QuadPart = -((LONGLONG)TimeMs * 10000);

    /* Do the wait */
    Status = KeDelayExecutionThread(KernelMode,
                                    FALSE,
                                    &Interval);
    if (!NT_SUCCESS(Status))
    {
        // log an error?
    }
}


/**
 *	Notes on implementation.
 *
 *
 *
 **/


