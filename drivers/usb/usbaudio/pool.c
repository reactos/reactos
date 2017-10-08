/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/pool.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/
#include "usbaudio.h"

PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize)
{
    PVOID Item = ExAllocatePoolWithTag(NonPagedPool, ItemSize, USBAUDIO_TAG);
    if (Item)
    {
        // zero item
        RtlZeroMemory(Item, ItemSize);
    }

    // done
    return Item;
}

VOID
NTAPI
FreeFunction(
    IN PVOID Item)
{
    /* free item */
    ExFreePoolWithTag(Item, USBAUDIO_TAG);
}

