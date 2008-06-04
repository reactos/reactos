/*
 * NTOSKRNL Pools test routines KM-Test
 * ReactOS Kernel Mode Regression Testing framework
 *
 * Copyright 2008 Aleksey Bragin <aleksey@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include <ntifs.h>
#include <ndk/ntndk.h>
#include "kmtest.h"

//#define NDEBUG
#include "debug.h"

#define TAG_POOLTEST TAG('P','t','s','t')

/* PRIVATE FUNCTIONS ***********************************************************/

VOID
PoolsTest()
{
    PVOID Ptr;
    ULONG AllocSize, i, AllocNumber;
    PVOID *Allocs;

    StartTest();

    // Stress-test nonpaged pool
    for (i=1; i<10000; i++)
    {
        // make up some increasing, a bit irregular size
        AllocSize = i*10;

        if (i % 10)
            AllocSize++;

        if (i % 25)
            AllocSize += 13;

        // start with non-paged pool
        Ptr = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);

        // it may fail due to no-memory condition
        if (!Ptr) break;

        // try to fully fill it
        RtlFillMemory(Ptr, AllocSize, 0xAB);

        // free it
        ExFreePoolWithTag(Ptr, TAG_POOLTEST);
    }

    // now paged one
    for (i=1; i<10000; i++)
    {
        // make up some increasing, a bit irregular size
        AllocSize = i*50;

        if (i % 10)
            AllocSize++;

        if (i % 25)
            AllocSize += 13;

        // start with non-paged pool
        Ptr = ExAllocatePoolWithTag(PagedPool, AllocSize, TAG_POOLTEST);

        // it may fail due to no-memory condition
        if (!Ptr) break;

        // try to fully fill it
        RtlFillMemory(Ptr, AllocSize, 0xAB);

        // free it
        ExFreePoolWithTag(Ptr, TAG_POOLTEST);
    }

    // test super-big allocations
    /*AllocSize = 2UL * 1024 * 1024 * 1024;
    Ptr = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);
    ok(Ptr == NULL, "Allocating 2Gb of nonpaged pool should fail\n");

    Ptr = ExAllocatePoolWithTag(PagedPool, AllocSize, TAG_POOLTEST);
    ok(Ptr == NULL, "Allocating 2Gb of paged pool should fail\n");*/

    // now test allocating lots of small/medium blocks
    AllocNumber = 100000;
    Allocs = ExAllocatePoolWithTag(PagedPool, sizeof(Allocs) * AllocNumber, TAG_POOLTEST);

    // alloc blocks
    for (i=0; i<AllocNumber; i++)
    {
        AllocSize = 42;
        Allocs[i] = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);
    }

    // now free them
    for (i=0; i<AllocNumber; i++)
    {
        ExFreePoolWithTag(Allocs[i], TAG_POOLTEST);
    }


    ExFreePoolWithTag(Allocs, TAG_POOLTEST);


    FinishTest("NTOSKRNL Pools Tests");
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NtoskrnlPoolsTest()
{
    PoolsTest();
}
