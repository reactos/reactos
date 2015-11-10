/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Performance Counter Functions
 * FILE:              win32ss/gdi/eng/perfcnt.c
 * PROGRAMER:         Ge van Geldorp
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
VOID APIENTRY
EngQueryPerformanceFrequency(LONGLONG *Frequency)
{
    LARGE_INTEGER Freq;

    KeQueryPerformanceCounter(&Freq);
    *Frequency = Freq.QuadPart;
}

/*
 * @implemented
 */
VOID APIENTRY
EngQueryPerformanceCounter(LONGLONG *Count)
{
    LARGE_INTEGER PerfCount;

    PerfCount = KeQueryPerformanceCounter(NULL);
    *Count = PerfCount.QuadPart;
}
