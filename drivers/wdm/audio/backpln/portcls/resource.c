/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/helpers/ResourceList.c
 * PURPOSE:         Port Class driver / ResourceList implementation
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"
#include <portcls.h>
#include <stdunk.h>
#include <intrin.h>

typedef struct CResourceList
{
    IResourceListVtbl *lpVtbl;
    LONG ref;
    PUNKNOWN OuterUnknown;
    POOL_TYPE PoolType;
    PCM_RESOURCE_LIST TranslatedResourceList;
    PCM_RESOURCE_LIST UntranslatedResourceList;
} IResourceListImpl;



/*
    Basic IUnknown methods
*/

NTSTATUS
STDMETHODCALLTYPE
IResourceList_fnQueryInterface(
    IResourceList* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    WCHAR Buffer[100];

    IResourceListImpl * This = (IResourceListImpl*)iface;
    if (IsEqualGUIDAligned(refiid, &IID_IResourceList) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
#if 0
    else if (IsEqualGUIDAligned(refiid, &IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, &IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }
#endif
    StringFromCLSID(refiid, Buffer);
    DPRINT1("IResourceList_fnQueryInterface no interface!!! iface %S\n", Buffer);
    KeBugCheckEx(0, 0, 0, 0, 0);

    return STATUS_UNSUCCESSFUL;
}

ULONG
STDMETHODCALLTYPE
IResourceList_fnAddRef(
    IResourceList* iface)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
IResourceList_fnRelease(
    IResourceList* iface)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;

    InterlockedDecrement(&This->ref);

    DPRINT("IResourceList_fnRelease %p ref %x\n", This, This->ref);

    if (This->ref == 0)
    {
        FreeItem(This->TranslatedResourceList, TAG_PORTCLASS);
        FreeItem(This->UntranslatedResourceList, TAG_PORTCLASS);
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}


/*
    IResourceList methods
*/

ULONG
STDMETHODCALLTYPE
IResourceList_fnNumberOfEntries(IResourceList* iface)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;

    return This->TranslatedResourceList->List[0].PartialResourceList.Count;
}

ULONG
STDMETHODCALLTYPE
IResourceList_fnNumberOfEntriesOfType(
    IResourceList* iface,
    IN  CM_RESOURCE_TYPE Type)
{
    /* I guess the translated and untranslated lists will be same length? */

    IResourceListImpl * This = (IResourceListImpl*)iface;
    ULONG Index, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    for (Index = 0; Index < This->TranslatedResourceList->List[0].PartialResourceList.Count; Index ++ )
    {
        PartialDescriptor = &This->TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
        DPRINT1("Descriptor Type %u\n", PartialDescriptor->Type);
        if (PartialDescriptor->Type == Type)
        {
            /* Yay! Finally found one that matches! */
            Count++;
        }
    }

    DPRINT("Found %d type %d\n", Count, Type);
    return Count;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
STDMETHODCALLTYPE
IResourceList_fnFindTranslatedEntry(
    IResourceList* iface,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;
    ULONG DescIndex, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    for (DescIndex = 0; DescIndex < This->TranslatedResourceList->List[0].PartialResourceList.Count; DescIndex ++ )
    {
        PartialDescriptor = &This->TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            /* Yay! Finally found one that matches! */
            if (Index == Count)
            {
                return PartialDescriptor;
            }
            Count++;
        }
    }

    return NULL;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
STDMETHODCALLTYPE
IResourceList_fnFindUntranslatedEntry(
    IResourceList* iface,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;
    ULONG DescIndex, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    for (DescIndex = 0; DescIndex < This->UntranslatedResourceList->List[0].PartialResourceList.Count; DescIndex ++ )
    {
        PartialDescriptor = &This->UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            /* Yay! Finally found one that matches! */
            if (Index == Count)
            {
                return PartialDescriptor;
            }
            Count++;
        }
    }
    return NULL;
}

NTSTATUS
STDMETHODCALLTYPE
IResourceList_fnAddEntry(
    IResourceList* iface,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated)
{
    PCM_RESOURCE_LIST NewUntranslatedResources, NewTranslatedResources;
    ULONG NewTranslatedSize, NewUntranslatedSize;
    IResourceListImpl * This = (IResourceListImpl*)iface;

    NewTranslatedSize = sizeof(CM_RESOURCE_LIST) + This->TranslatedResourceList[0].List->PartialResourceList.Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    NewTranslatedResources = AllocateItem(This->PoolType, NewTranslatedSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
        return STATUS_INSUFFICIENT_RESOURCES;

    NewUntranslatedSize = sizeof(CM_RESOURCE_LIST) + This->UntranslatedResourceList[0].List->PartialResourceList.Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    NewUntranslatedResources = AllocateItem(This->PoolType, NewUntranslatedSize, TAG_PORTCLASS);
    if (!NewUntranslatedResources)
    {
        FreeItem(NewTranslatedResources, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewTranslatedResources, This->TranslatedResourceList, sizeof(CM_RESOURCE_LIST) + (This->TranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    RtlCopyMemory(&NewTranslatedResources->List[0].PartialResourceList.PartialDescriptors[This->TranslatedResourceList[0].List->PartialResourceList.Count], Translated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    RtlCopyMemory(NewUntranslatedResources, This->UntranslatedResourceList, sizeof(CM_RESOURCE_LIST) + (This->UntranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    RtlCopyMemory(&NewUntranslatedResources->List[0].PartialResourceList.PartialDescriptors[This->UntranslatedResourceList[0].List->PartialResourceList.Count], Untranslated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    FreeItem(This->TranslatedResourceList, TAG_PORTCLASS);
    FreeItem(This->UntranslatedResourceList, TAG_PORTCLASS);

    This->UntranslatedResourceList = NewUntranslatedResources;
    This->TranslatedResourceList = NewTranslatedResources;

    NewUntranslatedResources->List[0].PartialResourceList.Count++;
    NewTranslatedResources->List[0].PartialResourceList.Count++;

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
IResourceList_fnAddEntryFromParent(
    IResourceList* iface,
    IN  IResourceList* Parent,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_RESOURCE_LIST NewTranslatedResources;
    ULONG NewTranslatedSize;
    IResourceListImpl * This = (IResourceListImpl*)iface;

    Translated = Parent->lpVtbl->FindTranslatedEntry(Parent, Type, Index);
    if (!Translated)
        return STATUS_INVALID_PARAMETER;

    NewTranslatedSize = sizeof(CM_RESOURCE_LIST) + This->TranslatedResourceList[0].List->PartialResourceList.Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    NewTranslatedResources = AllocateItem(This->PoolType, NewTranslatedSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(NewTranslatedResources, This->TranslatedResourceList, sizeof(CM_RESOURCE_LIST) + (This->TranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    RtlCopyMemory(&NewTranslatedResources->List[0].PartialResourceList.PartialDescriptors[This->TranslatedResourceList[0].List->PartialResourceList.Count], Translated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    FreeItem(This->TranslatedResourceList, TAG_PORTCLASS);
    This->TranslatedResourceList = NewTranslatedResources;
    NewTranslatedResources->List[0].PartialResourceList.Count++;

    return STATUS_SUCCESS;
}

PCM_RESOURCE_LIST
STDMETHODCALLTYPE
IResourceList_fnTranslatedList(
    IResourceList* iface)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;

    return This->TranslatedResourceList;
}

PCM_RESOURCE_LIST
STDMETHODCALLTYPE
IResourceList_fnUntranslatedList(
    IResourceList* iface)
{
    IResourceListImpl * This = (IResourceListImpl*)iface;

    return This->UntranslatedResourceList;
}


/*
    ResourceList V-Table
*/
static const IResourceListVtbl vt_ResourceListVtbl =
{
    /* IUnknown */
    IResourceList_fnQueryInterface,
    IResourceList_fnAddRef,
    IResourceList_fnRelease,
    /* IResourceList */
    IResourceList_fnNumberOfEntries,
    IResourceList_fnNumberOfEntriesOfType,
    IResourceList_fnFindTranslatedEntry,
    IResourceList_fnFindUntranslatedEntry,
    IResourceList_fnAddEntry,
    IResourceList_fnAddEntryFromParent,
    IResourceList_fnTranslatedList,
    IResourceList_fnUntranslatedList
};


/*
    Factory for creating a resource list
*/
PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceList(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PCM_RESOURCE_LIST TranslatedResourceList,
    IN  PCM_RESOURCE_LIST UntranslatedResourceList)
{
    PCM_RESOURCE_LIST NewUntranslatedResources, NewTranslatedResources;
    ULONG NewTranslatedSize, NewUntranslatedSize;
    IResourceListImpl* NewList;

    /* TODO: Validate parameters */

    DPRINT1("PcNewResourceList\n");

    NewList = AllocateItem(PoolType, sizeof(IResourceListImpl), TAG_PORTCLASS);

    if (!NewList)
    {
        DPRINT("AllocateItem failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize */

    NewTranslatedSize = sizeof(CM_RESOURCE_LIST) + (TranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    NewTranslatedResources = AllocateItem(PoolType, NewTranslatedSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
    {
        FreeItem(NewList, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewUntranslatedSize = sizeof(CM_RESOURCE_LIST) + (UntranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    NewUntranslatedResources = AllocateItem(PoolType, NewUntranslatedSize, TAG_PORTCLASS);
    if (!NewUntranslatedResources)
    {
        FreeItem(NewList, TAG_PORTCLASS);
        FreeItem(NewTranslatedResources, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewTranslatedResources, TranslatedResourceList, sizeof(CM_RESOURCE_LIST) + (TranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    RtlCopyMemory(NewUntranslatedResources, UntranslatedResourceList, sizeof(CM_RESOURCE_LIST) + (UntranslatedResourceList[0].List->PartialResourceList.Count-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    NewList->lpVtbl = (IResourceListVtbl*)&vt_ResourceListVtbl;
    NewList->ref = 1;
    NewList->OuterUnknown = OuterUnknown;
    NewList->TranslatedResourceList= NewTranslatedResources;
    NewList->UntranslatedResourceList = NewUntranslatedResources;
    NewList->PoolType = PoolType;

    /* Increment our usage count and set the pointer to this object */
    *OutResourceList = (IResourceList*)&NewList->lpVtbl;

    return STATUS_SUCCESS;
}

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceSublist(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ParentList,
    IN  ULONG MaximumEntries)
{
    IResourceListImpl* NewList, *Parent;

    if (!OutResourceList || !ParentList || !MaximumEntries)
        return STATUS_INVALID_PARAMETER;

    Parent = (IResourceListImpl*)ParentList;

    DPRINT1("PcNewResourceSublist entered\n");

    if (!Parent->TranslatedResourceList->List->PartialResourceList.Count ||
        !Parent->UntranslatedResourceList->List->PartialResourceList.Count)
    {
        /* parent list can't be empty */
        return STATUS_INVALID_PARAMETER;
    }

    NewList = AllocateItem(PoolType, sizeof(IResourceListImpl), TAG_PORTCLASS);
    if (!NewList)
        return STATUS_INSUFFICIENT_RESOURCES;

    NewList->TranslatedResourceList = AllocateItem(PoolType, sizeof(CM_RESOURCE_LIST), TAG_PORTCLASS);
    if (!NewList->TranslatedResourceList)
    {
        FreeItem(NewList, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewList->UntranslatedResourceList = AllocateItem(PoolType, sizeof(CM_RESOURCE_LIST), TAG_PORTCLASS);
    if (!NewList->UntranslatedResourceList)
    {
        FreeItem(NewList->TranslatedResourceList, TAG_PORTCLASS);
        FreeItem(NewList, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewList->TranslatedResourceList, Parent->TranslatedResourceList, sizeof(CM_RESOURCE_LIST));
    RtlCopyMemory(NewList->UntranslatedResourceList, Parent->UntranslatedResourceList, sizeof(CM_RESOURCE_LIST));

    /* mark list as empty */
    NewList->TranslatedResourceList->List->PartialResourceList.Count = 0;
    NewList->UntranslatedResourceList->List->PartialResourceList.Count = 0;

    NewList->lpVtbl = (IResourceListVtbl*)&vt_ResourceListVtbl;
    NewList->ref = 1;
    NewList->OuterUnknown = OuterUnknown;
    NewList->PoolType = PoolType;

    *OutResourceList = (IResourceList*)&NewList->lpVtbl;

    DPRINT1("PcNewResourceSublist OutResourceList %p OuterUnknown %p ParentList %p\n", *OutResourceList, OuterUnknown, ParentList);
    return STATUS_SUCCESS;
}
