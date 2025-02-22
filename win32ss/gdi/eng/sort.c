/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:          MenuOS win32 subsystem
 * PURPOSE:
 * FILE:             win32ss/gdi/eng/sort.c
 * PROGRAMER:        MenuOS Team
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
