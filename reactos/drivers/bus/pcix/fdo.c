/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/fdo.c
 * PURPOSE:         FDO Device Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

SINGLE_LIST_ENTRY PciFdoExtensionListHead;
BOOLEAN PciBreakOnDefault;

PCI_MN_DISPATCH_TABLE PciFdoDispatchPowerTable[] =
{
    {IRP_DISPATCH, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported}
};

PCI_MN_DISPATCH_TABLE PciFdoDispatchPnpTable[] =
{
    {IRP_UPWARD,   PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DISPATCH, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DISPATCH, PciIrpNotSupported},
    {IRP_UPWARD,   PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_UPWARD,   PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported},
    {IRP_DOWNWARD, PciIrpNotSupported}
};

PCI_MJ_DISPATCH_TABLE PciFdoDispatchTable =
{
    IRP_MN_QUERY_LEGACY_BUS_INFORMATION,
    PciFdoDispatchPnpTable,
    IRP_MN_QUERY_POWER,
    PciFdoDispatchPowerTable,
    IRP_DOWNWARD,
    PciIrpNotSupported,
    IRP_DOWNWARD,
    PciIrpNotSupported
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PciGetHotPlugParameters(IN PPCI_FDO_EXTENSION FdoExtension)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ULONG Length;
    NTSTATUS Status;
    PAGED_CODE();

    /* We should receive 4 parameters, per the HPP specification */
    Length = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 4 * sizeof(ACPI_METHOD_ARGUMENT);

    /* Allocate the buffer to hold the parameters */
    OutputBuffer = ExAllocatePoolWithTag(PagedPool, Length, PCI_POOL_TAG);
    if (!OutputBuffer) return;

    /* Initialize the output and input buffers. The method is _HPP */
    RtlZeroMemory(OutputBuffer, Length);
    *(PULONG)InputBuffer.MethodName = 'PPH_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    do
    {
        /* Send the IOCTL to the ACPI driver */
        Status = PciSendIoctl(FdoExtension->PhysicalDeviceObject,
                              IOCTL_ACPI_EVAL_METHOD,
                              &InputBuffer,
                              sizeof(InputBuffer),
                              OutputBuffer,
                              Length);
        if (!NT_SUCCESS(Status))
        {
            /* The method failed, check if we can salvage data from parent */
            if (!PCI_IS_ROOT_FDO(FdoExtension))
            {
                /* Copy the root bus' hot plug parameters */
                FdoExtension->HotPlugParameters = FdoExtension->ParentFdoExtension->HotPlugParameters;
            }

            /* Nothing more to do on this path */
            break;
        }

        /* ACPI sent back some data. 4 parameters are expected in the output */
        if (OutputBuffer->Count != 4) break;

        /* HotPlug PCI Support not yet implemented */
        UNIMPLEMENTED;
        while (TRUE);
    } while (FALSE);

    /* Free the buffer and return */
    ExFreePoolWithTag(OutputBuffer, 0);
}

VOID
NTAPI
PciInitializeFdoExtensionCommonFields(PPCI_FDO_EXTENSION FdoExtension,
                                      IN PDEVICE_OBJECT DeviceObject,
                                      IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    /* Initialize the extension */
    RtlZeroMemory(FdoExtension, sizeof(PCI_FDO_EXTENSION));

    /* Setup the common fields */
    FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    FdoExtension->FunctionalDeviceObject = DeviceObject;
    FdoExtension->ExtensionType = PciFdoExtensionType;
    FdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
    FdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    FdoExtension->IrpDispatchTable = &PciFdoDispatchTable;

    /* Initialize the extension locks */
    KeInitializeEvent(&FdoExtension->SecondaryExtLock, SynchronizationEvent, TRUE);
    KeInitializeEvent(&FdoExtension->ChildListLock, SynchronizationEvent, TRUE);

    /* Initialize the default state */
    PciInitializeState(FdoExtension);
}

NTSTATUS
NTAPI
PciAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PCM_RESOURCE_LIST Descriptor;
    PDEVICE_OBJECT AttachedTo;
    PPCI_FDO_EXTENSION FdoExtension;
    PPCI_FDO_EXTENSION ParentExtension;
    PDEVICE_OBJECT DeviceObject;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
    ULONG ResultLength;
    PAGED_CODE();
    DPRINT1("PCI - AddDevice (a new bus). PDO: %p (Driver: %wZ)\n",
            PhysicalDeviceObject, &PhysicalDeviceObject->DriverObject->DriverName);

    /* Zero out variables so failure path knows what to do */
    AttachedTo = NULL;
    do
    {
        /* Check if there's already a device extension for this bus */
        ParentExtension = PciFindParentPciFdoExtension(PhysicalDeviceObject,
                                                       &PciGlobalLock);
        if (ParentExtension)
        {
            /* More than one PCI bus, this is not expected yet */
            UNIMPLEMENTED;
            while (TRUE);
        }

        /* Create the FDO for the bus */
        Status = IoCreateDevice(DriverObject,
                                sizeof(PCI_FDO_EXTENSION),
                                NULL,
                                FILE_DEVICE_BUS_EXTENDER,
                                0,
                                0,
                                &DeviceObject);
        if (!NT_SUCCESS(Status)) break;

        /* Initialize the extension for the FDO */
        FdoExtension = DeviceObject->DeviceExtension;
        PciInitializeFdoExtensionCommonFields(DeviceObject->DeviceExtension,
                                              DeviceObject,
                                              PhysicalDeviceObject);

        /* Attach to the root PDO */
        Status = STATUS_NO_SUCH_DEVICE;
        AttachedTo = IoAttachDeviceToDeviceStack(DeviceObject,
                                                 PhysicalDeviceObject);
        ASSERT(AttachedTo != NULL);
        if (!AttachedTo) break;
        FdoExtension->AttachedDeviceObject = AttachedTo;
        if (ParentExtension)
        {
            /* More than one PCI bus, this is not expected yet */
            UNIMPLEMENTED;
            while (TRUE);
        }
        else
        {
            /* Query the boot configuration */
            Status = PciGetDeviceProperty(PhysicalDeviceObject,
                                          DevicePropertyBootConfiguration,
                                          (PVOID*)&Descriptor);
            if (!NT_SUCCESS(Status))
            {
                /* No configuration has been set */
                Descriptor = NULL;
            }
            else
            {
                /* Root PDO in ReactOS does not assign boot resources */
                UNIMPLEMENTED;
                while (TRUE);
            }

            if (Descriptor)
            {
                /* Root PDO in ReactOS does not assign boot resources */
                UNIMPLEMENTED;
                while (TRUE);
            }
            else
            {
                /* Default configuration isn't the normal path on Windows */
                if (PciBreakOnDefault)
                {
                    /* If a second bus is found and there's still no data, crash */
                    KeBugCheckEx(PCI_BUS_DRIVER_INTERNAL,
                                 0xDEAD0010u,
                                 (ULONG_PTR)DeviceObject,
                                 0,
                                 0);
                }

                /* Warn that a default configuration will be used, and set bus 0 */
                DPRINT1("PCI   Will use default configuration.\n");
                PciBreakOnDefault = TRUE;
                FdoExtension->BaseBus = 0;
            }

            /* This is the root bus */
            FdoExtension->BusRootFdoExtension = FdoExtension;
        }

        /* Get the HAL or ACPI Bus Handler Callbacks for Configuration Access */
        Status = PciGetConfigHandlers(FdoExtension);
        if (!NT_SUCCESS(Status)) break;

        /* Initialize all the supported PCI arbiters */
        Status = PciInitializeArbiters(FdoExtension);
        if (!NT_SUCCESS(Status)) break;

        /* This is a real FDO, insert it into the list */
        FdoExtension->Fake = FALSE;
        PciInsertEntryAtTail(&PciFdoExtensionListHead,
                             FdoExtension,
                             &PciGlobalLock);

        /* Open the device registry key so that we can query the errata flags */
        IoOpenDeviceRegistryKey(DeviceObject,
                                PLUGPLAY_REGKEY_DEVICE,
                                KEY_ALL_ACCESS,
                                &KeyHandle),

        /* Open the value that contains errata flags for this bus instance */
        RtlInitUnicodeString(&ValueName, L"HackFlags");
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(Buffer),
                                 &ResultLength);
        ZwClose(KeyHandle);
        if (NT_SUCCESS(Status))
        {
            /* Make sure the data is of expected type and size */
            if ((ValueInfo->Type == REG_DWORD) &&
                (ValueInfo->DataLength == sizeof(ULONG)))
            {
                /* Read the flags for this bus */
                FdoExtension->BusHackFlags = *(PULONG)&ValueInfo->Data;
            }
        }

        /* Query ACPI for PCI HotPlug Support */
        PciGetHotPlugParameters(FdoExtension);

        /* The Bus FDO is now initialized */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        DPRINT1("PCI Root FDO Added: %p %p\n", DeviceObject, FdoExtension);
        return STATUS_SUCCESS;
    } while (FALSE);

    /* This is the failure path */
    ASSERT(!NT_SUCCESS(Status));
    if (AttachedTo) IoDetachDevice(AttachedTo);
    if (DeviceObject) IoDeleteDevice(DeviceObject);
    return Status;
}

/* EOF */
