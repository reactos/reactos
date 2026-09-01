/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/arb/arb_comn.c
 * PURPOSE:         Common Arbitration Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCHAR PciArbiterNames[] =
{
    "I/O Port",
    "Memory",
    "Interrupt",
    "Bus Number"
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PciArbiter_Reference(_In_ PVOID Context)
{
    PARBITER_INSTANCE Arbiter = (PARBITER_INSTANCE)Context;

    InterlockedIncrement((PLONG)&Arbiter->ReferenceCount);
}

VOID
NTAPI
PciArbiter_Dereference(_In_ PVOID Context)
{
    PARBITER_INSTANCE Arbiter = (PARBITER_INSTANCE)Context;

    InterlockedDecrement((PLONG)&Arbiter->ReferenceCount);
}

NTSTATUS
NTAPI
PciArbiterConstructor(_In_ PPCI_FDO_EXTENSION FdoExtension,
                      _In_ PCI_SIGNATURE ArbiterType,
                      _Out_ PARBITER_INTERFACE Interface)
{
    PPCI_ARBITER_INSTANCE Arbiter;
    PAGED_CODE();

    if (!FdoExtension->ArbitersInitialized) return STATUS_NOT_SUPPORTED;

    /* Find the instance this bus built for the requested resource type */
    Arbiter = (PVOID)PciFindNextSecondaryExtension(FdoExtension->
                                                   SecondaryExtension.Next,
                                                   ArbiterType);
    if (!Arbiter)
    {
        DPRINT1("PCI - FDO ext 0x%p has no %s arbiter to hand out.\n",
                FdoExtension,
                PciArbiterNames[ArbiterType - PciArb_Io]);
        return STATUS_NOT_SUPPORTED;
    }

    Interface->Size = sizeof(ARBITER_INTERFACE);
    Interface->Version = ARBITER_INTERFACE_VERSION;
    Interface->Context = &Arbiter->CommonInstance;
    Interface->InterfaceReference = PciArbiter_Reference;
    Interface->InterfaceDereference = PciArbiter_Dereference;
    Interface->ArbiterHandler = ArbiterLibHandler;
    Interface->Flags = 0;
    return STATUS_SUCCESS;
}

VOID
NTAPI
PciArbiterDestructor(IN PPCI_ARBITER_INSTANCE Arbiter)
{
    PAGED_CODE();

    ArbiterLibDeleteInstance(&Arbiter->CommonInstance);
}

NTSTATUS
NTAPI
PciInitializeArbiters(IN PPCI_FDO_EXTENSION FdoExtension)
{
    PPCI_INTERFACE CurrentInterface, *Interfaces;
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_ARBITER_INSTANCE ArbiterInterface;
    NTSTATUS Status;
    PCI_SIGNATURE ArbiterType;
    ASSERT_FDO(FdoExtension);

    /* A bus that ends up needing no arbiter at all is not a failure */
    Status = STATUS_SUCCESS;

    /* Loop all the arbiters */
    for (ArbiterType = PciArb_Io; ArbiterType <= PciArb_BusNumber; ArbiterType++)
    {
        /* Check if this is the extension for the Root PCI Bus */
        if (!PCI_IS_ROOT_FDO(FdoExtension))
        {
            /* Get the PDO extension */
            PdoExtension = FdoExtension->PhysicalDeviceObject->DeviceExtension;
            ASSERT_PDO(PdoExtension);

            /* Skip this bus if it does subtractive decode */
            if (PdoExtension->Dependent.type1.SubtractiveDecode)
            {
                DPRINT1("PCI Not creating arbiters for subtractive bus %u\n",
                        PdoExtension->Dependent.type1.SubtractiveDecode);
                continue;
            }
        }

        /* Query all the registered arbiter interfaces */
        Interfaces = PciInterfaces;
        while (*Interfaces)
        {
            /* Find the one that matches the arbiter currently being setup */
            CurrentInterface = *Interfaces;
            if (CurrentInterface->Signature == ArbiterType) break;
            Interfaces++;
        }

        /* Check if the required arbiter was not found in the list */
        if (!*Interfaces)
        {
            /* Skip this arbiter and try the next one */
            DPRINT1("PCI - FDO ext 0x%p no %s arbiter.\n",
                    FdoExtension,
                    PciArbiterNames[ArbiterType - PciArb_Io]);
            continue;
        }

        /* An arbiter was found, allocate an instance for it */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ArbiterInterface = ExAllocatePoolWithTag(PagedPool,
                                                 sizeof(PCI_ARBITER_INSTANCE),
                                                 PCI_POOL_TAG);
        if (!ArbiterInterface) break;

        /* Setup the instance */
        ArbiterInterface->BusFdoExtension = FdoExtension;
        ArbiterInterface->Interface = CurrentInterface;
        _swprintf(ArbiterInterface->InstanceName,
                  L"PCI %S (b=%02x)",
                  PciArbiterNames[ArbiterType - PciArb_Io],
                  FdoExtension->BaseBus);

        /* Call the interface initializer for it */
        Status = CurrentInterface->Initializer(ArbiterInterface);
        if (!NT_SUCCESS(Status)) break;

        /* Link it with this FDO */
        PcipLinkSecondaryExtension(&FdoExtension->SecondaryExtension,
                                   &FdoExtension->SecondaryExtLock,
                                   &ArbiterInterface->Header,
                                   ArbiterType,
                                   PciArbiterDestructor);

        /* This arbiter is now initialized, move to the next one */
        DPRINT1("PCI - FDO ext 0x%p %S arbiter initialized (context 0x%p).\n",
                FdoExtension,
                L"ARBITER HEADER MISSING", //ArbiterInterface->CommonInstance.Name,
                ArbiterInterface);
        Status = STATUS_SUCCESS;
    }

    /* Return to caller */
    return Status;
}

/**
 * @brief
 * Builds a resource list of the ranges a PCI-to-PCI bridge forwards to its secondary bus.
 *
 * @param[in] BridgeExtension
 * The PDO extension of a positive decode bridge.
 *
 * @return
 * The open windows and, with the VGA enable set, the legacy video ranges. NULL if out of memory.
 */
static
PCM_RESOURCE_LIST
NTAPI
PciCollectForwardedRanges(
    _In_ PPCI_PDO_EXTENSION BridgeExtension)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR First, Descriptor, Window;
    PCM_RESOURCE_LIST RangeList;
    SIZE_T Size;
    ULONG Index;

    PAGED_CODE();

    /* Room for the I/O, memory and prefetchable windows plus three VGA ranges */
    Size = sizeof(*RangeList) + (5 * sizeof(*Descriptor));
    RangeList = ExAllocatePoolWithTag(PagedPool, Size, PCI_POOL_TAG);
    if (!RangeList)
        return NULL;

    RtlZeroMemory(RangeList, Size);
    RangeList->Count = 1;
    RangeList->List[0].InterfaceType = PCIBus;
    RangeList->List[0].BusNumber = BridgeExtension->Dependent.type1.SecondaryBus;
    RangeList->List[0].PartialResourceList.Version = 1;
    RangeList->List[0].PartialResourceList.Revision = 1;
    First = RangeList->List[0].PartialResourceList.PartialDescriptors;
    Descriptor = First;

    /* The windows follow the BARs, a closed one was saved as a null descriptor */
    if (BridgeExtension->Resources)
    {
        for (Index = PCI_TYPE1_ADDRESSES; Index < (PCI_TYPE1_ADDRESSES + 3); Index++)
        {
            Window = &BridgeExtension->Resources->Current[Index];
            if ((Window->Type == CmResourceTypeNull) || !Window->u.Generic.Length)
                continue;

            *Descriptor = *Window;
            Descriptor++;
        }
    }

    if (BridgeExtension->Dependent.type1.VgaBitSet)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start.QuadPart = 0xA0000;
        Descriptor->u.Memory.Length = 0x20000;
        Descriptor++;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->Flags = CM_RESOURCE_PORT_POSITIVE_DECODE | CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Start.QuadPart = 0x3B0;
        Descriptor->u.Port.Length = 0xC;
        Descriptor++;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->Flags = CM_RESOURCE_PORT_POSITIVE_DECODE | CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Start.QuadPart = 0x3C0;
        Descriptor->u.Port.Length = 0x20;
        Descriptor++;
    }

    RangeList->List[0].PartialResourceList.Count = (ULONG)(Descriptor - First);
    return RangeList;
}

/**
 * @brief
 * Tells whether a resource list holds any range of one resource type.
 *
 * @param[in] RangeList
 * A resource list with a single full descriptor.
 *
 * @param[in] Type
 * The CmResourceType to look for.
 *
 * @return
 * TRUE if at least one descriptor of that type is present. A large memory range counts as memory.
 */
static
BOOLEAN
NTAPI
PciHasRangeOfType(
    _In_ PCM_RESOURCE_LIST RangeList,
    _In_ UCHAR Type)
{
    PCM_PARTIAL_RESOURCE_LIST PartialList;
    UCHAR RangeType;
    ULONG Index;

    PartialList = &RangeList->List[0].PartialResourceList;
    for (Index = 0; Index < PartialList->Count; Index++)
    {
        RangeType = PartialList->PartialDescriptors[Index].Type;
        if ((RangeType == Type) ||
            ((RangeType == CmResourceTypeMemoryLarge) && (Type == CmResourceTypeMemory)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
NTAPI
PciInitializeArbiterRanges(IN PPCI_FDO_EXTENSION DeviceExtension,
                           IN PCM_RESOURCE_LIST Resources)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_ARBITER_INSTANCE Instance;
    PCM_RESOURCE_LIST Ranges;
    PCI_SIGNATURE ArbiterType;
    NTSTATUS Status;

    /* Arbiters should not already be initialized */
    if (DeviceExtension->ArbitersInitialized)
    {
        /* Duplicated start request, fail initialization */
        DPRINT1("PCI Warning hot start FDOx %p, resource ranges not checked.\n", DeviceExtension);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* A root bus decodes the ranges it was started with */
    Ranges = Resources;

    /* Check for non-root FDO */
    if (!PCI_IS_ROOT_FDO(DeviceExtension))
    {
        /* Grab the PDO */
        PdoExtension = (PPCI_PDO_EXTENSION)DeviceExtension->PhysicalDeviceObject->DeviceExtension;
        ASSERT_PDO(PdoExtension);

        /* Check if this is a subtractive bus */
        if (PdoExtension->Dependent.type1.SubtractiveDecode)
        {
            /* There is nothing to do regarding arbitration of resources */
            DPRINT1("PCI Skipping arbiter initialization for subtractive bridge FDOX %p\n", DeviceExtension);
            return STATUS_SUCCESS;
        }

        /* The start resources hold the bridge's own BARs, the bus only gets what it forwards */
        Ranges = PciCollectForwardedRanges(PdoExtension);
        if (!Ranges)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Loop the arbiters that hand out the ranges a bus decodes */
    for (ArbiterType = PciArb_Io; ArbiterType <= PciArb_Memory; ArbiterType++)
    {
        /* Find an arbiter of this type */
        Instance = (PVOID)PciFindNextSecondaryExtension(DeviceExtension->
                                                        SecondaryExtension.Next,
                                                        ArbiterType);
        if (Instance)
        {
            /* The decoded ranges bound what the arbiter may hand out */
            Status = Instance->CommonInstance.StartArbiter(&Instance->CommonInstance,
                                                           Ranges);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PCI - FDO ext 0x%p %s arbiter failed to start: %X\n",
                        DeviceExtension,
                        PciArbiterNames[ArbiterType - PciArb_Io],
                        Status);
                goto Exit;
            }

            /* An empty seed leaves everything open, but a bridge without the range forwards none */
            if (!PCI_IS_ROOT_FDO(DeviceExtension) &&
                !PciHasRangeOfType(Ranges, (UCHAR)Instance->CommonInstance.ResourceType))
            {
                ArbiterLibReserveRange(&Instance->CommonInstance,
                                       0,
                                       ARBITER_MAXIMUM_ADDRESS,
                                       NULL,
                                       FALSE);
            }
        }
        else
        {
            /* The arbiter was not found, this is an error! */
            DPRINT1("PCI - FDO ext 0x%p %s arbiter (REQUIRED) is missing.\n",
                    DeviceExtension,
                    PciArbiterNames[ArbiterType - PciArb_Io]);
        }
    }

    /* Arbiters are now initialized */
    DeviceExtension->ArbitersInitialized = TRUE;
    Status = STATUS_SUCCESS;

Exit:
    if (Ranges != Resources)
        ExFreePoolWithTag(Ranges, PCI_POOL_TAG);

    return Status;
}

/* EOF */
