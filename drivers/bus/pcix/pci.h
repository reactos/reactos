/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _PCIX_PCH_
#define _PCIX_PCH_

#include <ntifs.h>
#include <wdmguid.h>
#include <wchar.h>
#include <acpiioct.h>
#include <drivers/pci/pci.h>
#include <drivers/acpi/acpi.h>
#include <ndk/halfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/vffuncs.h>
#include <arbiter.h>
#include <cmreslist.h>

//
// Tag used in all pool allocations (Pci Bus)
//
#define PCI_POOL_TAG    'BicP'

//
// Checks if the specified FDO is the FDO for the Root PCI Bus
//
#define PCI_IS_ROOT_FDO(x)                  ((x)->BusRootFdoExtension == x)

//
// Assertions to make sure we are dealing with the right kind of extension
//
#define ASSERT_FDO(x)                       ASSERT((x)->ExtensionType == PciFdoExtensionType);
#define ASSERT_PDO(x)                       ASSERT((x)->ExtensionType == PciPdoExtensionType);

//
// PCI Hack Entry Name Lengths
//
#define PCI_HACK_ENTRY_SIZE                 sizeof(L"VVVVdddd") - sizeof(UNICODE_NULL)
#define PCI_HACK_ENTRY_REV_SIZE             sizeof(L"VVVVddddRR") - sizeof(UNICODE_NULL)
#define PCI_HACK_ENTRY_SUBSYS_SIZE          sizeof(L"VVVVddddssssIIII") - sizeof(UNICODE_NULL)
#define PCI_HACK_ENTRY_FULL_SIZE            sizeof(L"VVVVddddssssIIIIRR") - sizeof(UNICODE_NULL)

//
// PCI Hack Entry Flags
//
#define PCI_HACK_HAS_REVISION_INFO          0x01
#define PCI_HACK_HAS_SUBSYSTEM_INFO         0x02

//
// PCI Interface Flags
//
#define PCI_INTERFACE_PDO                   0x01
#define PCI_INTERFACE_FDO                   0x02
#define PCI_INTERFACE_ROOT                  0x04

//
// PCI Skip Function Flags
//
#define PCI_SKIP_DEVICE_ENUMERATION         0x01
#define PCI_SKIP_RESOURCE_ENUMERATION       0x02

//
// PCI Apply Hack Flags
//
#define PCI_HACK_FIXUP_BEFORE_CONFIGURATION 0x00
#define PCI_HACK_FIXUP_AFTER_CONFIGURATION  0x01
#define PCI_HACK_FIXUP_BEFORE_UPDATE        0x03

//
// PCI Debugging Device Support
//
#define MAX_DEBUGGING_DEVICES_SUPPORTED     0x04

//
// PCI Driver Verifier Failures
//
#define PCI_VERIFIER_CODES                  0x04

//
// PCI ID Buffer ANSI Strings
//
#define MAX_ANSI_STRINGS                    0x08

//
// Device Extension, Interface, Translator and Arbiter Signatures
//
typedef enum _PCI_SIGNATURE
{
    PciPdoExtensionType = 'icP0',
    PciFdoExtensionType = 'icP1',
    PciArb_Io = 'icP2',
    PciArb_Memory = 'icP3',
    PciArb_Interrupt = 'icP4',
    PciArb_BusNumber = 'icP5',
    PciTrans_Interrupt = 'icP6',
    PciInterface_BusHandler = 'icP7',
    PciInterface_IntRouteHandler = 'icP8',
    PciInterface_PciCb = 'icP9',
    PciInterface_LegacyDeviceDetection = 'icP:',
    PciInterface_PmeHandler = 'icP;',
    PciInterface_DevicePresent = 'icP<',
    PciInterface_NativeIde = 'icP=',
    PciInterface_AgpTarget = 'icP>',
    PciInterface_Location  = 'icP?'
} PCI_SIGNATURE, *PPCI_SIGNATURE;

//
// Driver-handled PCI Device Types
//
typedef enum _PCI_DEVICE_TYPES
{
    PciTypeInvalid,
    PciTypeHostBridge,
    PciTypePciBridge,
    PciTypeCardbusBridge,
    PciTypeDevice
} PCI_DEVICE_TYPES;

//
// Device Extension Logic States
//
typedef enum _PCI_STATE
{
    PciNotStarted,
    PciStarted,
    PciDeleted,
    PciStopped,
    PciSurpriseRemoved,
    PciSynchronizedOperation,
    PciMaxObjectState
} PCI_STATE;

//
// IRP Dispatch Logic Style
//
typedef enum _PCI_DISPATCH_STYLE
{
    IRP_COMPLETE,
    IRP_DOWNWARD,
    IRP_UPWARD,
    IRP_DISPATCH,
} PCI_DISPATCH_STYLE;

//
// PCI Hack Entry Information
//
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
// Power State Information for Device Extension
//
typedef struct _PCI_POWER_STATE
{
    SYSTEM_POWER_STATE CurrentSystemState;
    DEVICE_POWER_STATE CurrentDeviceState;
    SYSTEM_POWER_STATE SystemWakeLevel;
    DEVICE_POWER_STATE DeviceWakeLevel;
    DEVICE_POWER_STATE SystemStateMapping[7];
    PIRP WaitWakeIrp;
    PVOID SavedCancelRoutine;
    LONG Paging;
    LONG Hibernate;
    LONG CrashDump;
} PCI_POWER_STATE, *PPCI_POWER_STATE;

//
// Internal PCI Lock Structure
//
typedef struct _PCI_LOCK
{
    LONG Atom;
    BOOLEAN OldIrql;
} PCI_LOCK, *PPCI_LOCK;

//
// Device Extension for a Bus FDO
//
typedef struct _PCI_FDO_EXTENSION
{
    SINGLE_LIST_ENTRY List;
    ULONG ExtensionType;
    struct _PCI_MJ_DISPATCH_TABLE *IrpDispatchTable;
    BOOLEAN DeviceState;
    BOOLEAN TentativeNextState;
    KEVENT SecondaryExtLock;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT FunctionalDeviceObject;
    PDEVICE_OBJECT AttachedDeviceObject;
    KEVENT ChildListLock;
    struct _PCI_PDO_EXTENSION *ChildPdoList;
    struct _PCI_FDO_EXTENSION *BusRootFdoExtension;
    struct _PCI_FDO_EXTENSION *ParentFdoExtension;
    struct _PCI_PDO_EXTENSION *ChildBridgePdoList;
    PPCI_BUS_INTERFACE_STANDARD PciBusInterface;
    BOOLEAN MaxSubordinateBus;
    BUS_HANDLER *BusHandler;
    BOOLEAN BaseBus;
    BOOLEAN Fake;
    BOOLEAN ChildDelete;
    BOOLEAN Scanned;
    BOOLEAN ArbitersInitialized;
    BOOLEAN BrokenVideoHackApplied;
    BOOLEAN Hibernated;
    PCI_POWER_STATE PowerState;
    SINGLE_LIST_ENTRY SecondaryExtension;
    LONG ChildWaitWakeCount;
    PPCI_COMMON_CONFIG PreservedConfig;
    PCI_LOCK Lock;
    struct
    {
        BOOLEAN Acquired;
        BOOLEAN CacheLineSize;
        BOOLEAN LatencyTimer;
        BOOLEAN EnablePERR;
        BOOLEAN EnableSERR;
    } HotPlugParameters;
    LONG BusHackFlags;
} PCI_FDO_EXTENSION, *PPCI_FDO_EXTENSION;

typedef struct _PCI_FUNCTION_RESOURCES
{
    IO_RESOURCE_DESCRIPTOR Limit[7];
    CM_PARTIAL_RESOURCE_DESCRIPTOR Current[7];
} PCI_FUNCTION_RESOURCES, *PPCI_FUNCTION_RESOURCES;

typedef union _PCI_HEADER_TYPE_DEPENDENT
{
    struct
    {
        UCHAR Spare[4];
    } type0;
    struct
    {
        UCHAR PrimaryBus;
        UCHAR SecondaryBus;
        UCHAR SubordinateBus;
        UCHAR SubtractiveDecode:1;
        UCHAR IsaBitSet:1;
        UCHAR VgaBitSet:1;
        UCHAR WeChangedBusNumbers:1;
        UCHAR IsaBitRequired:1;
    } type1;
    struct
    {
        UCHAR Spare[4];
    } type2;
} PCI_HEADER_TYPE_DEPENDENT, *PPCI_HEADER_TYPE_DEPENDENT;

typedef struct _PCI_PDO_EXTENSION
{
    PVOID Next;
    ULONG ExtensionType;
    struct _PCI_MJ_DISPATCH_TABLE *IrpDispatchTable;
    BOOLEAN DeviceState;
    BOOLEAN TentativeNextState;

    KEVENT SecondaryExtLock;
    PCI_SLOT_NUMBER Slot;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PPCI_FDO_EXTENSION ParentFdoExtension;
    SINGLE_LIST_ENTRY SecondaryExtension;
    LONG BusInterfaceReferenceCount;
    LONG AgpInterfaceReferenceCount;
    USHORT VendorId;
    USHORT DeviceId;
    USHORT SubsystemVendorId;
    USHORT SubsystemId;
    BOOLEAN RevisionId;
    BOOLEAN ProgIf;
    BOOLEAN SubClass;
    BOOLEAN BaseClass;
    BOOLEAN AdditionalResourceCount;
    BOOLEAN AdjustedInterruptLine;
    BOOLEAN InterruptPin;
    BOOLEAN RawInterruptLine;
    BOOLEAN CapabilitiesPtr;
    BOOLEAN SavedLatencyTimer;
    BOOLEAN SavedCacheLineSize;
    BOOLEAN HeaderType;
    BOOLEAN NotPresent;
    BOOLEAN ReportedMissing;
    BOOLEAN ExpectedWritebackFailure;
    BOOLEAN NoTouchPmeEnable;
    BOOLEAN LegacyDriver;
    BOOLEAN UpdateHardware;
    BOOLEAN MovedDevice;
    BOOLEAN DisablePowerDown;
    BOOLEAN NeedsHotPlugConfiguration;
    BOOLEAN IDEInNativeMode;
    BOOLEAN BIOSAllowsIDESwitchToNativeMode;
    BOOLEAN IoSpaceUnderNativeIdeControl;
    BOOLEAN OnDebugPath;
    BOOLEAN IoSpaceNotRequired;
    PCI_POWER_STATE PowerState;
    PCI_HEADER_TYPE_DEPENDENT Dependent;
    ULONGLONG HackFlags;
    PCI_FUNCTION_RESOURCES *Resources;
    PCI_FDO_EXTENSION *BridgeFdoExtension;
    struct _PCI_PDO_EXTENSION *NextBridge;
    struct _PCI_PDO_EXTENSION *NextHashEntry;
    PCI_LOCK Lock;
    PCI_PMC PowerCapabilities;
    BOOLEAN TargetAgpCapabilityId;
    USHORT CommandEnables;
    USHORT InitialCommand;
} PCI_PDO_EXTENSION, *PPCI_PDO_EXTENSION;

//
// IRP Dispatch Function Type
//
typedef NTSTATUS (NTAPI *PCI_DISPATCH_FUNCTION)(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PVOID DeviceExtension
);

//
// IRP Dispatch Minor Table
//
typedef struct _PCI_MN_DISPATCH_TABLE
{
    PCI_DISPATCH_STYLE DispatchStyle;
    PCI_DISPATCH_FUNCTION DispatchFunction;
} PCI_MN_DISPATCH_TABLE, *PPCI_MN_DISPATCH_TABLE;

//
// IRP Dispatch Major Table
//
typedef struct _PCI_MJ_DISPATCH_TABLE
{
    ULONG PnpIrpMaximumMinorFunction;
    PPCI_MN_DISPATCH_TABLE PnpIrpDispatchTable;
    ULONG PowerIrpMaximumMinorFunction;
    PPCI_MN_DISPATCH_TABLE PowerIrpDispatchTable;
    PCI_DISPATCH_STYLE SystemControlIrpDispatchStyle;
    PCI_DISPATCH_FUNCTION SystemControlIrpDispatchFunction;
    PCI_DISPATCH_STYLE OtherIrpDispatchStyle;
    PCI_DISPATCH_FUNCTION OtherIrpDispatchFunction;
} PCI_MJ_DISPATCH_TABLE, *PPCI_MJ_DISPATCH_TABLE;

//
// Generic PCI Interface Constructor and Initializer
//
struct _PCI_INTERFACE;
typedef NTSTATUS (NTAPI *PCI_INTERFACE_CONSTRUCTOR)(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

typedef NTSTATUS (NTAPI *PCI_INTERFACE_INITIALIZER)(
    IN PVOID Instance
);

//
// Generic PCI Interface (Interface, Translator, Arbiter)
//
typedef struct _PCI_INTERFACE
{
    CONST GUID *InterfaceType;
    USHORT MinSize;
    USHORT MinVersion;
    USHORT MaxVersion;
    USHORT Flags;
    LONG ReferenceCount;
    PCI_SIGNATURE Signature;
    PCI_INTERFACE_CONSTRUCTOR Constructor;
    PCI_INTERFACE_INITIALIZER Initializer;
} PCI_INTERFACE, *PPCI_INTERFACE;

//
// Generic Secondary Extension Instance Header (Interface, Translator, Arbiter)
//
typedef struct PCI_SECONDARY_EXTENSION
{
    SINGLE_LIST_ENTRY List;
    PCI_SIGNATURE ExtensionType;
    PVOID Destructor;
} PCI_SECONDARY_EXTENSION, *PPCI_SECONDARY_EXTENSION;

//
// PCI Arbiter Instance
//
typedef struct PCI_ARBITER_INSTANCE
{
    PCI_SECONDARY_EXTENSION Header;
    PPCI_INTERFACE Interface;
    PPCI_FDO_EXTENSION BusFdoExtension;
    WCHAR InstanceName[24];
    ARBITER_INSTANCE CommonInstance;
} PCI_ARBITER_INSTANCE, *PPCI_ARBITER_INSTANCE;

//
// PCI Verifier Data
//
typedef struct _PCI_VERIFIER_DATA
{
    ULONG FailureCode;
    VF_FAILURE_CLASS FailureClass;
    ULONG AssertionControl;
    PCHAR DebuggerMessageText;
} PCI_VERIFIER_DATA, *PPCI_VERIFIER_DATA;

//
// PCI ID Buffer Descriptor
//
typedef struct _PCI_ID_BUFFER
{
    ULONG Count;
    ANSI_STRING Strings[MAX_ANSI_STRINGS];
    ULONG StringSize[MAX_ANSI_STRINGS];
    ULONG TotalLength;
    PCHAR CharBuffer;
    CHAR BufferData[256];
} PCI_ID_BUFFER, *PPCI_ID_BUFFER;

//
// PCI Configuration Callbacks
//
struct _PCI_CONFIGURATOR_CONTEXT;

typedef VOID (NTAPI *PCI_CONFIGURATOR_INITIALIZE)(
    IN struct _PCI_CONFIGURATOR_CONTEXT* Context
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_RESTORE_CURRENT)(
    IN struct _PCI_CONFIGURATOR_CONTEXT* Context
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_SAVE_LIMITS)(
    IN struct _PCI_CONFIGURATOR_CONTEXT* Context
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_SAVE_CURRENT_SETTINGS)(
    IN struct _PCI_CONFIGURATOR_CONTEXT* Context
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_CHANGE_RESOURCE_SETTINGS)(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_GET_ADDITIONAL_RESOURCE_DESCRIPTORS)(
    IN struct _PCI_CONFIGURATOR_CONTEXT* Context,
    IN PPCI_COMMON_HEADER PciData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

typedef VOID (NTAPI *PCI_CONFIGURATOR_RESET_DEVICE)(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

//
// PCI Configurator
//
typedef struct _PCI_CONFIGURATOR
{
    PCI_CONFIGURATOR_INITIALIZE Initialize;
    PCI_CONFIGURATOR_RESTORE_CURRENT RestoreCurrent;
    PCI_CONFIGURATOR_SAVE_LIMITS SaveLimits;
    PCI_CONFIGURATOR_SAVE_CURRENT_SETTINGS SaveCurrentSettings;
    PCI_CONFIGURATOR_CHANGE_RESOURCE_SETTINGS ChangeResourceSettings;
    PCI_CONFIGURATOR_GET_ADDITIONAL_RESOURCE_DESCRIPTORS GetAdditionalResourceDescriptors;
    PCI_CONFIGURATOR_RESET_DEVICE ResetDevice;
} PCI_CONFIGURATOR, *PPCI_CONFIGURATOR;

//
// PCI Configurator Context
//
typedef struct _PCI_CONFIGURATOR_CONTEXT
{
    PPCI_PDO_EXTENSION PdoExtension;
    PPCI_COMMON_HEADER Current;
    PPCI_COMMON_HEADER PciData;
    PPCI_CONFIGURATOR Configurator;
    USHORT SecondaryStatus;
    USHORT Status;
    USHORT Command;
} PCI_CONFIGURATOR_CONTEXT, *PPCI_CONFIGURATOR_CONTEXT;

//
// PCI IPI Function
//
typedef VOID (NTAPI *PCI_IPI_FUNCTION)(
    IN PVOID Reserved,
    IN PVOID Context
);

//
// PCI IPI Context
//
typedef struct _PCI_IPI_CONTEXT
{
    LONG RunCount;
    ULONG Barrier;
    PVOID DeviceExtension;
    PCI_IPI_FUNCTION Function;
    PVOID Context;
} PCI_IPI_CONTEXT, *PPCI_IPI_CONTEXT;

//
// PCI Legacy Device Location Cache
//
typedef struct _PCI_LEGACY_DEVICE
{
    struct _PCI_LEGACY_DEVICE *Next;
    PDEVICE_OBJECT DeviceObject;
    ULONG BusNumber;
    ULONG SlotNumber;
    UCHAR InterruptLine;
    UCHAR InterruptPin;
    UCHAR BaseClass;
    UCHAR SubClass;
    PDEVICE_OBJECT PhysicalDeviceObject;
    ROUTING_TOKEN RoutingToken;
    PPCI_PDO_EXTENSION PdoExtension;
} PCI_LEGACY_DEVICE, *PPCI_LEGACY_DEVICE;

//
// IRP Dispatch Routines
//

DRIVER_DISPATCH PciDispatchIrp;

NTSTATUS
NTAPI
PciDispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
NTAPI
PciIrpNotSupported(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPassIrpFromFdoToPdo(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PIRP Irp
);

NTSTATUS
NTAPI
PciCallDownIrpStack(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PIRP Irp
);

NTSTATUS
NTAPI
PciIrpInvalidDeviceRequest(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

//
// Power Routines
//
NTSTATUS
NTAPI
PciFdoWaitWake(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoSetPowerState(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryPower(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciSetPowerManagedDevicePowerState(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN DEVICE_POWER_STATE DeviceState,
    IN BOOLEAN IrpSet
);

//
// Bus FDO Routines
//

DRIVER_ADD_DEVICE PciAddDevice;

NTSTATUS
NTAPI
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
NTAPI
PciFdoIrpStartDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpCancelRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpCancelStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryDeviceRelations(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryInterface(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryCapabilities(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpDeviceUsageNotification(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpSurpriseRemoval(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciFdoIrpQueryLegacyBusInformation(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension
);

//
// Device PDO Routines
//
NTSTATUS
NTAPI
PciPdoCreate(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCI_SLOT_NUMBER Slot,
    OUT PDEVICE_OBJECT *PdoDeviceObject
);

NTSTATUS
NTAPI
PciPdoWaitWake(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoSetPowerState(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryPower(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpStartDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpCancelRemoveDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpCancelStopDevice(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceRelations(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryInterface(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryCapabilities(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryResources(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryResourceRequirements(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceText(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpReadConfig(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpWriteConfig(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryId(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceState(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryBusInformation(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpDeviceUsageNotification(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpSurpriseRemoval(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciPdoIrpQueryLegacyBusInformation(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_PDO_EXTENSION DeviceExtension
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

PPCI_VERIFIER_DATA
NTAPI
PciVerifierRetrieveFailureData(
    IN ULONG FailureCode
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

PPCI_FDO_EXTENSION
NTAPI
PciFindParentPciFdoExtension(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEVENT Lock
);

VOID
NTAPI
PciInsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PKEVENT Lock
);

NTSTATUS
NTAPI
PciGetDeviceProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
    OUT PVOID *OutputBuffer
);

NTSTATUS
NTAPI
PciSendIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
);

VOID
NTAPI
PcipLinkSecondaryExtension(
    IN PSINGLE_LIST_ENTRY List,
    IN PVOID Lock,
    IN PPCI_SECONDARY_EXTENSION SecondaryExtension,
    IN PCI_SIGNATURE ExtensionType,
    IN PVOID Destructor
);

PPCI_SECONDARY_EXTENSION
NTAPI
PciFindNextSecondaryExtension(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PCI_SIGNATURE ExtensionType
);

ULONGLONG
NTAPI
PciGetHackFlags(
    IN USHORT VendorId,
    IN USHORT DeviceId,
    IN USHORT SubVendorId,
    IN USHORT SubSystemId,
    IN UCHAR RevisionId
);

PPCI_PDO_EXTENSION
NTAPI
PciFindPdoByFunction(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN ULONG FunctionNumber,
    IN PPCI_COMMON_HEADER PciData
);

BOOLEAN
NTAPI
PciIsCriticalDeviceClass(
    IN UCHAR BaseClass,
    IN UCHAR SubClass
);

BOOLEAN
NTAPI
PciIsDeviceOnDebugPath(
    IN PPCI_PDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciGetBiosConfig(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    OUT PPCI_COMMON_HEADER PciData
);

NTSTATUS
NTAPI
PciSaveBiosConfig(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    OUT PPCI_COMMON_HEADER PciData
);

UCHAR
NTAPI
PciReadDeviceCapability(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN UCHAR Offset,
    IN ULONG CapabilityId,
    OUT PPCI_CAPABILITIES_HEADER Buffer,
    IN ULONG Length
);

BOOLEAN
NTAPI
PciCanDisableDecodes(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN PPCI_COMMON_HEADER Config,
    IN ULONGLONG HackFlags,
    IN BOOLEAN ForPowerDown
);

PCI_DEVICE_TYPES
NTAPI
PciClassifyDeviceType(
    IN PPCI_PDO_EXTENSION PdoExtension
);

KIPI_BROADCAST_WORKER PciExecuteCriticalSystemRoutine;

ULONG_PTR
NTAPI
PciExecuteCriticalSystemRoutine(
    IN ULONG_PTR IpiContext
);

BOOLEAN
NTAPI
PciCreateIoDescriptorFromBarLimit(
    PIO_RESOURCE_DESCRIPTOR ResourceDescriptor,
    IN PULONG BarArray,
    IN BOOLEAN Rom
);

BOOLEAN
NTAPI
PciIsSlotPresentInParentMethod(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG Method
);

VOID
NTAPI
PciDecodeEnable(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BOOLEAN Enable,
    OUT PUSHORT Command
);

NTSTATUS
NTAPI
PciQueryBusInformation(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPNP_BUS_INFORMATION* Buffer
);

NTSTATUS
NTAPI
PciQueryCapabilities(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_CAPABILITIES DeviceCapability
);

//
// Configuration Routines
//
NTSTATUS
NTAPI
PciGetConfigHandlers(
    IN PPCI_FDO_EXTENSION FdoExtension
);

VOID
NTAPI
PciReadSlotConfig(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

VOID
NTAPI
PciWriteDeviceConfig(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

VOID
NTAPI
PciReadDeviceConfig(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

UCHAR
NTAPI
PciGetAdjustedInterruptLine(
    IN PPCI_PDO_EXTENSION PdoExtension
);

//
// State Machine Logic Transition Routines
//
VOID
NTAPI
PciInitializeState(
    IN PPCI_FDO_EXTENSION DeviceExtension
);

NTSTATUS
NTAPI
PciBeginStateTransition(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCI_STATE NewState
);

NTSTATUS
NTAPI
PciCancelStateTransition(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCI_STATE NewState
);

VOID
NTAPI
PciCommitStateTransition(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCI_STATE NewState
);

//
// Arbiter Support
//
NTSTATUS
NTAPI
PciInitializeArbiters(
    IN PPCI_FDO_EXTENSION FdoExtension
);

NTSTATUS
NTAPI
PciInitializeArbiterRanges(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN PCM_RESOURCE_LIST Resources
);

//
// Debug Helpers
//
BOOLEAN
NTAPI
PciDebugIrpDispatchDisplay(
    IN PIO_STACK_LOCATION IoStackLocation,
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN USHORT MaxMinor
);

VOID
NTAPI
PciDebugDumpCommonConfig(
    IN PPCI_COMMON_HEADER PciData
);

VOID
NTAPI
PciDebugDumpQueryCapabilities(
    IN PDEVICE_CAPABILITIES DeviceCaps
);

VOID
NTAPI
PciDebugPrintIoResReqList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST Requirements
);

VOID
NTAPI
PciDebugPrintCmResList(
    IN PCM_RESOURCE_LIST ResourceList
);

VOID
NTAPI
PciDebugPrintPartialResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource
);

//
// Interface Support
//
NTSTATUS
NTAPI
PciQueryInterface(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN CONST GUID* InterfaceType,
    IN ULONG Size,
    IN ULONG Version,
    IN PVOID InterfaceData,
    IN PINTERFACE Interface,
    IN BOOLEAN LastChance
);

NTSTATUS
NTAPI
PciPmeInterfaceInitializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
routeintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
arbusno_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
agpintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
tranirq_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
busintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
armem_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
ario_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
locintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
pcicbintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
lddintrf_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
devpresent_Initializer(
    IN PVOID Instance
);

NTSTATUS
NTAPI
agpintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
arbusno_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
tranirq_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
armem_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
busintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
ario_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

VOID
NTAPI
ario_ApplyBrokenVideoHack(
    IN PPCI_FDO_EXTENSION FdoExtension
);

NTSTATUS
NTAPI
pcicbintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
lddintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
locintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
PciPmeInterfaceConstructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
routeintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

NTSTATUS
NTAPI
devpresent_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID Instance,
    IN PVOID InterfaceData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE Interface
);

//
// PCI Enumeration and Resources
//
NTSTATUS
NTAPI
PciQueryDeviceRelations(
    IN PPCI_FDO_EXTENSION DeviceExtension,
    IN OUT PDEVICE_RELATIONS *pDeviceRelations
);

NTSTATUS
NTAPI
PciQueryResources(
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT PCM_RESOURCE_LIST *Buffer
);

NTSTATUS
NTAPI
PciQueryTargetDeviceRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *pDeviceRelations
);

NTSTATUS
NTAPI
PciQueryEjectionRelations(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_RELATIONS *pDeviceRelations
);

NTSTATUS
NTAPI
PciQueryRequirements(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
);

BOOLEAN
NTAPI
PciComputeNewCurrentSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PCM_RESOURCE_LIST ResourceList
);

NTSTATUS
NTAPI
PciSetResources(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN BOOLEAN DoReset,
    IN BOOLEAN SomethingSomethingDarkSide
);

NTSTATUS
NTAPI
PciBuildRequirementsList(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST* Buffer
);

//
// Identification Functions
//
PWCHAR
NTAPI
PciGetDeviceDescriptionMessage(
    IN UCHAR BaseClass,
    IN UCHAR SubClass
);

NTSTATUS
NTAPI
PciQueryDeviceText(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN DEVICE_TEXT_TYPE QueryType,
    IN ULONG Locale,
    OUT PWCHAR *Buffer
);

NTSTATUS
NTAPI
PciQueryId(
    IN PPCI_PDO_EXTENSION DeviceExtension,
    IN BUS_QUERY_ID_TYPE QueryType,
    OUT PWCHAR *Buffer
);

//
// CardBUS Support
//
VOID
NTAPI
Cardbus_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Cardbus_SaveCurrentSettings(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Cardbus_SaveLimits(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Cardbus_RestoreCurrent(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Cardbus_GetAdditionalResourceDescriptors(
    IN PPCI_CONFIGURATOR_CONTEXT Context,
    IN PPCI_COMMON_HEADER PciData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

VOID
NTAPI
Cardbus_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

VOID
NTAPI
Cardbus_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

//
// PCI Device Support
//
VOID
NTAPI
Device_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Device_SaveCurrentSettings(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Device_SaveLimits(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Device_RestoreCurrent(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
Device_GetAdditionalResourceDescriptors(
    IN PPCI_CONFIGURATOR_CONTEXT Context,
    IN PPCI_COMMON_HEADER PciData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

VOID
NTAPI
Device_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

VOID
NTAPI
Device_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

//
// PCI-to-PCI Bridge Device Support
//
VOID
NTAPI
PPBridge_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
PPBridge_SaveCurrentSettings(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
PPBridge_SaveLimits(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
PPBridge_RestoreCurrent(
    IN PPCI_CONFIGURATOR_CONTEXT Context
);

VOID
NTAPI
PPBridge_GetAdditionalResourceDescriptors(
    IN PPCI_CONFIGURATOR_CONTEXT Context,
    IN PPCI_COMMON_HEADER PciData,
    IN PIO_RESOURCE_DESCRIPTOR IoDescriptor
);

VOID
NTAPI
PPBridge_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

VOID
NTAPI
PPBridge_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_HEADER PciData
);

//
// Bus Number Routines
//
BOOLEAN
NTAPI
PciAreBusNumbersConfigured(
    IN PPCI_PDO_EXTENSION PdoExtension
);

//
// Routine Interface
//
NTSTATUS
NTAPI
PciCacheLegacyDeviceRouting(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN UCHAR InterruptLine,
    IN UCHAR InterruptPin,
    IN UCHAR BaseClass,
    IN UCHAR SubClass,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PPCI_PDO_EXTENSION PdoExtension,
    OUT PDEVICE_OBJECT *pFoundDeviceObject
);

//
// External Resources
//
extern SINGLE_LIST_ENTRY PciFdoExtensionListHead;
extern KEVENT PciGlobalLock;
extern PPCI_INTERFACE PciInterfaces[];
extern PCI_INTERFACE ArbiterInterfaceBusNumber;
extern PCI_INTERFACE ArbiterInterfaceMemory;
extern PCI_INTERFACE ArbiterInterfaceIo;
extern PCI_INTERFACE BusHandlerInterface;
extern PCI_INTERFACE PciRoutingInterface;
extern PCI_INTERFACE PciCardbusPrivateInterface;
extern PCI_INTERFACE PciLegacyDeviceDetectionInterface;
extern PCI_INTERFACE PciPmeInterface;
extern PCI_INTERFACE PciDevicePresentInterface;
//extern PCI_INTERFACE PciNativeIdeInterface;
extern PCI_INTERFACE PciLocationInterface;
extern PCI_INTERFACE AgpTargetInterface;
extern PCI_INTERFACE TranslatorInterfaceInterrupt;
extern PDRIVER_OBJECT PciDriverObject;
extern PWATCHDOG_TABLE WdTable;
extern PPCI_HACK_ENTRY PciHackTable;
extern BOOLEAN PciAssignBusNumbers;
extern BOOLEAN PciEnableNativeModeATA;
extern PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable;
extern BOOLEAN PciRunningDatacenter;

/* Exported by NTOS, should this go in the NDK? */
extern NTSYSAPI BOOLEAN InitSafeBootMode;

#endif /* _PCIX_PCH_ */
