/*++

Module Name:

    pciintrf.h

Abstract:

    Contains interface GUIDs and structures for non-wdm modules that
    interface directly with the PCI driver via the PNP QUERY_INTERFACE
    mechanism.

Author:

    Peter Johnston (peterj) November 1997

Revision History:

--*/

#ifndef _PCIINTRF_
#define _PCIINTRF_


//
// CardBus
//

DEFINE_GUID(GUID_PCI_CARDBUS_INTERFACE_PRIVATE, 0xcca82f31, 0x54d6, 0x11d1, 0x82, 0x24, 0x00, 0xa0, 0xc9, 0x32, 0x43, 0x85);
DEFINE_GUID(GUID_PCI_PME_INTERFACE, 0xaac7e6ac, 0xbb0b, 0x11d2, 0xb4, 0x84, 0x00, 0xc0, 0x4f, 0x72, 0xde, 0x8b);
#define PCI_CB_INTRF_VERSION    1

typedef
NTSTATUS
(*PCARDBUSADD)(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID * DeviceContext
    );

typedef
NTSTATUS
(*PCARDBUSDELETE)(
    IN PVOID DeviceContext
    );

typedef
NTSTATUS
(*PCARDBUSPCIDISPATCH)(
    IN PVOID DeviceContext,
    IN PIRP  Irp
    );


typedef struct _PCI_CARDBUS_INTERFACE_PRIVATE {

    //
    // generic interface header
    //

    USHORT Size;
    USHORT Version;
    PVOID Context;                      // not actually used in this interface
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // Pci data.
    //

    PDRIVER_OBJECT DriverObject;        // returned ptr to PCI driver

    //
    // Pci-Cardbus private interfaces
    //

    PCARDBUSADD AddCardBus;
    PCARDBUSDELETE DeleteCardBus;
    PCARDBUSPCIDISPATCH DispatchPnp;


} PCI_CARDBUS_INTERFACE_PRIVATE, *PPCI_CARDBUS_INTERFACE_PRIVATE;

typedef
VOID
(*PPME_GET_INFORMATION) (
    IN  PDEVICE_OBJECT  Pdo,
    OUT PBOOLEAN        PmeCapable,
    OUT PBOOLEAN        PmeStatus,
    OUT PBOOLEAN        PmeEnable
    );

typedef
VOID
(*PPME_CLEAR_PME_STATUS) (
    IN  PDEVICE_OBJECT  Pdo
    );

typedef
VOID
(*PPME_SET_PME_ENABLE) (
    IN  PDEVICE_OBJECT  Pdo,
    IN  BOOLEAN         PmeEnable
    );

typedef struct _PCI_PME_INTERFACE {

    //
    // generic interface header
    //
    USHORT              Size;
    USHORT          Version;
    PVOID           Context;
    PINTERFACE_REFERENCE    InterfaceReference;
    PINTERFACE_DEREFERENCE  InterfaceDereference;

    //
    // PME Signal interfaces
    //
    PPME_GET_INFORMATION    GetPmeInformation;
    PPME_CLEAR_PME_STATUS   ClearPmeStatus;
    PPME_SET_PME_ENABLE     UpdateEnable;

} PCI_PME_INTERFACE, *PPCI_PME_INTERFACE;

// Some well-known interface versions supported by the PCI Bus Driver

#define PCI_PME_INTRF_STANDARD_VER 1


#endif // _PCIINTRF_
