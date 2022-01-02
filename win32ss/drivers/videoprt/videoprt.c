/*
 * VideoPort driver
 *
 * Copyright (C) 2002-2004, 2007 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#include <stdio.h>
#include <ndk/exfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES ***********************************************************/

ULONG VideoDebugLevel = 0;

BOOLEAN VpBaseVideo = FALSE;
BOOLEAN VpNoVesa = FALSE;

PKPROCESS CsrProcess = NULL;
static ULONG VideoPortMaxObjectNumber = -1;
KMUTEX VideoPortInt10Mutex;
KSPIN_LOCK HwResetAdaptersLock;
RTL_STATIC_LIST_HEAD(HwResetAdaptersList);

/* PRIVATE FUNCTIONS **********************************************************/

ULONG
NTAPI
DriverEntry(
    IN PVOID Context1,
    IN PVOID Context2)
{
    return STATUS_SUCCESS;
}

static
NTSTATUS
IntVideoPortAddDeviceMapLink(
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension)
{
    WCHAR DeviceBuffer[20];
    UNICODE_STRING DeviceName;
    WCHAR SymlinkBuffer[20];
    UNICODE_STRING SymlinkName;
    ULONG DeviceNumber;
    NTSTATUS Status;

    /* Create a unicode device name. */
    DeviceNumber = DeviceExtension->DeviceNumber;
    swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);

    /* Add entry to DEVICEMAP\VIDEO key in registry. */
    Status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"VIDEO",
                                   DeviceBuffer,
                                   REG_SZ,
                                   DeviceExtension->NewRegistryPath.Buffer,
                                   DeviceExtension->NewRegistryPath.Length + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "Failed to create DEViCEMAP registry entry: 0x%X\n", Status);
        return Status;
    }

    Status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"VIDEO",
                                   L"MaxObjectNumber",
                                   REG_DWORD,
                                   &DeviceNumber,
                                   sizeof(DeviceNumber));
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "Failed to write MaxObjectNumber: 0x%X\n", Status);
        return Status;
    }

    /* Create symbolic link "\??\DISPLAYx" */
    swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
    RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
    RtlInitUnicodeString(&DeviceName, DeviceBuffer);
    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "Failed to create symbolic link: 0x%X\n", Status);
        return Status;
    }

    /* Update MaxObjectNumber */
    VideoPortMaxObjectNumber = DeviceNumber;

    return STATUS_SUCCESS;
}

PVOID
NTAPI
IntVideoPortImageDirectoryEntryToData(
    PVOID BaseAddress,
    ULONG Directory)
{
    PIMAGE_NT_HEADERS NtHeader;
    ULONG Va;

    NtHeader = RtlImageNtHeader(BaseAddress);
    if (NtHeader == NULL)
        return NULL;

    if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
        return NULL;

    Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
    if (Va == 0)
        return NULL;

    return (PVOID)((ULONG_PTR)BaseAddress + Va);
}

VOID
NTAPI
IntVideoPortDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    PVOID HwDeviceExtension =
        &((PVIDEO_PORT_DEVICE_EXTENSION)DeferredContext)->MiniPortDeviceExtension;
    ((PMINIPORT_DPC_ROUTINE)SystemArgument1)(HwDeviceExtension, SystemArgument2);
}

NTSTATUS
NTAPI
IntVideoPortCreateAdapterDeviceObject(
   _In_ PDRIVER_OBJECT DriverObject,
   _In_ PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
   _In_ USHORT AdapterNumber,
   _In_ USHORT DisplayNumber,
   _Out_opt_ PDEVICE_OBJECT *DeviceObject)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    ULONG DeviceNumber;
    ULONG PciSlotNumber;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG Size;
    NTSTATUS Status;
    WCHAR DeviceBuffer[20];
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject_;

    if (DeviceObject == NULL)
        DeviceObject = &DeviceObject_;

    /*
     * Find the first free device number that can be used for video device
     * object names and symlinks.
     */
    DeviceNumber = VideoPortMaxObjectNumber + 1;
    if (DeviceNumber == (ULONG)-1)
    {
        WARN_(VIDEOPRT, "Can't find free device number\n");
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Create the device object.
     */

    /* Create a unicode device name. */
    swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
    RtlInitUnicodeString(&DeviceName, DeviceBuffer);

    INFO_(VIDEOPRT, "HwDeviceExtension size is: 0x%x\n",
          DriverExtension->InitializationData.HwDeviceExtensionSize);

    /* Create the device object. */
    Size = sizeof(VIDEO_PORT_DEVICE_EXTENSION) +
        DriverExtension->InitializationData.HwDeviceExtensionSize;
    Status = IoCreateDevice(DriverObject,
                            Size,
                            &DeviceName,
                            FILE_DEVICE_VIDEO,
                            0,
                            TRUE,
                            DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        WARN_(VIDEOPRT, "IoCreateDevice call failed with status 0x%08x\n", Status);
        return Status;
    }

    /*
     * Set the buffering strategy here. If you change this, remember
     * to change VidDispatchDeviceControl too.
     */

    (*DeviceObject)->Flags |= DO_BUFFERED_IO;

    /* Initialize device extension. */
    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)((*DeviceObject)->DeviceExtension);
    DeviceExtension->Common.Fdo = TRUE;
    DeviceExtension->DeviceNumber = DeviceNumber;
    DeviceExtension->DriverObject = DriverObject;
    DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    DeviceExtension->FunctionalDeviceObject = *DeviceObject;
    DeviceExtension->DriverExtension = DriverExtension;
    DeviceExtension->SessionId = -1;
    DeviceExtension->AdapterNumber = AdapterNumber;
    DeviceExtension->DisplayNumber = DisplayNumber;

    InitializeListHead(&DeviceExtension->ChildDeviceList);

    /* Get the registry path associated with this device. */
    Status = IntCreateRegistryPath(&DriverExtension->RegistryPath,
                                   DeviceExtension->AdapterNumber,
                                   &DeviceExtension->RegistryPath);
    if (!NT_SUCCESS(Status))
    {
        WARN_(VIDEOPRT, "IntCreateRegistryPath() call failed with status 0x%08x\n", Status);
        IoDeleteDevice(*DeviceObject);
        *DeviceObject = NULL;
        return Status;
    }

    if (PhysicalDeviceObject != NULL)
    {
        /* Get bus number from the upper level bus driver. */
        Size = sizeof(ULONG);
        Status = IoGetDeviceProperty(PhysicalDeviceObject,
                                     DevicePropertyBusNumber,
                                     Size,
                                     &DeviceExtension->SystemIoBusNumber,
                                     &Size);
        if (!NT_SUCCESS(Status))
        {
            WARN_(VIDEOPRT, "Couldn't get an information from bus driver. We will try to\n"
                  "use legacy detection method, but even that doesn't mean that\n"
                  "it will work.\n");
            DeviceExtension->PhysicalDeviceObject = NULL;
        }
    }

    DeviceExtension->AdapterInterfaceType =
        DriverExtension->InitializationData.AdapterInterfaceType;

    if (PhysicalDeviceObject != NULL)
    {
        /* Get bus type from the upper level bus driver. */
        Size = sizeof(ULONG);
        IoGetDeviceProperty(PhysicalDeviceObject,
                            DevicePropertyLegacyBusType,
                            Size,
                            &DeviceExtension->AdapterInterfaceType,
                            &Size);

        /* Get bus device address from the upper level bus driver. */
        Size = sizeof(ULONG);
        IoGetDeviceProperty(PhysicalDeviceObject,
                            DevicePropertyAddress,
                            Size,
                            &PciSlotNumber,
                            &Size);

        /* Convert slotnumber to PCI_SLOT_NUMBER */
        SlotNumber.u.AsULONG = 0;
        SlotNumber.u.bits.DeviceNumber = (PciSlotNumber >> 16) & 0xFFFF;
        SlotNumber.u.bits.FunctionNumber = PciSlotNumber & 0xFFFF;
        DeviceExtension->SystemIoSlotNumber = SlotNumber.u.AsULONG;
    }

    InitializeListHead(&DeviceExtension->AddressMappingListHead);
    InitializeListHead(&DeviceExtension->DmaAdapterList);

    KeInitializeDpc(&DeviceExtension->DpcObject,
                    IntVideoPortDeferredRoutine,
                    DeviceExtension);

    KeInitializeMutex(&DeviceExtension->DeviceLock, 0);

    /* Attach the device. */
    if ((PhysicalDeviceObject != NULL) && (DisplayNumber == 0))
        DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(
                                                *DeviceObject,
                                                PhysicalDeviceObject);

    Status = IntCreateNewRegistryPath(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "IntCreateNewRegistryPath() failed with status 0x%08x\n", Status);
        IoDeleteDevice(*DeviceObject);
        *DeviceObject = NULL;
        return Status;
    }

    IntSetupDeviceSettingsKey(DeviceExtension);

    /* Remove the initailizing flag */
    (*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Set up the VIDEO/DEVICEMAP registry keys */
    Status = IntVideoPortAddDeviceMapLink(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        ERR_(VIDEOPRT, "IntVideoPortAddDeviceMapLink() failed with status 0x%08x\n", Status);
        IoDeleteDevice(*DeviceObject);
        *DeviceObject = NULL;
        return Status;
    }

    if (DisplayNumber == 0)
    {
        DriverExtension->InitializationData.StartingDeviceNumber++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IntVideoPortFindAdapter(
    IN PDRIVER_OBJECT DriverObject,
    IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
    IN PDEVICE_OBJECT DeviceObject)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    VP_STATUS vpStatus;
    VIDEO_PORT_CONFIG_INFO ConfigInfo;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    UCHAR Again = FALSE;
    BOOL LegacyDetection = FALSE;

    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Setup a ConfigInfo structure that we will pass to HwFindAdapter. */
    RtlZeroMemory(&ConfigInfo, sizeof(VIDEO_PORT_CONFIG_INFO));
    ConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);
    ConfigInfo.AdapterInterfaceType = DeviceExtension->AdapterInterfaceType;
    if (ConfigInfo.AdapterInterfaceType == PCIBus)
        ConfigInfo.InterruptMode = LevelSensitive;
    else
        ConfigInfo.InterruptMode = Latched;
    ConfigInfo.DriverRegistryPath = DriverExtension->RegistryPath.Buffer;
    ConfigInfo.VideoPortGetProcAddress = IntVideoPortGetProcAddress;
    ConfigInfo.SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
    ConfigInfo.BusInterruptLevel = DeviceExtension->InterruptLevel;
    ConfigInfo.BusInterruptVector = DeviceExtension->InterruptVector;

    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SystemBasicInfo,
                                      sizeof(SystemBasicInfo),
                                      NULL);
    if (NT_SUCCESS(Status))
    {
        ConfigInfo.SystemMemorySize = SystemBasicInfo.NumberOfPhysicalPages *
                                      SystemBasicInfo.PageSize;
    }

    // FIXME: Check the adapter key and update VideoDebugLevel variable.

    /*
     * Call miniport HwVidFindAdapter entry point to detect if
     * particular device is present. There are two possible code
     * paths. The first one is for Legacy drivers (NT4) and cases
     * when we don't have information about what bus we're on. The
     * second case is the standard one for Plug & Play drivers.
     */
    if (DeviceExtension->PhysicalDeviceObject == NULL)
    {
        LegacyDetection = TRUE;
    }

    if (LegacyDetection)
    {
        ULONG BusNumber, MaxBuses;

        MaxBuses = DeviceExtension->AdapterInterfaceType == PCIBus ? PCI_MAX_BRIDGE_NUMBER : 1;

        for (BusNumber = 0; BusNumber < MaxBuses; BusNumber++)
        {
            DeviceExtension->SystemIoBusNumber =
                ConfigInfo.SystemIoBusNumber = BusNumber;

            RtlZeroMemory(&DeviceExtension->MiniPortDeviceExtension,
                          DriverExtension->InitializationData.HwDeviceExtensionSize);

            /* FIXME: Need to figure out what string to pass as param 3. */
            vpStatus = DriverExtension->InitializationData.HwFindAdapter(
                         &DeviceExtension->MiniPortDeviceExtension,
                         DriverExtension->HwContext,
                         NULL,
                         &ConfigInfo,
                         &Again);

            if (vpStatus == ERROR_DEV_NOT_EXIST)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        /* FIXME: Need to figure out what string to pass as param 3. */
        vpStatus = DriverExtension->InitializationData.HwFindAdapter(
                     &DeviceExtension->MiniPortDeviceExtension,
                     DriverExtension->HwContext,
                     NULL,
                     &ConfigInfo,
                     &Again);
    }

    if (vpStatus != NO_ERROR)
    {
        ERR_(VIDEOPRT, "HwFindAdapter call failed with error 0x%X\n", vpStatus);
        Status = STATUS_UNSUCCESSFUL;
        goto Failure;
    }

    /*
     * Now we know the device is present, so let's do all additional tasks
     * such as creating symlinks or setting up interrupts and timer.
     */

    /* FIXME: Allocate hardware resources for device. */

    /* Allocate interrupt for device. */
    if (!IntVideoPortSetupInterrupt(DeviceObject, DriverExtension, &ConfigInfo))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    /* Allocate timer for device. */
    if (!IntVideoPortSetupTimer(DeviceObject, DriverExtension))
    {
        if (DeviceExtension->InterruptObject != NULL)
            IoDisconnectInterrupt(DeviceExtension->InterruptObject);
        ERR_(VIDEOPRT, "IntVideoPortSetupTimer failed\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    /* If the device can be reset, insert it in the list of resettable adapters */
    InitializeListHead(&DeviceExtension->HwResetListEntry);
    if (DriverExtension->InitializationData.HwResetHw != NULL)
    {
        ExInterlockedInsertTailList(&HwResetAdaptersList,
                                    &DeviceExtension->HwResetListEntry,
                                    &HwResetAdaptersLock);
    }

    INFO_(VIDEOPRT, "STATUS_SUCCESS\n");
    return STATUS_SUCCESS;

Failure:
    RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
    if (DeviceExtension->NextDeviceObject)
        IoDetachDevice(DeviceExtension->NextDeviceObject);
    IoDeleteDevice(DeviceObject);
    return Status;
}

VOID
FASTCALL
IntAttachToCSRSS(
    PKPROCESS *CallingProcess,
    PKAPC_STATE ApcState)
{
    *CallingProcess = (PKPROCESS)PsGetCurrentProcess();
    if (*CallingProcess != CsrProcess)
    {
        KeStackAttachProcess(CsrProcess, ApcState);
    }
}

VOID
FASTCALL
IntDetachFromCSRSS(
    PKPROCESS *CallingProcess,
    PKAPC_STATE ApcState)
{
    if (*CallingProcess != CsrProcess)
    {
        KeUnstackDetachProcess(ApcState);
    }
}

VOID
FASTCALL
IntLoadRegistryParameters(VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING Path = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"SystemStartOptions");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    ULONG Length, NewLength;

    /* Initialize object attributes with the path we want */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        VideoPortDebugPrint(Error, "ZwOpenKey failed (0x%x)\n", Status);
        return;
    }

    /* Find out how large our buffer should be */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &Length);
    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
    {
        VideoPortDebugPrint(Error, "ZwQueryValueKey failed (0x%x)\n", Status);
        ObCloseHandle(KeyHandle, KernelMode);
        return;
    }

    /* Allocate it */
    KeyInfo = ExAllocatePoolWithTag(PagedPool, Length, TAG_VIDEO_PORT);
    if (!KeyInfo)
    {
        VideoPortDebugPrint(Error, "Out of memory\n");
        ObCloseHandle(KeyHandle, KernelMode);
        return;
    }

    /* Now for real this time */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             KeyInfo,
                             Length,
                             &NewLength);
    ObCloseHandle(KeyHandle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        VideoPortDebugPrint(Error, "ZwQueryValueKey failed (0x%x)\n", Status);
        ExFreePoolWithTag(KeyInfo, TAG_VIDEO_PORT);
        return;
    }

    /* Sanity check */
    if (KeyInfo->Type != REG_SZ)
    {
        VideoPortDebugPrint(Error, "Invalid type for SystemStartOptions\n");
        ExFreePoolWithTag(KeyInfo, TAG_VIDEO_PORT);
        return;
    }

    /* Check if BASEVIDEO or NOVESA is present in the start options */
    if (wcsstr((PWCHAR)KeyInfo->Data, L"BASEVIDEO"))
        VpBaseVideo = TRUE;
    if (wcsstr((PWCHAR)KeyInfo->Data, L"NOVESA"))
        VpNoVesa = TRUE;

    ExFreePoolWithTag(KeyInfo, TAG_VIDEO_PORT);

    /* FIXME: Old ReactOS-compatibility... */
    if (VpBaseVideo) VpNoVesa = TRUE;

    if (VpNoVesa)
        VideoPortDebugPrint(Info, "VESA mode disabled\n");
    else
        VideoPortDebugPrint(Info, "VESA mode enabled\n");

    /* If we are in BASEVIDEO, create the volatile registry key for Win32k */
    if (VpBaseVideo)
    {
        RtlInitUnicodeString(&Path, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\BaseVideo");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &Path,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        Status = ZwCreateKey(&KeyHandle,
                             READ_CONTROL, // Non-0 placeholder: no use for this handle.
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        if (NT_SUCCESS(Status))
            ObCloseHandle(KeyHandle, KernelMode);
        else
            ERR_(VIDEOPRT, "Failed to create the BaseVideo key (0x%x)\n", Status);
    }

    return;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
ULONG
NTAPI
VideoPortInitialize(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext)
{
    PDRIVER_OBJECT DriverObject = Context1;
    PUNICODE_STRING RegistryPath = Context2;
    NTSTATUS Status;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    BOOLEAN PnpDriver = FALSE, LegacyDetection = FALSE;
    static BOOLEAN FirstInitialization;

    TRACE_(VIDEOPRT, "VideoPortInitialize\n");

    if (!FirstInitialization)
    {
        FirstInitialization = TRUE;
        KeInitializeMutex(&VideoPortInt10Mutex, 0);
        KeInitializeSpinLock(&HwResetAdaptersLock);
        IntLoadRegistryParameters();
    }

    /* As a first thing do parameter checks. */
    if (HwInitializationData->HwInitDataSize > sizeof(VIDEO_HW_INITIALIZATION_DATA))
    {
        ERR_(VIDEOPRT, "Invalid HwInitializationData\n");
        return STATUS_REVISION_MISMATCH;
    }

    if ((HwInitializationData->HwFindAdapter == NULL) ||
        (HwInitializationData->HwInitialize == NULL) ||
        (HwInitializationData->HwStartIO == NULL))
    {
        ERR_(VIDEOPRT, "Invalid HwInitializationData\n");
        return STATUS_INVALID_PARAMETER;
    }

    switch (HwInitializationData->HwInitDataSize)
    {
            /*
             * NT4 drivers are special case, because we must use legacy method
             * of detection instead of the Plug & Play one.
             */
        case SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA:
            INFO_(VIDEOPRT, "We were loaded by a Windows NT miniport driver.\n");
            break;

        case SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA:
            INFO_(VIDEOPRT, "We were loaded by a Windows 2000 miniport driver.\n");
            break;

        case sizeof(VIDEO_HW_INITIALIZATION_DATA):
            INFO_(VIDEOPRT, "We were loaded by a Windows XP or later miniport driver.\n");
            break;

        default:
            ERR_(VIDEOPRT, "Invalid HwInitializationData size.\n");
            return STATUS_UNSUCCESSFUL;
    }

    /* Set dispatching routines */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IntVideoPortDispatchOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IntVideoPortDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        IntVideoPortDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] =
        IntVideoPortDispatchDeviceControl;
    DriverObject->DriverUnload = IntVideoPortUnload;

    /* Determine type of the miniport driver */
    if ((HwInitializationData->HwInitDataSize >=
            FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwQueryInterface)) &&
        (HwInitializationData->HwSetPowerState != NULL) &&
        (HwInitializationData->HwGetPowerState != NULL) &&
        (HwInitializationData->HwGetVideoChildDescriptor != NULL))
    {
        INFO_(VIDEOPRT, "The miniport is a PnP miniport driver\n");
        PnpDriver = TRUE;
    }

    /* Check if legacy detection should be applied */
    if (!PnpDriver || HwContext)
    {
        INFO_(VIDEOPRT, "Legacy detection for adapter interface %d\n",
              HwInitializationData->AdapterInterfaceType);

        /* FIXME: Move the code for legacy detection
           to another function and call it here */
        LegacyDetection = TRUE;
    }

    /*
     * NOTE:
     * The driver extension can be already allocated in case that we were
     * called by legacy driver and failed detecting device. Some miniport
     * drivers in that case adjust parameters and call VideoPortInitialize
     * again.
     */
    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
    if (DriverExtension == NULL)
    {
        Status = IoAllocateDriverObjectExtension(DriverObject,
                                                 DriverObject,
                                                 sizeof(VIDEO_PORT_DRIVER_EXTENSION),
                                                 (PVOID *)&DriverExtension);
        if (!NT_SUCCESS(Status))
        {
            ERR_(VIDEOPRT, "IoAllocateDriverObjectExtension failed 0x%x\n", Status);
            return Status;
        }

        /*
         * Save the registry path. This should be done only once even if
         * VideoPortInitialize is called multiple times.
         */
        if (RegistryPath->Length != 0)
        {
            DriverExtension->RegistryPath.Length = 0;
            DriverExtension->RegistryPath.MaximumLength =
                RegistryPath->Length + sizeof(UNICODE_NULL);
            DriverExtension->RegistryPath.Buffer =
                ExAllocatePoolWithTag(
                    PagedPool,
                    DriverExtension->RegistryPath.MaximumLength,
                    'RTSU');
            if (DriverExtension->RegistryPath.Buffer == NULL)
            {
                RtlInitUnicodeString(&DriverExtension->RegistryPath, NULL);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyUnicodeString(&DriverExtension->RegistryPath, RegistryPath);
            INFO_(VIDEOPRT, "RegistryPath: %wZ\n", &DriverExtension->RegistryPath);
        }
        else
        {
            RtlInitUnicodeString(&DriverExtension->RegistryPath, NULL);
        }
    }

    /* Copy the correct miniport initialization data to the device extension. */
    RtlCopyMemory(&DriverExtension->InitializationData,
                  HwInitializationData,
                  HwInitializationData->HwInitDataSize);
    if (HwInitializationData->HwInitDataSize <
            sizeof(VIDEO_HW_INITIALIZATION_DATA))
    {
        RtlZeroMemory((PVOID)((ULONG_PTR)&DriverExtension->InitializationData +
                              HwInitializationData->HwInitDataSize),
                      sizeof(VIDEO_HW_INITIALIZATION_DATA) -
                      HwInitializationData->HwInitDataSize);
    }
    DriverExtension->HwContext = HwContext;

    /*
     * Plug & Play drivers registers the device in AddDevice routine.
     * For legacy drivers we must do it now.
     */
    if (LegacyDetection)
    {
        PDEVICE_OBJECT DeviceObject;

        if (HwInitializationData->HwInitDataSize != SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA)
        {
            /* Power management */
            DriverObject->MajorFunction[IRP_MJ_POWER] = IntVideoPortDispatchPower;
        }

        Status = IntVideoPortCreateAdapterDeviceObject(DriverObject,
                                                       DriverExtension,
                                                       NULL,
                                                       DriverExtension->InitializationData.StartingDeviceNumber,
                                                       0,
                                                       &DeviceObject);
        if (!NT_SUCCESS(Status))
        {
            ERR_(VIDEOPRT, "IntVideoPortCreateAdapterDeviceObject returned 0x%x\n", Status);
            return Status;
        }

        Status = IntVideoPortFindAdapter(DriverObject, DriverExtension, DeviceObject);
        if (!NT_SUCCESS(Status))
            ERR_(VIDEOPRT, "IntVideoPortFindAdapter returned 0x%x\n", Status);

        return Status;
    }
    else
    {
        DriverObject->DriverExtension->AddDevice = IntVideoPortAddDevice;
        DriverObject->MajorFunction[IRP_MJ_PNP] = IntVideoPortDispatchPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] = IntVideoPortDispatchPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = IntVideoPortDispatchSystemControl;

        return STATUS_SUCCESS;
    }
}

/*
 * @implemented
 */
VOID
VideoPortDebugPrint(
    IN VIDEO_DEBUG_LEVEL DebugPrintLevel,
    IN PCHAR DebugMessage,
    ...)
{
    va_list ap;

    if (VideoDebugLevel >= DebugPrintLevel)
        DebugPrintLevel = Error;

    va_start(ap, DebugMessage);
    vDbgPrintEx(DPFLTR_IHVVIDEO_ID, DebugPrintLevel, DebugMessage, ap);
    va_end(ap);
}

/*
 * @unimplemented
 */
VOID
NTAPI
VideoPortLogError(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
    IN VP_STATUS ErrorCode,
    IN ULONG UniqueId)
{
    UNIMPLEMENTED;

    INFO_(VIDEOPRT, "VideoPortLogError ErrorCode %d (0x%x) UniqueId %lu (0x%lx)\n",
          ErrorCode, ErrorCode, UniqueId, UniqueId);
    if (Vrp)
        INFO_(VIDEOPRT, "Vrp->IoControlCode %lu (0x%lx)\n", Vrp->IoControlCode, Vrp->IoControlCode);
}

/*
 * @implemented
 */
UCHAR
NTAPI
VideoPortGetCurrentIrql(VOID)
{
    return KeGetCurrentIrql();
}

typedef struct QueryRegistryCallbackContext
{
    PVOID HwDeviceExtension;
    PVOID HwContext;
    PMINIPORT_GET_REGISTRY_ROUTINE HwGetRegistryRoutine;
} QUERY_REGISTRY_CALLBACK_CONTEXT, *PQUERY_REGISTRY_CALLBACK_CONTEXT;

static
NTSTATUS
NTAPI
QueryRegistryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    PQUERY_REGISTRY_CALLBACK_CONTEXT CallbackContext = (PQUERY_REGISTRY_CALLBACK_CONTEXT) Context;

    INFO_(VIDEOPRT, "Found registry value for name %S: type %d, length %d\n",
          ValueName, ValueType, ValueLength);
    return (*(CallbackContext->HwGetRegistryRoutine))(
               CallbackContext->HwDeviceExtension,
               CallbackContext->HwContext,
               ValueName,
               ValueData,
               ValueLength);
}

/*
 * @unimplemented
 */

VP_STATUS
NTAPI
VideoPortGetRegistryParameters(
    IN PVOID HwDeviceExtension,
    IN PWSTR ParameterName,
    IN UCHAR IsParameterFileName,
    IN PMINIPORT_GET_REGISTRY_ROUTINE GetRegistryRoutine,
    IN PVOID HwContext)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {{0}};
    QUERY_REGISTRY_CALLBACK_CONTEXT Context;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    TRACE_(VIDEOPRT, "VideoPortGetRegistryParameters ParameterName %S, RegPath: %wZ\n",
           ParameterName, &DeviceExtension->RegistryPath);

    Context.HwDeviceExtension = HwDeviceExtension;
    Context.HwContext = HwContext;
    Context.HwGetRegistryRoutine = GetRegistryRoutine;

    QueryTable[0].QueryRoutine = QueryRegistryCallback;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = ParameterName;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    DeviceExtension->RegistryPath.Buffer,
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        WARN_(VIDEOPRT, "VideoPortGetRegistryParameters could not find the "
              "requested parameter\n");
        return ERROR_INVALID_PARAMETER;
    }

    if (IsParameterFileName)
    {
        /* FIXME: need to read the contents of the file */
        UNIMPLEMENTED;
    }

    return NO_ERROR;
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortSetRegistryParameters(
    IN PVOID HwDeviceExtension,
    IN PWSTR ValueName,
    IN PVOID ValueData,
    IN ULONG ValueLength)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    VP_STATUS Status;

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    TRACE_(VIDEOPRT, "VideoPortSetRegistryParameters ParameterName %S, RegPath: %wZ\n",
           ValueName,
           &DeviceExtension->RegistryPath);
    ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   DeviceExtension->RegistryPath.Buffer,
                                   ValueName,
                                   REG_BINARY,
                                   ValueData,
                                   ValueLength);
    if (Status != NO_ERROR)
        WARN_(VIDEOPRT, "VideoPortSetRegistryParameters error 0x%x\n", Status);

    return Status;
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortGetVgaStatus(
    IN PVOID HwDeviceExtension,
    OUT PULONG VgaStatus)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    TRACE_(VIDEOPRT, "VideoPortGetVgaStatus\n");

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        if (DeviceExtension->AdapterInterfaceType == PCIBus)
        {
            /* VgaStatus: 0 == VGA not enabled, 1 == VGA enabled. */
            /* Assumed for now */
            *VgaStatus = 1;
            return NO_ERROR;
        }
    }

    return ERROR_INVALID_FUNCTION;
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortGetRomImage(
    IN PVOID HwDeviceExtension,
    IN PVOID Unused1,
    IN ULONG Unused2,
    IN ULONG Length)
{
    static PVOID RomImageBuffer = NULL;
    PKPROCESS CallingProcess;
    KAPC_STATE ApcState;

    TRACE_(VIDEOPRT, "VideoPortGetRomImage(HwDeviceExtension 0x%X Length 0x%X)\n",
           HwDeviceExtension, Length);

    /* If the length is zero then free the existing buffer. */
    if (Length == 0)
    {
        if (RomImageBuffer != NULL)
        {
            ExFreePool(RomImageBuffer);
            RomImageBuffer = NULL;
        }
        return NULL;
    }
    else
    {
        /*
         * The DDK says we shouldn't use the legacy C0000 method but get the
         * rom base address from the corresponding pci or acpi register but
         * lets ignore that and use C0000 anyway. We have already mapped the
         * bios area into memory so we'll copy from there.
         */

        /* Copy the bios. */
        Length = min(Length, 0x10000);
        if (RomImageBuffer != NULL)
        {
            ExFreePool(RomImageBuffer);
        }

        RomImageBuffer = ExAllocatePool(PagedPool, Length);
        if (RomImageBuffer == NULL)
        {
            return NULL;
        }

        IntAttachToCSRSS(&CallingProcess, &ApcState);
        RtlCopyMemory(RomImageBuffer, (PUCHAR)0xC0000, Length);
        IntDetachFromCSRSS(&CallingProcess, &ApcState);

        return RomImageBuffer;
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortScanRom(
    IN PVOID HwDeviceExtension,
    IN PUCHAR RomBase,
    IN ULONG RomLength,
    IN PUCHAR String)
{
    SIZE_T StringLength;
    BOOLEAN Found;
    PUCHAR SearchLocation;

    TRACE_(VIDEOPRT, "VideoPortScanRom RomBase %p RomLength 0x%x String %s\n", RomBase, RomLength, String);

    StringLength = strlen((PCHAR)String);
    Found = FALSE;
    for (SearchLocation = RomBase;
            !Found && SearchLocation < RomBase + RomLength - StringLength;
            SearchLocation++)
    {
        Found = (RtlCompareMemory(SearchLocation, String, StringLength) == StringLength);
        if (Found)
        {
            INFO_(VIDEOPRT, "Match found at %p\n", SearchLocation);
        }
    }

    return Found;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortSynchronizeExecution(
    IN PVOID HwDeviceExtension,
    IN VIDEO_SYNCHRONIZE_PRIORITY Priority,
    IN PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    OUT PVOID Context)
{
    BOOLEAN Ret;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    KIRQL OldIrql;

    switch (Priority)
    {
        case VpLowPriority:
            Ret = (*SynchronizeRoutine)(Context);
            break;

        case VpMediumPriority:
            DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
            if (DeviceExtension->InterruptObject == NULL)
                Ret = (*SynchronizeRoutine)(Context);
            else
                Ret = KeSynchronizeExecution(
                          DeviceExtension->InterruptObject,
                          SynchronizeRoutine,
                          Context);
            break;

        case VpHighPriority:
            OldIrql = KeGetCurrentIrql();
            if (OldIrql < SYNCH_LEVEL)
                KeRaiseIrql(SYNCH_LEVEL, &OldIrql);

            Ret = (*SynchronizeRoutine)(Context);

            if (OldIrql < SYNCH_LEVEL)
                KeLowerIrql(OldIrql);
            break;

        default:
            Ret = FALSE;
    }

    return Ret;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
IntVideoPortEnumerateChildren(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    ULONG Status;
    VIDEO_CHILD_ENUM_INFO ChildEnumInfo;
    BOOLEAN bHaveLastMonitorID = FALSE;
    UCHAR LastMonitorID[10];
    ULONG Unused;
    UINT i;
    PDEVICE_OBJECT ChildDeviceObject;
    PVIDEO_PORT_CHILD_EXTENSION ChildExtension;

    INFO_(VIDEOPRT, "Starting child device probe\n");
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->DriverExtension->InitializationData.HwGetVideoChildDescriptor == NULL)
    {
        WARN_(VIDEOPRT, "Miniport's HwGetVideoChildDescriptor is NULL!\n");
        return STATUS_SUCCESS;
    }

    if (!IsListEmpty(&DeviceExtension->ChildDeviceList))
    {
        ERR_(VIDEOPRT, "FIXME: Support calling VideoPortEnumerateChildren again!\n");
        return STATUS_SUCCESS;
    }

    /* Enumerate the children */
    for (i = 1; ; i++)
    {
        Status = IoCreateDevice(DeviceExtension->DriverObject,
                                sizeof(VIDEO_PORT_CHILD_EXTENSION) +
                                DeviceExtension->DriverExtension->InitializationData.HwChildDeviceExtensionSize,
                                NULL,
                                FILE_DEVICE_CONTROLLER,
                                FILE_DEVICE_SECURE_OPEN,
                                FALSE,
                                &ChildDeviceObject);
        if (!NT_SUCCESS(Status))
            return Status;

        ChildExtension = ChildDeviceObject->DeviceExtension;

        RtlZeroMemory(ChildExtension,
                      sizeof(VIDEO_PORT_CHILD_EXTENSION) +
                        DeviceExtension->DriverExtension->InitializationData.HwChildDeviceExtensionSize);

        ChildExtension->Common.Fdo = FALSE;
        ChildExtension->ChildId = i;
        ChildExtension->PhysicalDeviceObject = ChildDeviceObject;
        ChildExtension->DriverObject = DeviceExtension->DriverObject;

        /* Setup the ChildEnumInfo */
        ChildEnumInfo.Size = sizeof(ChildEnumInfo);
        ChildEnumInfo.ChildDescriptorSize = sizeof(ChildExtension->ChildDescriptor);
        ChildEnumInfo.ACPIHwId = 0;

        if (DeviceExtension->DriverExtension->InitializationData.HwChildDeviceExtensionSize)
            ChildEnumInfo.ChildHwDeviceExtension = VIDEO_PORT_GET_CHILD_EXTENSION(ChildExtension);
        else
            ChildEnumInfo.ChildHwDeviceExtension = NULL;

        ChildEnumInfo.ChildIndex = ChildExtension->ChildId;

        INFO_(VIDEOPRT, "Probing child: %d\n", ChildEnumInfo.ChildIndex);
        Status = DeviceExtension->DriverExtension->InitializationData.HwGetVideoChildDescriptor(
                     DeviceExtension->MiniPortDeviceExtension,
                     &ChildEnumInfo,
                     &ChildExtension->ChildType,
                     ChildExtension->ChildDescriptor,
                     &ChildExtension->ChildId,
                     &Unused);
        if (Status == VIDEO_ENUM_MORE_DEVICES)
        {
            if (ChildExtension->ChildType == Monitor)
            {
                // Check if the EDID is valid
                if (ChildExtension->ChildDescriptor[0] == 0x00 &&
                        ChildExtension->ChildDescriptor[1] == 0xFF &&
                        ChildExtension->ChildDescriptor[2] == 0xFF &&
                        ChildExtension->ChildDescriptor[3] == 0xFF &&
                        ChildExtension->ChildDescriptor[4] == 0xFF &&
                        ChildExtension->ChildDescriptor[5] == 0xFF &&
                        ChildExtension->ChildDescriptor[6] == 0xFF &&
                        ChildExtension->ChildDescriptor[7] == 0x00)
                {
                    if (bHaveLastMonitorID)
                    {
                        // Compare the previous monitor ID with the current one, break the loop if they are identical
                        if (RtlCompareMemory(LastMonitorID, &ChildExtension->ChildDescriptor[8], sizeof(LastMonitorID)) == sizeof(LastMonitorID))
                        {
                            INFO_(VIDEOPRT, "Found identical Monitor ID two times, stopping enumeration\n");
                            IoDeleteDevice(ChildDeviceObject);
                            break;
                        }
                    }

                    // Copy 10 bytes from the EDID, which can be used to uniquely identify the monitor
                    RtlCopyMemory(LastMonitorID, &ChildExtension->ChildDescriptor[8], sizeof(LastMonitorID));
                    bHaveLastMonitorID = TRUE;

                    /* Mark it valid */
                    ChildExtension->EdidValid = TRUE;
                }
                else
                {
                    /* Mark it invalid */
                    ChildExtension->EdidValid = FALSE;
                }
            }
        }
        else if (Status == VIDEO_ENUM_INVALID_DEVICE)
        {
            WARN_(VIDEOPRT, "Child device %d is invalid!\n", ChildEnumInfo.ChildIndex);
            IoDeleteDevice(ChildDeviceObject);
            continue;
        }
        else if (Status == VIDEO_ENUM_NO_MORE_DEVICES)
        {
            INFO_(VIDEOPRT, "End of child enumeration! (%d children enumerated)\n", i - 1);
            IoDeleteDevice(ChildDeviceObject);
            break;
        }
        else
        {
            WARN_(VIDEOPRT, "HwGetVideoChildDescriptor returned unknown status code 0x%x!\n", Status);
            IoDeleteDevice(ChildDeviceObject);
            break;
        }

        if (ChildExtension->ChildType == Monitor)
        {
            UINT j;
            PUCHAR p = ChildExtension->ChildDescriptor;
            INFO_(VIDEOPRT, "Monitor device enumerated! (ChildId = 0x%x)\n", ChildExtension->ChildId);
            for (j = 0; j < sizeof (ChildExtension->ChildDescriptor); j += 8)
            {
                INFO_(VIDEOPRT, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                      p[j + 0], p[j + 1], p[j + 2], p[j + 3],
                      p[j + 4], p[j + 5], p[j + 6], p[j + 7]);
            }
        }
        else if (ChildExtension->ChildType == Other)
        {
            INFO_(VIDEOPRT, "\"Other\" device enumerated: DeviceId = %S\n", (PWSTR)ChildExtension->ChildDescriptor);
        }
        else
        {
            ERR_(VIDEOPRT, "HwGetVideoChildDescriptor returned unsupported type: %d\n", ChildExtension->ChildType);
        }

        /* Clear the init flag */
        ChildDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        InsertTailList(&DeviceExtension->ChildDeviceList,
                       &ChildExtension->ListEntry);
    }

    return STATUS_SUCCESS;
}

VP_STATUS
NTAPI
VideoPortEnumerateChildren(
    IN PVOID HwDeviceExtension,
    IN PVOID Reserved)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    ASSERT(DeviceExtension);

    if (DeviceExtension->PhysicalDeviceObject)
    {
        /* Trigger reenumeration by the PnP manager */
        IoInvalidateDeviceRelations(DeviceExtension->PhysicalDeviceObject, BusRelations);
    }

    return NO_ERROR;
}

/*
 * @unimplemented
 */
VP_STATUS
NTAPI
VideoPortCreateSecondaryDisplay(
    IN PVOID HwDeviceExtension,
    IN OUT PVOID *SecondaryDeviceExtension,
    IN ULONG Flag)
{
    PDEVICE_OBJECT DeviceObject;
    PVIDEO_PORT_DEVICE_EXTENSION FirstDeviceExtension, DeviceExtension;
    NTSTATUS Status;

    ASSERT(SecondaryDeviceExtension);

    if (Flag != 0)
    {
        UNIMPLEMENTED;
    }

    FirstDeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    if (FirstDeviceExtension->DisplayNumber != 0)
    {
        DPRINT1("Calling VideoPortCreateSecondaryDisplay for InstanceId %lu\n",
                FirstDeviceExtension->DisplayNumber);
    }

    Status = IntVideoPortCreateAdapterDeviceObject(FirstDeviceExtension->DriverObject,
                                                   FirstDeviceExtension->DriverExtension,
                                                   FirstDeviceExtension->PhysicalDeviceObject,
                                                   FirstDeviceExtension->AdapterNumber,
                                                   FirstDeviceExtension->NumberOfSecondaryDisplays + 1,
                                                   &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IntVideoPortCreateAdapterDeviceObject() failed with status 0x%08x\n", Status);
        return ERROR_DEV_NOT_EXIST;
    }

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Increment secondary display count */
    FirstDeviceExtension->NumberOfSecondaryDisplays++;

    *SecondaryDeviceExtension = DeviceExtension->MiniPortDeviceExtension;
    return NO_ERROR;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortQueueDpc(
    IN PVOID HwDeviceExtension,
    IN PMINIPORT_DPC_ROUTINE CallbackRoutine,
    IN PVOID Context)
{
    return KeInsertQueueDpc(
               &VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->DpcObject,
               (PVOID)CallbackRoutine,
               (PVOID)Context);
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortGetAssociatedDeviceExtension(
    IN PVOID DeviceObject)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    TRACE_(VIDEOPRT, "VideoPortGetAssociatedDeviceExtension\n");
    DeviceExtension = ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;
    if (!DeviceExtension)
        return NULL;
    return DeviceExtension->MiniPortDeviceExtension;
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortGetVersion(
    IN PVOID HwDeviceExtension,
    IN OUT PVPOSVERSIONINFO VpOsVersionInfo)
{
    RTL_OSVERSIONINFOEXW Version;

    Version.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    if (VpOsVersionInfo->Size >= sizeof(VPOSVERSIONINFO))
    {
#if 1
        if (NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&Version)))
        {
            VpOsVersionInfo->MajorVersion = Version.dwMajorVersion;
            VpOsVersionInfo->MinorVersion = Version.dwMinorVersion;
            VpOsVersionInfo->BuildNumber = Version.dwBuildNumber;
            VpOsVersionInfo->ServicePackMajor = Version.wServicePackMajor;
            VpOsVersionInfo->ServicePackMinor = Version.wServicePackMinor;
            return NO_ERROR;
        }
        return ERROR_INVALID_PARAMETER;
#else
        VpOsVersionInfo->MajorVersion = 5;
        VpOsVersionInfo->MinorVersion = 0;
        VpOsVersionInfo->BuildNumber = 2195;
        VpOsVersionInfo->ServicePackMajor = 4;
        VpOsVersionInfo->ServicePackMinor = 0;
        return NO_ERROR;
#endif
    }

    return ERROR_INVALID_PARAMETER;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortCheckForDeviceExistence(
    IN PVOID HwDeviceExtension,
    IN USHORT VendorId,
    IN USHORT DeviceId,
    IN UCHAR RevisionId,
    IN USHORT SubVendorId,
    IN USHORT SubSystemId,
    IN ULONG Flags)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PCI_DEVICE_PRESENT_INTERFACE PciDevicePresentInterface;
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION IoStack;
    ULONG PciFlags = 0;
    NTSTATUS Status;
    BOOL DevicePresent;

    TRACE_(VIDEOPRT, "VideoPortCheckForDeviceExistence\n");

    if (Flags & ~(CDE_USE_REVISION | CDE_USE_SUBSYSTEM_IDS))
    {
        WARN_(VIDEOPRT, "VideoPortCheckForDeviceExistence: Unknown flags 0x%lx\n", Flags & ~(CDE_USE_REVISION | CDE_USE_SUBSYSTEM_IDS));
        return FALSE;
    }

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    PciDevicePresentInterface.Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
    PciDevicePresentInterface.Version = 1;
    IoStack.Parameters.QueryInterface.Size = PciDevicePresentInterface.Size;
    IoStack.Parameters.QueryInterface.Version = PciDevicePresentInterface.Version;
    IoStack.Parameters.QueryInterface.Interface = (PINTERFACE)&PciDevicePresentInterface;
    IoStack.Parameters.QueryInterface.InterfaceType =
        &GUID_PCI_DEVICE_PRESENT_INTERFACE;
    Status = IopInitiatePnpIrp(DeviceExtension->NextDeviceObject,
                               &IoStatusBlock, IRP_MN_QUERY_INTERFACE, &IoStack);
    if (!NT_SUCCESS(Status))
    {
        WARN_(VIDEOPRT, "IopInitiatePnpIrp() failed! (Status 0x%lx)\n", Status);
        return FALSE;
    }

    if (Flags & CDE_USE_REVISION)
        PciFlags |= PCI_USE_REVISION;
    if (Flags & CDE_USE_SUBSYSTEM_IDS)
        PciFlags |= PCI_USE_SUBSYSTEM_IDS;

    DevicePresent = PciDevicePresentInterface.IsDevicePresent(
                        VendorId, DeviceId, RevisionId,
                        SubVendorId, SubSystemId, PciFlags);

    PciDevicePresentInterface.InterfaceDereference(PciDevicePresentInterface.Context);

    return DevicePresent;
}

/*
 * @unimplemented
 */
VP_STATUS
NTAPI
VideoPortRegisterBugcheckCallback(
    IN PVOID HwDeviceExtension,
    IN ULONG BugcheckCode,
    IN PVIDEO_BUGCHECK_CALLBACK Callback,
    IN ULONG BugcheckDataSize)
{
    UNIMPLEMENTED;
    return NO_ERROR;
}

/*
 * @implemented
 */
LONGLONG
NTAPI
VideoPortQueryPerformanceCounter(
    IN PVOID HwDeviceExtension,
    OUT PLONGLONG PerformanceFrequency OPTIONAL)
{
    LARGE_INTEGER Result;

    TRACE_(VIDEOPRT, "VideoPortQueryPerformanceCounter\n");
    Result = KeQueryPerformanceCounter((PLARGE_INTEGER)PerformanceFrequency);
    return Result.QuadPart;
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortAcquireDeviceLock(
    IN PVOID  HwDeviceExtension)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    TRACE_(VIDEOPRT, "VideoPortAcquireDeviceLock\n");
    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    KeWaitForMutexObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
    // ASSERT(Status == STATUS_SUCCESS);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortReleaseDeviceLock(
    IN PVOID HwDeviceExtension)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    TRACE_(VIDEOPRT, "VideoPortReleaseDeviceLock\n");
    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    KeReleaseMutex(&DeviceExtension->DeviceLock, FALSE);
    //ASSERT(Status == STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
VOID
NTAPI
VpNotifyEaData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Data)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortAllocateContiguousMemory(
    IN PVOID HwDeviceExtension,
    IN ULONG NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
)
{
    return MmAllocateContiguousMemory(NumberOfBytes, HighestAcceptableAddress);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortIsNoVesa(VOID)
{
    return VpNoVesa;
}
