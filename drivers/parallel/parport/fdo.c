/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/fdo.c
 * PURPOSE:         FDO functions
 */

#include "parport.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
AddDeviceInternal(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT Pdo,
                  IN PULONG pLptPortNumber OPTIONAL,
                  OUT PDEVICE_OBJECT* pFdo OPTIONAL)
{
    PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT Fdo = NULL;
    WCHAR DeviceNameBuffer[32];
    UNICODE_STRING DeviceName;
    NTSTATUS Status;

    DPRINT("AddDeviceInternal()\n");

    ASSERT(DriverObject);
    ASSERT(Pdo);

    /* Create new device object */
    swprintf(DeviceNameBuffer,
             L"\\Device\\ParallelPort%lu",
             IoGetConfigurationInformation()->ParallelCount);
    RtlInitUnicodeString(&DeviceName,
                         DeviceNameBuffer);

    Status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_PARALLEL_PORT,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed (Status 0x%08lx)\n", Status);
        Fdo = NULL;
        goto done;
    }

    DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
    RtlZeroMemory(DeviceExtension,
                  sizeof(FDO_DEVICE_EXTENSION));

    DeviceExtension->Common.IsFDO = TRUE;
    DeviceExtension->Common.PnpState = dsStopped;

    DeviceExtension->PortNumber = IoGetConfigurationInformation()->ParallelCount++;
    DeviceExtension->Pdo = Pdo;

    Status = IoAttachDeviceToDeviceStackSafe(Fdo,
                                             Pdo,
                                             &DeviceExtension->LowerDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoAttachDeviceToDeviceStackSafe() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (DeviceExtension->LowerDevice->Flags & DO_POWER_PAGABLE)
        Fdo->Flags |= DO_POWER_PAGABLE;

    if (DeviceExtension->LowerDevice->Flags & DO_BUFFERED_IO)
        Fdo->Flags |= DO_BUFFERED_IO;

    if (DeviceExtension->LowerDevice->Flags & DO_DIRECT_IO)
        Fdo->Flags |= DO_DIRECT_IO;

    /* Choose default strategy */
    if ((Fdo->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO)) == 0)
        Fdo->Flags |= DO_BUFFERED_IO;

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    if (pFdo)
    {
        *pFdo = Fdo;
    }

    return STATUS_SUCCESS;

done:
    if (Fdo)
    {
        IoDeleteDevice(Fdo);
    }

    return Status;
}


NTSTATUS
NTAPI
FdoStartDevice(IN PDEVICE_OBJECT DeviceObject,
               IN PCM_RESOURCE_LIST ResourceList,
               IN PCM_RESOURCE_LIST ResourceListTranslated)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    ULONG i;
//    ULONG Vector = 0;
//    KIRQL Dirql = 0;
//    KAFFINITY Affinity = 0;
//    KINTERRUPT_MODE InterruptMode = Latched;
//    BOOLEAN ShareInterrupt = TRUE;

    DPRINT("FdoStartDevice ()\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    ASSERT(DeviceExtension);
    ASSERT(DeviceExtension->Common.IsFDO == TRUE);

    if (!ResourceList)
    {
        DPRINT1("No allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ResourceList->Count != 1)
    {
        DPRINT1("Wrong number of allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((ResourceList->List[0].PartialResourceList.Version != 1) ||
        (ResourceList->List[0].PartialResourceList.Revision != 1) ||
        (ResourceListTranslated->List[0].PartialResourceList.Version != 1) ||
        (ResourceListTranslated->List[0].PartialResourceList.Revision != 1))
    {
        DPRINT1("Revision mismatch: %u.%u != 1.1 or %u.%u != 1.1\n",
                ResourceList->List[0].PartialResourceList.Version,
                ResourceList->List[0].PartialResourceList.Revision,
                ResourceListTranslated->List[0].PartialResourceList.Version,
                ResourceListTranslated->List[0].PartialResourceList.Revision);
        return STATUS_REVISION_MISMATCH;
    }

    DeviceExtension->BaseAddress = 0;

    for (i = 0; i < ResourceList->List[0].PartialResourceList.Count; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptorTranslated = &ResourceListTranslated->List[0].PartialResourceList.PartialDescriptors[i];

        switch (PartialDescriptor->Type)
        {
            case CmResourceTypePort:
                DPRINT("Port: BaseAddress 0x%lx  Length %lu\n",
                       PartialDescriptor->u.Port.Start.u.LowPart,
                       PartialDescriptor->u.Port.Length);

                if (DeviceExtension->BaseAddress == 0)
                {
                    if (PartialDescriptor->u.Port.Length < 3)
                        return STATUS_INSUFFICIENT_RESOURCES;

                    DeviceExtension->BaseAddress = PartialDescriptor->u.Port.Start.u.LowPart;
                }
                break;

            case CmResourceTypeInterrupt:
                DPRINT("Interrupt: Level %lu  Vector %lu\n",
                       PartialDescriptorTranslated->u.Interrupt.Level,
                       PartialDescriptorTranslated->u.Interrupt.Vector);

//                Dirql = (KIRQL)PartialDescriptorTranslated->u.Interrupt.Level;
//                Vector = PartialDescriptorTranslated->u.Interrupt.Vector;
//                Affinity = PartialDescriptorTranslated->u.Interrupt.Affinity;

//                if (PartialDescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
//                    InterruptMode = Latched;
//                else
//                    InterruptMode = LevelSensitive;

//                ShareInterrupt = (PartialDescriptorTranslated->ShareDisposition == CmResourceShareShared);
                break;

            default:
                DPRINT1("Other resource: \n");
                break;
        }
    }

    DPRINT("New LPT port: Base 0x%lx\n",
           DeviceExtension->BaseAddress);

    if (!DeviceExtension->BaseAddress)
        return STATUS_INSUFFICIENT_RESOURCES;

#if 0
    if (!Dirql)
        return STATUS_INSUFFICIENT_RESOURCES;
#endif

    DeviceExtension->Common.PnpState = dsStarted;


    /* We don't really care if the call succeeded or not... */

    return STATUS_SUCCESS;
}


static
NTSTATUS
FdoCreateRawParallelPdo(
    IN PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension = NULL;
    PDEVICE_OBJECT Pdo = NULL;
    WCHAR DeviceNameBuffer[32];
    WCHAR LinkNameBuffer[32];
    WCHAR LptPortBuffer[32];
    UNICODE_STRING DeviceName;
    UNICODE_STRING LinkName;
    UNICODE_STRING LptPort;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    DPRINT("FdoCreateRawParallelPdo()\n");

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Create new device object */
    swprintf(DeviceNameBuffer,
             L"\\Device\\Parallel%lu",
             FdoDeviceExtension->PortNumber);
    RtlInitUnicodeString(&DeviceName,
                         DeviceNameBuffer);

    Status = IoCreateDevice(DeviceObject->DriverObject,
                            sizeof(PDO_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            0,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed with status 0x%08x\n", Status);
        goto done;
    }

    Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
    Pdo->Flags |= DO_POWER_PAGABLE;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Pdo->DeviceExtension;
    RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

    PdoDeviceExtension->Common.IsFDO = FALSE;
    PdoDeviceExtension->Common.PnpState = dsStopped;

    Pdo->StackSize = DeviceObject->StackSize + 1;

    FdoDeviceExtension->AttachedRawPdo = Pdo;
    PdoDeviceExtension->AttachedFdo = DeviceObject;

    PdoDeviceExtension->PortNumber = FdoDeviceExtension->PortNumber;
    PdoDeviceExtension->LptPort = PdoDeviceExtension->PortNumber + 1;


    /* Create link \DosDevices\LPTX -> \Device\ParallelY */
    swprintf(LinkNameBuffer, L"\\DosDevices\\LPT%lu", PdoDeviceExtension->LptPort);
    RtlInitUnicodeString(&LinkName, LinkNameBuffer);
    Status = IoCreateSymbolicLink(&LinkName,
                                  &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateSymbolicLink() failed with status 0x%08x\n", Status);
        goto done;
    }

    swprintf(LptPortBuffer, L"LPT%lu", PdoDeviceExtension->LptPort);
    RtlInitUnicodeString(&LptPort, LptPortBuffer);

    /* Write an entry value under HKLM\HARDWARE\DeviceMap\PARALLEL PORTS. */
    /* This step is not mandatory, so do not exit in case of error. */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\DeviceMap\\PARALLEL PORTS");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateKey(&KeyHandle,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (NT_SUCCESS(Status))
    {
        /* Key = \Device\Parallelx, Value = LPTx */
        ZwSetValueKey(KeyHandle,
                      &DeviceName,
                      0,
                      REG_SZ,
                      LptPortBuffer,
                      LptPort.Length + sizeof(WCHAR));
        ZwClose(KeyHandle);
    }

    Pdo->Flags |= DO_BUFFERED_IO;
    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Pdo)
        {
            ASSERT(PdoDeviceExtension);
            IoDeleteDevice(Pdo);
        }
    }

    return Status;
}


static
NTSTATUS
FdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG Size;
    ULONG i;
    ULONG PdoCount = 0;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(IrpSp);

    DPRINT("FdoQueryBusRelations()\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->Common.IsFDO);

    /* TODO: Enumerate parallel devices and create their PDOs */

    Status = FdoCreateRawParallelPdo(DeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    PdoCount++;

    /* Allocate a buffer for the device relations */
    Size = sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) * (PdoCount - 1);
    DeviceRelations = ExAllocatePoolWithTag(PagedPool, Size, PARPORT_TAG);
    if (DeviceRelations == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Fill the buffer */
    i = 0;
    ObReferenceObject(DeviceExtension->AttachedRawPdo);
    DeviceRelations->Objects[i] = DeviceExtension->AttachedRawPdo;
    DeviceRelations->Count = 1;

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    DPRINT("Done\n");

    return STATUS_SUCCESS;
}


/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
AddDevice(IN PDRIVER_OBJECT DriverObject,
          IN PDEVICE_OBJECT Pdo)
{
    DPRINT("AddDevice(%p %p)\n", DriverObject, Pdo);

    /* Serial.sys is a legacy driver. AddDevice is called once
     * with a NULL Pdo just after the driver initialization.
     * Detect this case and return success.
     */
    if (Pdo == NULL)
        return STATUS_SUCCESS;

    /* We have here a PDO not null. It represents a real serial
     * port. So call the internal AddDevice function.
     */
    return AddDeviceInternal(DriverObject, Pdo, NULL, NULL);
}


NTSTATUS
NTAPI
FdoCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("FdoCreate()\n");

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
    {
        DPRINT1("Not a directory\n");
        Status = STATUS_NOT_A_DIRECTORY;
        goto done;
    }

    DPRINT("Open parallel port %lu: successful\n", DeviceExtension->PortNumber);
    DeviceExtension->OpenCount++;

done:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
FdoClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION pDeviceExtension;

    DPRINT("FdoClose()\n");

    pDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    pDeviceExtension->OpenCount--;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
FdoCleanup(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    DPRINT("FdoCleanup()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
FdoRead(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
    DPRINT("FdoRead()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
NTAPI
FdoWrite(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT("FdoWrite()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
NTAPI
FdoPnp(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp)
{
    ULONG MinorFunction;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    DPRINT("FdoPnp()\n");

    Stack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = Stack->MinorFunction;

    switch (MinorFunction)
    {
        /* FIXME: do all these minor functions
        IRP_MN_QUERY_REMOVE_DEVICE 0x1
        IRP_MN_REMOVE_DEVICE 0x2
        {
            TRACE_(SERIAL, "IRP_MJ_PNP / IRP_MN_REMOVE_DEVICE\n");
            IoAcquireRemoveLock
            IoReleaseRemoveLockAndWait
            pass request to DeviceExtension-LowerDriver
            disable interface
            IoDeleteDevice(Fdo) and/or IoDetachDevice
            break;
        }
        IRP_MN_CANCEL_REMOVE_DEVICE 0x3
        IRP_MN_STOP_DEVICE 0x4
        IRP_MN_QUERY_STOP_DEVICE 0x5
        IRP_MN_CANCEL_STOP_DEVICE 0x6
        IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations (optional) 0x7
        IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations (optional) 0x7
        IRP_MN_QUERY_INTERFACE (optional) 0x8
        IRP_MN_QUERY_CAPABILITIES (optional) 0x9
        IRP_MN_FILTER_RESOURCE_REQUIREMENTS (optional) 0xd
        IRP_MN_QUERY_PNP_DEVICE_STATE (optional) 0x14
        IRP_MN_DEVICE_USAGE_NOTIFICATION (required or optional) 0x16
        IRP_MN_SURPRISE_REMOVAL 0x17
        */
        case IRP_MN_START_DEVICE: /* 0x0 */
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

            ASSERT(((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.PnpState == dsStopped);

            /* Call lower driver */
            Status = ForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status))
            {
                Status = FdoStartDevice(DeviceObject,
                                        Stack->Parameters.StartDevice.AllocatedResources,
                                        Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
            }
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x7 */
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    Status = FdoQueryBusRelations(DeviceObject, Irp, Stack);
                    Irp->IoStatus.Status = Status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return Status;

                case RemovalRelations:
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                default:
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                        Stack->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* (optional) 0xd */
            DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        default:
            DPRINT("Unknown minor function 0x%x\n", MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
FdoPower(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    DPRINT("FdoPower()\n");

    LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(LowerDevice, Irp);
}

/* EOF */
