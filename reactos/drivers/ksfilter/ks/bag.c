/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/bag.c
 * PURPOSE:         KS Object Bag functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectBag(
    IN PKSDEVICE Device,
    OUT KSOBJECT_BAG* ObjectBag)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsAddItemToObjectBag(
    IN KSOBJECT_BAG  ObjectBag,
    IN PVOID  Item,
    IN PFNKSFREE  Free  OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
ULONG
NTAPI
KsRemoveItemFromObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN BOOLEAN Free)
{
    UNIMPLEMENTED
    return 0;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCopyObjectBagItems(
    IN KSOBJECT_BAG ObjectBagDestination,
    IN KSOBJECT_BAG ObjectBagSource)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

KSDDKAPI
VOID
NTAPI
KsFreeObjectBag(
    IN KSOBJECT_BAG ObjectBag)
{
    UNIMPLEMENTED
}



