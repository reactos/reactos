/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/devhere.c
 * PURPOSE:         Device Presence Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciDevicePresentInterface =
{
    &GUID_PCI_DEVICE_PRESENT_INTERFACE,
    (USHORT)FIELD_OFFSET(PCI_DEVICE_PRESENT_INTERFACE, IsDevicePresentEx),
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,
    PCI_DEVICE_PRESENT_INTERFACE_VERSION,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_DevicePresent,
    devpresent_Constructor,
    devpresent_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
devpresent_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI devpresent_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

static
VOID
NTAPI
devpresent_Reference(
    _In_ PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = Context;

    InterlockedIncrement(&PdoExtension->BusInterfaceReferenceCount);
}

static
VOID
NTAPI
devpresent_Dereference(
    _In_ PVOID Context)
{
    PPCI_PDO_EXTENSION PdoExtension = Context;

    InterlockedDecrement(&PdoExtension->BusInterfaceReferenceCount);
}

/**
 * @brief Compares one child against the fields selected by the search flags.
 */
static
BOOLEAN
NTAPI
devpresent_IsMatch(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    ULONG Flags = Parameters->Flags;

    /* The PDO stays on the list after its function leaves the slot */
    if (PdoExtension->ReportedMissing)
        return FALSE;

    if ((Flags & PCI_USE_VENDEV_IDS) &&
        ((PdoExtension->VendorId != Parameters->VendorID) ||
         (PdoExtension->DeviceId != Parameters->DeviceID)))
    {
        return FALSE;
    }

    if ((Flags & PCI_USE_SUBSYSTEM_IDS) &&
        ((PdoExtension->SubsystemVendorId != Parameters->SubVendorID) ||
         (PdoExtension->SubsystemId != Parameters->SubSystemID)))
    {
        return FALSE;
    }

    if ((Flags & PCI_USE_REVISION) &&
        (PdoExtension->RevisionId != Parameters->RevisionID))
    {
        return FALSE;
    }

    if ((Flags & PCI_USE_CLASS_SUBCLASS) &&
        ((PdoExtension->BaseClass != Parameters->BaseClass) ||
         (PdoExtension->SubClass != Parameters->SubClass)))
    {
        return FALSE;
    }

    if ((Flags & PCI_USE_PROGIF) &&
        (PdoExtension->ProgIf != Parameters->ProgIf))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Searches the children of one bus.
 * @param Slot  When not NULL, only functions of the device in this slot are checked.
 */
static
BOOLEAN
NTAPI
devpresent_SearchBus(
    _In_ PPCI_FDO_EXTENSION FdoExtension,
    _In_opt_ PPCI_SLOT_NUMBER Slot,
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    PPCI_PDO_EXTENSION Child;
    PAGED_CODE();

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&FdoExtension->ChildListLock, Executive, KernelMode, FALSE, NULL);

    for (Child = FdoExtension->ChildPdoList; Child; Child = Child->Next)
    {
        if (Slot && (Child->Slot.u.bits.DeviceNumber != Slot->u.bits.DeviceNumber))
            continue;

        if (devpresent_IsMatch(Child, Parameters))
            break;
    }

    KeSetEvent(&FdoExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    return (Child != NULL);
}

/**
 * @brief Searches every PCI bus in the machine.
 */
static
BOOLEAN
NTAPI
devpresent_SearchAllBuses(
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    PPCI_FDO_EXTENSION FdoExtension;
    BOOLEAN Found = FALSE;
    PAGED_CODE();

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PciGlobalLock, Executive, KernelMode, FALSE, NULL);

    FdoExtension = (PPCI_FDO_EXTENSION)PciFdoExtensionListHead.Next;
    while (FdoExtension && !Found)
    {
        Found = devpresent_SearchBus(FdoExtension, NULL, Parameters);
        FdoExtension = (PPCI_FDO_EXTENSION)FdoExtension->List.Next;
    }

    KeSetEvent(&PciGlobalLock, IO_NO_INCREMENT, FALSE);
    KeLeaveCriticalRegion();

    return Found;
}

static
BOOLEAN
NTAPI
devpresent_IsDevicePresent(
    _In_ USHORT VendorID,
    _In_ USHORT DeviceID,
    _In_ UCHAR RevisionID,
    _In_ USHORT SubVendorID,
    _In_ USHORT SubSystemID,
    _In_ ULONG Flags)
{
    PCI_DEVICE_PRESENCE_PARAMETERS Parameters;
    PAGED_CODE();

    RtlZeroMemory(&Parameters, sizeof(Parameters));
    Parameters.Size = sizeof(Parameters);
    Parameters.VendorID = VendorID;
    Parameters.DeviceID = DeviceID;
    Parameters.RevisionID = RevisionID;
    Parameters.SubVendorID = SubVendorID;
    Parameters.SubSystemID = SubSystemID;

    /* The first version always matches the IDs and only knows these two flags */
    Parameters.Flags = PCI_USE_VENDEV_IDS |
                       (Flags & (PCI_USE_SUBSYSTEM_IDS | PCI_USE_REVISION));

    return devpresent_SearchAllBuses(&Parameters);
}

/**
 * @brief Checks that the search flags name the device by IDs, class, or both.
 * Subsystem and revision refine an ID match, programming interface refines a class match.
 */
static
BOOLEAN
NTAPI
devpresent_FlagsValid(
    _In_ ULONG Flags)
{
    ULONG Needed = 0;

    if (Flags & (PCI_USE_SUBSYSTEM_IDS | PCI_USE_REVISION))
        Needed |= PCI_USE_VENDEV_IDS;
    if (Flags & PCI_USE_PROGIF)
        Needed |= PCI_USE_CLASS_SUBCLASS;

    return ((Flags & (PCI_USE_VENDEV_IDS | PCI_USE_CLASS_SUBCLASS)) != 0) &&
           ((Flags & Needed) == Needed);
}

static
BOOLEAN
NTAPI
devpresent_IsDevicePresentEx(
    _In_ PVOID Context,
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    PPCI_PDO_EXTENSION PdoExtension = Context;
    PPCI_SLOT_NUMBER Slot = NULL;
    PAGED_CODE();

    if (!Parameters ||
        (Parameters->Size < sizeof(*Parameters)) ||
        !devpresent_FlagsValid(Parameters->Flags))
    {
        return FALSE;
    }

    switch (Parameters->Flags & (PCI_USE_LOCAL_BUS | PCI_USE_LOCAL_DEVICE))
    {
        case 0:
            return devpresent_SearchAllBuses(Parameters);

        case PCI_USE_LOCAL_BUS:
            break;

        default:
            if (PdoExtension)
                Slot = &PdoExtension->Slot;
            break;
    }

    /* A local search is relative to the device that was asked */
    if (!PdoExtension)
        return FALSE;

    return devpresent_SearchBus(PdoExtension->ParentFdoExtension, Slot, Parameters);
}

NTSTATUS
NTAPI
devpresent_Constructor(IN PVOID DeviceExtension,
                       IN PVOID Instance,
                       IN PVOID InterfaceData,
                       IN USHORT Version,
                       IN USHORT Size,
                       IN PINTERFACE Interface)
{
    PPCI_DEVICE_PRESENT_INTERFACE PresentInterface = (PPCI_DEVICE_PRESENT_INTERFACE)Interface;
    BOOLEAN HasEx;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);

    /* Callers built before IsDevicePresentEx existed pass the shorter size */
    HasEx = (Size >= sizeof(*PresentInterface));

    PresentInterface->IsDevicePresent = devpresent_IsDevicePresent;
    if (HasEx)
        PresentInterface->IsDevicePresentEx = devpresent_IsDevicePresentEx;

    PresentInterface->InterfaceReference = devpresent_Reference;
    PresentInterface->InterfaceDereference = devpresent_Dereference;
    PresentInterface->Context = DeviceExtension;
    PresentInterface->Version = PCI_DEVICE_PRESENT_INTERFACE_VERSION;
    PresentInterface->Size = (USHORT)(HasEx ? sizeof(*PresentInterface) :
                                      FIELD_OFFSET(PCI_DEVICE_PRESENT_INTERFACE, IsDevicePresentEx));

    return STATUS_SUCCESS;
}

/* EOF */
