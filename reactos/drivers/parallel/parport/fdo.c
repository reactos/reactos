/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/fdo.c
 * PURPOSE:         FDO functions
 */

#include "parport.h"

/*
 * The following constants describe the various signals of the printer port
 * hardware.  Note that the hardware inverts some signals and that some
 * signals are active low.  An example is LP_STROBE, which must be programmed
 * with 1 for being active and 0 for being inactive, because the strobe signal
 * gets inverted, but it is also active low.
 */

/*
 * bit defines for 8255 status port
 * base + 1
 * accessed with LP_S(minor), which gets the byte...
 */
#define LP_PBUSY    0x80  /* inverted input, active high */
#define LP_PACK     0x40  /* unchanged input, active low */
#define LP_POUTPA   0x20  /* unchanged input, active high */
#define LP_PSELECD  0x10  /* unchanged input, active high */
#define LP_PERRORP  0x08  /* unchanged input, active low */

/*
 * defines for 8255 control port
 * base + 2
 * accessed with LP_C(minor)
 */
#define LP_PINTEN   0x10
#define LP_PSELECP  0x08  /* inverted output, active low */
#define LP_PINITP   0x04  /* unchanged output, active low */
#define LP_PAUTOLF  0x02  /* inverted output, active low */
#define LP_PSTROBE  0x01  /* inverted output, active low */


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

    DeviceExtension->ParallelPortNumber = IoGetConfigurationInformation()->ParallelCount++;
    if (pLptPortNumber == NULL)
        DeviceExtension->LptPort = DeviceExtension->ParallelPortNumber + 1;
    else
        DeviceExtension->LptPort = *pLptPortNumber;
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
    WCHAR DeviceNameBuffer[32];
    WCHAR LinkNameBuffer[32];
    WCHAR LptPortBuffer[32];
    UNICODE_STRING DeviceName;
    UNICODE_STRING LinkName;
    UNICODE_STRING LptPort;
    ULONG i;
//    ULONG Vector = 0;
//    KIRQL Dirql = 0;
//    KAFFINITY Affinity = 0;
//    KINTERRUPT_MODE InterruptMode = Latched;
//    BOOLEAN ShareInterrupt = TRUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;

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
                    if (PartialDescriptor->u.Port.Length < 8)
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
                DPRINT1("Other ressource: \n");
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

    /* Create link \DosDevices\LPTX -> \Device\ParallelPortX */
    swprintf(DeviceNameBuffer, L"\\Device\\ParallelPort%lu", DeviceExtension->ParallelPortNumber);
    swprintf(LinkNameBuffer, L"\\DosDevices\\LPT%lu", DeviceExtension->LptPort);
    swprintf(LptPortBuffer, L"LPT%lu", DeviceExtension->LptPort);
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
    RtlInitUnicodeString(&LinkName, LinkNameBuffer);
    RtlInitUnicodeString(&LptPort, LptPortBuffer);
    Status = IoCreateSymbolicLink(&LinkName,
                                  &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateSymbolicLink() failed with status 0x%08x\n", Status);
        return Status;
    }


    /* Write an entry value under HKLM\HARDWARE\DeviceMap\PARALLEL PORTS. */
    /* This step is not mandatory, so do not exit in case of error. */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\DeviceMap\\PARALLEL PORTS");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
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

    DeviceExtension->Common.PnpState = dsStarted;


    /* We don't really care if the call succeeded or not... */

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

    DPRINT("Open LPT%lu: successful\n", DeviceExtension->LptPort);
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
FdoWrite(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PUCHAR Buffer;
    ULONG i;
    UCHAR PortStatus;
    ULONG ulCount;

    DPRINT("FdoWrite()\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Buffer = GetUserBuffer(Irp);
    DPRINT("Length: %lu\n", IoStack->Parameters.Write.Length);
    DPRINT("Buffer: %p\n", Buffer);

    if (Buffer != NULL)
    {
        DPRINT("%s\n", Buffer);
    }

    for (i = 0; i < IoStack->Parameters.Write.Length; i++)
    {
        DPRINT("%lu: %c\n", i, Buffer[i]);

        ulCount = 0;

        do
        {
            KeStallExecutionProcessor(10);
            PortStatus = READ_PORT_UCHAR((PUCHAR)(DeviceExtension->BaseAddress + 1));
            ulCount++;
        }
        while (ulCount < 500000 && !(PortStatus & LP_PBUSY));

        if (ulCount == 500000)
        {
            DPRINT1("Timed out\n");

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_TIMEOUT;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return STATUS_TIMEOUT;
        }

        /* Write character */
        WRITE_PORT_UCHAR((PUCHAR)DeviceExtension->BaseAddress, Buffer[i]);

        KeStallExecutionProcessor(10);

        WRITE_PORT_UCHAR((PUCHAR)(DeviceExtension->BaseAddress + 2), (LP_PSELECP | LP_PINITP | LP_PSTROBE));

        KeStallExecutionProcessor(10);

        WRITE_PORT_UCHAR((PUCHAR)(DeviceExtension->BaseAddress + 2), (LP_PSELECP | LP_PINITP));
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
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
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                case RemovalRelations:
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                default:
                    DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                        Stack->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* (optional) 0xd */
            DPRINT1("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        default:
            DPRINT1("Unknown minor function 0x%x\n", MinorFunction);
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
    DPRINT("FdoPower()\n");

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
