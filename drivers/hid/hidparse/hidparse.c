/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser kernel mode
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 */

#include "hidparse.h"
#include <hidpmem.h>

#define NDEBUG
#include <debug.h>

PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize)
{
    PVOID Item = ExAllocatePoolWithTag(NonPagedPool, ItemSize, HIDPARSE_TAG);
    if (Item)
    {
        // zero item
        RtlZeroMemory(Item, ItemSize);
    }

    return Item;
}

VOID
NTAPI
FreeFunction(
    IN PVOID Item)
{
    ExFreePoolWithTag(Item, HIDPARSE_TAG);
}

VOID
NTAPI
ZeroFunction(
    IN PVOID Item,
    IN ULONG ItemSize)
{
    RtlZeroMemory(Item, ItemSize);
}

VOID
NTAPI
CopyFunction(
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Length)
{
    RtlCopyMemory(Target, Source, Length);
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{
    DPRINT("********* HID PARSE *********\n");
    return STATUS_SUCCESS;
}
