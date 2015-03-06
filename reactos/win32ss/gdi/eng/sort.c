/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:          ReactOS win32 subsystem
 * PURPOSE:
 * FILE:             subsystems/win32k/eng/sort.c
 * PROGRAMER:        ReactOS Team
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
void
APIENTRY
EngSort(IN OUT PBYTE Buf, IN ULONG ElemSize, IN ULONG ElemCount, IN SORTCOMP CompFunc)
{
    qsort(Buf, ElemCount, ElemSize, CompFunc);
}

/* EOF */
