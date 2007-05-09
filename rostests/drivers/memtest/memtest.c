/*
 * Memory Manager Information and Test driver
 *
 * Copyright 2006 Aleksey Bragin <alekset@reactos.org>
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

//#define NDEBUG
#include <debug.h>

#include <ddk/ntddk.h>
#include "memtest.h"

HANDLE MonitorThreadHandle;

/* FUNCTIONS ***********************************************************/

static VOID NTAPI
MonitorThread(PVOID Ignored)
{
    SYSTEM_PERFORMANCE_INFORMATION PerformanceInfo;
    ULONG Length;
    LARGE_INTEGER Interval, SystemTime;

    /* Main loop */
    while (TRUE)
    {
        Interval.QuadPart = -300 * 10000; // 300 ms

        /* Query information */
        if (ZwQuerySystemInformation(SystemPerformanceInformation,
            (PVOID) &PerformanceInfo,
            sizeof(SYSTEM_PERFORMANCE_INFORMATION),
            &Length) != NO_ERROR)
        {
            break;
        }

        /* Query current system time */
        KeQuerySystemTime(&SystemTime);


        DbgPrint("%I64d;%d;%d\n", SystemTime.QuadPart,
            PerformanceInfo.CommittedPages, PerformanceInfo.AvailablePages);

        /* Wait for a bit. */
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }

    DPRINT("Finishing monitoring thread.\n");

    PsTerminateSystemThread(0);
}


VOID
StartMemoryMonitor()
{
    NTSTATUS Status;

    Status = PsCreateSystemThread(
        &MonitorThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        MonitorThread,
        NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to start a monitoring thread\n");
        return;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * DriverEntry
 */
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    DbgPrint("\n===============================================\n Memory Manager Information and Test driver\n");
    DbgPrint("Time;Memory pages allocated;Memory pages free\n");


    StartMemoryMonitor();

    return STATUS_SUCCESS;
}
