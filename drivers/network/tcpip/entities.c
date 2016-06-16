/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: entity list implementation
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define TAG_ENTITIES 'nIpI'

static const ULONG EntityList[] = {
    AT_ENTITY,
    CL_NL_ENTITY,
    CO_NL_ENTITY,
    CL_TL_ENTITY,
    CO_TL_ENTITY,
    ER_ENTITY,
    IF_ENTITY
};

static LIST_ENTRY AtInstancesListHead;
static LIST_ENTRY ClNlInstancesListHead;
static LIST_ENTRY CoNlInstancesListHead;
static LIST_ENTRY ClTlInstancesListHead;
static LIST_ENTRY CoTlInstancesListHead;
static LIST_ENTRY ErInstancesListHead;
static LIST_ENTRY IfInstancesListHead;

/* The corresponding locks */
static KSPIN_LOCK AtInstanceLock;
static KSPIN_LOCK ClNlInstanceLock;
static KSPIN_LOCK CoNlInstanceLock;
static KSPIN_LOCK ClTlInstanceLock;
static KSPIN_LOCK CoTlInstanceLock;
static KSPIN_LOCK ErInstanceLock;
static KSPIN_LOCK IfInstanceLock;

/* We keep track of those just for the sake of speed,
 * as our network stack thinks it's clever to get the entity list often */
static ULONG InstanceCount;

static
PLIST_ENTRY
GetInstanceListHeadAcquireLock(
    _In_ ULONG Entity,
    _Out_ PKIRQL OldIrql
)
{
    switch (Entity)
    {
        case AT_ENTITY:
            KeAcquireSpinLock(&AtInstanceLock, OldIrql);
            return &AtInstancesListHead;
        case CL_NL_ENTITY:
            KeAcquireSpinLock(&ClNlInstanceLock, OldIrql);
            return &ClNlInstancesListHead;
        case CO_NL_ENTITY:
            KeAcquireSpinLock(&CoNlInstanceLock, OldIrql);
            return &CoNlInstancesListHead;
        case CL_TL_ENTITY:
            KeAcquireSpinLock(&ClTlInstanceLock, OldIrql);
            return &ClTlInstancesListHead;
        case CO_TL_ENTITY:
            KeAcquireSpinLock(&CoTlInstanceLock, OldIrql);
            return &CoTlInstancesListHead;
        case ER_ENTITY:
            KeAcquireSpinLock(&ErInstanceLock, OldIrql);
            return &ErInstancesListHead;
        case IF_ENTITY:
            KeAcquireSpinLock(&IfInstanceLock, OldIrql);
            return &IfInstancesListHead;
        default:
            DPRINT1("Got unknown entity ID %x\n", Entity);
            return NULL;
    }
}

static
void
AcquireEntityLock(
    _In_ ULONG Entity,
    _Out_ KIRQL* OldIrql)
{
    switch (Entity)
    {
        case AT_ENTITY:
            KeAcquireSpinLock(&AtInstanceLock, OldIrql);
            return;
        case CL_NL_ENTITY:
            KeAcquireSpinLock(&ClNlInstanceLock, OldIrql);
            return;
        case CO_NL_ENTITY:
            KeAcquireSpinLock(&CoNlInstanceLock, OldIrql);
            return;
        case CL_TL_ENTITY:
            KeAcquireSpinLock(&ClTlInstanceLock, OldIrql);
            return;
        case CO_TL_ENTITY:
            KeAcquireSpinLock(&CoTlInstanceLock, OldIrql);
            return;
        case ER_ENTITY:
            KeAcquireSpinLock(&ErInstanceLock, OldIrql);
            return;
        case IF_ENTITY:
            KeAcquireSpinLock(&IfInstanceLock, OldIrql);
            return;
        default:
            DPRINT1("Got unknown entity ID %x\n", Entity);
            ASSERT(FALSE);
    }
}

static
void
ReleaseEntityLock(
    _In_ ULONG Entity,
    _In_ KIRQL OldIrql)
{
    switch (Entity)
    {
        case AT_ENTITY:
            KeReleaseSpinLock(&AtInstanceLock, OldIrql);
            return;
        case CL_NL_ENTITY:
            KeReleaseSpinLock(&ClNlInstanceLock, OldIrql);
            return;
        case CO_NL_ENTITY:
            KeReleaseSpinLock(&CoNlInstanceLock, OldIrql);
            return;
        case CL_TL_ENTITY:
            KeReleaseSpinLock(&ClTlInstanceLock, OldIrql);
            return;
        case CO_TL_ENTITY:
            KeReleaseSpinLock(&CoTlInstanceLock, OldIrql);
            return;
        case ER_ENTITY:
            KeReleaseSpinLock(&ErInstanceLock, OldIrql);
            return;
        case IF_ENTITY:
            KeReleaseSpinLock(&IfInstanceLock, OldIrql);
            return;
        default:
            DPRINT1("Got unknown entity ID %x\n", Entity);
            ASSERT(FALSE);
    }
}

VOID
TcpIpInitializeEntities(void)
{
    /* Initialize the locks */
    KeInitializeSpinLock(&AtInstanceLock);
    KeInitializeSpinLock(&ClNlInstanceLock);
    KeInitializeSpinLock(&CoNlInstanceLock);
    KeInitializeSpinLock(&ClTlInstanceLock);
    KeInitializeSpinLock(&CoTlInstanceLock);
    KeInitializeSpinLock(&ErInstanceLock);
    KeInitializeSpinLock(&IfInstanceLock);

    /* And the list heads */
    InitializeListHead(&AtInstancesListHead);
    InitializeListHead(&ClNlInstancesListHead);
    InitializeListHead(&CoNlInstancesListHead);
    InitializeListHead(&ClTlInstancesListHead);
    InitializeListHead(&CoTlInstancesListHead);
    InitializeListHead(&ErInstancesListHead);
    InitializeListHead(&IfInstancesListHead);

    /* We don't have anything for now */
    InstanceCount = 0;
}

NTSTATUS
QueryEntityList(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize)
{
    KIRQL OldIrql;
    TDIEntityID* Entity = OutBuffer;
    ULONG RemainingSize = *BufferSize, TotalSize = 0;
    TCPIP_INSTANCE* Instance;
    LIST_ENTRY* ListHead;
    LIST_ENTRY* ListEntry;
    ULONG i;

    UNREFERENCED_PARAMETER(ID);
    UNREFERENCED_PARAMETER(Context);

    DPRINT("Gathering the entity list.\n");

    if (!OutBuffer)
    {
        *BufferSize = InstanceCount * sizeof(TDIEntityID);
        return STATUS_SUCCESS;
    }

    /* Go through the bitmaps */
    for (i = 0; i < sizeof(EntityList)/sizeof(EntityList[0]); i++)
    {
        ListHead = GetInstanceListHeadAcquireLock(EntityList[i], &OldIrql);

        ListEntry = ListHead->Flink;
        while(ListEntry != ListHead)
        {
            if (RemainingSize < sizeof(TDIEntityID))
            {
                *BufferSize = InstanceCount * sizeof(TDIEntityID);
                ReleaseEntityLock(EntityList[i], OldIrql);
                return STATUS_BUFFER_OVERFLOW;
            }

            Instance = CONTAINING_RECORD(ListEntry, TCPIP_INSTANCE, ListEntry);

            *Entity++ = Instance->InstanceId;
            RemainingSize -= sizeof(*Entity);
            TotalSize += sizeof(*Entity);

            ListEntry = ListEntry->Flink;
        }

        ReleaseEntityLock(EntityList[i], OldIrql);
    }

    *BufferSize = TotalSize;
    return STATUS_SUCCESS;
}

VOID
InsertEntityInstance(
    _In_ ULONG Entity,
    _Out_ TCPIP_INSTANCE* OutInstance)
{
    KIRQL OldIrql;
    LIST_ENTRY* ListHead;
    LIST_ENTRY* ListEntry;
    TCPIP_INSTANCE* Instance;
    ULONG InstanceId = 1;

    ListHead = GetInstanceListHeadAcquireLock(Entity, &OldIrql);
    NT_ASSERT(ListHead);

    ListEntry = ListHead->Flink;

    /* Find an instance number for this guy */
    while (ListEntry != ListHead)
    {
        Instance = CONTAINING_RECORD(ListEntry, TCPIP_INSTANCE, ListEntry);

        if (Instance->InstanceId.tei_instance != InstanceId)
            break;
        InstanceId++;
        ListEntry = ListEntry->Flink;
    }

    OutInstance->InstanceId.tei_entity = Entity;
    OutInstance->InstanceId.tei_instance = InstanceId;

    /* Keep this list sorted */
    InsertHeadList(ListEntry, &OutInstance->ListEntry);

    ReleaseEntityLock(Entity, OldIrql);

    InterlockedIncrement((LONG*)&InstanceCount);
}

void
RemoveEntityInstance(
    _In_ TCPIP_INSTANCE* Instance)
{
    KIRQL OldIrql;

    AcquireEntityLock(Instance->InstanceId.tei_entity, &OldIrql);

    RemoveEntryList(&Instance->ListEntry);

    ReleaseEntityLock(Instance->InstanceId.tei_entity, OldIrql);

    InterlockedDecrement((LONG*)&InstanceCount);
}

NTSTATUS
GetInstance(
    _In_ TDIEntityID ID,
    _Out_ TCPIP_INSTANCE** OutInstance)
{
    KIRQL OldIrql;
    LIST_ENTRY *ListHead, *ListEntry;
    TCPIP_INSTANCE* Instance;

    ListHead = GetInstanceListHeadAcquireLock(ID.tei_entity, &OldIrql);
    NT_ASSERT(ListHead != NULL);

    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead)
    {
        Instance = CONTAINING_RECORD(ListEntry, TCPIP_INSTANCE, ListEntry);

        NT_ASSERT(Instance->InstanceId.tei_entity == ID.tei_entity);
        if (Instance->InstanceId.tei_instance == ID.tei_instance)
        {
            *OutInstance = Instance;
            ReleaseEntityLock(ID.tei_entity, OldIrql);
            return STATUS_SUCCESS;
        }

#if 0
        /* The list is sorted, so we can cut the loop a bit */
        if (ID.tei_instance < Instance->InstanceId.tei_instance)
            break;
#endif

        ListEntry = ListEntry->Flink;
    }

    ReleaseEntityLock(ID.tei_entity, OldIrql);
    /* Maybe we could find a more descriptive status */
    return STATUS_INVALID_PARAMETER;
}
