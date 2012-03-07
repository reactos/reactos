/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/abios.c
 * PURPOSE:         Routines for ABIOS Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386FlatToGdtSelector(IN ULONG Base,
                        IN USHORT Length,
                        IN USHORT Selector)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386ReleaseGdtSelectors(OUT PULONG SelArray,
                          IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386AllocateGdtSelectors(OUT PULONG SelArray,
                           IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
