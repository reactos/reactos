/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/wdm/audio/backpln/portcls/resource.cpp
 * PURPOSE:         Port Class driver / ResourceList implementation
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CResourceList : public CUnknownImpl<IResourceList>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IResourceList;

    CResourceList(IUnknown * OuterUnknown) :
        m_OuterUnknown(OuterUnknown),
        m_PoolType(NonPagedPool),
        m_TranslatedResourceList(0),
        m_UntranslatedResourceList(0),
        m_NumberOfEntries(0),
        m_MaxEntries(0)
    {
    }
    virtual ~CResourceList();

public:
    PUNKNOWN m_OuterUnknown;
    POOL_TYPE m_PoolType;
    PCM_RESOURCE_LIST m_TranslatedResourceList;
    PCM_RESOURCE_LIST m_UntranslatedResourceList;
    ULONG m_NumberOfEntries;
    ULONG m_MaxEntries;
};

CResourceList::~CResourceList()
{
    if (m_TranslatedResourceList)
    {
        /* Free resource list */
        FreeItem(m_TranslatedResourceList, TAG_PORTCLASS);
    }

    if (m_UntranslatedResourceList)
    {
        /* Free resource list */
        FreeItem(m_UntranslatedResourceList, TAG_PORTCLASS);
    }
}

NTSTATUS
NTAPI
CResourceList::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IResourceList) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PRESOURCELIST(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IResourceList_QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
CResourceList::NumberOfEntries()
{
   PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return m_NumberOfEntries;
}

ULONG
NTAPI
CResourceList::NumberOfEntriesOfType(
    IN  CM_RESOURCE_TYPE Type)
{
    ULONG Index, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* Is there a resource list? */
    if (!m_UntranslatedResourceList)
    {
        /* No resource list provided */
        return 0;
    }

    for (Index = 0; Index < m_NumberOfEntries; Index ++ )
    {

        /* Get descriptor */
        PartialDescriptor = &m_UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
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
NTAPI
CResourceList::FindTranslatedEntry(
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    ULONG DescIndex, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* Is there a resource list? */
    if (!m_TranslatedResourceList)
    {
        /* No resource list */
        return NULL;
    }

    for (DescIndex = 0; DescIndex < m_NumberOfEntries; DescIndex ++ )
    {
        /* Get descriptor */
        PartialDescriptor = &m_TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            /* Found type, is it the requested index? */
            if (Index == Count)
            {
                /* Found */
                return PartialDescriptor;
            }

            /* Need to continue search */
            Count++;
        }
    }

    /* No such descriptor */
    return NULL;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
CResourceList::FindUntranslatedEntry(
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    ULONG DescIndex, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* Is there a resource list? */
    if (!m_UntranslatedResourceList)
    {
        /* Empty resource list */
        return NULL;
    }

    /* Search descriptors */
    for (DescIndex = 0; DescIndex < m_NumberOfEntries; DescIndex ++ )
    {
        /* Get descriptor */
        PartialDescriptor = &m_UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            /* Found type, is it the requested index? */
            if (Index == Count)
            {
                /* Found */
                return PartialDescriptor;
            }

            /* Need to continue search */
            Count++;
        }
    }

    /* No such descriptor */
    return NULL;
}

NTSTATUS
NTAPI
CResourceList::AddEntry(
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    /* Sanity check */
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);


    /* Is there still room for another entry */
    if (m_NumberOfEntries >= m_MaxEntries)
    {
        /* No more space */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get free descriptor */
    PartialDescriptor = &m_UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[m_NumberOfEntries];

    /* Copy descriptor */
    RtlCopyMemory(PartialDescriptor, Untranslated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    /* Get free descriptor */
    PartialDescriptor = &m_TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[m_NumberOfEntries];

    /* Copy descriptor */
    RtlCopyMemory(PartialDescriptor, Translated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    /* Add entry count */
    m_NumberOfEntries++;
    m_UntranslatedResourceList->List[0].PartialResourceList.Count++;
    m_TranslatedResourceList->List[0].PartialResourceList.Count++;

    /* Done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CResourceList::AddEntryFromParent(
    IN  IResourceList* Parent,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated, Untranslated;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* Get entries from parent */
    Translated = Parent->FindTranslatedEntry(Type, Index);
    Untranslated = Parent->FindUntranslatedEntry(Type, Index);

    /* Are both found? */
    if (Translated && Untranslated)
    {
        /* Add entry from parent */
        return AddEntry(Translated, Untranslated);
    }

    /* Entry not found */
    return STATUS_INVALID_PARAMETER;
}

PCM_RESOURCE_LIST
NTAPI
CResourceList::TranslatedList()
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return m_TranslatedResourceList;
}

PCM_RESOURCE_LIST
NTAPI
CResourceList::UntranslatedList()
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return m_UntranslatedResourceList;
}


PORTCLASSAPI
NTSTATUS
NTAPI
PcNewResourceList(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PCM_RESOURCE_LIST TranslatedResourceList,
    IN  PCM_RESOURCE_LIST UntranslatedResourceList)
{
    PCM_RESOURCE_LIST NewUntranslatedResources, NewTranslatedResources;
    ULONG ResourceSize, ResourceCount;
    CResourceList* NewList;
    NTSTATUS Status;

    if (!TranslatedResourceList)
    {
        /* If the untranslated resource list is also not provided, it becomes an empty resource list */
        if (UntranslatedResourceList)
        {
            /* Invalid parameter mix */
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* If the translated resource list is also not provided, it becomes an empty resource list */
        if (!UntranslatedResourceList)
        {
            /* Invalid parameter mix */
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* Allocate resource list */
    NewList = new(PoolType, TAG_PORTCLASS)CResourceList(OuterUnknown);
    if (!NewList)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Query resource list */
    Status = NewList->QueryInterface(IID_IResourceList, (PVOID*)OutResourceList);
    if (!NT_SUCCESS(Status))
    {
        /* Ouch, FIX ME */
        delete NewList;
        return STATUS_INVALID_PARAMETER;
    }

    /* Is there a resource list */
    if (!TranslatedResourceList)
    {
        /* Empty resource list */
        return STATUS_SUCCESS;
    }

    /* Sanity check */
    ASSERT(UntranslatedResourceList->List[0].PartialResourceList.Count == TranslatedResourceList->List[0].PartialResourceList.Count);

    /* Get resource count */
    ResourceCount = UntranslatedResourceList->List[0].PartialResourceList.Count;
#ifdef _MSC_VER
    ResourceSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount]);
#else
    ResourceSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif

    /* Allocate translated resource list */
    NewTranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(PoolType, ResourceSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
    {
        /* No memory */
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate untranslated resource list */
    NewUntranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(PoolType, ResourceSize, TAG_PORTCLASS);
    if (!NewUntranslatedResources)
    {
        /* No memory */
        delete NewList;
        FreeItem(NewTranslatedResources, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy resource lists */
    RtlCopyMemory(NewTranslatedResources, TranslatedResourceList, ResourceSize);
    RtlCopyMemory(NewUntranslatedResources, UntranslatedResourceList, ResourceSize);

    /* Init resource list */
    NewList->m_TranslatedResourceList= NewTranslatedResources;
    NewList->m_UntranslatedResourceList = NewUntranslatedResources;
    NewList->m_NumberOfEntries = ResourceCount;
    NewList->m_MaxEntries = ResourceCount;
    NewList->m_PoolType = PoolType;

    /* Done */
    return STATUS_SUCCESS;
}

PORTCLASSAPI
NTSTATUS
NTAPI
PcNewResourceSublist(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ParentList,
    IN  ULONG MaximumEntries)
{
    CResourceList* NewList;
    ULONG ResourceSize;

    if (!OutResourceList || !ParentList || !MaximumEntries)
        return STATUS_INVALID_PARAMETER;

    /* Allocate new list */
    NewList = new(PoolType, TAG_PORTCLASS) CResourceList(OuterUnknown);
    if (!NewList)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get resource size */
#ifdef _MSC_VER
    ResourceSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[MaximumEntries]);
#else
    ResourceSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (MaximumEntries) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif

    /* Allocate resource list */
    NewList->m_TranslatedResourceList = (PCM_RESOURCE_LIST)AllocateItem(PoolType, ResourceSize, TAG_PORTCLASS);
    if (!NewList->m_TranslatedResourceList)
    {
        /* No memory */
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate resource list */
    NewList->m_UntranslatedResourceList = (PCM_RESOURCE_LIST)AllocateItem(PoolType, ResourceSize, TAG_PORTCLASS);
    if (!NewList->m_UntranslatedResourceList)
    {
        /* No memory */
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy resource lists */
    RtlCopyMemory(NewList->m_TranslatedResourceList, ParentList->TranslatedList(), sizeof(CM_RESOURCE_LIST));
    RtlCopyMemory(NewList->m_UntranslatedResourceList, ParentList->UntranslatedList(), sizeof(CM_RESOURCE_LIST));

    /* Resource list is empty */
    NewList->m_UntranslatedResourceList->List[0].PartialResourceList.Count = 0;
    NewList->m_TranslatedResourceList->List[0].PartialResourceList.Count = 0;

    /* Store members */
    NewList->m_OuterUnknown = OuterUnknown;
    NewList->m_PoolType = PoolType;
    NewList->AddRef();
    NewList->m_NumberOfEntries = 0;
    NewList->m_MaxEntries = MaximumEntries;

    /* Store result */
    *OutResourceList = (IResourceList*)NewList;

    /* Done */
    return STATUS_SUCCESS;
}
