/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller FDO, power, recovery and the UCX controller callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"
#include <ntstrsafe.h>
#include <acpiioct.h>

#define NDEBUG
#include <debug.h>

/* GUIDs kept local so the driver needs no GUID library */
static const GUID XhciGuidBusInterfaceStandard =
    { 0x496b8280, 0x6f25, 0x11d0, { 0xbe, 0xaf, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f } };
static const GUID XhciGuidD3ColdSupport =
    { 0xb38290e5, 0x3cd0, 0x4f9d, { 0x99, 0x37, 0xf5, 0xfe, 0x2b, 0x44, 0xd4, 0x7a } };
static const GUID XhciGuidBusTypeUsb =
    { 0x9d7debbc, 0xc85d, 0x11d1, { 0x9e, 0xb4, 0x00, 0x60, 0x08, 0xc3, 0xa1, 0x9a } };
static const GUID XhciGuidHostControllerInterface =
    { 0x3abf6f2d, 0x71c4, 0x462a, { 0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27 } };
static const GUID XhciGuidChainedMdls =
    { 0xf5ceeb23, 0xad90, 0x458c, { 0x97, 0x9a, 0xd5, 0x9b, 0x3a, 0xd6, 0x88, 0x4f } };
static const GUID XhciGuidStaticStreams =
    { 0x09051e1f, 0x0dc9, 0x4e6b, { 0x8b, 0x12, 0x96, 0x0c, 0xbd, 0x81, 0xaa, 0x8f } };
static const GUID XhciGuidSelectiveSuspend =
    { 0x755c630d, 0xc8a0, 0x4765, { 0x94, 0x6f, 0x90, 0xcc, 0x70, 0x38, 0x56, 0xa4 } };
static const GUID XhciGuidFunctionSuspend =
    { 0xf4563183, 0xd66e, 0x42bd, { 0xbd, 0x53, 0x1a, 0xc7, 0xb0, 0x4c, 0xd5, 0x9d } };
static const GUID XhciGuidHighBandwidthIsoch =
    { 0x9c8a8a27, 0x15f9, 0x42e3, { 0xa3, 0x56, 0xcd, 0xa6, 0xae, 0x97, 0xa8, 0xc8 } };
static const GUID XhciGuidClearTtBufferOnCancel =
    { 0x09b76d52, 0x0a6d, 0x4e4f, { 0xa9, 0x11, 0xd0, 0x36, 0xd1, 0x92, 0x94, 0x97 } };
static const GUID XhciGuidDsm =
    { 0xac340cb7, 0xe901, 0x45bf, { 0xb7, 0xe6, 0x2b, 0x34, 0xec, 0x93, 0x1e, 0x23 } };

/* Recovery action bits */
#define XHCI_RECOVER_INTERNAL_RESET     0x01
#define XHCI_RECOVER_RESET              0x02
#define XHCI_RECOVER_NO_RESTART         0x04
#define XHCI_RECOVER_GONE               0x08
#define XHCI_RECOVER_GONE_PNP           0x10
#define XHCI_RECOVER_REPORT_FAILED      0x20

/* Hardware verifier conditions raised here */
#define XHCI_VERIFY_FIRMWARE_OUTDATED   0x00000001
#define XHCI_VERIFY_HSE                 0x00000002
#define XHCI_VERIFY_HCE                 0x00000004
#define XHCI_VERIFY_STOP_TIMEOUT        0x00000008
#define XHCI_VERIFY_RESET_TIMEOUT       0x00000010
#define XHCI_VERIFY_START_TIMEOUT       0x00000020
#define XHCI_VERIFY_ALL_ONES            0x00000040
#define XHCI_VERIFY_SAVE_RESTORE        0x00400000

#define XHCI_WATCHDOG_PERIOD_MS         5000
#define XHCI_WATCHDOG_TOLERANCE_MS      1000
#define XHCI_RESET_LIMIT                10
#define XHCI_RESET_WINDOW_TICKS         60
#define XHCI_IDLE_TIMEOUT_LONG_MS       5000
#define XHCI_IDLE_TIMEOUT_SHORT_MS      1
#define XHCI_PAD_BUFFER_SIZE            512
#define XHCI_ACPI_PARENT_ID             0x7FFFFFFF

#define XHCI_BUGCHECK_USB3_DRIVER       0x144
#define XHCI_TEST_RESET_IOCTL           0x00220FB7
#define XHCI_DEVICE_USED_BY_DEBUGGER    0x02000000

#define XHCI_ACPI_GET_DEVICE_INFORMATION \
    CTL_CODE(FILE_DEVICE_ACPI, 10, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/** Output of the ACPI device information IOCTL (public WDK acpiioct.h layout). */
typedef struct _XHCI_ACPI_DEVICE_INFO
{
    ULONG Signature;
    USHORT Size;
    UCHAR Revision;
    UCHAR Reserved0;
    USHORT VendorIdStringOffset;
    USHORT VendorStringLength;
    USHORT DeviceIdStringOffset;
    USHORT SubSystemIdStringOffset;
    USHORT SubSystemStringLength;
    USHORT SubDeviceIdStringOffset;
    USHORT InstanceIdLength;
    USHORT InstanceIdOffset;
    USHORT BaseClassCode;
    USHORT HardwareRevision;
    UCHAR ProgrammingInterface;
    UCHAR Reserved1;
    USHORT SubClassCode;
} XHCI_ACPI_DEVICE_INFO, *PXHCI_ACPI_DEVICE_INFO;

typedef
NTSTATUS
(NTAPI *PFN_XHCI_QUERY_DEVICE_FLAGS)(
    _In_ PCWSTR DeviceKey,
    _In_ PCWSTR Category,
    _Out_ PULONG64 Flags);

/** FDO context; the controller pointer is NULL until AddDevice finished. */
typedef struct _XHCI_FDO_CONTEXT
{
    XhciController* Controller;
} XHCI_FDO_CONTEXT, *PXHCI_FDO_CONTEXT;

/** Context of the watchdog timer and the work items. */
typedef struct _XHCI_OBJECT_CONTEXT
{
    XhciController* Controller;
} XHCI_OBJECT_CONTEXT, *PXHCI_OBJECT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_FDO_CONTEXT, XhciGetFdoContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciController, XhciGetControllerContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_OBJECT_CONTEXT, XhciGetObjectContext)

static EVT_WDF_DEVICE_PREPARE_HARDWARE XhciEvtDevicePrepareHardware;
static EVT_WDF_DEVICE_RELEASE_HARDWARE XhciEvtDeviceReleaseHardware;
static EVT_WDF_DEVICE_D0_ENTRY XhciEvtDeviceD0Entry;
static EVT_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED XhciEvtDeviceD0EntryPostInterruptsEnabled;
static EVT_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED XhciEvtDeviceD0ExitPreInterruptsDisabled;
static EVT_WDF_DEVICE_D0_EXIT XhciEvtDeviceD0Exit;
static EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT XhciEvtDeviceSelfManagedIoInit;
static EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP XhciEvtDeviceSelfManagedIoCleanup;
static EVT_WDF_DEVICE_USAGE_NOTIFICATION XhciEvtDeviceUsageNotification;
static EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 XhciEvtDeviceArmWakeFromS0;
static EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 XhciEvtDeviceDisarmWakeFromS0;
static EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED XhciEvtDeviceWakeFromS0Triggered;
static EVT_WDF_DEVICE_ARM_WAKE_FROM_SX XhciEvtDeviceArmWakeFromSx;
static EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX XhciEvtDeviceDisarmWakeFromSx;
static EVT_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS XhciEvtDeviceFilterRemoveResourceRequirements;
static EVT_WDF_DEVICE_FILTER_RESOURCE_REQUIREMENTS XhciEvtDeviceFilterAddResourceRequirements;
static EVT_WDF_DEVICE_REMOVE_ADDED_RESOURCES XhciEvtDeviceRemoveAddedResources;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS XhciEvtPreprocessSetPower;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS XhciEvtPreprocessInternalIoctl;
static EVT_WDF_OBJECT_CONTEXT_CLEANUP XhciEvtFdoCleanup;
static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL XhciEvtIoDeviceControl;
static EVT_WDF_TIMER XhciEvtWatchdog;
static EVT_WDF_WORKITEM XhciEvtRecoveryWork;
static EVT_WDF_WORKITEM XhciEvtIdleTimeoutWork;
static EVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY XhciEvtControllerQueryUsbCapability;
static EVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER XhciEvtControllerGetCurrentFrameNumber;
static EVT_UCX_CONTROLLER_RESET XhciEvtControllerReset;

static
NTSTATUS
NTAPI
XhciEvtControllerEnableForwardProgress(
    _In_ UCXCONTROLLER UcxController);

/* Object lookup **************************************************************/

XhciController*
XhciController::FromDevice(
    _In_ WDFDEVICE Device)
{
    return XhciGetFdoContext(Device)->Controller;
}

XhciController*
XhciController::FromUcx(
    _In_ UCXCONTROLLER UcxController)
{
    return XhciGetControllerContext(UcxController);
}

VOID
XhciController::SetErrata(
    _In_ XhciErrata Bit)
{
    if (Bit != XhciErrata::RegSplit64BitAccess && Bit != XhciErrata::IntPrimaryOnly)
        DPRINT1("Errata bit %lu set after population\n", static_cast<ULONG>(Bit));

    m_Errata |= XhciErrataBit(Bit);
}

BOOLEAN
XhciController::IsAccessible() const
{
    if (!m_Registers.IsMapped() || m_Gone)
    {
        DPRINT("Controller %p not accessible\n", this);
        return FALSE;
    }

    return TRUE;
}

/* Device identity ************************************************************/

static
NTSTATUS
NTAPI
XhciQueryPciIdentity(
    _In_ WDFDEVICE Device,
    _Inout_ XhciIdentity* Id)
{
    PCI_COMMON_CONFIG Config;
    PBUS_INTERFACE_STANDARD Bus = &Id->PciInterface;
    ULONG Address;
    ULONG Length;
    ULONG Value;
    UCHAR Bytes[3];
    ULONG Index;
    NTSTATUS Status;

    /* QUIRK: a missing bus number is only reported */
    Status = WdfDeviceQueryProperty(Device, DevicePropertyBusNumber, sizeof(Id->PciBus), &Id->PciBus, &Length);
    if (!NT_SUCCESS(Status))
        DPRINT1("Bus number query failed 0x%lx\n", Status);

    Status = WdfDeviceQueryProperty(Device, DevicePropertyAddress, sizeof(Address), &Address, &Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Bus address query failed 0x%lx\n", Status);
        return Status;
    }
    Id->PciDevice = Address >> 16;
    Id->PciFunction = Address & 0xFFFF;

    Status = WdfFdoQueryForInterface(Device, &XhciGuidBusInterfaceStandard, (PINTERFACE)Bus,
                                     sizeof(*Bus), 1, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PCI bus interface query failed 0x%lx\n", Status);
        RtlZeroMemory(Bus, sizeof(*Bus));
        return Status;
    }

    if (Bus->GetBusData(Bus->Context, PCI_WHICHSPACE_CONFIG, &Config, 0, sizeof(Config)) != sizeof(Config))
    {
        DPRINT1("PCI config space read came back short\n");
        return STATUS_UNSUCCESSFUL;
    }

    Id->PciVendorId = Config.VendorID;
    Id->PciDeviceId = Config.DeviceID;
    Id->PciRevisionId = Config.RevisionID;
    Id->PciSubsystemVendorId = Config.u.type0.SubVendorID;
    Id->PciSubsystemId = Config.u.type0.SubSystemID;

    /* VIA keeps its firmware version in config space */
    if (Id->PciVendorId == 0x1106 && Id->PciDeviceId == 0x3432)
    {
        for (Index = 0; Index < RTL_NUMBER_OF(Bytes); Index++)
        {
            if (Bus->GetBusData(Bus->Context, PCI_WHICHSPACE_CONFIG, &Bytes[Index], 0x261 + Index, 1) != 1)
            {
                DPRINT1("VIA firmware version read failed\n");
                return STATUS_SUCCESS;
            }
        }
        Id->FirmwareVersion = ((ULONG)Bytes[2] << 16) | ((ULONG)Bytes[1] << 8) | Bytes[0];
    }
    else if (Id->PciVendorId == 0x1106 && (Id->PciDeviceId == 0x3483 || Id->PciDeviceId == 0x9201))
    {
        if (Bus->GetBusData(Bus->Context, PCI_WHICHSPACE_CONFIG, &Value, 0x50, sizeof(Value)) == sizeof(Value))
            Id->FirmwareVersion = Value;
        else
            DPRINT1("VIA firmware version read failed\n");
    }

    return STATUS_SUCCESS;
}

/** Copies Length bytes, stopping at a NUL, into a 5 byte id field. */
static
VOID
NTAPI
XhciCopyAcpiId(
    _Out_writes_(MAX_VENDOR_ID_STRING_LENGTH) PCHAR Destination,
    _In_reads_(Length) const CHAR* Source,
    _In_ ULONG Length)
{
    ULONG Index;

    /* Clamp the copy to the 5 byte field */
    for (Index = 0; Index < Length && Index < MAX_VENDOR_ID_STRING_LENGTH - 1 && Source[Index] != ANSI_NULL; Index++)
        Destination[Index] = Source[Index];

    Destination[Index] = ANSI_NULL;
}

static
NTSTATUS
NTAPI
XhciQueryAcpiIdentity(
    _In_ WDFDEVICE Device,
    _Inout_ XhciIdentity* Id)
{
    XHCI_ACPI_DEVICE_INFO Header;
    PXHCI_ACPI_DEVICE_INFO Info;
    WDF_MEMORY_DESCRIPTOR Output;
    WDFIOTARGET Target = WdfDeviceGetIoTarget(Device);
    const CHAR* Strings;
    ULONG_PTR Returned;
    ULONG Size;
    ULONG VendorOffset;
    ULONG DeviceOffset;
    ULONG Length;
    NTSTATUS Status;

    Id->PciVendorId = XHCI_ACPI_PARENT_ID;
    Id->PciDeviceId = XHCI_ACPI_PARENT_ID;

    RtlZeroMemory(&Header, sizeof(Header));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&Output, &Header, sizeof(Header));
    Status = WdfIoTargetSendIoctlSynchronously(Target, NULL, XHCI_ACPI_GET_DEVICE_INFORMATION,
                                               NULL, &Output, NULL, &Returned);
    if (Status != STATUS_BUFFER_OVERFLOW)
    {
        /* QUIRK: a success here leaves the identity empty and is passed on as is */
        DPRINT1("ACPI device information size query returned 0x%lx\n", Status);
        return Status;
    }

    Size = Header.Size;
    if (Size < sizeof(Header))
    {
        DPRINT1("ACPI device information size %lu too small\n", Size);
        return STATUS_ACPI_INVALID_DATA;
    }

    Info = (PXHCI_ACPI_DEVICE_INFO)ExAllocatePoolWithTag(NonPagedPool, Size, XHCI_TAG_CONTROLLER);
    if (Info == NULL)
    {
        DPRINT1("No memory for %lu bytes of ACPI device information\n", Size);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Info, Size);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&Output, Info, Size);
    Status = WdfIoTargetSendIoctlSynchronously(Target, NULL, XHCI_ACPI_GET_DEVICE_INFORMATION,
                                               NULL, &Output, NULL, &Returned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ACPI device information query failed 0x%lx\n", Status);
        ExFreePoolWithTag(Info, XHCI_TAG_CONTROLLER);
        return Status;
    }

    RtlStringCbPrintfA(Id->AcpiRevisionId, sizeof(Id->AcpiRevisionId), "%04X", Info->HardwareRevision);

    Strings = (const CHAR*)Info;
    VendorOffset = Info->VendorIdStringOffset;
    DeviceOffset = Info->DeviceIdStringOffset;
    Length = Info->VendorStringLength;
    if (VendorOffset > Size || Length > Size - VendorOffset)
        VendorOffset = 0;

    if (VendorOffset == 0)
    {
        RtlStringCbCopyA(Id->AcpiVendorId, sizeof(Id->AcpiVendorId), "UKWN");
        RtlStringCbCopyA(Id->AcpiDeviceId, sizeof(Id->AcpiDeviceId), "FFFF");
    }
    else if (DeviceOffset == 0 || DeviceOffset < VendorOffset || DeviceOffset > VendorOffset + Length)
    {
        XhciCopyAcpiId(Id->AcpiVendorId, Strings + VendorOffset, Length);
        RtlStringCbCopyA(Id->AcpiDeviceId, sizeof(Id->AcpiDeviceId), "FFFF");
    }
    else
    {
        XhciCopyAcpiId(Id->AcpiVendorId, Strings + VendorOffset, DeviceOffset - VendorOffset);
        XhciCopyAcpiId(Id->AcpiDeviceId, Strings + DeviceOffset, VendorOffset + Length - DeviceOffset);
    }

    ExFreePoolWithTag(Info, XHCI_TAG_CONTROLLER);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciQueryIdentity(
    _In_ WDFDEVICE Device,
    _Out_ XhciIdentity* Id)
{
    WCHAR Enumerator[8];
    ULONG Length;
    NTSTATUS Status;

    RtlZeroMemory(Id, sizeof(*Id));
    Id->FirmwareVersion = XHCI_FIRMWARE_UNKNOWN;
    Id->ParentBus = UcxControllerParentBusTypePci;

    Status = IoGetDeviceProperty(WdfDeviceWdmGetPhysicalDevice(Device), DevicePropertyEnumeratorName,
                                 sizeof(Enumerator), Enumerator, &Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Enumerator name query failed 0x%lx, assuming PCI\n", Status);
    }
    else if (_wcsicmp(Enumerator, L"ACPI") == 0)
    {
        Id->ParentBus = UcxControllerParentBusTypeAcpi;
    }
    else if (_wcsicmp(Enumerator, L"URS") == 0)
    {
        /* ReactOS has no USB role switch enumerator */
        DPRINT1("Controllers below the role switch enumerator are not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (Id->ParentBus == UcxControllerParentBusTypeAcpi)
        Status = XhciQueryAcpiIdentity(Device, Id);
    else
        Status = XhciQueryPciIdentity(Device, Id);

    if (!NT_SUCCESS(Status))
        DPRINT1("Controller identity unavailable 0x%lx\n", Status);

    return Status;
}

/* Errata *********************************************************************/

VOID
XhciController::QueryErrataDatabase(
    _In_ BOOLEAN AtAddDevice)
{
    static const PCWSTR Providers[] = { L"USBXHCI", L"USBXHCI2" };
    PFN_XHCI_QUERY_DEVICE_FLAGS Query = (PFN_XHCI_QUERY_DEVICE_FLAGS)XhciDriver.QueryDeviceFlags;
    PULONG64 Words[] = { &m_Errata, &m_ErrataWord2 };
    WCHAR Keys[7][96];
    ULONG KeyCount;
    ULONG64 Flags;
    BOOLEAN Any = FALSE;
    BOOLEAN Pci = (m_Identity.ParentBus != UcxControllerParentBusTypeAcpi);
    ULONG Provider;
    ULONG Index;

    if (Query == NULL)
        return;

    /* Keys go from the broadest match to the most specific one */
    RtlStringCchCopyW(Keys[0], RTL_NUMBER_OF(Keys[0]), L"USBXHCI:ALL");
    if (Pci)
    {
        RtlStringCchPrintfW(Keys[1], RTL_NUMBER_OF(Keys[1]), L"USBXHCI:PCI\\VEN_%04X",
                            m_Identity.PciVendorId);
        RtlStringCchPrintfW(Keys[2], RTL_NUMBER_OF(Keys[2]), L"%s&DEV_%04X",
                            Keys[1], m_Identity.PciDeviceId);
        RtlStringCchPrintfW(Keys[3], RTL_NUMBER_OF(Keys[3]), L"%s&REV_%02X",
                            Keys[2], m_Identity.PciRevisionId);
    }
    else
    {
        RtlStringCchPrintfW(Keys[1], RTL_NUMBER_OF(Keys[1]), L"USBXHCI:ACPI\\VEN_%S",
                            m_Identity.AcpiVendorId);
        RtlStringCchPrintfW(Keys[2], RTL_NUMBER_OF(Keys[2]), L"%s&DEV_%S",
                            Keys[1], m_Identity.AcpiDeviceId);
        RtlStringCchPrintfW(Keys[3], RTL_NUMBER_OF(Keys[3]), L"%s&REV_%S",
                            Keys[2], m_Identity.AcpiRevisionId);
    }
    KeyCount = 4;

    if (m_Identity.FirmwareVersion != XHCI_FIRMWARE_UNKNOWN)
    {
        RtlStringCchPrintfW(Keys[KeyCount], RTL_NUMBER_OF(Keys[0]), L"%s&%I64X",
                            Keys[3], m_Identity.FirmwareVersion);
        KeyCount++;
    }

    if (Pci)
    {
        RtlStringCchPrintfW(Keys[KeyCount], RTL_NUMBER_OF(Keys[0]), L"%s&SUBSYS_%04X%04X",
                            Keys[2], m_Identity.PciSubsystemId, m_Identity.PciSubsystemVendorId);
        RtlStringCchPrintfW(Keys[KeyCount + 1], RTL_NUMBER_OF(Keys[0]), L"%s&REV_%02X",
                            Keys[KeyCount], m_Identity.PciRevisionId);
        KeyCount += 2;
    }

    for (Provider = 0; Provider < RTL_NUMBER_OF(Providers); Provider++)
    {
        for (Index = 0; Index < KeyCount; Index++)
        {
            Flags = 0;
            if (!NT_SUCCESS(Query(Keys[Index], Providers[Provider], &Flags)))
                continue;

            DPRINT("Errata word %lu 0x%I64x from %S\n", Provider + 1, Flags, Keys[Index]);
            *Words[Provider] |= Flags;

            /* The catch all key does not identify this controller */
            if (Index != 0)
                Any = TRUE;
        }
    }

    if (!Any)
        DPRINT("No errata database entry for this controller\n");

    if (HasErrata(XhciErrata::HostRefuseToStart))
    {
        /* Skip recovery at AddDevice; that device is about to be deleted */
        if (!AtAddDevice)
            QueueRecovery(XHCI_RECOVER_NO_RESTART, 0x100C);
    }
    else if (HasErrata(XhciErrata::HostStaleFirmware))
    {
        DPRINT1("Controller firmware 0x%I64x is outdated\n", m_Identity.FirmwareVersion);
        VerifierCheck(XHCI_VERIFY_FIRMWARE_OUTDATED);
        QueueRecovery(0, 0x100F);
    }
}

VOID
XhciController::ReadRegistryErrata()
{
    DECLARE_CONST_UNICODE_STRING(ValueName, L"UseStrictBiosHandoff");
    WDFKEY Keys[2] = { NULL, NULL };
    ULONG Value;
    ULONG Index;
    NTSTATUS Status;

    Status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(), KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &Keys[0]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Driver parameters key open failed 0x%lx\n", Status);
        Keys[0] = NULL;
    }

    Status = WdfDeviceOpenRegistryKey(m_Device, PLUGPLAY_REGKEY_DEVICE, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &Keys[1]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device hardware key open failed 0x%lx\n", Status);
        Keys[1] = NULL;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(Keys); Index++)
    {
        if (Keys[Index] == NULL)
            continue;

        if (NT_SUCCESS(WdfRegistryQueryULong(Keys[Index], &ValueName, &Value)))
        {
            if (Value == 0)
                m_Errata &= ~XhciErrataBit(XhciErrata::BootStrictBiosRelease);
            else if (Value == 1)
                m_Errata |= XhciErrataBit(XhciErrata::BootStrictBiosRelease);
        }

        WdfRegistryClose(Keys[Index]);
    }
}

VOID
XhciController::SetInternalFlags()
{
    ULONG Vendor = m_Identity.PciVendorId;
    ULONG Device = m_Identity.PciDeviceId;

    m_InternalFlags = 0;

    if (Vendor == 0x1B73 && Device == 0x1100 && m_Identity.PciRevisionId == 0x10)
        m_InternalFlags = FixFrescoShadow;
    else if (Vendor == 0x1B73 && Device == 0x1009)
        m_InternalFlags = FixFrescoControl;
    else if (Vendor == 0x1B6F && Device == 0x7023)
        m_InternalFlags |= FixEtronSet | FixEtronClear;
    else if (Vendor == 0x1B21 && Device >= 0x1040 && Device <= 0x1042)
        m_InternalFlags |= FixAsmediaResetDelay;
}

VOID
XhciController::PopulateErrata()
{
    m_Errata = XhciErrataBit(XhciErrata::BootTolerateBiosHold);
    m_ErrataWord2 = 0;
    QueryErrataDatabase(TRUE);
    ReadRegistryErrata();
    SetInternalFlags();

    DPRINT1("Controller %04lx:%04lx rev %02x errata 0x%I64x 0x%I64x internal 0x%I64x\n",
            m_Identity.PciVendorId, m_Identity.PciDeviceId, m_Identity.PciRevisionId,
            m_Errata, m_ErrataWord2, m_InternalFlags);
}

/* Hardware verifier **********************************************************/

VOID
XhciController::ReadVerifierFlags()
{
    WCHAR Ids[3][24];
    WCHAR Path[160];
    UNICODE_STRING PathString;
    DECLARE_CONST_UNICODE_STRING(ValueName, L"controller");
    PCWSTR Version = (m_Registers.VersionMajor() == 0) ? L"xHCI96" : L"xHCI10";
    ULONG64 Flags = 0;
    ULONG Length;
    ULONG Type;
    WDFKEY Key;
    ULONG Index;
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (m_Identity.ParentBus == UcxControllerParentBusTypeAcpi)
    {
        RtlStringCchPrintfW(Ids[0], RTL_NUMBER_OF(Ids[0]), L"%S%S%S", m_Identity.AcpiVendorId,
                            m_Identity.AcpiDeviceId, m_Identity.AcpiRevisionId);
        RtlStringCchPrintfW(Ids[1], RTL_NUMBER_OF(Ids[1]), L"%S%S", m_Identity.AcpiVendorId,
                            m_Identity.AcpiDeviceId);
    }
    else
    {
        RtlStringCchPrintfW(Ids[0], RTL_NUMBER_OF(Ids[0]), L"%04X%04X%02X", m_Identity.PciVendorId,
                            m_Identity.PciDeviceId, m_Identity.PciRevisionId);
        RtlStringCchPrintfW(Ids[1], RTL_NUMBER_OF(Ids[1]), L"%04X%04X", m_Identity.PciVendorId,
                            m_Identity.PciDeviceId);
    }
    RtlStringCchCopyW(Ids[2], RTL_NUMBER_OF(Ids[2]), L"global");

    for (Index = 0; Index < RTL_NUMBER_OF(Ids) && Status == STATUS_OBJECT_NAME_NOT_FOUND; Index++)
    {
        RtlStringCchPrintfW(Path, RTL_NUMBER_OF(Path),
                            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usb\\HardwareVerifier\\%s\\%s",
                            Ids[Index], Version);
        RtlInitUnicodeString(&PathString, Path);

        Status = WdfRegistryOpenKey(NULL, &PathString, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &Key);
        if (!NT_SUCCESS(Status))
            continue;

        Status = WdfRegistryQueryValue(Key, &ValueName, sizeof(Flags), &Flags, &Length, &Type);
        WdfRegistryClose(Key);
    }

    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Hardware verifier settings read failed 0x%lx\n", Status);
        Flags = 0;
    }

    m_VerifierFlags = (ULONG)Flags;
}

VOID
XhciController::VerifierCheck(
    _In_ ULONG Flag)
{
    DPRINT1("Controller %p hardware verifier condition 0x%lx\n", this, Flag);

    if ((m_VerifierFlags & Flag) && !KdRefreshDebuggerNotPresent())
        DbgBreakPoint();
}

/* ACPI methods ***************************************************************/

static
VOID
NTAPI
XhciEvaluateAcpiMethod(
    _In_ WDFDEVICE Device,
    _In_reads_bytes_(InputSize) PVOID Input,
    _In_ ULONG InputSize)
{
    ACPI_EVAL_OUTPUT_BUFFER Output;
    WDF_MEMORY_DESCRIPTOR InputDescriptor;
    WDF_MEMORY_DESCRIPTOR OutputDescriptor;
    NTSTATUS Status;

    RtlZeroMemory(&Output, sizeof(Output));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&InputDescriptor, Input, InputSize);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&OutputDescriptor, &Output, sizeof(Output));

    Status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(Device), NULL, IOCTL_ACPI_EVAL_METHOD,
                                               &InputDescriptor, &OutputDescriptor, NULL, NULL);
    if (!NT_SUCCESS(Status))
        DPRINT("ACPI method evaluation failed 0x%lx\n", Status);
    else if (Output.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
        DPRINT("ACPI method returned signature 0x%lx\n", Output.Signature);
}

VOID
XhciController::EvaluateDsm(
    _In_ ULONG Function)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX Input;
    PACPI_METHOD_ARGUMENT Argument;
    ULONG Size;
    ULONG Index;

    Size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + sizeof(GUID) - sizeof(ULONG) +
           3 * sizeof(ACPI_METHOD_ARGUMENT);

    Input = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)ExAllocatePoolWithTag(NonPagedPool, Size, XHCI_TAG_CONTROLLER);
    if (Input == NULL)
    {
        DPRINT1("No memory for the _DSM(%lu) call\n", Function);
        return;
    }

    RtlZeroMemory(Input, Size);
    Input->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    Input->MethodNameAsUlong = 'MSD_';
    Input->Size = Size;
    Input->ArgumentCount = 4;

    /* UUID, then the function value, then two reserved zeros */
    Argument = &Input->Argument[0];
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, &XhciGuidDsm, sizeof(GUID));
    for (Index = 0; Index < 3; Index++)
    {
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
        ACPI_METHOD_SET_ARGUMENT_INTEGER(Argument, (Index == 0) ? Function : 0);
    }

    XhciEvaluateAcpiMethod(m_Device, Input, Size);
    ExFreePoolWithTag(Input, XHCI_TAG_CONTROLLER);
}

VOID
XhciController::EvaluatePrmi()
{
    ACPI_EVAL_INPUT_BUFFER Input;

    RtlZeroMemory(&Input, sizeof(Input));
    Input.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    Input.MethodNameAsUlong = 'IMRP';

    XhciEvaluateAcpiMethod(m_Device, &Input, sizeof(Input));
}

/* S0 idle and wake ***********************************************************/

VOID
XhciController::SetIdleTimeout(
    _In_ ULONG Milliseconds)
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings;
    NTSTATUS Status;

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&Settings, IdleCanWakeFromS0);
    Settings.UserControlOfIdleSettings = IdleAllowUserControl;
    Settings.IdleTimeout = Milliseconds;
    Settings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

    Status = WdfDeviceAssignS0IdleSettings(m_Device, &Settings);
    if (!NT_SUCCESS(Status))
    {
        /* QUIRK: a failure downgrades the idle status after the fact */
        DPRINT("Idle timeout %lu ms refused 0x%lx\n", Milliseconds, Status);
        m_IdleStatus = IdleStatus::NoS0Wake;
        return;
    }

    m_LastIdleTimeout = Milliseconds;
}

VOID
XhciController::ConfigureS0Idle()
{
    D3COLD_SUPPORT_INTERFACE D3Cold;
    DEVICE_WAKE_DEPTH Depth = DeviceWakeDepthNotWakeable;
    NTSTATUS Status;

    if (HasErrata(XhciErrata::PwrStayInD0))
    {
        DPRINT1("S0 idle disabled by errata\n");
        m_IdleStatus = IdleStatus::NotConfigured;
        return;
    }

    if (HasErrata(XhciErrata::AcpiDsmAllowD3Cold))
        EvaluateDsm(3);

    RtlZeroMemory(&D3Cold, sizeof(D3Cold));
    Status = WdfFdoQueryForInterface(m_Device, &XhciGuidD3ColdSupport, (PINTERFACE)&D3Cold,
                                     sizeof(D3Cold), D3COLD_SUPPORT_INTERFACE_VERSION, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("No D3cold interface 0x%lx, S0 idle off\n", Status);
        m_IdleStatus = IdleStatus::NoS0Wake;
        return;
    }

    if (D3Cold.GetIdleWakeInfo != NULL)
    {
        Status = D3Cold.GetIdleWakeInfo(D3Cold.Context, PowerSystemWorking, &Depth);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Idle wake information query failed 0x%lx\n", Status);
            Depth = DeviceWakeDepthNotWakeable;
        }
    }

    if (D3Cold.InterfaceDereference != NULL)
        D3Cold.InterfaceDereference(D3Cold.Context);

    if (Depth <= DeviceWakeDepthD0)
    {
        DPRINT("Controller cannot wake from a Dx state in S0, S0 idle off\n");
        m_IdleStatus = IdleStatus::NoS0Wake;
        return;
    }

    m_IdleStatus = IdleStatus::Configured;
    SetIdleTimeout(XHCI_IDLE_TIMEOUT_LONG_MS);

    /* ReactOS has no runtime power framework */
    DPRINT("Power framework registration skipped\n");
}

VOID
XhciController::OnRootHubD0Entry()
{
    if (m_IdleStatus == IdleStatus::Configured && m_LastIdleTimeout != XHCI_IDLE_TIMEOUT_SHORT_MS)
        WdfWorkItemEnqueue(m_IdleItem);
}

VOID
XhciController::IdleTimeoutWorker()
{
    SetIdleTimeout(XHCI_IDLE_TIMEOUT_SHORT_MS);
}

VOID
XhciController::NotifyPortConnectState(
    _In_ BOOLEAN AnyConnected)
{
    /* Only meaningful with a power framework handle, which ReactOS never gives */
    DPRINT("Port connect state %u not reported, no power framework\n", AnyConnected);
}

VOID
XhciController::SetPortWake(
    _In_ BOOLEAN Arm)
{
    m_WaitWakeQueued = Arm;

    if (!Arm && HasErrata(XhciErrata::PortHoldWakeMaskAwake))
        return;

    m_RootHub.SetWakeEnables(Arm);
}

/* AddDevice ******************************************************************/

NTSTATUS
XhciController::CreateWdfObjects()
{
    WDF_TIMER_CONFIG TimerConfig;
    WDF_WORKITEM_CONFIG WorkConfig;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_TIMER_CONFIG_INIT_PERIODIC(&TimerConfig, XhciEvtWatchdog, XHCI_WATCHDOG_PERIOD_MS);
    TimerConfig.TolerableDelay = XHCI_WATCHDOG_TOLERANCE_MS;
    TimerConfig.AutomaticSerialization = FALSE;

    /* The UCXCONTROLLER is passive; every watchdog step is fine at DISPATCH */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_OBJECT_CONTEXT);
    Attributes.ParentObject = m_Ucx;
    Attributes.ExecutionLevel = WdfExecutionLevelDispatch;

    Status = WdfTimerCreate(&TimerConfig, &Attributes, &m_Watchdog);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Watchdog timer creation failed 0x%lx\n", Status);
        return Status;
    }
    XhciGetObjectContext(m_Watchdog)->Controller = this;

    WDF_WORKITEM_CONFIG_INIT(&WorkConfig, XhciEvtIdleTimeoutWork);
    WorkConfig.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_OBJECT_CONTEXT);
    Attributes.ParentObject = m_Ucx;

    Status = WdfWorkItemCreate(&WorkConfig, &Attributes, &m_IdleItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Idle timeout work item creation failed 0x%lx\n", Status);
        return Status;
    }
    XhciGetObjectContext(m_IdleItem)->Controller = this;

    WDF_WORKITEM_CONFIG_INIT(&WorkConfig, XhciEvtRecoveryWork);
    WorkConfig.AutomaticSerialization = FALSE;

    Status = WdfWorkItemCreate(&WorkConfig, &Attributes, &m_RecoveryItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Recovery work item creation failed 0x%lx\n", Status);
        return Status;
    }
    XhciGetObjectContext(m_RecoveryItem)->Controller = this;

    KeInitializeMutex(&m_RecoveryMutex, 0);
    KeInitializeSpinLock(&m_RecoveryLock);
    m_RecoveryActions = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
XhciController::Initialize(
    _In_ WDFDEVICE Device,
    _In_ UCXCONTROLLER Ucx,
    _In_ const XhciIdentity* Identity)
{
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS WakeSettings;
    NTSTATUS Status;

    m_Device = Device;
    m_WdmDevice = WdfDeviceWdmGetDeviceObject(Device);
    m_Ucx = Ucx;
    m_Identity = *Identity;
    m_PowerState = WdfPowerDeviceD3Final;
    m_SystemAction = SystemAction::Boot;
    m_TimeIncrement = KeQueryTimeIncrement();
    m_TestMode = XhciDriver.TestMode;
    m_IdleStatus = IdleStatus::NotConfigured;

    if (m_Identity.ParentBus == UcxControllerParentBusTypeAcpi)
    {
        m_Identity.PciVendorId = XHCI_ACPI_PARENT_ID;
        m_Identity.PciDeviceId = XHCI_ACPI_PARENT_ID;
        m_Identity.PciRevisionId = 0;
        m_Identity.PciBus = 0;
        m_Identity.PciDevice = 0;
        m_Identity.PciFunction = 0;
    }
    else
    {
        RtlZeroMemory(m_Identity.AcpiVendorId, sizeof(m_Identity.AcpiVendorId));
        RtlZeroMemory(m_Identity.AcpiDeviceId, sizeof(m_Identity.AcpiDeviceId));
        RtlZeroMemory(m_Identity.AcpiRevisionId, sizeof(m_Identity.AcpiRevisionId));
    }

    Status = CreateWdfObjects();
    if (!NT_SUCCESS(Status))
        return Status;

    PopulateErrata();
    if (HasErrata(XhciErrata::HostRefuseToStart))
    {
        DPRINT1("Controller is marked as not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchSequential);
    QueueConfig.EvtIoDeviceControl = XhciEvtIoDeviceControl;
    Status = WdfIoQueueCreate(m_Device, &QueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Default queue creation failed 0x%lx\n", Status);
        return Status;
    }

    Status = m_Buffers.Create(this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Common buffer creation failed 0x%lx\n", Status);
        return Status;
    }

    Status = m_Interrupters.Create(this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interrupter creation failed 0x%lx\n", Status);
        return Status;
    }

    Status = m_Slots.Create(this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device slot table creation failed 0x%lx\n", Status);
        return Status;
    }

    Status = m_Commands.Create(this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Command ring creation failed 0x%lx\n", Status);
        return Status;
    }

    if (HasErrata(XhciErrata::IntelPchBandwidthCap))
        DPRINT1("Panther Point command filter not implemented, endpoint limit not enforced\n");

    Status = m_RootHub.Create(this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub creation failed 0x%lx\n", Status);
        return Status;
    }

    DPRINT("Controller capability WMI block not provided\n");

    ConfigureS0Idle();

    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&WakeSettings);
    WakeSettings.Enabled = WdfFalse;
    WakeSettings.ArmForWakeIfChildrenAreArmedForWake = TRUE;
    WakeSettings.IndicateChildWakeOnParentWake = TRUE;
    WakeSettings.UserControlOfWakeSettings = WakeDoNotAllowUserControl;
    Status = WdfDeviceAssignSxWakeSettings(m_Device, &WakeSettings);
    if (!NT_SUCCESS(Status))
        DPRINT1("Sx wake settings refused 0x%lx\n", Status);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciCreateFdo(
    _Inout_ PWDFDEVICE_INIT DeviceInit,
    _In_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ WDFDEVICE* Device)
{
    DECLARE_CONST_UNICODE_STRING(Sddl, L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)");
    WCHAR Buffer[32];
    UNICODE_STRING Name;
    PNP_BUS_INFORMATION BusInfo;
    ULONG Instance;
    NTSTATUS Status;

    /* QUIRK: the name search has no upper bound */
    for (Instance = 0;; Instance++)
    {
        RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"\\Device\\USBFDO-%lu", Instance);
        RtlInitUnicodeString(&Name, Buffer);

        Status = WdfDeviceInitAssignName(DeviceInit, &Name);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Assigning the FDO name failed 0x%lx\n", Status);
            return Status;
        }

        Status = WdfDeviceInitAssignSDDLString(DeviceInit, &Sddl);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Assigning the FDO security failed 0x%lx\n", Status);
            return Status;
        }

        Status = WdfDeviceCreate(&DeviceInit, Attributes, Device);
        if (Status == STATUS_OBJECT_NAME_COLLISION)
        {
            DPRINT1("FDO name %wZ taken, trying the next one\n", &Name);
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("WdfDeviceCreate failed 0x%lx\n", Status);
            return Status;
        }

        break;
    }

    WdfDeviceSetSpecialFileSupport(*Device, WdfSpecialFilePaging, TRUE);
    WdfDeviceSetSpecialFileSupport(*Device, WdfSpecialFileHibernation, TRUE);
    WdfDeviceSetSpecialFileSupport(*Device, WdfSpecialFileDump, TRUE);
    WdfDeviceSetSpecialFileSupport(*Device, WdfSpecialFileBoot, TRUE);

    /* User mode tools open \\.\HCDn */
    RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"\\DosDevices\\HCD%lu", Instance);
    RtlInitUnicodeString(&Name, Buffer);
    Status = WdfDeviceCreateSymbolicLink(*Device, &Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Symbolic link %wZ failed 0x%lx\n", &Name, Status);
        return Status;
    }

    BusInfo.BusTypeGuid = XhciGuidBusTypeUsb;
    BusInfo.LegacyBusType = PNPBus;
    BusInfo.BusNumber = 0;
    WdfDeviceSetBusInformationForChildren(*Device, &BusInfo);

    /* QUIRK: UCX registers this interface again */
    Status = WdfDeviceCreateDeviceInterface(*Device, &XhciGuidHostControllerInterface, NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("Host controller interface registration failed 0x%lx\n", Status);

    return Status;
}

NTSTATUS
NTAPI
XhciEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPower;
    WDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicy;
    WDF_FDO_EVENT_CALLBACKS FdoCallbacks;
    WDF_OBJECT_ATTRIBUTES Attributes;
    UCX_CONTROLLER_CONFIG Config;
    XhciIdentity Identity;
    XhciController* Controller;
    UCXCONTROLLER Ucx;
    WDFDEVICE Device;
    UCHAR SetPower = IRP_MN_SET_POWER;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    Status = UcxInitializeDeviceInit(DeviceInit);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxInitializeDeviceInit failed 0x%lx\n", Status);
        return Status;
    }

    /* Requests UCX forwards into the transfer queues are built by this device, so they get this context */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciRequestData);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &Attributes);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPower);
    PnpPower.EvtDevicePrepareHardware = XhciEvtDevicePrepareHardware;
    PnpPower.EvtDeviceReleaseHardware = XhciEvtDeviceReleaseHardware;
    PnpPower.EvtDeviceUsageNotification = XhciEvtDeviceUsageNotification;
    PnpPower.EvtDeviceD0Entry = XhciEvtDeviceD0Entry;
    PnpPower.EvtDeviceD0EntryPostInterruptsEnabled = XhciEvtDeviceD0EntryPostInterruptsEnabled;
    PnpPower.EvtDeviceD0ExitPreInterruptsDisabled = XhciEvtDeviceD0ExitPreInterruptsDisabled;
    PnpPower.EvtDeviceD0Exit = XhciEvtDeviceD0Exit;
    PnpPower.EvtDeviceSelfManagedIoInit = XhciEvtDeviceSelfManagedIoInit;
    PnpPower.EvtDeviceSelfManagedIoCleanup = XhciEvtDeviceSelfManagedIoCleanup;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPower);

    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&PowerPolicy);
    PowerPolicy.EvtDeviceArmWakeFromS0 = XhciEvtDeviceArmWakeFromS0;
    PowerPolicy.EvtDeviceDisarmWakeFromS0 = XhciEvtDeviceDisarmWakeFromS0;
    PowerPolicy.EvtDeviceWakeFromS0Triggered = XhciEvtDeviceWakeFromS0Triggered;
    PowerPolicy.EvtDeviceArmWakeFromSx = XhciEvtDeviceArmWakeFromSx;
    PowerPolicy.EvtDeviceDisarmWakeFromSx = XhciEvtDeviceDisarmWakeFromSx;
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &PowerPolicy);

    WDF_FDO_EVENT_CALLBACKS_INIT(&FdoCallbacks);
    FdoCallbacks.EvtDeviceFilterRemoveResourceRequirements = XhciEvtDeviceFilterRemoveResourceRequirements;
    FdoCallbacks.EvtDeviceFilterAddResourceRequirements = XhciEvtDeviceFilterAddResourceRequirements;
    FdoCallbacks.EvtDeviceRemoveAddedResources = XhciEvtDeviceRemoveAddedResources;
    WdfFdoInitSetEventCallbacks(DeviceInit, &FdoCallbacks);

    WdfDeviceInitSetReleaseHardwareOrderOnFailure(DeviceInit, WdfReleaseHardwareOrderOnFailureAfterDescendants);

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, XhciEvtPreprocessSetPower,
                                                         IRP_MJ_POWER, &SetPower, 1);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Power IRP preprocess registration failed 0x%lx\n", Status);
        return Status;
    }

    if (XhciDriver.TestMode)
    {
        Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, XhciEvtPreprocessInternalIoctl,
                                                             IRP_MJ_INTERNAL_DEVICE_CONTROL, NULL, 0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Internal IOCTL preprocess registration failed 0x%lx\n", Status);
            return Status;
        }
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_FDO_CONTEXT);
    Attributes.EvtCleanupCallback = XhciEvtFdoCleanup;

    Status = XhciCreateFdo(DeviceInit, &Attributes, &Device);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = XhciQueryIdentity(Device, &Identity);
    if (!NT_SUCCESS(Status))
        goto Fail;

    UCX_CONTROLLER_CONFIG_INIT(&Config, "USBXHCI");
    Config.EvtControllerUsbDeviceAdd = XhciEvtControllerUsbDeviceAdd;
    Config.EvtControllerGetCurrentFrameNumber = XhciEvtControllerGetCurrentFrameNumber;
    Config.EvtControllerReset = XhciEvtControllerReset;
    Config.EvtControllerQueryUsbCapability = XhciEvtControllerQueryUsbCapability;
    Config.Reserved2 = (HANDLE)XhciEvtControllerEnableForwardProgress;

    if (Identity.ParentBus == UcxControllerParentBusTypeAcpi)
    {
        UCX_CONTROLLER_CONFIG_SET_ACPI_INFO(&Config, Identity.AcpiVendorId, Identity.AcpiDeviceId,
                                            Identity.AcpiRevisionId);
    }
    else
    {
        UCX_CONTROLLER_CONFIG_SET_PCI_INFO(&Config, Identity.PciVendorId, Identity.PciDeviceId,
                                           Identity.PciRevisionId, Identity.PciBus,
                                           Identity.PciDevice, Identity.PciFunction);
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciController);
    Status = UcxControllerCreate(Device, &Config, &Attributes, &Ucx);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxControllerCreate failed 0x%lx\n", Status);
        goto Fail;
    }

    Controller = new (XhciGetControllerContext(Ucx)) XhciController();

    /* From here the FDO cleanup owns the PCI interface reference */
    XhciGetFdoContext(Device)->Controller = Controller;

    Status = Controller->Initialize(Device, Ucx, &Identity);
    if (!NT_SUCCESS(Status))
        DPRINT1("Controller setup failed 0x%lx\n", Status);

    return Status;

Fail:
    if (Identity.PciInterface.InterfaceDereference != NULL)
        Identity.PciInterface.InterfaceDereference(Identity.PciInterface.Context);
    return Status;
}

VOID
XhciController::FdoCleanup()
{
    PBUS_INTERFACE_STANDARD Bus = &m_Identity.PciInterface;

    if (Bus->InterfaceDereference != NULL)
        Bus->InterfaceDereference(Bus->Context);

    RtlZeroMemory(Bus, sizeof(*Bus));
}

static
VOID
NTAPI
XhciEvtFdoCleanup(
    _In_ WDFOBJECT Object)
{
    XhciController* Controller = XhciGetFdoContext((WDFDEVICE)Object)->Controller;

    if (Controller != NULL)
        Controller->FdoCleanup();
}

/* PrepareHardware and ReleaseHardware ****************************************/

NTSTATUS
XhciController::PrepareHardware(
    _In_ WDFCMRESLIST Raw,
    _In_ WDFCMRESLIST Translated)
{
    NTSTATUS Status;

    Status = m_Registers.Prepare(this, Translated);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Register prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    Status = m_Buffers.Prepare();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Common buffer prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    Status = m_Interrupters.Prepare(Raw, Translated);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interrupter prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    Status = m_Slots.Prepare();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device slot prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    Status = m_Commands.Prepare();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Command ring prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    Status = m_RootHub.Prepare();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub prepare failed 0x%lx\n", Status);
        goto Fail;
    }

    return STATUS_SUCCESS;

Fail:
    /* Keeps PnP from restarting a controller that cannot start */
    DPRINT1("Controller %p failed to prepare hardware 0x%lx\n", this, Status);
    QueueRecovery(XHCI_RECOVER_REPORT_FAILED, 0x101C);
    return Status;
}

VOID
XhciController::ReleaseHardware()
{
    /* D0 exit did not run when D0 entry failed */
    WdfWorkItemFlush(m_RecoveryItem);
    WdfWorkItemFlush(m_IdleItem);

    m_RootHub.Release();
    m_Commands.Release();
    m_Slots.Release();
    m_Interrupters.Release();
    m_Buffers.Release();
    m_Registers.Release();
}

/* D0 entry *******************************************************************/

NTSTATUS
XhciController::RunD0Pass(
    _In_ BOOLEAN Restore)
{
    NTSTATUS Status;

    if (Restore)
    {
        m_Registers.ProgramSsicPortUnused(FALSE);
        m_Registers.RestoreVendorBits();
    }

    Status = m_Interrupters.D0Entry(Restore);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interrupter D0 entry failed 0x%lx\n", Status);
        return Status;
    }

    m_Slots.D0Entry();

    Status = m_Commands.D0Entry();
    if (!NT_SUCCESS(Status))
        DPRINT1("Command ring D0 entry failed 0x%lx\n", Status);

    return Status;
}

NTSTATUS
XhciController::D0Entry(
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    UCX_CONTROLLER_RESET_COMPLETE_INFO ResetInfo;
    BOOLEAN FromHibernate = (m_SystemAction == SystemAction::ResumeFromHibernate);
    BOOLEAN WasReset = FALSE;
    BOOLEAN AllOnes = FALSE;
    BOOLEAN Restore;
    NTSTATUS Status;

    if (m_Registers.ReadCapability32(XHCI_CAP_LENGTH_VERSION) == MAXULONG)
    {
        DPRINT1("Controller registers read all ones at D0 entry\n");
        AllOnes = TRUE;
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (FromHibernate)
    {
        /* Without a crash dump stack the firmware owned the controller during resume */
        Status = m_Registers.BiosHandoff();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("BIOS handoff on hibernate resume failed 0x%lx\n", Status);
            goto Exit;
        }

        m_Slots.DiscardState();

        Status = m_Registers.Reset();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Reset on hibernate resume failed 0x%lx\n", Status);
            goto Exit;
        }
        WasReset = TRUE;
    }

    if (!m_FrameSnapshotReady)
        InitFrameSnapshot();

    if (HasErrata(XhciErrata::AcpiDsmHsicSuspendDetach))
        EvaluateDsm(5);

    Restore = !HasErrata(XhciErrata::PwrSkipSaveRestore) &&
              !m_SaveFailed &&
              PreviousState != WdfPowerDeviceD3Final &&
              !FromHibernate;

    Status = RunD0Pass(Restore);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (PreviousState != WdfPowerDeviceD3Final && !FromHibernate)
    {
        if (Restore)
        {
            Status = m_Registers.RestoreState();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Controller state restore failed 0x%lx\n", Status);
                m_RestoreFailures++;
                VerifierCheck(XHCI_VERIFY_SAVE_RESTORE);
            }
        }

        if (!Restore || !NT_SUCCESS(Status))
        {
            m_Slots.DiscardState();

            Status = m_Registers.Reset();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Controller reset at D0 entry failed 0x%lx\n", Status);
                goto Exit;
            }
            WasReset = TRUE;

            Status = RunD0Pass(FALSE);
            if (!NT_SUCCESS(Status))
                goto Exit;
        }
    }

    m_Registers.ApplyD0Workarounds();
    m_RootHub.D0Entry();
    m_PowerState = WdfPowerDeviceD0;

    if (WasReset)
    {
        UCX_CONTROLLER_RESET_COMPLETE_INFO_INIT(&ResetInfo, UcxControllerStateLost, FALSE);
        UcxControllerResetComplete(m_Ucx, &ResetInfo);
    }

    Status = STATUS_SUCCESS;

Exit:
    if (NT_SUCCESS(Status))
    {
        if (m_SystemAction <= SystemAction::ResumeFromHibernate)
        {
            m_SystemAction = SystemAction::None;

            if (m_IdleStatus == IdleStatus::Configured && m_LastIdleTimeout != XHCI_IDLE_TIMEOUT_LONG_MS)
                SetIdleTimeout(XHCI_IDLE_TIMEOUT_LONG_MS);
        }

        return STATUS_SUCCESS;
    }

    m_Slots.DisableAll();
    if (AllOnes)
    {
        SetGone(TRUE);
    }
    else
    {
        QueueRecovery(0, 0x101C);
        SetGone(FALSE);
    }

    return Status;
}

NTSTATUS
XhciController::StartController()
{
    NTSTATUS Status;

    Status = m_Registers.Run();
    if (!NT_SUCCESS(Status))
        return Status;

    WdfTimerStart(m_Watchdog, WDF_REL_TIMEOUT_IN_MS(XHCI_WATCHDOG_PERIOD_MS));
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
XhciFirmwareCommandDone(
    _In_ XhciCommand* Command)
{
    UNREFERENCED_PARAMETER(Command);
}

/** Sends one vendor firmware query; TRUE with the completion parameter on success. */
static
BOOLEAN
NTAPI
XhciSendFirmwareCommand(
    _In_ XhciController* Controller,
    _In_ ULONG Type,
    _Out_ PULONG Parameter)
{
    XhciCommand Command;
    NTSTATUS Status;

    RtlZeroMemory(&Command, sizeof(Command));
    Command.Trb.Dword[3] = Type << XHCI_TRB_TYPE_SHIFT;
    Command.Done = XhciFirmwareCommandDone;
    Command.Context = Controller;
    Command.Internal = TRUE;

    Status = Controller->m_Commands.SubmitAndWait(&Command);
    if (!NT_SUCCESS(Status) || Command.Code != XhciCompletionCode::Success)
    {
        DPRINT1("Firmware version command %lu failed, status 0x%lx code %lu\n",
                Type, Status, static_cast<ULONG>(Command.Code));
        *Parameter = 0;
        return FALSE;
    }

    *Parameter = Command.Completion.Dword[2] & 0x00FFFFFF;
    return TRUE;
}

VOID
XhciController::RunFirmwareCommands()
{
    BOOLEAN Pci = (m_Identity.ParentBus != UcxControllerParentBusTypeAcpi);
    BOOLEAN Renesas = Pci && (m_Identity.PciVendorId == 0x1033 || m_Identity.PciVendorId == 0x1912);
    BOOLEAN Asmedia = Pci && m_Identity.PciVendorId == 0x1B21;
    BOOLEAN Nvidia;
    ULONG Parameter;
    ULONG64 Version;

    Nvidia = !Pci && _strnicmp(m_Identity.AcpiVendorId, "NVDA", 4) == 0;
    if (!Renesas && !Asmedia && !Nvidia)
        return;

    m_Identity.FirmwareVersion = XHCI_FIRMWARE_UNKNOWN;

    if (Asmedia)
    {
        if (!XhciSendFirmwareCommand(this, static_cast<ULONG>(XhciTrbType::VendorFirmwareVersionAltSetup), &Parameter))
            return;

        Version = Parameter;
        if (!XhciSendFirmwareCommand(this, static_cast<ULONG>(XhciTrbType::VendorFirmwareVersionAlt), &Parameter))
            return;

        m_Identity.FirmwareVersion = Version | ((ULONG64)Parameter << 24);
    }
    else
    {
        if (!XhciSendFirmwareCommand(this, static_cast<ULONG>(XhciTrbType::VendorFirmwareVersion), &Parameter))
            return;

        m_Identity.FirmwareVersion = Parameter & 0xFFFF;
    }

    DPRINT1("Controller firmware version 0x%I64x\n", m_Identity.FirmwareVersion);
}

NTSTATUS
XhciController::D0EntryPostInterruptsEnabled(
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    NTSTATUS Status;

    Status = StartController();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller start failed 0x%lx\n", Status);
        return Status;
    }

    m_RootHub.PostInterruptsEntry();
    m_Commands.PostInterruptsEntry();

    if (PreviousState == WdfPowerDeviceD3Final)
        RunFirmwareCommands();

    if (HasErrata(XhciErrata::HostRefuseToStart))
    {
        DPRINT1("Controller is marked as not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

/* D0 exit ********************************************************************/

VOID
XhciController::D0ExitPreInterruptsDisabled()
{
    if (HasErrata(XhciErrata::AcpiNotifyBeforeIntOff))
        EvaluatePrmi();

    m_Interrupters.D0ExitPreInterruptsDisabled();
}

VOID
XhciController::D0Exit(
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    POWER_ACTION Action = WdfDeviceGetSystemPowerAction(m_Device);
    BOOLEAN Save;
    NTSTATUS Status;

    m_PowerState = TargetState;

    m_Buffers.D0Exit();
    m_RootHub.D0Exit();

    WdfTimerStop(m_Watchdog, TRUE);
    Status = m_Registers.Stop();
    if (!NT_SUCCESS(Status))
        DPRINT1("Controller stop at D0 exit failed 0x%lx\n", Status);

    m_Interrupters.D0Exit();

    Save = !HasErrata(XhciErrata::PwrSkipSaveRestore) &&
           !m_SaveFailed &&
           TargetState != WdfPowerDeviceD3Final &&
           m_SystemAction != SystemAction::Hibernate;
    if (Save)
    {
        Status = m_Registers.SaveState();
        if (!NT_SUCCESS(Status))
        {
            /* QUIRK: one failure turns save and restore off for good */
            DPRINT("Controller state save failed 0x%lx\n", Status);
            m_SaveFailed = TRUE;
            VerifierCheck(XHCI_VERIFY_SAVE_RESTORE);
        }

        m_Registers.ProgramSsicPortUnused(TRUE);
        m_Registers.SaveVendorBits();

        if (HasErrata(XhciErrata::AcpiDsmHsicSuspendDetach))
            EvaluateDsm(6);
    }

    if (HasErrata(XhciErrata::PwrResetBeforeShutdown) && Action == PowerActionShutdownReset)
    {
        Status = m_Registers.ResetController(TRUE);
        if (!NT_SUCCESS(Status))
            DPRINT("Reset before restart failed 0x%lx\n", Status);
    }

    WdfWorkItemFlush(m_RecoveryItem);
    WdfWorkItemFlush(m_IdleItem);
}

/* Self managed I/O and usage *************************************************/

VOID
XhciController::SelfManagedIoInit()
{
    PDEVICE_OBJECT Pdo = WdfDeviceWdmGetPhysicalDevice(m_Device);

    if (Pdo->Flags & XHCI_DEVICE_USED_BY_DEBUGGER)
        DPRINT1("Controller shared with the kernel debugger; low power epoch tracking unavailable\n");

    DPRINT("Friendly name not set\n");

    ReadVerifierFlags();
    QueryErrataDatabase(FALSE);

    if (HasErrata(XhciErrata::XferPacketAlignChunks) && m_SplitPadBuffer == NULL)
    {
        m_SplitPadBuffer = m_Buffers.Acquire(XHCI_PAD_BUFFER_SIZE);
        if (m_SplitPadBuffer == NULL)
            DPRINT1("No split transfer pad buffer, transfers run without it\n");
    }
}

VOID
XhciController::UpdateRegistryCounters()
{
    DECLARE_CONST_UNICODE_STRING(RestoreName, L"HCRestoreStateFailureCount");
    DECLARE_CONST_UNICODE_STRING(RecoveryName, L"HCRecoveryCount");
    PCUNICODE_STRING Names[2] = { &RestoreName, &RecoveryName };
    ULONG Counts[2] = { m_RestoreFailures, m_RecoveryCount };
    WDFKEY Key;
    ULONG Value;
    ULONG Index;
    NTSTATUS Status;

    Status = WdfDeviceOpenRegistryKey(m_Device, PLUGPLAY_REGKEY_DEVICE, KEY_READ | KEY_WRITE,
                                      WDF_NO_OBJECT_ATTRIBUTES, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device key open for the recovery counters failed 0x%lx\n", Status);
        return;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(Names); Index++)
    {
        if (!NT_SUCCESS(WdfRegistryQueryULong(Key, Names[Index], &Value)))
            Value = 0;

        Status = WdfRegistryAssignULong(Key, Names[Index], Value + Counts[Index]);
        if (!NT_SUCCESS(Status))
            DPRINT1("Writing %wZ failed 0x%lx\n", Names[Index], Status);
    }

    WdfRegistryClose(Key);
}

VOID
XhciController::SelfManagedIoCleanup()
{
    if (m_SplitPadBuffer != NULL)
    {
        m_Buffers.Free(m_SplitPadBuffer);
        m_SplitPadBuffer = NULL;
    }

    UpdateRegistryCounters();
}

VOID
XhciController::UsageNotification(
    _In_ WDF_SPECIAL_FILE_TYPE Type,
    _In_ BOOLEAN InPath)
{
    /* No pageable code in this driver, so there is nothing to lock down */
    if (InPath)
        m_SpecialFiles |= 1UL << Type;
    else
        m_SpecialFiles &= ~(1UL << Type);
}

VOID
XhciController::TrackSystemPowerIrp(
    _In_ PIO_STACK_LOCATION Stack)
{
    SYSTEM_POWER_STATE_CONTEXT Context = Stack->Parameters.Power.SystemPowerStateContext;

    switch (Stack->Parameters.Power.State.SystemState)
    {
        case PowerSystemWorking:
            m_SystemAction = (Context.CurrentSystemState == PowerSystemHibernate) ?
                             SystemAction::ResumeFromHibernate : SystemAction::ResumeFromSleep;
            break;

        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:
            m_SystemAction = (Context.EffectiveSystemState == PowerSystemHibernate) ?
                             SystemAction::HybridSleep : SystemAction::Sleep;
            break;

        case PowerSystemHibernate:
            m_SystemAction = SystemAction::Hibernate;
            break;

        case PowerSystemShutdown:
            m_SystemAction = (Context.EffectiveSystemState == PowerSystemHibernate) ?
                             SystemAction::Hibernate : SystemAction::Shutdown;
            break;

        default:
            DPRINT1("Unexpected system power state %d\n", Stack->Parameters.Power.State.SystemState);
            break;
    }
}

/* Frame number ***************************************************************/

ULONG64
XhciController::NowMs() const
{
    LARGE_INTEGER Ticks;

    KeQueryTickCount(&Ticks);
    return (ULONG64)Ticks.QuadPart * m_TimeIncrement / 10000;
}

/* Snapshot layout: bits 20:0 are frame bits 31:11, bits 63:21 the time in ms of frame xxx000 */
static
LONG64
NTAPI
XhciPackFrameSnapshot(
    _In_ ULONG Frame,
    _In_ ULONG64 Now)
{
    ULONG64 Base = Now - (Frame & 0x7FF);

    return (LONG64)((Base << 21) | ((Frame >> 11) & 0x1FFFFF));
}

VOID
XhciController::InitFrameSnapshot()
{
    InterlockedExchange64(&m_FrameSnapshot, XhciPackFrameSnapshot(m_Registers.CurrentFrame(1), NowMs()));
    m_FrameSnapshotReady = TRUE;
}

ULONG
XhciController::GetFrameNumber(
    _In_ ULONG Increment)
{
    ULONG64 Now = NowMs();
    ULONG64 Snapshot = (ULONG64)InterlockedCompareExchange64(&m_FrameSnapshot, 0, 0);
    ULONG Hardware;
    ULONG Estimate;
    ULONG Result;
    ULONG Low;

    if (!m_FrameSnapshotReady)
        return 0;

    Estimate = (ULONG)((Snapshot & 0x1FFFFF) << 11) + (ULONG)(Now - (Snapshot >> 21));

    /* MFINDEX is meaningless outside D0, so run on the clock and leave the snapshot alone */
    if (m_PowerState != WdfPowerDeviceD0 || !IsAccessible())
        return Estimate;

    Hardware = m_Registers.CurrentFrame(Increment);
    Result = (Estimate & ~0x7FFUL) | Hardware;
    Low = Estimate & 0x7FF;

    /* The estimate decides the high bits; correct when it sits across a wrap */
    if (Low < Hardware && Hardware - Low > 0x400)
        Result -= 0x800;
    else if (Low >= Hardware && Low - Hardware > 0x400)
        Result += 0x800;

    InterlockedExchange64(&m_FrameSnapshot, XhciPackFrameSnapshot(Result, Now));
    return Result;
}

/* Watchdog *******************************************************************/

VOID
XhciController::WatchdogTick()
{
    ULONG Usbsts;

    if (!IsAccessible())
        return;

    Usbsts = m_Registers.ReadOperational32(XHCI_OP_USBSTS);
    if (Usbsts == MAXULONG)
    {
        DPRINT1("Watchdog read all ones, controller gone\n");
        SetGone(TRUE);
        return;
    }

    /* QUIRK: neither error is cleared here, so each tick reports it again */
    if (Usbsts & XHCI_USBSTS_HSE)
    {
        DPRINT1("Host System Error, USBSTS 0x%08lx\n", Usbsts);
        VerifierCheck(XHCI_VERIFY_HSE);
        QueueRecovery(XHCI_RECOVER_RESET, 0x1001);
        return;
    }

    if (Usbsts & XHCI_USBSTS_HCE)
    {
        DPRINT1("Host Controller Error, USBSTS 0x%08lx\n", Usbsts);
        VerifierCheck(XHCI_VERIFY_HCE);
        QueueRecovery(XHCI_RECOVER_RESET, 0x1002);
        return;
    }

    if (InterlockedIncrement(&m_ResetInterval) >= XHCI_RESET_WINDOW_TICKS)
    {
        InterlockedExchange(&m_ResetInterval, 0);
        InterlockedExchange(&m_ResetCount, 0);
    }

    m_Buffers.Rebalance();
}

/* Recovery *******************************************************************/

VOID
XhciController::RaiseControllerFault(
    _In_ XhciRecovery Action,
    _In_ ULONG Reason)
{
    ULONG Actions;

    switch (Action)
    {
        case XhciRecovery::Ignore:
            Actions = 0;
            break;
        case XhciRecovery::ResetHost:
            Actions = XHCI_RECOVER_RESET;
            break;
        case XhciRecovery::InternalReset:
            Actions = XHCI_RECOVER_INTERNAL_RESET;
            break;
        case XhciRecovery::Gone:
            DPRINT1("Controller gone, reason 0x%lx\n", Reason);
            SetGone(TRUE);
            return;
        case XhciRecovery::Disable:
        case XhciRecovery::HoldStopped:
        default:
            Actions = XHCI_RECOVER_NO_RESTART;
            break;
    }

    QueueRecovery(Actions, Reason);
}

VOID
XhciController::MarkGone()
{
    SetGone(TRUE);
}

VOID
XhciController::SetGone(
    _In_ BOOLEAN ReportToPnp)
{
    if (ReportToPnp && !m_Gone)
    {
        DPRINT1("Controller %p registers read all ones, marking it gone\n", this);
        VerifierCheck(XHCI_VERIFY_ALL_ONES);
    }

    m_Gone = TRUE;

    if (ReportToPnp)
        QueueRecovery(XHCI_RECOVER_GONE_PNP, 0);
    else
        QueueRecovery(XHCI_RECOVER_GONE, 0x100D);
}

VOID
XhciController::QueueRecovery(
    _In_ ULONG Actions,
    _In_ ULONG Reason)
{
    KIRQL Irql;

    if (m_SpecialFiles & ((1UL << WdfSpecialFilePaging) | (1UL << WdfSpecialFileBoot)))
    {
        BootRecovery(Actions);
        return;
    }

    if (Reason != 0)
        DPRINT1("Controller %p recovery 0x%lx requested, reason 0x%lx\n", this, Actions, Reason);

    KeAcquireSpinLock(&m_RecoveryLock, &Irql);
    m_RecoveryActions |= Actions;
    KeReleaseSpinLock(&m_RecoveryLock, Irql);

    WdfWorkItemEnqueue(m_RecoveryItem);
}

VOID
XhciController::BootRecovery(
    _In_ ULONG Actions)
{
    static LONG InlineResets;

    /* The branches are exclusive even when several bits are set */
    if (Actions & (XHCI_RECOVER_GONE | XHCI_RECOVER_GONE_PNP | XHCI_RECOVER_NO_RESTART))
    {
        KeBugCheckEx(XHCI_BUGCHECK_USB3_DRIVER, 2, (ULONG_PTR)WdfDeviceWdmGetPhysicalDevice(m_Device), 3, 0);
    }
    else if (Actions & XHCI_RECOVER_INTERNAL_RESET)
    {
        if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        {
            UcxControllerNeedsReset(m_Ucx);
            return;
        }

        /* QUIRK: one counter shared by every controller */
        if (InterlockedIncrement(&InlineResets) != 1)
            DPRINT1("Boot path internal resets overlap\n");
        InternalReset();
        InterlockedDecrement(&InlineResets);
    }
    else if (Actions & XHCI_RECOVER_RESET)
    {
        UcxControllerNeedsReset(m_Ucx);
    }
    else
    {
        DPRINT1("Boot path recovery 0x%lx has no handler\n", Actions);
    }
}

VOID
XhciController::DisableController()
{
    PBUS_INTERFACE_STANDARD Bus = &m_Identity.PciInterface;
    USHORT Command = 0;

    SetGone(FALSE);

    /* Turns off bus mastering and MMIO decode */
    if (m_Identity.ParentBus != UcxControllerParentBusTypeAcpi && Bus->SetBusData != NULL)
        Bus->SetBusData(Bus->Context, PCI_WHICHSPACE_CONFIG, &Command, 4, sizeof(Command));
    else
        DPRINT1("Controller is not on PCI, cannot disable it\n");

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        KeFlushQueuedDpcs();
}

VOID
XhciController::RunRecovery(
    _In_ ULONG Actions)
{
    WDF_DEVICE_STATE State;

    if ((Actions & XHCI_RECOVER_RESET) && m_ResetCount > XHCI_RESET_LIMIT)
    {
        DPRINT1("Controller reset %ld times in 5 minutes, giving up\n", m_ResetCount);
        Actions |= XHCI_RECOVER_NO_RESTART;
    }

    while (Actions != 0)
    {
        if (Actions & (XHCI_RECOVER_GONE | XHCI_RECOVER_GONE_PNP))
        {
            if (!m_GoneHandled)
            {
                m_GoneHandled = TRUE;
                m_Commands.OnHostLost();
                KeFlushQueuedDpcs();
                m_Buffers.OnHostLost();
                m_Slots.OnHostLost();
                m_RootHub.OnHostLost();
                KeFlushQueuedDpcs();
                UcxControllerSetFailed(m_Ucx);

                if (Actions & XHCI_RECOVER_GONE)
                    WdfDeviceSetFailed(m_Device, WdfDeviceFailedNoRestart);
            }

            if ((Actions & XHCI_RECOVER_GONE_PNP) && !m_PnpToldRemoved)
            {
                WDF_DEVICE_STATE_INIT(&State);
                State.Removed = WdfTrue;
                WdfDeviceSetDeviceState(m_Device, &State);
                m_PnpToldRemoved = TRUE;
            }

            Actions &= ~(XHCI_RECOVER_GONE | XHCI_RECOVER_GONE_PNP | XHCI_RECOVER_INTERNAL_RESET |
                         XHCI_RECOVER_RESET | XHCI_RECOVER_NO_RESTART);
        }
        else if (Actions & XHCI_RECOVER_NO_RESTART)
        {
            DisableController();
            WdfDeviceSetFailed(m_Device, WdfDeviceFailedNoRestart);
            Actions &= ~(XHCI_RECOVER_INTERNAL_RESET | XHCI_RECOVER_RESET | XHCI_RECOVER_NO_RESTART);
        }
        else if (Actions & XHCI_RECOVER_INTERNAL_RESET)
        {
            InternalReset();
            Actions &= ~XHCI_RECOVER_INTERNAL_RESET;
        }
        else if (Actions & XHCI_RECOVER_RESET)
        {
            UcxControllerNeedsReset(m_Ucx);
            Actions &= ~XHCI_RECOVER_RESET;
        }
        else if (Actions & XHCI_RECOVER_REPORT_FAILED)
        {
            WdfDeviceSetFailed(m_Device, WdfDeviceFailedNoRestart);
            Actions &= ~XHCI_RECOVER_REPORT_FAILED;
        }
        else
        {
            DPRINT1("Recovery actions 0x%lx not understood\n", Actions);
            break;
        }
    }
}

VOID
XhciController::RecoveryWorker()
{
    ULONG Actions;
    KIRQL Irql;

    KeWaitForSingleObject(&m_RecoveryMutex, Executive, KernelMode, FALSE, NULL);

    KeAcquireSpinLock(&m_RecoveryLock, &Irql);
    Actions = m_RecoveryActions;
    m_RecoveryActions = 0;
    KeReleaseSpinLock(&m_RecoveryLock, Irql);

    RunRecovery(Actions);

    KeReleaseMutex(&m_RecoveryMutex, FALSE);
}

VOID
XhciController::InternalReset()
{
    NTSTATUS StopStatus;
    NTSTATUS ResetStatus = STATUS_SUCCESS;
    NTSTATUS Status;

    /* QUIRK: overlap is only reported */
    if (InterlockedIncrement(&m_ResetLock) != 1)
        DPRINT1("Controller %p internal reset entered twice\n", this);

    InterlockedIncrement(&m_ResetCount);
    m_RecoveryCount++;

    WdfTimerStop(m_Watchdog, TRUE);
    m_Commands.PreReset();
    m_Slots.PreReset();

    KeFlushQueuedDpcs();
    StopStatus = m_Registers.Stop();
    KeFlushQueuedDpcs();

    if (!NT_SUCCESS(StopStatus))
    {
        DPRINT1("Stop before internal reset failed 0x%lx\n", StopStatus);
        DisableController();
    }
    else
    {
        ResetStatus = m_Registers.ResetController(TRUE);
    }

    m_Commands.PostReset();
    m_Buffers.PostResetFlush();
    m_Slots.PostReset();
    m_RootHub.PostReset();

    if (!NT_SUCCESS(StopStatus))
    {
        VerifierCheck(XHCI_VERIFY_STOP_TIMEOUT);
        QueueRecovery(XHCI_RECOVER_NO_RESTART, 0x100B);
        Status = StopStatus;
    }
    else
    {
        m_Interrupters.PostReset();

        if (!NT_SUCCESS(ResetStatus))
        {
            DPRINT1("Internal reset failed 0x%lx\n", ResetStatus);
            VerifierCheck(XHCI_VERIFY_RESET_TIMEOUT);
            QueueRecovery(XHCI_RECOVER_NO_RESTART, 0x1007);
            Status = ResetStatus;
        }
        else
        {
            Status = StartController();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Start after internal reset failed 0x%lx\n", Status);
                VerifierCheck(XHCI_VERIFY_START_TIMEOUT);
                QueueRecovery(XHCI_RECOVER_NO_RESTART, 0x1008);
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        m_Commands.PostResetSuccess();
    }
    else
    {
        DPRINT1("Controller %p internal reset ended with 0x%lx\n", this, Status);
        m_Commands.FailAll();
        UcxControllerSetFailed(m_Ucx);
    }

    InterlockedDecrement(&m_ResetLock);
}

VOID
XhciController::UcxReset()
{
    UCX_CONTROLLER_RESET_COMPLETE_INFO Info;

    if (IsAccessible())
        InternalReset();
    else
        DPRINT1("UCX reset of an inaccessible controller skipped\n");

    /* QUIRK: reported even after a failed reset; SetFailed already told UCX */
    UCX_CONTROLLER_RESET_COMPLETE_INFO_INIT(&Info, UcxControllerStateLost, TRUE);
    UcxControllerResetComplete(m_Ucx, &Info);
}

/* UCX capability query *******************************************************/

NTSTATUS
XhciController::QueryUsbCapability(
    _In_ const GUID* Capability,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ResultLength)
{
    ULONG Hcc2;

    *ResultLength = 0;

    if (IsEqualGUID(*Capability, XhciGuidChainedMdls))
        return HasErrata(XhciErrata::XferFlattenMdlChain) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;

    if (IsEqualGUID(*Capability, XhciGuidStaticStreams))
    {
        *ResultLength = sizeof(USHORT);
        if (OutputBufferLength < sizeof(USHORT))
        {
            DPRINT1("Stream capability buffer of %lu bytes too small\n", OutputBufferLength);
            return STATUS_BUFFER_TOO_SMALL;
        }
        if (OutputBuffer == NULL)
        {
            DPRINT1("Stream capability buffer missing\n");
            return STATUS_INVALID_PARAMETER;
        }

        *(PUSHORT)OutputBuffer = (USHORT)m_Registers.SupportedStreams();
        return STATUS_SUCCESS;
    }

    if (IsEqualGUID(*Capability, XhciGuidFunctionSuspend) ||
        IsEqualGUID(*Capability, XhciGuidSelectiveSuspend))
    {
        return STATUS_SUCCESS;
    }

    if (IsEqualGUID(*Capability, XhciGuidClearTtBufferOnCancel))
        return HasErrata(XhciErrata::XferFlushTtOnAbort) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

    if (IsEqualGUID(*Capability, XhciGuidHighBandwidthIsoch))
    {
        *ResultLength = sizeof(ULONG);
        if (OutputBufferLength < sizeof(ULONG))
        {
            DPRINT1("Isoch capability buffer of %lu bytes too small\n", OutputBufferLength);
            return STATUS_BUFFER_TOO_SMALL;
        }
        if (OutputBuffer == NULL)
        {
            DPRINT1("Isoch capability buffer missing\n");
            return STATUS_INVALID_PARAMETER;
        }

        Hcc2 = m_Registers.Hccparams2();
        if (!(Hcc2 & XHCI_HCC2_LEC))
            return STATUS_NOT_SUPPORTED;

        *(PULONG)OutputBuffer = (Hcc2 & XHCI_HCC2_ETC) ? 32 : 4;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

/* KMDF callbacks *************************************************************/

static
NTSTATUS
NTAPI
XhciEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    return XhciController::FromDevice(Device)->PrepareHardware(ResourcesRaw, ResourcesTranslated);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    XhciController::FromDevice(Device)->ReleaseHardware();
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    return XhciController::FromDevice(Device)->D0Entry(PreviousState);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceD0EntryPostInterruptsEnabled(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    return XhciController::FromDevice(Device)->D0EntryPostInterruptsEnabled(PreviousState);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceD0ExitPreInterruptsDisabled(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    UNREFERENCED_PARAMETER(TargetState);

    XhciController::FromDevice(Device)->D0ExitPreInterruptsDisabled();
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    XhciController::FromDevice(Device)->D0Exit(TargetState);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciEvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->SelfManagedIoInit();
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
XhciEvtDeviceSelfManagedIoCleanup(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->SelfManagedIoCleanup();
}

static
VOID
NTAPI
XhciEvtDeviceUsageNotification(
    _In_ WDFDEVICE Device,
    _In_ WDF_SPECIAL_FILE_TYPE NotificationType,
    _In_ BOOLEAN IsInNotificationPath)
{
    XhciController::FromDevice(Device)->UsageNotification(NotificationType, IsInNotificationPath);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceArmWakeFromS0(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->SetPortWake(TRUE);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
XhciEvtDeviceDisarmWakeFromS0(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->SetPortWake(FALSE);
}

static
VOID
NTAPI
XhciEvtDeviceWakeFromS0Triggered(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->m_RootHub.SignalPortChange();
}

static
NTSTATUS
NTAPI
XhciEvtDeviceArmWakeFromSx(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->m_WaitWakeQueued = TRUE;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
XhciEvtDeviceDisarmWakeFromSx(
    _In_ WDFDEVICE Device)
{
    XhciController::FromDevice(Device)->m_WaitWakeQueued = FALSE;
}

static
NTSTATUS
NTAPI
XhciEvtDeviceFilterRemoveResourceRequirements(
    _In_ WDFDEVICE Device,
    _In_ WDFIORESREQLIST IoResourceRequirementsList)
{
    XhciController* Controller = XhciController::FromDevice(Device);

    if (Controller == NULL)
        return STATUS_SUCCESS;

    return Controller->m_Interrupters.FilterResourceRequirements(IoResourceRequirementsList);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceFilterAddResourceRequirements(
    _In_ WDFDEVICE Device,
    _In_ WDFIORESREQLIST IoResourceRequirementsList)
{
    XhciController* Controller = XhciController::FromDevice(Device);

    if (Controller == NULL)
        return STATUS_SUCCESS;

    return Controller->m_Interrupters.AffinitizeResourceRequirements(IoResourceRequirementsList);
}

static
NTSTATUS
NTAPI
XhciEvtDeviceRemoveAddedResources(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciEvtPreprocessSetPower(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    XhciController* Controller = XhciController::FromDevice(Device);
    NTSTATUS Status;

    if (Controller != NULL && Stack->Parameters.Power.Type == SystemPowerState)
        Controller->TrackSystemPowerIrp(Stack);

    IoSkipCurrentIrpStackLocation(Irp);
    Status = WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
    if (!NT_SUCCESS(Status))
        DPRINT1("Set power IRP dispatch returned 0x%lx\n", Status);

    return Status;
}

static
NTSTATUS
NTAPI
XhciEvtPreprocessInternalIoctl(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    XhciController* Controller = XhciController::FromDevice(Device);

    if (Controller != NULL && Stack->Parameters.DeviceIoControl.IoControlCode == XHCI_TEST_RESET_IOCTL)
    {
        Controller->RaiseControllerFault(XhciRecovery::ResetHost, 0x103F);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

static
VOID
NTAPI
XhciEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    WDFDEVICE Device = WdfIoQueueGetDevice(Queue);

    if (UcxIoDeviceControl(Device, Request, OutputBufferLength, InputBufferLength, IoControlCode))
        return;

    if (WdfRequestGetRequestorMode(Request) != UserMode)
    {
        DPRINT1("Kernel mode IOCTL 0x%lx on the controller FDO\n", IoControlCode);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    DPRINT1("Unsupported user IOCTL 0x%lx\n", IoControlCode);
    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

static
VOID
NTAPI
XhciEvtWatchdog(
    _In_ WDFTIMER Timer)
{
    XhciGetObjectContext(Timer)->Controller->WatchdogTick();
}

static
VOID
NTAPI
XhciEvtRecoveryWork(
    _In_ WDFWORKITEM WorkItem)
{
    XhciGetObjectContext(WorkItem)->Controller->RecoveryWorker();
}

static
VOID
NTAPI
XhciEvtIdleTimeoutWork(
    _In_ WDFWORKITEM WorkItem)
{
    XhciGetObjectContext(WorkItem)->Controller->IdleTimeoutWorker();
}

/* UCX controller callbacks ***************************************************/

static
NTSTATUS
NTAPI
XhciEvtControllerQueryUsbCapability(
    _In_ UCXCONTROLLER UcxController,
    _In_ PGUID CapabilityType,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ResultLength)
{
    return XhciController::FromUcx(UcxController)->QueryUsbCapability(CapabilityType, OutputBufferLength,
                                                                      OutputBuffer, ResultLength);
}

static
NTSTATUS
NTAPI
XhciEvtControllerGetCurrentFrameNumber(
    _In_ UCXCONTROLLER UcxController,
    _Out_ PULONG FrameNumber)
{
    /* Outside D0 this returns the clock based estimate, since UCX caches the value */
    *FrameNumber = XhciController::FromUcx(UcxController)->GetFrameNumber(1);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
XhciEvtControllerReset(
    _In_ UCXCONTROLLER UcxController)
{
    XhciController::FromUcx(UcxController)->UcxReset();
}

static
NTSTATUS
NTAPI
XhciEvtControllerEnableForwardProgress(
    _In_ UCXCONTROLLER UcxController)
{
    XhciController::FromUcx(UcxController)->m_ReservedIoArmed = TRUE;
    return STATUS_SUCCESS;
}
