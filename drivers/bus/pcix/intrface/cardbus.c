/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/cardbus.c
 * PURPOSE:         CardBus Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE PciCardbusPrivateInterface =
{
    &GUID_PCI_CARDBUS_INTERFACE_PRIVATE,
    sizeof(PCI_CARDBUS_INTERFACE_PRIVATE),
    PCI_CB_INTRF_VERSION,
    PCI_CB_INTRF_VERSION,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_PciCb,
    pcicbintrf_Constructor,
    pcicbintrf_Initializer
};

/* The VGA descriptors of a PCI-to-PCI bridge are reused, both headers keep this register here */
C_ASSERT(FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.BridgeControl) ==
         FIELD_OFFSET(PCI_COMMON_HEADER, u.type2.BridgeControl));

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
Cardbus_SaveCurrentSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_COMMON_HEADER Current;
    PIO_RESOURCE_DESCRIPTOR Limit;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial;
    ULONG Address;

    PdoExtension = Context->PdoExtension;
    Current = Context->Current;
    Limit = &PdoExtension->Resources->Limit[0];
    Partial = &PdoExtension->Resources->Current[0];

    /* Only the socket register BAR is tracked, the four windows are left to the socket driver */
    Partial->Type = Limit->Type;
    if (Limit->Type != CmResourceTypeNull)
    {
        Address = Current->u.type2.SocketRegistersBaseAddress;
        if (Limit->Type == CmResourceTypePort)
        {
            Address &= PCI_ADDRESS_IO_ADDRESS_MASK;
        }
        else
        {
            Address &= PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        }

        Partial->Flags = Limit->Flags;
        Partial->ShareDisposition = Limit->ShareDisposition;
        Partial->u.Generic.Start.QuadPart = Address;
        Partial->u.Generic.Length = Limit->u.Generic.Length;

        /* A BAR the firmware never placed decodes nothing yet */
        if (!Address)
            Partial->Type = CmResourceTypeNull;
    }

    /* Remember the bus numbers so they survive the function losing power */
    PdoExtension->Dependent.type1.PrimaryBus = Current->u.type2.PrimaryBus;
    PdoExtension->Dependent.type1.SecondaryBus = Current->u.type2.SecondaryBus;
    PdoExtension->Dependent.type1.SubordinateBus = Current->u.type2.SubordinateBus;

    /* A bridge that forwards VGA also claims the legacy video ranges */
    if (Current->u.type2.BridgeControl & PCI_ENABLE_BRIDGE_VGA)
    {
        PdoExtension->Dependent.type1.VgaBitSet = TRUE;
        PdoExtension->AdditionalResourceCount = 4;
    }
}

VOID
NTAPI
Cardbus_SaveLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    /* The socket register BAR is 32-bit only, so it never takes a high half from the next field */
    PciCreateIoDescriptorFromBarLimit(&Context->PdoExtension->Resources->Limit[0],
                                      Context->PciData->u.type2.SocketRegistersBaseAddress,
                                      0,
                                      FALSE);
}

VOID
NTAPI
Cardbus_MassageHeaderForLimitsDetermination(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER Current, PciData;

    Current = Context->Current;
    PciData = Context->PciData;

    /* Only the socket register BAR is sized, the windows keep what the firmware wrote */
    PciData->u.type2.SocketRegistersBaseAddress = 0xFFFFFFFF;

    /* Writing the error bits back would clear them, so keep them out of the probe */
    Context->SecondaryStatus = Current->u.type2.SecondaryStatus;
    Current->u.type2.SecondaryStatus = 0;
    PciData->u.type2.SecondaryStatus = 0;
}

VOID
NTAPI
Cardbus_RestoreCurrent(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    /* Put back the secondary status that was held out of the probe */
    Context->Current->u.type2.SecondaryStatus = Context->SecondaryStatus;
}

VOID
NTAPI
Cardbus_GetAdditionalResourceDescriptors(IN PPCI_CONFIGURATOR_CONTEXT Context,
                                         IN PPCI_COMMON_HEADER PciData,
                                         IN PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    /* The VGA enable is the same bit as on a PCI-to-PCI bridge */
    PPBridge_GetAdditionalResourceDescriptors(Context, PciData, IoDescriptor);
}

VOID
NTAPI
Cardbus_ResetDevice(IN PPCI_PDO_EXTENSION PdoExtension,
                    IN PPCI_COMMON_HEADER PciData)
{
    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(PciData);

    /* The socket reset bit belongs to the socket driver, so it goes back as it was read */
}

NTSTATUS
NTAPI
Cardbus_ChangeResourceSettings(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _Inout_ PPCI_COMMON_HEADER PciData)
{
    PPCI_FUNCTION_RESOURCES Resources;
    ULONG Bar, BarMask;

    /* The socket register BAR gets its assigned address, or zero when it was given none */
    Resources = PdoExtension->Resources;
    if ((Resources) && (Resources->Limit[0].Type != CmResourceTypeNull))
    {
        Bar = PciData->u.type2.SocketRegistersBaseAddress;
        if (Bar & PCI_ADDRESS_IO_SPACE)
        {
            BarMask = PCI_ADDRESS_IO_ADDRESS_MASK;
        }
        else
        {
            BarMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        }

        Bar &= ~BarMask;
        if (Resources->Current[0].Type != CmResourceTypeNull)
            Bar |= Resources->Current[0].u.Generic.Start.LowPart & BarMask;

        PciData->u.type2.SocketRegistersBaseAddress = Bar;
    }

    /* The windows stay as read, only what a power loss would wipe is written again */
    PciData->u.type2.PrimaryBus = PdoExtension->Dependent.type1.PrimaryBus;
    PciData->u.type2.SecondaryBus = PdoExtension->Dependent.type1.SecondaryBus;
    PciData->u.type2.SubordinateBus = PdoExtension->Dependent.type1.SubordinateBus;

    if (PdoExtension->Dependent.type1.VgaBitSet)
        PciData->u.type2.BridgeControl |= PCI_ENABLE_BRIDGE_VGA;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
pcicbintrf_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI pcicbintrf_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
pcicbintrf_Constructor(IN PVOID DeviceExtension,
                       IN PVOID Instance,
                       IN PVOID InterfaceData,
                       IN USHORT Version,
                       IN USHORT Size,
                       IN PINTERFACE Interface)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(InterfaceData);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Interface);

    /* Not provided, so the query fails and the socket driver goes without it */
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
