/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/rtl/libsupp.c
 * PURPOSE:         RTL Support Routines
 * PROGRAMMER:      Mark Jansen (mark.jansen@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* FUNCTIONS *****************************************************************/

/* Ldr access to IMAGE_NT_HEADERS without SEH */

/* Rtl SEH-Free version of this */
NTSTATUS
NTAPI
RtlpImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders);


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders)
{
    return RtlpImageNtHeaderEx(Flags, Base, Size, OutHeaders);
}
