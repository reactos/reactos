/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/irq.c
 * PURPOSE:         I/O Wrappers (called Completion Ports) for Kernel Queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <devpropdef.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoConnectInterrupt(OUT PKINTERRUPT *InterruptObject,
                   IN PKSERVICE_ROUTINE ServiceRoutine,
                   IN PVOID ServiceContext,
                   IN PKSPIN_LOCK SpinLock,
                   IN ULONG Vector,
                   IN KIRQL Irql,
                   IN KIRQL SynchronizeIrql,
                   IN KINTERRUPT_MODE InterruptMode,
                   IN BOOLEAN ShareVector,
                   IN KAFFINITY ProcessorEnableMask,
                   IN BOOLEAN FloatingSave)
{
    PKINTERRUPT Interrupt;
    PKINTERRUPT InterruptUsed;
    PIO_INTERRUPT IoInterrupt;
    BOOLEAN FirstRun;
    CCHAR Count = 0;
    KAFFINITY Affinity;

    PAGED_CODE();

    /* Assume failure */
    *InterruptObject = NULL;

    /* Get the affinity */
    Affinity = ProcessorEnableMask & KeActiveProcessors;
    while (Affinity)
    {
        /* Increase count */
        if (Affinity & 1) Count++;
        Affinity >>= 1;
    }

    /* Make sure we have a valid CPU count */
    if (!Count) return STATUS_INVALID_PARAMETER;

    /* Allocate the array of I/O interrupts */
    IoInterrupt = ExAllocatePoolZero(NonPagedPool,
                                     (Count - 1) * sizeof(KINTERRUPT) +
                                     sizeof(IO_INTERRUPT),
                                     TAG_IO_INTERRUPT);
    if (!IoInterrupt) return STATUS_INSUFFICIENT_RESOURCES;

    /* Use the structure's spinlock, if none was provided */
    if (!SpinLock)
    {
        SpinLock = &IoInterrupt->SpinLock;
        KeInitializeSpinLock(SpinLock);
    }

    /* We first start with a built-in interrupt inside the I/O structure */
    Interrupt = (PKINTERRUPT)(IoInterrupt + 1);
    FirstRun = TRUE;

    /* Now create all the interrupts */
    Affinity = ProcessorEnableMask & KeActiveProcessors;
    for (Count = 0; Affinity; Count++, Affinity >>= 1)
    {
        /* Check if it's enabled for this CPU */
        if (!(Affinity & 1))
            continue;

        /* Check which one we will use */
        InterruptUsed = FirstRun ? &IoInterrupt->FirstInterrupt : Interrupt;

        /* Initialize it */
        KeInitializeInterrupt(InterruptUsed,
                              ServiceRoutine,
                              ServiceContext,
                              SpinLock,
                              Vector,
                              Irql,
                              SynchronizeIrql,
                              InterruptMode,
                              ShareVector,
                              Count,
                              FloatingSave);

        /* Connect it */
        if (!KeConnectInterrupt(InterruptUsed))
        {
            /* Check how far we got */
            if (FirstRun)
            {
                /* We failed early so just free this */
                ExFreePoolWithTag(IoInterrupt, TAG_IO_INTERRUPT);
            }
            else
            {
                /* Far enough, so disconnect everything */
                IoDisconnectInterrupt(&IoInterrupt->FirstInterrupt);
            }

            /* And fail */
            return STATUS_INVALID_PARAMETER;
        }

        /* Now we've used up our First Run */
        if (FirstRun)
        {
            FirstRun = FALSE;
        }
        else
        {
            /* Move on to the next one */
            IoInterrupt->Interrupt[(UCHAR)Count] = Interrupt++;
        }
    }

    /* Return success */
    *InterruptObject = &IoInterrupt->FirstInterrupt;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoDisconnectInterrupt(PKINTERRUPT InterruptObject)
{
    PIO_INTERRUPT IoInterrupt;
    ULONG i;

    PAGED_CODE();

    /* Get the I/O interrupt */
    IoInterrupt = CONTAINING_RECORD(InterruptObject,
                                    IO_INTERRUPT,
                                    FirstInterrupt);

    /* Disconnect the first one */
    KeDisconnectInterrupt(&IoInterrupt->FirstInterrupt);

    /* Now disconnect the others */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Make sure one was registered */
        if (!IoInterrupt->Interrupt[i])
            continue;

        /* Disconnect it */
        KeDisconnectInterrupt(IoInterrupt->Interrupt[i]);
    }

    /* Free the I/O interrupt */
    ExFreePoolWithTag(IoInterrupt, TAG_IO_INTERRUPT);
}

/* CONNECTION DATA BASED CONNECTS *********************************************/

#ifndef DRIVER_VIOLATION
#define DRIVER_VIOLATION 0x121
#endif

/* Device property with the INTERRUPT_CONNECTION_DATA of the device */
static const DEVPROPKEY IopInterruptConnectionDataKey =
{
    { 0xF0E20F09, 0xD97A, 0x49A9, { 0x80, 0x46, 0xBB, 0x6E, 0x22, 0xE6, 0xBB, 0x2E } },
    2
};

#define IOP_CONNECTION_DATA_SIZE(Count) \
    (FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) + (Count) * sizeof(INTERRUPT_VECTOR_DATA))

/* An interrupt connected for one vector of the connection data of a device */
typedef struct _IOP_VECTOR_CONNECTION
{
    PKINTERRUPT Interrupt;
    INTERRUPT_CONNECTION_DATA Vector;
} IOP_VECTOR_CONNECTION, *PIOP_VECTOR_CONNECTION;

/*
 * The line interrupts of a device connected with CONNECT_LINE_BASED. The
 * driver gets a copy of the interrupt object of the first line.
 */
typedef struct _IOP_LINE_INTERRUPTS
{
    KINTERRUPT Interrupt;
    KIRQL UnifiedIrql;
    ULONG Count;
    IOP_VECTOR_CONNECTION Lines[ANYSIZE_ARRAY];
} IOP_LINE_INTERRUPTS, *PIOP_LINE_INTERRUPTS;

/* Context of the interrupt object of one message */
typedef struct _IOP_MESSAGE_INTERRUPT_LINK
{
    PKMESSAGE_SERVICE_ROUTINE ServiceRoutine;
    PVOID ServiceContext;
    ULONG MessageNumber;
    IOP_VECTOR_CONNECTION Connection;
} IOP_MESSAGE_INTERRUPT_LINK, *PIOP_MESSAGE_INTERRUPT_LINK;

/*
 * The messages of a device connected with CONNECT_MESSAGE_BASED. The table
 * returned to the driver is part of this structure, and the links follow the
 * table in the same allocation.
 */
typedef struct _IOP_MESSAGE_INTERRUPTS
{
    PIOP_MESSAGE_INTERRUPT_LINK Links;
    IO_INTERRUPT_MESSAGE_INFO Table;
} IOP_MESSAGE_INTERRUPTS, *PIOP_MESSAGE_INTERRUPTS;

#define IOP_MESSAGE_TABLE_SIZE(Count) \
    (FIELD_OFFSET(IOP_MESSAGE_INTERRUPTS, Table.MessageInfo) + \
     (Count) * sizeof(IO_INTERRUPT_MESSAGE_INFO_ENTRY))

/**
 * @brief
 * Reads the INTERRUPT_CONNECTION_DATA property of a device.
 *
 * @param[out] ConnectionData
 * Receives a copy of the property, or NULL if the property is empty. The
 * caller frees it.
 *
 * @return
 * The status of the property query, or STATUS_DATA_ERROR if the property is
 * shorter than its vector count says.
 */
static
NTSTATUS
IopReadInterruptConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PINTERRUPT_CONNECTION_DATA *ConnectionData)
{
    PINTERRUPT_CONNECTION_DATA Data;
    DEVPROPTYPE Type;
    ULONG Size = 0;
    NTSTATUS Status;

    PAGED_CODE();

    *ConnectionData = NULL;

    Status = IoGetDevicePropertyData(Pdo, &IopInterruptConnectionDataKey,
                                     0, 0, 0, NULL, &Size, &Type);
    if (Status != STATUS_BUFFER_TOO_SMALL || Size < IOP_CONNECTION_DATA_SIZE(1))
        return Status;

    Data = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_IO_INTERRUPT);
    if (Data == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = IoGetDevicePropertyData(Pdo, &IopInterruptConnectionDataKey,
                                     0, 0, Size, Data, &Size, &Type);
    if (NT_SUCCESS(Status) && Size < IOP_CONNECTION_DATA_SIZE(Data->Count))
        Status = STATUS_DATA_ERROR;

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Data, TAG_IO_INTERRUPT);
        return Status;
    }

    *ConnectionData = Data;
    return Status;
}

/* Interrupts are only connected from the connection data of a PnP device */
static
BOOLEAN
IopIsPnpPhysicalDevice(
    _In_opt_ PDEVICE_OBJECT Pdo)
{
    PDEVICE_NODE DeviceNode;

    if (Pdo == NULL)
        return FALSE;

    DeviceNode = IopGetDeviceNode(Pdo);
    return DeviceNode != NULL && !(DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE);
}

static
BOOLEAN
IopIsMessageVector(
    _In_ PINTERRUPT_VECTOR_DATA VectorData)
{
    switch (VectorData->Type)
    {
        case InterruptTypeXapicMessage:
        case InterruptTypeHypertransport:
        case InterruptTypeMessageRequest:
            return TRUE;

        default:
            return FALSE;
    }
}

/* Processor groups only exist in the connection data from Windows 7 on */
static
USHORT
IopGetVectorGroup(
    _In_ PINTERRUPT_VECTOR_DATA VectorData)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    return VectorData->TargetProcessors.Group;
#else
    UNREFERENCED_PARAMETER(VectorData);
    return 0;
#endif
}

static
KAFFINITY
IopGetVectorTarget(
    _In_ PINTERRUPT_VECTOR_DATA VectorData)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    return VectorData->TargetProcessors.Mask;
#else
    return VectorData->TargetProcessors;
#endif
}

/**
 * @brief
 * Enables the interrupt of one connection data element in the HAL, then
 * connects an interrupt object for it on the target processors that are
 * active.
 *
 * @param[in] SynchronizeIrql
 * The IRQL the service routine is synchronized at. PASSIVE_LEVEL asks for a
 * passive level interrupt, which the kernel does not support.
 */
static
NTSTATUS
IopConnectVector(
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _In_ PKSERVICE_ROUTINE ServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _In_opt_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL SynchronizeIrql,
    _In_ BOOLEAN ShareVector,
    _In_ BOOLEAN FloatingSave,
    _Out_ PIOP_VECTOR_CONNECTION Connection)
{
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(Connection, sizeof(*Connection));
    Connection->Vector.Count = 1;
    Connection->Vector.Vectors[0] = *VectorData;

    if (SynchronizeIrql == PASSIVE_LEVEL)
    {
        DPRINT1("Passive level interrupts are not supported (vector %lu)\n", VectorData->Vector);
        return STATUS_NOT_SUPPORTED;
    }

    Status = HalEnableInterrupt(&Connection->Vector);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IoConnectInterrupt(&Connection->Interrupt,
                                ServiceRoutine,
                                ServiceContext,
                                SpinLock,
                                VectorData->Vector,
                                VectorData->Irql,
                                SynchronizeIrql,
                                VectorData->Mode,
                                ShareVector,
                                IopGetVectorTarget(VectorData),
                                FloatingSave);
    if (!NT_SUCCESS(Status))
        HalDisableInterrupt(&Connection->Vector);

    return Status;
}

static
VOID
IopDisconnectVector(
    _In_ PIOP_VECTOR_CONNECTION Connection)
{
    IoDisconnectInterrupt(Connection->Interrupt);
    HalDisableInterrupt(&Connection->Vector);
}

/* Disconnects the line interrupts of a device and frees them */
static
VOID
IopDisconnectLineInterrupts(
    _In_ PIOP_LINE_INTERRUPTS Lines)
{
    ULONG Index;

    for (Index = 0; Index < Lines->Count; Index++)
        IopDisconnectVector(&Lines->Lines[Index]);

    ExFreePoolWithTag(Lines, TAG_IO_INTERRUPT);
}

/**
 * @brief
 * Connects every line interrupt of the connection data of a device, for
 * CONNECT_LINE_BASED. All lines are synchronized at the highest IRQL among
 * them, or at SynchronizeIrql when it is given.
 *
 * @param[out] InterruptObject
 * Receives the interrupt object of the first line.
 */
static
NTSTATUS
IopConnectLineInterrupts(
    _In_opt_ PDEVICE_OBJECT Pdo,
    _Out_ PKINTERRUPT *InterruptObject,
    _In_opt_ PKSERVICE_ROUTINE ServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _In_opt_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL SynchronizeIrql,
    _In_ BOOLEAN FloatingSave)
{
    PINTERRUPT_CONNECTION_DATA ConnectionData;
    PIOP_LINE_INTERRUPTS Lines = NULL;
    KIRQL HighestIrql = PASSIVE_LEVEL;
    ULONG LineCount = 0;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    *InterruptObject = NULL;

    if (!IopIsPnpPhysicalDevice(Pdo) || ServiceRoutine == NULL)
        return STATUS_INVALID_PARAMETER;

    Status = IopReadInterruptConnectionData(Pdo, &ConnectionData);
    if (!NT_SUCCESS(Status) || ConnectionData == NULL)
        return Status;

    for (Index = 0; Index < ConnectionData->Count; Index++)
    {
        if (ConnectionData->Vectors[Index].Type != InterruptTypeControllerInput)
            continue;

        LineCount++;
        HighestIrql = max(HighestIrql, ConnectionData->Vectors[Index].Irql);
    }

    if (LineCount == 0 || (SynchronizeIrql != PASSIVE_LEVEL && SynchronizeIrql < HighestIrql))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    Lines = ExAllocatePoolZero(NonPagedPool,
                               FIELD_OFFSET(IOP_LINE_INTERRUPTS, Lines) +
                               LineCount * sizeof(IOP_VECTOR_CONNECTION),
                               TAG_IO_INTERRUPT);
    if (Lines == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    Lines->UnifiedIrql = (SynchronizeIrql != PASSIVE_LEVEL) ? SynchronizeIrql : HighestIrql;

    for (Index = 0; Index < ConnectionData->Count && Lines->Count < LineCount; Index++)
    {
        if (ConnectionData->Vectors[Index].Type != InterruptTypeControllerInput)
            continue;

        Status = IopConnectVector(&ConnectionData->Vectors[Index],
                                  ServiceRoutine,
                                  ServiceContext,
                                  SpinLock,
                                  Lines->UnifiedIrql,
                                  TRUE,
                                  FloatingSave,
                                  &Lines->Lines[Lines->Count]);
        if (!NT_SUCCESS(Status))
            goto Done;

        Lines->Count++;
    }

    /* The copy synchronizes like the interrupt object of the first line */
    RtlCopyMemory(&Lines->Interrupt, Lines->Lines[0].Interrupt, sizeof(KINTERRUPT));
    *InterruptObject = &Lines->Interrupt;

Done:
    ExFreePoolWithTag(ConnectionData, TAG_IO_INTERRUPT);

    if (!NT_SUCCESS(Status) && Lines != NULL)
        IopDisconnectLineInterrupts(Lines);

    return Status;
}

/* Calls the message service routine of the driver with the message number */
static
BOOLEAN
NTAPI
IopMessageInterruptService(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID ServiceContext)
{
    PIOP_MESSAGE_INTERRUPT_LINK Link = ServiceContext;

    return Link->ServiceRoutine(Interrupt, Link->ServiceContext, Link->MessageNumber);
}

/* Disconnects the interrupts of a message table and frees it */
static
VOID
IopDisconnectMessageInterrupts(
    _In_ PIO_INTERRUPT_MESSAGE_INFO Table)
{
    PIOP_MESSAGE_INTERRUPTS Messages = CONTAINING_RECORD(Table, IOP_MESSAGE_INTERRUPTS, Table);
    ULONG Index;

    PAGED_CODE();

    for (Index = 0; Index < Table->MessageCount; Index++)
        IopDisconnectVector(&Messages->Links[Index].Connection);

    ExFreePoolWithTag(Messages, TAG_IO_INTERRUPT);
}

/**
 * @brief
 * Connects one message and adds it to the message table. The table entry
 * describes the message as the HAL routes it.
 */
static
NTSTATUS
IopConnectOneMessage(
    _Inout_ PIOP_MESSAGE_INTERRUPTS Messages,
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _In_ PKMESSAGE_SERVICE_ROUTINE MessageServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _In_opt_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL SynchronizeIrql,
    _In_ BOOLEAN FloatingSave)
{
    PIO_INTERRUPT_MESSAGE_INFO Table = &Messages->Table;
    PIO_INTERRUPT_MESSAGE_INFO_ENTRY Entry = &Table->MessageInfo[Table->MessageCount];
    PIOP_MESSAGE_INTERRUPT_LINK Link = &Messages->Links[Table->MessageCount];
    PINTERRUPT_VECTOR_DATA Routed = VectorData;
    INTERRUPT_CONNECTION_DATA RoutingInfo;
    HAL_MESSAGE_SIGNAL_TARGET_REQUEST Request;
    NTSTATUS Status;

    Link->ServiceRoutine = MessageServiceRoutine;
    Link->ServiceContext = ServiceContext;
    Link->MessageNumber = Table->MessageCount;

    Status = IopConnectVector(VectorData,
                              IopMessageInterruptService,
                              Link,
                              SpinLock,
                              SynchronizeIrql,
                              TRUE,
                              FloatingSave,
                              &Link->Connection);
    if (!NT_SUCCESS(Status))
        return Status;

    /* The HAL builds the address and data of a message request */
    if (VectorData->Type == InterruptTypeMessageRequest)
    {
        RtlZeroMemory(&Request, sizeof(Request));
        Request.InterruptTargetType = TargetApic;
        Request.ApicTarget.InterruptVector = VectorData->Vector;
        Request.ApicTarget.ApicDestinationMode = VectorData->MessageRequest.DestinationMode;
        Request.ApicTarget.TargetProcessors = VectorData->TargetProcessors;
#if (NTDDI_VERSION >= NTDDI_WIN7)
        Request.ApicTarget.InterruptRemapInfo = VectorData->IntRemapInfo;
#endif

        Status = HalGetMessageRoutingInfo(&Request, &RoutingInfo);
        if (!NT_SUCCESS(Status))
        {
            IopDisconnectVector(&Link->Connection);
            return Status;
        }

        Routed = &RoutingInfo.Vectors[0];
    }

    Entry->MessageAddress = Routed->XapicMessage.Address;
    Entry->MessageData = Routed->XapicMessage.DataPayload;
    Entry->TargetProcessorSet = IopGetVectorTarget(Routed);
    Entry->InterruptObject = Link->Connection.Interrupt;
    Entry->Vector = Routed->Vector;
    Entry->Irql = Routed->Irql;
    Entry->Mode = Routed->Mode;
    Entry->Polarity = Routed->Polarity;

    Table->MessageCount++;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Connects all messages of the connection data of a device, for
 * CONNECT_MESSAGE_BASED and CONNECT_MESSAGE_BASED_PASSIVE.
 *
 * @remarks
 * The messages are synchronized at SynchronizeIrql when it is given, at the
 * highest message IRQL when they share a spin lock, and each at its own IRQL
 * otherwise. Passive messages are synchronized at PASSIVE_LEVEL.
 */
static
NTSTATUS
IopConnectMessageInterrupts(
    _In_ ULONG Version,
    _In_opt_ PDEVICE_OBJECT Pdo,
    _Out_ PIO_INTERRUPT_MESSAGE_INFO *MessageTable,
    _In_opt_ PKMESSAGE_SERVICE_ROUTINE MessageServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _In_opt_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL SynchronizeIrql,
    _In_ BOOLEAN FloatingSave)
{
    PINTERRUPT_CONNECTION_DATA ConnectionData;
    PIOP_MESSAGE_INTERRUPTS Messages = NULL;
    BOOLEAN Passive = (Version == CONNECT_MESSAGE_BASED_PASSIVE);
    KIRQL UnifiedIrql = PASSIVE_LEVEL;
    ULONG MessageCount = 0;
    ULONG VectorCount;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    *MessageTable = NULL;

    if (!IopIsPnpPhysicalDevice(Pdo) || MessageServiceRoutine == NULL)
        return STATUS_INVALID_PARAMETER;

    Status = IopReadInterruptConnectionData(Pdo, &ConnectionData);
    if (!NT_SUCCESS(Status))
        return Status;

    VectorCount = (ConnectionData != NULL) ? ConnectionData->Count : 0;

    for (Index = 0; Index < VectorCount; Index++)
    {
        PINTERRUPT_VECTOR_DATA VectorData = &ConnectionData->Vectors[Index];

        if (!IopIsMessageVector(VectorData))
            continue;

        MessageCount++;

        if (Passive)
            continue;

        if (SynchronizeIrql != PASSIVE_LEVEL)
        {
            if (SynchronizeIrql < VectorData->Irql)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            UnifiedIrql = SynchronizeIrql;
        }
        else if (SpinLock != NULL)
        {
            UnifiedIrql = max(UnifiedIrql, VectorData->Irql);
        }
    }

    if (MessageCount == 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    Messages = ExAllocatePoolZero(NonPagedPool,
                                  IOP_MESSAGE_TABLE_SIZE(MessageCount) +
                                  MessageCount * sizeof(IOP_MESSAGE_INTERRUPT_LINK),
                                  TAG_IO_INTERRUPT);
    if (Messages == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    Messages->Links = (PIOP_MESSAGE_INTERRUPT_LINK)
        ((PUCHAR)Messages + IOP_MESSAGE_TABLE_SIZE(MessageCount));
    Messages->Table.UnifiedIrql = UnifiedIrql;

    for (Index = 0; Index < VectorCount; Index++)
    {
        PINTERRUPT_VECTOR_DATA VectorData = &ConnectionData->Vectors[Index];
        KIRQL ConnectIrql;

        if (!IopIsMessageVector(VectorData))
            continue;

        if (Passive)
            ConnectIrql = PASSIVE_LEVEL;
        else if (UnifiedIrql != PASSIVE_LEVEL)
            ConnectIrql = UnifiedIrql;
        else
            ConnectIrql = VectorData->Irql;

        Status = IopConnectOneMessage(Messages,
                                      VectorData,
                                      MessageServiceRoutine,
                                      ServiceContext,
                                      SpinLock,
                                      ConnectIrql,
                                      FloatingSave);
        if (!NT_SUCCESS(Status))
            goto Done;
    }

    *MessageTable = &Messages->Table;

Done:
    if (ConnectionData != NULL)
        ExFreePoolWithTag(ConnectionData, TAG_IO_INTERRUPT);

    if (!NT_SUCCESS(Status) && Messages != NULL)
        IopDisconnectMessageInterrupts(&Messages->Table);

    return Status;
}

/* Checks if a connection data element is the interrupt a caller specified */
static
BOOLEAN
IopVectorMatchesRequest(
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _In_ PIO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS Request,
    _In_ USHORT Group)
{
    /* An IRQL of 0 matches any IRQL */
    if (Request->Irql != PASSIVE_LEVEL && Request->Irql != VectorData->Irql)
        return FALSE;

    return VectorData->Vector == Request->Vector &&
           VectorData->Mode == Request->InterruptMode &&
           IopGetVectorGroup(VectorData) == Group &&
           IopGetVectorTarget(VectorData) == Request->ProcessorEnableMask;
}

/**
 * @brief
 * Connects an interrupt for CONNECT_FULLY_SPECIFIED and
 * CONNECT_FULLY_SPECIFIED_GROUP. When the device has connection data, the
 * element that matches the request is connected. Otherwise the interrupt is
 * connected as specified.
 *
 * @param[in] Group
 * The processor group, always 0 for CONNECT_FULLY_SPECIFIED.
 *
 * @remarks
 * Irql and SynchronizeIrql of 0 ask for a passive level interrupt.
 */
static
NTSTATUS
IopConnectFullySpecifiedInterrupt(
    _Inout_ PIO_CONNECT_INTERRUPT_PARAMETERS Parameters,
    _In_ USHORT Group)
{
    PIO_CONNECT_INTERRUPT_FULLY_SPECIFIED_PARAMETERS Request = &Parameters->FullySpecified;
    PINTERRUPT_CONNECTION_DATA ConnectionData;
    IOP_VECTOR_CONNECTION Connection;
    BOOLEAN Passive;
    ULONG VectorCount;
    ULONG Index;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    if (Request->PhysicalDeviceObject == NULL ||
        Request->ServiceRoutine == NULL ||
        Request->SynchronizeIrql < Request->Irql)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* A passive level interrupt has no spin lock */
    Passive = (Request->Irql == PASSIVE_LEVEL && Request->SynchronizeIrql == PASSIVE_LEVEL);
    if (Passive && Request->SpinLock != NULL)
        return STATUS_INVALID_PARAMETER;

    if (!NT_SUCCESS(IopReadInterruptConnectionData(Request->PhysicalDeviceObject, &ConnectionData)))
    {
        if (Passive)
            return STATUS_NOT_SUPPORTED;

        return IoConnectInterrupt(Request->InterruptObject,
                                  Request->ServiceRoutine,
                                  Request->ServiceContext,
                                  Request->SpinLock,
                                  Request->Vector,
                                  Request->Irql,
                                  Request->SynchronizeIrql,
                                  Request->InterruptMode,
                                  Request->ShareVector,
                                  Request->ProcessorEnableMask,
                                  Request->FloatingSave);
    }

    VectorCount = (ConnectionData != NULL) ? ConnectionData->Count : 0;

    for (Index = 0; Index < VectorCount; Index++)
    {
        PINTERRUPT_VECTOR_DATA Match = &ConnectionData->Vectors[Index];

        if (!IopVectorMatchesRequest(Match, Request, Group))
            continue;

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
        /* Wake interrupts are always shared */
        if ((Match->Type == InterruptTypeControllerInput) &&
            (Match->ControllerInput.WakeInterrupt != 0))
        {
            Request->ShareVector = TRUE;
        }
#endif

        Status = IopConnectVector(Match,
                                  Request->ServiceRoutine,
                                  Request->ServiceContext,
                                  Request->SpinLock,
                                  Request->SynchronizeIrql,
                                  Request->ShareVector,
                                  Request->FloatingSave,
                                  &Connection);
        if (NT_SUCCESS(Status))
            *Request->InterruptObject = Connection.Interrupt;

        break;
    }

    if (Index == VectorCount)
    {
        DPRINT1("IoConnectInterruptEx: PDO %p has no vector %lu at irql %u"
                " mode %u group %u affinity 0x%p\n",
                Request->PhysicalDeviceObject, Request->Vector, Request->Irql,
                Request->InterruptMode, Group, (PVOID)(ULONG_PTR)Request->ProcessorEnableMask);
    }

    if (ConnectionData != NULL)
        ExFreePoolWithTag(ConnectionData, TAG_IO_INTERRUPT);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoConnectInterruptEx(
    _Inout_ PIO_CONNECT_INTERRUPT_PARAMETERS Parameters)
{
    PIO_CONNECT_INTERRUPT_MESSAGE_BASED_PARAMETERS MessageBased = &Parameters->MessageBased;
    NTSTATUS Status;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        KeBugCheckEx(DRIVER_VIOLATION, 1, KeGetCurrentIrql(), 0, 0);

    DPRINT("IoConnectInterruptEx: version %lu\n", Parameters->Version);

    switch (Parameters->Version)
    {
        case CONNECT_FULLY_SPECIFIED:
            return IopConnectFullySpecifiedInterrupt(Parameters, 0);

        case CONNECT_FULLY_SPECIFIED_GROUP:
            return IopConnectFullySpecifiedInterrupt(Parameters, Parameters->FullySpecified.Group);

        case CONNECT_LINE_BASED:
            return IopConnectLineInterrupts(Parameters->LineBased.PhysicalDeviceObject,
                                            Parameters->LineBased.InterruptObject,
                                            Parameters->LineBased.ServiceRoutine,
                                            Parameters->LineBased.ServiceContext,
                                            Parameters->LineBased.SpinLock,
                                            Parameters->LineBased.SynchronizeIrql,
                                            Parameters->LineBased.FloatingSave);

        case CONNECT_MESSAGE_BASED:
        case CONNECT_MESSAGE_BASED_PASSIVE:
            break;

        default:
            /* Return the highest version that is supported */
            Parameters->Version = CONNECT_MESSAGE_BASED;
            return STATUS_NOT_SUPPORTED;
    }

    Status = IopConnectMessageInterrupts(Parameters->Version,
                                         MessageBased->PhysicalDeviceObject,
                                         MessageBased->ConnectionContext.InterruptMessageTable,
                                         MessageBased->MessageServiceRoutine,
                                         MessageBased->ServiceContext,
                                         MessageBased->SpinLock,
                                         MessageBased->SynchronizeIrql,
                                         MessageBased->FloatingSave);
    if (NT_SUCCESS(Status))
        return Status;

    if (MessageBased->FallBackServiceRoutine == NULL)
        return STATUS_NOT_SUPPORTED;

    /* Tell the caller that line interrupts are connected instead */
    Status = IopConnectLineInterrupts(MessageBased->PhysicalDeviceObject,
                                      MessageBased->ConnectionContext.InterruptObject,
                                      MessageBased->FallBackServiceRoutine,
                                      MessageBased->ServiceContext,
                                      MessageBased->SpinLock,
                                      MessageBased->SynchronizeIrql,
                                      MessageBased->FloatingSave);
    Parameters->Version = CONNECT_LINE_BASED;

    return Status;
}

VOID
NTAPI
IoDisconnectInterruptEx(
    _In_ PIO_DISCONNECT_INTERRUPT_PARAMETERS Parameters)
{
    PAGED_CODE();

    switch (Parameters->Version)
    {
        case CONNECT_FULLY_SPECIFIED:
        case CONNECT_FULLY_SPECIFIED_GROUP:
            IoDisconnectInterrupt(Parameters->ConnectionContext.InterruptObject);
            break;

        case CONNECT_LINE_BASED:
            IopDisconnectLineInterrupts(
                CONTAINING_RECORD(Parameters->ConnectionContext.InterruptObject,
                                  IOP_LINE_INTERRUPTS,
                                  Interrupt));
            break;

        case CONNECT_MESSAGE_BASED:
        case CONNECT_MESSAGE_BASED_PASSIVE:
            IopDisconnectMessageInterrupts(Parameters->ConnectionContext.InterruptMessageTable);
            break;

        default:
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 9, Parameters->Version, 0, 0);
    }
}

/**
 * @brief
 * Returns the processor affinity of an interrupt object. Only processor
 * group 0 is supported.
 */
NTSTATUS
NTAPI
IoGetAffinityInterrupt(
    _In_ PKINTERRUPT InterruptObject,
    _Out_ PGROUP_AFFINITY GroupAffinity)
{
    if (InterruptObject == NULL || GroupAffinity == NULL)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(GroupAffinity, sizeof(*GroupAffinity));
    GroupAffinity->Mask = (KAFFINITY)1 << InterruptObject->Number;

    return STATUS_SUCCESS;
}

/* EOF */
