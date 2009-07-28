/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/bag.c
 * PURPOSE:         KS Object Bag functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

typedef struct
{
    LIST_ENTRY Entry;
    PVOID Item;
    PFNKSFREE Free;
    ULONG References;
}KSIOBJECT_BAG_ENTRY, *PKSIOBJECT_BAG_ENTRY;


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectBag(
    IN PKSDEVICE Device,
    OUT KSOBJECT_BAG* ObjectBag)
{
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_BAG Bag;
    IKsDevice *KsDevice;

    /* get real device header */
    DeviceHeader = (PKSIDEVICE_HEADER)CONTAINING_RECORD(Device, KSIDEVICE_HEADER, KsDevice);

    /* allocate a object bag ctx */
    Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
    if (!Bag)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* get device interface */
    KsDevice = (IKsDevice*)&DeviceHeader->lpVtblIKsDevice;

    /* initialize object bag */
    return KsDevice->lpVtbl->InitializeObjectBag(KsDevice, Bag, NULL);
}

PKSIOBJECT_BAG_ENTRY
KspFindObjectBagItem(
    IN PLIST_ENTRY ObjectList,
    IN PVOID Item)
{
    PLIST_ENTRY Entry;
    PKSIOBJECT_BAG_ENTRY BagEntry;

    /* point to first item */
    Entry = ObjectList->Flink;
    /* first scan the list if the item is already inserted */
    while(Entry != ObjectList)
    {
        /* get bag entry */
        BagEntry = (PKSIOBJECT_BAG_ENTRY)CONTAINING_RECORD(Entry, KSIOBJECT_BAG_ENTRY, Entry);

        if (BagEntry->Item == Item)
        {
            /* found entry */
            return BagEntry;
        }
        /* move to next entry */
        Entry = Entry->Flink;
    }
    /* item not in this object bag */
    return NULL;
}


/*
    @implemented
*/
NTSTATUS
NTAPI
KsAddItemToObjectBag(
    IN KSOBJECT_BAG  ObjectBag,
    IN PVOID  Item,
    IN PFNKSFREE  Free  OPTIONAL)
{
    PKSIOBJECT_BAG Bag;
    PKSIOBJECT_BAG_ENTRY BagEntry;

    /* get real object bag */
    Bag = (PKSIOBJECT_BAG)ObjectBag;

    /* acquire bag mutex */
    KeWaitForSingleObject(Bag->BagMutex, Executive, KernelMode, FALSE, NULL);

    /* is the item already present in this object bag */
    BagEntry = KspFindObjectBagItem(&Bag->ObjectList, Item);

    if (BagEntry)
    {
        /* is is, update reference count */
        InterlockedIncrement((PLONG)&BagEntry->References);
        /* release mutex */
        KeReleaseMutex(Bag->BagMutex, FALSE);
        /* return result */
        return STATUS_SUCCESS;
    }

    /* item is new, allocate entry */
    BagEntry = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG_ENTRY));
    if (!BagEntry)
    {
        /* no memory */
        KeReleaseMutex(Bag->BagMutex, FALSE);
        /* return result */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize bag entry */
    BagEntry->References = 1;
    BagEntry->Item = Item;
    if (Free)
        BagEntry->Free = Free;
    else
        BagEntry->Free = ExFreePool;

    /* insert item */
    InsertTailList(&Bag->ObjectList, &Bag->Entry);

    /* release mutex */
    KeReleaseMutex(Bag->BagMutex, FALSE);

    /* done */
    return STATUS_SUCCESS;
}

ULONG
KspGetObjectItemReferenceCount(
    IN PKSIDEVICE_HEADER DeviceHeader,
    IN PVOID Item)
{
    PLIST_ENTRY Entry;
    PKSIOBJECT_BAG OtherBag;
    PKSIOBJECT_BAG_ENTRY OtherBagEntry;
    ULONG TotalRefs = 0;

    /* scan all object bags and see if item is present there */
    Entry = DeviceHeader->ObjectBags.Flink;
    while(Entry != &DeviceHeader->ObjectBags)
    {
        /* get other bag */
        OtherBag = (PKSIOBJECT_BAG)CONTAINING_RECORD(Entry, KSIOBJECT_BAG, Entry);

        /* is the item present there */
        OtherBagEntry = KspFindObjectBagItem(&OtherBag->ObjectList, Item);

        if (OtherBagEntry)
            TotalRefs++;

        /* move to next item */
        Entry = Entry->Flink;
    }

    return TotalRefs;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsRemoveItemFromObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN BOOLEAN Free)
{
    PKSIOBJECT_BAG Bag;
    PKSIOBJECT_BAG_ENTRY BagEntry;
    ULONG TotalRefs;

    /* get real object bag */
    Bag = (PKSIOBJECT_BAG)ObjectBag;

    /* acquire bag mutex */
    KeWaitForSingleObject(Bag->BagMutex, Executive, KernelMode, FALSE, NULL);

    /* is the item already present in this object bag */
    BagEntry = KspFindObjectBagItem(&Bag->ObjectList, Item);

    if (!BagEntry)
    {
        /* item was not in this object bag */
        KeReleaseMutex(Bag->BagMutex, FALSE);
        return 0;
    }

    /* set current refs count */
    TotalRefs = BagEntry->References;

    /* get total refs count */
    TotalRefs += KspGetObjectItemReferenceCount((PKSIDEVICE_HEADER)Bag->DeviceHeader, Item);

    /* decrease reference count */
    InterlockedDecrement((PLONG)&BagEntry->References);

    if (BagEntry->References == 0)
    {
        /* remove the entry */
        RemoveEntryList(&BagEntry->Entry);
    }

    if (TotalRefs == 1)
    {
        /* does the caller want to free the item */
        if (Free)
        {
            /* free the item */
            BagEntry->Free(BagEntry->Item);
        }
    }
    if (BagEntry->References == 0)
    {
        /* free bag item entry */
        FreeItem(BagEntry);
    }

    /* release mutex */
    KeReleaseMutex(Bag->BagMutex, FALSE);


    return TotalRefs;
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

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeObjectBag(
    IN KSOBJECT_BAG ObjectBag)
{
    PLIST_ENTRY Entry;
    PKSIOBJECT_BAG Bag;
    PKSIOBJECT_BAG_ENTRY BagEntry;
    ULONG TotalRefs;

    /* get real object bag */
    Bag = (PKSIOBJECT_BAG)ObjectBag;

    /* acquire bag mutex */
    KeWaitForSingleObject(Bag->BagMutex, Executive, KernelMode, FALSE, NULL);

    while(!IsListEmpty(&Bag->ObjectList))
    {
        /* get an bag entry */
        Entry = RemoveHeadList(&Bag->ObjectList);
        /* access bag entry item */
        BagEntry = (PKSIOBJECT_BAG_ENTRY)CONTAINING_RECORD(Entry, KSIOBJECT_BAG, Entry);

        /* check if the item is present in some other bag */
        TotalRefs = KspGetObjectItemReferenceCount((PKSIDEVICE_HEADER)Bag->DeviceHeader, &BagEntry->Item);

        if (TotalRefs == 0)
        {
            /* item is ready to be freed */
            BagEntry->Free(BagEntry->Item);
        }

        /* free bag entry item */
        FreeItem(BagEntry);
    }

    /* remove bag entry from device object list */
    RemoveEntryList(&Bag->Entry);

    /* release bag mutex */
    KeReleaseMutex(Bag->BagMutex, FALSE);

    /* now free object bag */
    FreeItem(Bag);
}



