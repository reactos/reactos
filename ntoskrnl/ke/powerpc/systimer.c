/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/powerpc/systimer.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Art Yerkes (ayerkes@speakeasy.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

ULONG
NTAPI
KiComputeTimerTableIndex(LONGLONG Timer)
{
    return 0; // XXX arty fixme
}
