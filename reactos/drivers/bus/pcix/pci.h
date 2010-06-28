/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <initguid.h>
#include <ntifs.h>
#include <ntagp.h>
#include <wdmguid.h>
#include <wchar.h>
#include <acpiioct.h>
#include <drivers/pci/pci.h>
#include <drivers/acpi/acpi.h>
#include "halfuncs.h"
#include "rtlfuncs.h"
#include "vffuncs.h"

//
// Tag used in all pool allocations (Pci Bus)
//
#define PCI_POOL_TAG    'BicP'

//
// PCI Hack Entry Name Lengths
//
#define PCI_HACK_ENTRY_SIZE                 sizeof(L"VVVVdddd")             // 16
#define PCI_HACK_ENTRY_REV_SIZE             sizeof(L"VVVVddddRR")           // 20
#define PCI_HACK_ENTRY_SUBSYS_SIZE          sizeof(L"VVVVddddssssIIII")     // 32
#define PCI_HACK_ENTRY_FULL_SIZE            sizeof(L"VVVVddddssssIIIIRR")   // 36

//
// PCI Hack Entry Information
//
#define PCI_HACK_HAS_REVISION_INFO          0x01
#define PCI_HACK_HAS_SUBSYSTEM_INFO         0x02
typedef struct _PCI_HACK_ENTRY
{
    USHORT VendorID;
    USHORT DeviceID;
    USHORT SubVendorID;
    USHORT SubSystemID;
    ULONGLONG HackFlags;
    USHORT RevisionID;
    UCHAR Flags;
} PCI_HACK_ENTRY, *PPCI_HACK_ENTRY;

//
// IRP Dispatch Routines
//
NTSTATUS
NTAPI
PciDispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

//
// Bus FDO Routines
//
NTSTATUS
NTAPI
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

//
// HAL Callback/Hook Routines
//
VOID
NTAPI
PciHookHal(
    VOID
);

//
// PCI Verifier Routines
//
VOID
NTAPI
PciVerifierInit(
    IN PDRIVER_OBJECT DriverObject
);

//
// Utility Routines
//
BOOLEAN
NTAPI
PciStringToUSHORT(
    IN PWCHAR String,
    OUT PUSHORT Value
);

BOOLEAN
NTAPI
PciIsDatacenter(
    VOID
);

NTSTATUS
NTAPI
PciBuildDefaultExclusionLists(
    VOID
);

BOOLEAN
NTAPI
PciUnicodeStringStrStr(
    IN PUNICODE_STRING InputString,
    IN PCUNICODE_STRING EqualString,
    IN BOOLEAN CaseInSensitive
);

BOOLEAN
NTAPI
PciOpenKey(
    IN PWCHAR KeyName,
    IN HANDLE RootKey,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE KeyHandle,
    OUT PNTSTATUS KeyStatus
);

NTSTATUS
NTAPI
PciGetRegistryValue(
    IN PWCHAR ValueName,
    IN PWCHAR KeyName,
    IN HANDLE RootHandle,
    IN ULONG Type,
    OUT PVOID *OutputBuffer,
    OUT PULONG OutputLength
);

/* EOF */
