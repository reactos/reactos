#include <ntddk.h>
#include <debug.h>
#include <ks.h>
#if 0
typedef struct
{
   LIST_ENTRY Entry;
   PVOID Item;
   PFNKSFREE Free;
   LONG ReferenceCount;
}KSOBJECT_BAG_ENTRY;

typedef struct
{
    LIST_ENTRY ListHead;
    KMUTEX Lock;
}KSOBJECT_BAG_IMPL;

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAddItemToObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN PFNKSFREE Free OPTIONAL)
{
    KSOBJECT_BAG_ENTRY * Entry;
    KSOBJECT_BAG_IMPL * Bag = (KSOBJECT_BAG_IMPL)ObjectBag;

    Entry = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_BAG_ENTRY));
    if (!Entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Entry->Free = Free;
    Entry->Item = Item;

    InsertTailList(&Bag->ListHead, &Entry->Entry);
    return STATUS_SUCCESS;
}

KSDDKAPI
ULONG
NTAPI
KsRemoveItemFromObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN BOOLEAN Free)
{
    KSOBJECT_BAG_IMPL * Bag = (KSOBJECT_BAG_IMPL)ObjectBag;

    


}
#endif
