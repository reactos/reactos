/*
 * NTOSKRNL Executive Regressions KM-Test
 * ReactOS Kernel Mode Regression Testing framework
 *
 * Copyright 2006 Aleksey Bragin <aleksey@reactos.org>
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

#define NDEBUG
#include "debug.h"

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
KeStallTest(HANDLE KeyHandle)
{
    ULONG i;
    LARGE_INTEGER TimeStart, TimeFinish;

    StartTest();

    DPRINT1("Waiting for 30 secs with 50us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000*20); i++)
    {
        KeStallExecutionProcessor(50);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30

    DPRINT1("Waiting for 30 secs with 1000us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000); i++)
    {
        KeStallExecutionProcessor(1000);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30

    DPRINT1("Waiting for 30 secs with 1us stalls...\n");
    KeQuerySystemTime(&TimeStart);
    for (i = 0; i < (30*1000*1000); i++)
    {
        KeStallExecutionProcessor(1);
    }
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 43

    DPRINT1("Waiting for 30 secs with one huge stall...\n");
    KeQuerySystemTime(&TimeStart);
    KeStallExecutionProcessor(30*1000000);
    KeQuerySystemTime(&TimeFinish);
    DPRINT1("Time elapsed: %d secs\n", (TimeFinish.QuadPart - TimeStart.QuadPart) / 10000000); // 30

    FinishTest(KeyHandle, L"KeStallmanExecutionTest");
}
