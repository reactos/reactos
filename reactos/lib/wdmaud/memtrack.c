/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/memtrack.c
 * PURPOSE:              WDM Audio Support - Memory Tracking
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 18, 2005: Created
 * 
 */

#include "wdmaud.h"

static int alloc_count = 0;

LPVOID AllocMem(DWORD size)
{
    HANDLE heap;
    LPVOID pointer;

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("SEVERE ERROR! Couldn't get process heap! (error %d)\n",
                (int) GetLastError());
        return NULL;
    }

    pointer = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);

    if ( pointer )
        alloc_count ++;

    ReportMem();

    return pointer;
}

VOID FreeMem(LPVOID pointer)
{
    HANDLE heap;

    if ( ! pointer )
    {
        DPRINT1("Trying to free a NULL pointer!\n");
        return;
    }

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("SEVERE ERROR! Couldn't get process heap! (error %d)\n",
                (int) GetLastError());
        return;
    }

    if ( ! HeapFree(heap, 0, pointer) )
    {
        DPRINT("Unable to free memory (error %d)\n", (int)GetLastError());
        return;
    }

    alloc_count --;

    ReportMem();
}

VOID ReportMem()
{
    DPRINT("Memory blocks allocated: %d\n", (int) alloc_count);

    if ( alloc_count < 0 )
        DPRINT1("FREEMEM HAS BEEN CALLED TOO MANY TIMES!\n");
}
