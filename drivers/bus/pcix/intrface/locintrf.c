/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/locintrf.c
 * PURPOSE:         Location Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciLocationInterface =
{
    &GUID_PNP_LOCATION_INTERFACE,
    sizeof(PNP_LOCATION_INTERFACE),
    PNP_LOCATION_INTERFACE_VERSION,
    PNP_LOCATION_INTERFACE_VERSION,
    PCI_INTERFACE_FDO | PCI_INTERFACE_ROOT | PCI_INTERFACE_PDO,
    0,
    PciInterface_Location,
    locintrf_Constructor,
    locintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
locintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI locintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

static
VOID
NTAPI
locintrf_Reference(
    _In_ PVOID Context)
{
    /* Extensions live as long as their device objects, nothing to count */
    UNREFERENCED_PARAMETER(Context);
}

static
VOID
NTAPI
locintrf_Dereference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/**
 * @brief Returns "PCI(DDFF)" for a device or "PCIROOT(B)" for a root bus as a multi-sz.
 * The caller frees the buffer.
 */
static
NTSTATUS
NTAPI
locintrf_GetLocationString(
    _In_ PVOID Context,
    _Outptr_ PZZWSTR *LocationStrings)
{
    PPCI_PDO_EXTENSION PdoExtension = Context;
    PPCI_FDO_EXTENSION FdoExtension = Context;
    BOOLEAN IsDevice;
    PWCHAR Buffer;
    SIZE_T Size;
    NTSTATUS Status;
    PAGED_CODE();

    *LocationStrings = NULL;

    IsDevice = (PdoExtension->ExtensionType == PciPdoExtensionType);
    if (!IsDevice && !PCI_IS_ROOT_FDO(FdoExtension))
        return STATUS_INVALID_PARAMETER;

    /* Room for the longer form plus the empty string that ends the list */
    Size = sizeof(L"PCIROOT(FF)") + sizeof(UNICODE_NULL);
    Buffer = ExAllocatePoolWithTag(PagedPool, Size, PCI_POOL_TAG);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Buffer, Size);

    if (IsDevice)
    {
        _swprintf(Buffer,
                  L"PCI(%02X%02X)",
                  PdoExtension->Slot.u.bits.DeviceNumber,
                  PdoExtension->Slot.u.bits.FunctionNumber);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* The root starts the path, so PnP stops asking further up */
        _swprintf(Buffer, L"PCIROOT(%X)", FdoExtension->BaseBus);
        Status = STATUS_TRANSLATION_COMPLETE;
    }

    *LocationStrings = Buffer;
    return Status;
}

NTSTATUS
NTAPI
locintrf_Constructor(IN PVOID DeviceExtension,
                     IN PVOID Instance,
                     IN PVOID InterfaceData,
                     IN USHORT Version,
                     IN USHORT Size,
                     IN PINTERFACE Interface)
{
    PPNP_LOCATION_INTERFACE LocationInterface = (PPNP_LOCATION_INTERFACE)Interface;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);

    LocationInterface->Size = sizeof(*LocationInterface);
    LocationInterface->Version = PNP_LOCATION_INTERFACE_VERSION;
    LocationInterface->Context = DeviceExtension;
    LocationInterface->InterfaceReference = locintrf_Reference;
    LocationInterface->InterfaceDereference = locintrf_Dereference;
    LocationInterface->GetLocationString = locintrf_GetLocationString;
    return STATUS_SUCCESS;
}

/* EOF */
