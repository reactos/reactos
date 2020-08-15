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

class CResourceList : public IResourceList
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    IMP_IResourceList;
#ifdef BUILD_WDK
    ULONG NTAPI NumberOfEntries();
#endif
    CResourceList(IUnknown * OuterUnknown) : m_OuterUnknown(OuterUnknown), m_PoolType(NonPagedPool), m_TranslatedResourceList(0), m_UntranslatedResourceList(0), m_NumberOfEntries(0) {}
    virtual ~CResourceList() {}

public:
    PUNKNOWN m_OuterUnknown;
    POOL_TYPE m_PoolType;
    PCM_RESOURCE_LIST m_TranslatedResourceList;
    PCM_RESOURCE_LIST m_UntranslatedResourceList;
    ULONG m_NumberOfEntries;

    LONG m_Ref;
};


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
        DPRINT("IResourceList_QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}
#if 1
ULONG
NTAPI
CResourceList::NumberOfEntries()
{
   PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return m_NumberOfEntries;
}
#endif

ULONG
NTAPI
CResourceList::NumberOfEntriesOfType(
    IN  CM_RESOURCE_TYPE Type)
{
    ULONG Index, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor, UnPartialDescriptor;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_TranslatedResourceList)
    {
        // no resource list
        return 0;
    }
     PC_ASSERT(m_TranslatedResourceList->List[0].PartialResourceList.Count == m_UntranslatedResourceList->List[0].PartialResourceList.Count);
    // I guess the translated and untranslated lists will be same length?
    for (Index = 0; Index < m_TranslatedResourceList->List[0].PartialResourceList.Count; Index ++ )
    {
        PartialDescriptor = &m_TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
        UnPartialDescriptor = &m_UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
        DPRINT("Descriptor Type %u\n", PartialDescriptor->Type);
        if (PartialDescriptor->Type == Type)
        {
            // Yay! Finally found one that matches!
            Count++;
        }

        if (PartialDescriptor->Type == CmResourceTypeInterrupt)
        {
            DPRINT("Index %u TRANS   Interrupt Number Affinity %x Level %u Vector %u Flags %x Share %x\n", Index, PartialDescriptor->u.Interrupt.Affinity, PartialDescriptor->u.Interrupt.Level, PartialDescriptor->u.Interrupt.Vector, PartialDescriptor->Flags, PartialDescriptor->ShareDisposition);
            DPRINT("Index %u UNTRANS Interrupt Number Affinity %x Level %u Vector %u Flags %x Share %x\\n", Index, UnPartialDescriptor->u.Interrupt.Affinity, UnPartialDescriptor->u.Interrupt.Level, UnPartialDescriptor->u.Interrupt.Vector, UnPartialDescriptor->Flags, UnPartialDescriptor->ShareDisposition);

        }
        else if (PartialDescriptor->Type == CmResourceTypePort)
        {
            DPRINT("Index %u TRANS    Port Length %u Start %u %u Flags %x Share %x\n", Index, PartialDescriptor->u.Port.Length, PartialDescriptor->u.Port.Start.HighPart, PartialDescriptor->u.Port.Start.LowPart, PartialDescriptor->Flags, PartialDescriptor->ShareDisposition);
            DPRINT("Index %u UNTRANS  Port Length %u Start %u %u Flags %x Share %x\n", Index, UnPartialDescriptor->u.Port.Length, UnPartialDescriptor->u.Port.Start.HighPart, UnPartialDescriptor->u.Port.Start.LowPart, UnPartialDescriptor->Flags, UnPartialDescriptor->ShareDisposition);
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

    if (!m_TranslatedResourceList)
    {
        // no resource list
        return NULL;
    }

    for (DescIndex = 0; DescIndex < m_TranslatedResourceList->List[0].PartialResourceList.Count; DescIndex ++ )
    {
        PartialDescriptor = &m_TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            // Yay! Finally found one that matches!
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
NTAPI
CResourceList::FindUntranslatedEntry(
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    ULONG DescIndex, Count = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_UntranslatedResourceList)
    {
        // no resource list
        return NULL;
    }

    for (DescIndex = 0; DescIndex < m_UntranslatedResourceList->List[0].PartialResourceList.Count; DescIndex ++ )
    {
        PartialDescriptor = &m_UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[DescIndex];

        if (PartialDescriptor->Type == Type)
        {
            // Yay! Finally found one that matches!
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
NTAPI
CResourceList::AddEntry(
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated)
{
    PCM_RESOURCE_LIST NewUntranslatedResources, NewTranslatedResources;
    ULONG NewTranslatedSize, NewUntranslatedSize, TranslatedSize, UntranslatedSize, ResourceCount;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    // calculate translated resource list size
    ResourceCount = m_TranslatedResourceList->List[0].PartialResourceList.Count;
#ifdef _MSC_VER
    NewTranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount+1]);
    TranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount]);
#else
    NewTranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount+1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    TranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif
    NewTranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(m_PoolType, NewTranslatedSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
        return STATUS_INSUFFICIENT_RESOURCES;


    // calculate untranslated resouce list size
    ResourceCount = m_UntranslatedResourceList->List[0].PartialResourceList.Count;

#ifdef _MSC_VER
    NewUntranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount+1]);
    UntranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount]);
#else
    NewUntranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount+1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    UntranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif


    // allocate untranslated resource list size
    NewUntranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(m_PoolType, NewUntranslatedSize, TAG_PORTCLASS);
    if (!NewUntranslatedResources)
    {
        FreeItem(NewTranslatedResources, TAG_PORTCLASS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // now copy translated resource list
    RtlMoveMemory(NewTranslatedResources, m_TranslatedResourceList, TranslatedSize);
    RtlMoveMemory((PUCHAR)NewTranslatedResources + TranslatedSize, Translated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    // now copy untranslated resource list
    RtlMoveMemory(NewUntranslatedResources, m_UntranslatedResourceList, UntranslatedSize);
    RtlMoveMemory((PUCHAR)NewUntranslatedResources + UntranslatedSize, Untranslated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    // free old lists
    FreeItem(m_TranslatedResourceList, TAG_PORTCLASS);
    FreeItem(m_UntranslatedResourceList, TAG_PORTCLASS);

    // store new lists
    m_UntranslatedResourceList = NewUntranslatedResources;
    m_TranslatedResourceList = NewTranslatedResources;

    // increment descriptor count
    NewUntranslatedResources->List[0].PartialResourceList.Count++;
    NewTranslatedResources->List[0].PartialResourceList.Count++;

    // add entry count
    m_NumberOfEntries++;

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

    Translated = Parent->FindTranslatedEntry(Type, Index);
    Untranslated = Parent->FindUntranslatedEntry(Type, Index);

    if (Translated && Untranslated)
    {
        // add entry from parent
        return AddEntry(Translated, Untranslated);
    }

    // entry not found
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
    ULONG NewTranslatedSize, NewUntranslatedSize, ResourceCount;
    CResourceList* NewList;
    NTSTATUS Status;

    if (!TranslatedResourceList)
    {
        //
        // if the untranslated resource list is also not provided, it becomes an empty resource list
        //
        if (UntranslatedResourceList)
        {
            // invalid parameter mix
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        //
        // if the translated resource list is also not provided, it becomes an empty resource list
        //
        if (!UntranslatedResourceList)
        {
            // invalid parameter mix
            return STATUS_INVALID_PARAMETER;
        }
    }

    NewList = new(PoolType, TAG_PORTCLASS)CResourceList(OuterUnknown);
    if (!NewList)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = NewList->QueryInterface(IID_IResourceList, (PVOID*)OutResourceList);

    if (!NT_SUCCESS(Status))
    {
        //
        // Ouch, FIX ME
        //
        delete NewList;
        return STATUS_INVALID_PARAMETER;
    }

    if (!TranslatedResourceList)
    {
        //
        // empty resource list
        //
        return STATUS_SUCCESS;
    }

    // calculate translated resource list size
    ResourceCount = TranslatedResourceList->List[0].PartialResourceList.Count;
#ifdef _MSC_VER
    NewTranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount]);
#else
    NewTranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif

    // store resource count
    NewList->m_NumberOfEntries = ResourceCount;

    // calculate untranslated resouce list size
    ResourceCount = UntranslatedResourceList->List[0].PartialResourceList.Count;
#ifdef _MSC_VER
    NewUntranslatedSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors[ResourceCount]);
#else
    NewUntranslatedSize = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + (ResourceCount) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
#endif

    // allocate translated resource list
    NewTranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(PoolType, NewTranslatedSize, TAG_PORTCLASS);
    if (!NewTranslatedResources)
    {
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // allocate untranslated resource list
    NewUntranslatedResources = (PCM_RESOURCE_LIST)AllocateItem(PoolType, NewUntranslatedSize, TAG_PORTCLASS);
    if (!NewUntranslatedResources)
    {
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy resource lists
    RtlCopyMemory(NewTranslatedResources, TranslatedResourceList, NewTranslatedSize);
    RtlCopyMemory(NewUntranslatedResources, UntranslatedResourceList, NewUntranslatedSize);

    // store resource lists
    NewList->m_TranslatedResourceList= NewTranslatedResources;
    NewList->m_UntranslatedResourceList = NewUntranslatedResources;

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
    CResourceList* NewList, *Parent;

    if (!OutResourceList || !ParentList || !MaximumEntries)
        return STATUS_INVALID_PARAMETER;

    Parent = (CResourceList*)ParentList;

    if (!Parent->m_TranslatedResourceList->List[0].PartialResourceList.Count ||
        !Parent->m_UntranslatedResourceList->List[0].PartialResourceList.Count)
    {
        // parent list can't be empty
        return STATUS_INVALID_PARAMETER;
    }

    NewList = new(PoolType, TAG_PORTCLASS) CResourceList(OuterUnknown);
    if (!NewList)
        return STATUS_INSUFFICIENT_RESOURCES;

    NewList->m_TranslatedResourceList = (PCM_RESOURCE_LIST)AllocateItem(PoolType, sizeof(CM_RESOURCE_LIST), TAG_PORTCLASS);
    if (!NewList->m_TranslatedResourceList)
    {
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewList->m_UntranslatedResourceList = (PCM_RESOURCE_LIST)AllocateItem(PoolType, sizeof(CM_RESOURCE_LIST), TAG_PORTCLASS);
    if (!NewList->m_UntranslatedResourceList)
    {
        delete NewList;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(NewList->m_TranslatedResourceList, Parent->m_TranslatedResourceList, sizeof(CM_RESOURCE_LIST));
    RtlCopyMemory(NewList->m_UntranslatedResourceList, Parent->m_UntranslatedResourceList, sizeof(CM_RESOURCE_LIST));

    // mark list as empty
    NewList->m_TranslatedResourceList->List[0].PartialResourceList.Count = 0;
    NewList->m_UntranslatedResourceList->List[0].PartialResourceList.Count = 0;
    // store members
    NewList->m_OuterUnknown = OuterUnknown;
    NewList->m_PoolType = PoolType;
    NewList->m_Ref = 1;
    NewList->m_NumberOfEntries = 0;

    *OutResourceList = (IResourceList*)NewList;

    DPRINT("PcNewResourceSublist OutResourceList %p OuterUnknown %p ParentList %p\n", *OutResourceList, OuterUnknown, ParentList);
    return STATUS_SUCCESS;
}

