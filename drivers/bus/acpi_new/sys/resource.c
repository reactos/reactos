/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     _CRS and _PRS to NT resource list conversion
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <reshub_downlevel.h>
#include <debug.h>

/* Raw AML resource item header, ACPI 6.5 section 6.4 */
#define UACPINT_RES_LARGE_ITEM          0x80
#define UACPINT_RES_SMALL_NAME_MASK     0x78
#define UACPINT_RES_SMALL_END_TAG       0x78
#define UACPINT_RES_SMALL_LENGTH_MASK   0x07
#define UACPINT_RES_SMALL_HEADER_SIZE   1
#define UACPINT_RES_LARGE_HEADER_SIZE   3

/* Log each window handed out and every dropped _CRS descriptor */
ULONG UacpiNtResVerbose = 1;

typedef struct _UACPINT_ADDRESS_WINDOW
{
    UCHAR     WindowType;    ///< UACPI_RANGE_*
    UCHAR     Direction;    ///< UACPI_PRODUCER or UACPI_CONSUMER
    ULONGLONG Start;
    ULONGLONG Length;
} UACPINT_ADDRESS_WINDOW, *PUACPINT_ADDRESS_WINDOW;

static
BOOLEAN
NTAPI
UacpiNtGetAddressWindow(
    _In_ uacpi_resource *Resource,
    _Out_ PUACPINT_ADDRESS_WINDOW Window)
{
    switch (Resource->type)
    {
        case UACPI_RESOURCE_TYPE_ADDRESS16:
            Window->WindowType = Resource->address16.common.type;
            Window->Direction = Resource->address16.common.direction;
            Window->Start = Resource->address16.minimum;
            Window->Length = Resource->address16.address_length;
            return TRUE;

        case UACPI_RESOURCE_TYPE_ADDRESS32:
            Window->WindowType = Resource->address32.common.type;
            Window->Direction = Resource->address32.common.direction;
            Window->Start = Resource->address32.minimum;
            Window->Length = Resource->address32.address_length;
            return TRUE;

        case UACPI_RESOURCE_TYPE_ADDRESS64:
            Window->WindowType = Resource->address64.common.type;
            Window->Direction = Resource->address64.common.direction;
            Window->Start = Resource->address64.minimum;
            Window->Length = Resource->address64.address_length;
            return TRUE;

        /* Extended Address Space (0x8B) describes the same window */
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            Window->WindowType = Resource->address64_extended.common.type;
            Window->Direction = Resource->address64_extended.common.direction;
            Window->Start = Resource->address64_extended.minimum;
            Window->Length = Resource->address64_extended.address_length;
            return TRUE;

        default:
            RtlZeroMemory(Window, sizeof(*Window));
            return FALSE;
    }
}

/* Interrupt list of an IRQ or Extended IRQ descriptor, 0 for anything else */
static
ULONG
NTAPI
UacpiNtGetInterrupts(
    _In_ uacpi_resource *Resource,
    _Out_ PUCHAR Triggering,
    _Out_ PUCHAR Sharing)
{
    if (Resource->type == UACPI_RESOURCE_TYPE_IRQ)
    {
        *Triggering = Resource->irq.triggering;
        *Sharing = Resource->irq.sharing;
        return Resource->irq.num_irqs;
    }

    if (Resource->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ)
    {
        *Triggering = Resource->extended_irq.triggering;
        *Sharing = Resource->extended_irq.sharing;
        return Resource->extended_irq.num_irqs;
    }

    *Triggering = 0;
    *Sharing = 0;
    return 0;
}

static
ULONG
NTAPI
UacpiNtGetInterrupt(
    _In_ uacpi_resource *Resource,
    _In_ ULONG Index)
{
    if (Resource->type == UACPI_RESOURCE_TYPE_IRQ)
        return Resource->irq.irqs[Index];

    return Resource->extended_irq.irqs[Index];
}

/* One _CRS entry to zero or more CM descriptors, only counts without Descriptor */
static
ULONG
NTAPI
UacpiNtEmitCm(
    _In_ uacpi_resource *Resource,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor)
{
    UACPINT_ADDRESS_WINDOW Window;
    UCHAR Triggering;
    UCHAR Sharing;
    ULONG Count;
    ULONG i;

    switch (Resource->type)
    {
        case UACPI_RESOURCE_TYPE_IRQ:
        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            Count = UacpiNtGetInterrupts(Resource, &Triggering, &Sharing);
            if (!Descriptor)
                return Count;

            for (i = 0; i < Count; i++)
            {
                RtlZeroMemory(&Descriptor[i], sizeof(Descriptor[i]));
                Descriptor[i].Type = CmResourceTypeInterrupt;
                Descriptor[i].ShareDisposition = (Sharing == UACPI_SHARED) ?
                                                 CmResourceShareShared :
                                                 CmResourceShareDeviceExclusive;
                Descriptor[i].Flags = (Triggering == UACPI_TRIGGERING_EDGE) ?
                                      CM_RESOURCE_INTERRUPT_LATCHED :
                                      CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                Descriptor[i].u.Interrupt.Level = UacpiNtGetInterrupt(Resource, i);
                Descriptor[i].u.Interrupt.Vector = UacpiNtGetInterrupt(Resource, i);
                Descriptor[i].u.Interrupt.Affinity = (KAFFINITY)-1;
            }
            return Count;

        case UACPI_RESOURCE_TYPE_IO:
        case UACPI_RESOURCE_TYPE_FIXED_IO:
            if (!Descriptor)
                return 1;

            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->Type = CmResourceTypePort;
            Descriptor->Flags = CM_RESOURCE_PORT_IO;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            if (Resource->type == UACPI_RESOURCE_TYPE_IO)
            {
                Descriptor->u.Port.Start.QuadPart = Resource->io.minimum;
                Descriptor->u.Port.Length = Resource->io.length;
            }
            else
            {
                Descriptor->u.Port.Start.QuadPart = Resource->fixed_io.address;
                Descriptor->u.Port.Length = Resource->fixed_io.length;
            }
            return 1;

        case UACPI_RESOURCE_TYPE_MEMORY32:
        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            if (!Descriptor)
                return 1;

            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->Type = CmResourceTypeMemory;
            Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            if (Resource->type == UACPI_RESOURCE_TYPE_MEMORY32)
            {
                Descriptor->u.Memory.Start.QuadPart = Resource->memory32.minimum;
                Descriptor->u.Memory.Length = Resource->memory32.length;
            }
            else
            {
                Descriptor->u.Memory.Start.QuadPart = Resource->fixed_memory32.address;
                Descriptor->u.Memory.Length = Resource->fixed_memory32.length;
            }
            return 1;

        case UACPI_RESOURCE_TYPE_ADDRESS16:
        case UACPI_RESOURCE_TYPE_ADDRESS32:
        case UACPI_RESOURCE_TYPE_ADDRESS64:
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            UacpiNtGetAddressWindow(Resource, &Window);
            if (Window.Length == 0)
                return 0;

            if (!Descriptor)
                return 1;

            /* Producer windows stay exclusive, resarb.c sub-allocates them */
            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

            if (Window.WindowType == UACPI_RANGE_MEMORY)
            {
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                if (Window.Length <= 0xFFFFFFFFull)
                {
                    Descriptor->Type = CmResourceTypeMemory;
                    Descriptor->u.Memory.Start.QuadPart = Window.Start;
                    Descriptor->u.Memory.Length = (ULONG)Window.Length;
                }
                else
                {
                    /* Length40 holds the length shifted right by 8 */
                    Descriptor->Type = CmResourceTypeMemoryLarge;
                    Descriptor->Flags |= CM_RESOURCE_MEMORY_LARGE_40;
                    Descriptor->u.Memory40.Start.QuadPart = Window.Start;
                    Descriptor->u.Memory40.Length40 = (ULONG)(Window.Length >> 8);
                }
            }
            else if (Window.WindowType == UACPI_RANGE_IO)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->Flags = CM_RESOURCE_PORT_IO;
                Descriptor->u.Port.Start.QuadPart = Window.Start;
                Descriptor->u.Port.Length = (ULONG)Window.Length;
            }
            else
            {
                Descriptor->Type = CmResourceTypeBusNumber;
                Descriptor->u.BusNumber.Start = (ULONG)Window.Start;
                Descriptor->u.BusNumber.Length = (ULONG)Window.Length;
            }
            return 1;

        /* DMA, GPIO, serial bus, dependent functions and vendor data */
        default:
            return 0;
    }
}

/* One _CRS or _PRS entry to zero or more fixed IO descriptors (min == max) */
static
ULONG
NTAPI
UacpiNtEmitIo(
    _In_ uacpi_resource *Resource,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    UACPINT_ADDRESS_WINDOW Window;
    ULONGLONG Start;
    UCHAR Triggering;
    UCHAR Sharing;
    ULONG Length;
    ULONG Vector;
    ULONG Count;
    ULONG i;

    switch (Resource->type)
    {
        case UACPI_RESOURCE_TYPE_IRQ:
        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            Count = UacpiNtGetInterrupts(Resource, &Triggering, &Sharing);
            if (!Descriptor)
                return Count;

            for (i = 0; i < Count; i++)
            {
                Vector = UacpiNtGetInterrupt(Resource, i);

                RtlZeroMemory(&Descriptor[i], sizeof(Descriptor[i]));
                Descriptor[i].Option = (i == 0) ? 0 : IO_RESOURCE_ALTERNATIVE;
                Descriptor[i].Type = CmResourceTypeInterrupt;
                Descriptor[i].ShareDisposition = (Sharing == UACPI_SHARED) ?
                                                 CmResourceShareShared :
                                                 CmResourceShareDeviceExclusive;
                Descriptor[i].Flags = (Triggering == UACPI_TRIGGERING_EDGE) ?
                                      CM_RESOURCE_INTERRUPT_LATCHED :
                                      CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                Descriptor[i].u.Interrupt.MinimumVector = Vector;
                Descriptor[i].u.Interrupt.MaximumVector = Vector;
            }
            return Count;

        case UACPI_RESOURCE_TYPE_IO:
        case UACPI_RESOURCE_TYPE_FIXED_IO:
            if (!Descriptor)
                return 1;

            if (Resource->type == UACPI_RESOURCE_TYPE_IO)
            {
                Start = Resource->io.minimum;
                Length = Resource->io.length;
            }
            else
            {
                Start = Resource->fixed_io.address;
                Length = Resource->fixed_io.length;
            }

            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->Type = CmResourceTypePort;
            Descriptor->Flags = CM_RESOURCE_PORT_IO;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->u.Port.Length = Length;
            Descriptor->u.Port.Alignment = 1;
            Descriptor->u.Port.MinimumAddress.QuadPart = Start;
            Descriptor->u.Port.MaximumAddress.QuadPart = Start + Length - 1;
            return 1;

        case UACPI_RESOURCE_TYPE_MEMORY32:
        case UACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            if (!Descriptor)
                return 1;

            if (Resource->type == UACPI_RESOURCE_TYPE_MEMORY32)
            {
                Start = Resource->memory32.minimum;
                Length = Resource->memory32.length;
            }
            else
            {
                Start = Resource->fixed_memory32.address;
                Length = Resource->fixed_memory32.length;
            }

            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->Type = CmResourceTypeMemory;
            Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->u.Memory.Length = Length;
            Descriptor->u.Memory.Alignment = 1;
            Descriptor->u.Memory.MinimumAddress.QuadPart = Start;
            Descriptor->u.Memory.MaximumAddress.QuadPart = Start + Length - 1;
            return 1;

        case UACPI_RESOURCE_TYPE_ADDRESS16:
        case UACPI_RESOURCE_TYPE_ADDRESS32:
        case UACPI_RESOURCE_TYPE_ADDRESS64:
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            UacpiNtGetAddressWindow(Resource, &Window);
            if (Window.Length == 0)
                return 0;

            /* A producer window is followed by a DevicePrivate marker */
            Count = (Window.Direction == UACPI_PRODUCER) ? 2 : 1;
            if (!Descriptor)
                return Count;

            RtlZeroMemory(Descriptor, sizeof(*Descriptor));
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

            if (Window.WindowType == UACPI_RANGE_MEMORY && Window.Length <= 0xFFFFFFFFull)
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                Descriptor->u.Memory.Length = (ULONG)Window.Length;
                Descriptor->u.Memory.Alignment = 1;
                Descriptor->u.Memory.MinimumAddress.QuadPart = Window.Start;
                Descriptor->u.Memory.MaximumAddress.QuadPart = Window.Start + Window.Length - 1;
            }
            else if (Window.WindowType == UACPI_RANGE_MEMORY)
            {
                /* Same 256 byte unit encoding as the CM path */
                Descriptor->Type = CmResourceTypeMemoryLarge;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_LARGE_40;
                Descriptor->u.Memory40.Length40 = (ULONG)(Window.Length >> 8);
                Descriptor->u.Memory40.Alignment40 = 1;
                Descriptor->u.Memory40.MinimumAddress.QuadPart = Window.Start;
                Descriptor->u.Memory40.MaximumAddress.QuadPart = Window.Start + Window.Length - 1;
            }
            else if (Window.WindowType == UACPI_RANGE_IO)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->Flags = CM_RESOURCE_PORT_IO;
                Descriptor->u.Port.Length = (ULONG)Window.Length;
                Descriptor->u.Port.Alignment = 1;
                Descriptor->u.Port.MinimumAddress.QuadPart = Window.Start;
                Descriptor->u.Port.MaximumAddress.QuadPart = Window.Start + Window.Length - 1;
            }
            else
            {
                Descriptor->Type = CmResourceTypeBusNumber;
                Descriptor->u.BusNumber.Length = (ULONG)Window.Length;
                Descriptor->u.BusNumber.MinBusNumber = (ULONG)Window.Start;
                Descriptor->u.BusNumber.MaxBusNumber = (ULONG)(Window.Start + Window.Length - 1);
            }

            if (Count == 2)
            {
                RtlZeroMemory(&Descriptor[1], sizeof(Descriptor[1]));
                Descriptor[1].Type = CmResourceTypeDevicePrivate;
                Descriptor[1].Flags = 1;
            }
            return Count;

        default:
            return 0;
    }
}

static
VOID
NTAPI
UacpiNtDumpCmList(
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_resources *Resources,
    _In_reads_(Count) PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptors,
    _In_ ULONG Count)
{
    uacpi_object_name Name = uacpi_namespace_node_name(Node);
    uacpi_resource *Resource;
    ULONG i;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        if (UacpiNtEmitCm(Resource, NULL) == 0 &&
            Resource->type != UACPI_RESOURCE_TYPE_START_DEPENDENT &&
            Resource->type != UACPI_RESOURCE_TYPE_END_DEPENDENT)
        {
            DPRINT1("uACPI-NT: %.4s dropped _CRS descriptor type %lu\n",
                    Name.text,
                    (ULONG)Resource->type);
        }
    }

    for (i = 0; i < Count; i++)
    {
        switch (Descriptors[i].Type)
        {
            case CmResourceTypeMemory:
                DPRINT("uACPI-NT: %.4s MEM  %010I64X len %08lX\n",
                       Name.text,
                       Descriptors[i].u.Memory.Start.QuadPart,
                       Descriptors[i].u.Memory.Length);
                break;

            case CmResourceTypeMemoryLarge:
                DPRINT("uACPI-NT: %.4s MEML %010I64X len40 %08lX\n",
                       Name.text,
                       Descriptors[i].u.Memory40.Start.QuadPart,
                       Descriptors[i].u.Memory40.Length40);
                break;

            case CmResourceTypePort:
                DPRINT("uACPI-NT: %.4s IO   %010I64X len %08lX\n",
                       Name.text,
                       Descriptors[i].u.Port.Start.QuadPart,
                       Descriptors[i].u.Port.Length);
                break;

            case CmResourceTypeBusNumber:
                DPRINT("uACPI-NT: %.4s BUS  %lu to %lu\n",
                       Name.text,
                       Descriptors[i].u.BusNumber.Start,
                       Descriptors[i].u.BusNumber.Start + Descriptors[i].u.BusNumber.Length - 1);
                break;

            default:
                break;
        }
    }
}

NTSTATUS
NTAPI
UacpiNtCrsToCmList(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PCM_RESOURCE_LIST *ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    PCM_RESOURCE_LIST List;
    ULONG Connections = 0;
    ULONG Count = 0;
    SIZE_T Size;

    *ResourceList = NULL;

    if (uacpi_unlikely_error(uacpi_get_current_resources(Node, &Resources)) || !Resources)
        return STATUS_NOT_FOUND;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        Count += UacpiNtEmitCm(Resource, NULL);
    }

    /* Connection descriptors come from the raw _CRS */
    if (DeviceObject)
    {
        Connections = UacpiNtCrsConnectionCount(Node, DeviceObject);
        Count += Connections;
    }

    if (Count == 0)
    {
        uacpi_free_resources(Resources);
        return STATUS_NOT_FOUND;
    }

    Size = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors) +
           (SIZE_T)Count * sizeof(*Descriptor);
    List = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
    if (!List)
    {
        uacpi_free_resources(Resources);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(List, Size);
    List->Count = 1;
    List->List[0].InterfaceType = Internal;
    List->List[0].BusNumber = 0;
    List->List[0].PartialResourceList.Version = 1;
    List->List[0].PartialResourceList.Revision = 1;
    List->List[0].PartialResourceList.Count = Count;

    Descriptor = List->List[0].PartialResourceList.PartialDescriptors;
    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        Descriptor += UacpiNtEmitCm(Resource, Descriptor);
    }

    if (Connections != 0)
        UacpiNtCrsEmitConnectionsCm(Node, DeviceObject, Descriptor);

    if (UacpiNtResVerbose)
    {
        UacpiNtDumpCmList(Node,
                          Resources,
                          List->List[0].PartialResourceList.PartialDescriptors,
                          Count);
    }

    uacpi_free_resources(Resources);
    *ResourceList = List;
    return STATUS_SUCCESS;
}

/* _PRS gives the arbiter choices committed with _SRS, _CRS gives fixed ones */
NTSTATUS
NTAPI
UacpiNtPrsToRequirements(
    _In_ uacpi_namespace_node *Node,
    _In_ BOOLEAN Possible,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Requirements)
{
    PIO_RESOURCE_REQUIREMENTS_LIST List;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    uacpi_status UacpiStatus;
    ULONG Connections = 0;
    ULONG Count = 0;
    SIZE_T Size;

    *Requirements = NULL;

    if (Possible)
        UacpiStatus = uacpi_get_possible_resources(Node, &Resources);
    else
        UacpiStatus = uacpi_get_current_resources(Node, &Resources);

    if (uacpi_unlikely_error(UacpiStatus) || !Resources)
        return STATUS_NOT_FOUND;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        Count += UacpiNtEmitIo(Resource, NULL);
    }

    /* Connection() only appears in the raw _CRS, never in _PRS */
    if (!Possible && DeviceObject)
    {
        Connections = UacpiNtCrsConnectionCount(Node, DeviceObject);
        Count += Connections;
    }

    if (Count == 0)
    {
        uacpi_free_resources(Resources);
        return STATUS_NOT_FOUND;
    }

    Size = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors) +
           (SIZE_T)Count * sizeof(*Descriptor);
    List = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
    if (!List)
    {
        uacpi_free_resources(Resources);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(List, Size);
    List->ListSize = (ULONG)Size;
    List->InterfaceType = Internal;
    List->BusNumber = 0;
    List->AlternativeLists = 1;
    List->List[0].Version = 1;
    List->List[0].Revision = 1;
    List->List[0].Count = Count;

    Descriptor = List->List[0].Descriptors;
    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        Descriptor += UacpiNtEmitIo(Resource, Descriptor);
    }

    if (Connections != 0)
        UacpiNtCrsEmitConnections(Node, DeviceObject, Descriptor);

    uacpi_free_resources(Resources);
    *Requirements = List;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UacpiNtCrsToRequirements(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Requirements)
{
    return UacpiNtPrsToRequirements(Node, FALSE, DeviceObject, Requirements);
}

/* CM form of a connection, built from the IO form the hub returned */
static
VOID
NTAPI
UacpiNtConnectionIoToCm(
    _In_ PIO_RESOURCE_DESCRIPTOR Io,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm)
{
    RtlZeroMemory(Cm, sizeof(*Cm));

    Cm->Type = CmResourceTypeConnection;
    Cm->ShareDisposition = Io->ShareDisposition;
    Cm->Flags = Io->Flags;
    Cm->u.Connection.Class = Io->u.Connection.Class;
    Cm->u.Connection.Type = Io->u.Connection.Type;
    Cm->u.Connection.IdLowPart = Io->u.Connection.IdLowPart;
    Cm->u.Connection.IdHighPart = Io->u.Connection.IdHighPart;
}

/* Length of the raw resource item at Offset, 0 at the end tag or when malformed */
static
SIZE_T
NTAPI
UacpiNtRawItemLength(
    _In_reads_bytes_(Length) const UCHAR *Buffer,
    _In_ SIZE_T Length,
    _In_ SIZE_T Offset)
{
    const UCHAR *Item = Buffer + Offset;
    SIZE_T ItemLength;

    if (!(Item[0] & UACPINT_RES_LARGE_ITEM))
    {
        if ((Item[0] & UACPINT_RES_SMALL_NAME_MASK) == UACPINT_RES_SMALL_END_TAG)
            return 0;

        ItemLength = (Item[0] & UACPINT_RES_SMALL_LENGTH_MASK) + UACPINT_RES_SMALL_HEADER_SIZE;
    }
    else
    {
        if (Offset + UACPINT_RES_LARGE_HEADER_SIZE > Length)
            return 0;

        ItemLength = ((SIZE_T)Item[1] | ((SIZE_T)Item[2] << 8)) + UACPINT_RES_LARGE_HEADER_SIZE;
    }

    if (Offset + ItemLength > Length)
        return 0;

    return ItemLength;
}

/*
 * The hub wants the raw descriptor bytes, so Connection() entries are taken
 * from the unparsed _CRS buffer. The hub is asked on both the count and the
 * fill pass so the two agree.
 */
static
ULONG
NTAPI
UacpiNtWalkCrsConnections(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR IoDescriptors,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptors)
{
    IO_RESOURCE_DESCRIPTOR Translated;
    uacpi_object *Object = NULL;
    uacpi_data_view View;
    const UCHAR *Buffer;
    SIZE_T ItemLength;
    SIZE_T Offset;
    ULONG Count = 0;

    if (uacpi_unlikely_error(uacpi_eval_typed(Node, "_CRS", NULL, UACPI_OBJECT_BUFFER_BIT, &Object)) ||
        !Object)
    {
        return 0;
    }

    if (uacpi_unlikely_error(uacpi_object_get_buffer(Object, &View)) || !View.bytes)
    {
        uacpi_object_unref(Object);
        return 0;
    }

    Buffer = View.bytes;
    for (Offset = 0; Offset < View.length; Offset += ItemLength)
    {
        ItemLength = UacpiNtRawItemLength(Buffer, View.length, Offset);
        if (ItemLength == 0)
            break;

        if (Buffer[Offset] != GPIO_INTERRUPT_IO_DESCRIPTOR &&
            Buffer[Offset] != FUNCTION_CONFIG_DESCRIPTOR &&
            Buffer[Offset] != SERIAL_BUS_DESCRIPTOR)
        {
            continue;
        }

        RtlZeroMemory(&Translated, sizeof(Translated));
        if (!NT_SUCCESS(UacpiNtTranslateConnectionDescriptor(DeviceObject,
                                                             (PVOID)(Buffer + Offset),
                                                             (ULONG)ItemLength,
                                                             &Translated)))
        {
            continue;
        }

        if (IoDescriptors)
            IoDescriptors[Count] = Translated;

        if (CmDescriptors)
            UacpiNtConnectionIoToCm(&Translated, &CmDescriptors[Count]);

        Count++;
    }

    uacpi_object_unref(Object);
    return Count;
}

ULONG
NTAPI
UacpiNtCrsConnectionCount(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject)
{
    return UacpiNtWalkCrsConnections(Node, DeviceObject, NULL, NULL);
}

ULONG
NTAPI
UacpiNtCrsEmitConnections(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR Descriptors)
{
    return UacpiNtWalkCrsConnections(Node, DeviceObject, Descriptors, NULL);
}

ULONG
NTAPI
UacpiNtCrsEmitConnectionsCm(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptors)
{
    return UacpiNtWalkCrsConnections(Node, DeviceObject, NULL, Descriptors);
}
