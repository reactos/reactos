/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Device Driver Helper
 * FILE:            base/setup/setupdd/setupdd.c
 * PURPOSE:         Management Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "setupdd.h"

#define NDEBUG
#include <debug.h>


/* UTILITY FUNCTIONS **********************************************************/

VOID
DumpDetectedDeviceRegistry(
    IN PDETECTED_DEVICE_REGISTRY DeviceRegistry)
{
    DbgPrint("DETECTED_DEVICE_REGISTRY 0x%p\n"
             "    Next = 0x%p\n"
             "    KeyName = 0x%p '%s'\n"
             "    ValueName = 0x%p '%s'\n"
             "    ValueType = %lu\n"
             "    Buffer = 0x%p\n"
             "    BufferSize = %lu\n",
        DeviceRegistry,
        DeviceRegistry->Next,
        DeviceRegistry->KeyName, DeviceRegistry->KeyName,
        DeviceRegistry->ValueName, DeviceRegistry->ValueName,
        DeviceRegistry->ValueType,
        DeviceRegistry->Buffer,
        DeviceRegistry->BufferSize);

    if (DeviceRegistry->Next)
    {
        DbgPrint("--> ");
        DumpDetectedDeviceRegistry(DeviceRegistry->Next);
    }
}

VOID
DumpDetectedDeviceFile(
    IN PDETECTED_DEVICE_FILE DeviceFile)
{
    DbgPrint("DETECTED_DEVICE_FILE 0x%p\n"
             "    Next = 0x%p\n"
             "    FileName = 0x%p '%s'\n"
             "    FileType = %lu\n"
             "    ConfigName = 0x%p '%s'\n"
             "    RegistryValueList = 0x%p\n"
             "    DiskDescription = 0x%p '%s'\n"
             "    DiskTagfile = 0x%p '%s'\n"
             "    Directory = 0x%p '%s'\n"
             "    ArcDeviceName = 0x%p '%s'\n",
        DeviceFile,
        DeviceFile->Next,
        DeviceFile->FileName, DeviceFile->FileName,
        DeviceFile->FileType,
        DeviceFile->ConfigName, DeviceFile->ConfigName,
        DeviceFile->RegistryValueList,
        DeviceFile->DiskDescription, DeviceFile->DiskDescription,
        DeviceFile->DiskTagfile, DeviceFile->DiskTagfile,
        DeviceFile->Directory, DeviceFile->Directory,
        DeviceFile->ArcDeviceName, DeviceFile->ArcDeviceName);

    if (DeviceFile->RegistryValueList)
    {
        DbgPrint("RegistryValueList ");
        DumpDetectedDeviceRegistry(DeviceFile->RegistryValueList);
        /*DbgPrint("\n");*/
    }

    if (DeviceFile->Next)
    {
        DbgPrint("--> ");
        DumpDetectedDeviceFile(DeviceFile->Next);
    }
}

VOID
DumpPnpHwID(
    IN PPNP_HARDWARE_ID PnpHwID)
{
    DbgPrint("PNP_HARDWARE_ID 0x%p\n"
             "    Next = 0x%p\n"
             "    Id = 0x%p '%s'\n"
             "    DriverName = 0x%p '%s'\n"
             "    ClassGuid = 0x%p '%s'\n",
        PnpHwID,
        PnpHwID->Next,
        PnpHwID->Id, PnpHwID->Id,
        PnpHwID->DriverName, PnpHwID->DriverName,
        PnpHwID->ClassGuid, PnpHwID->ClassGuid);

    if (PnpHwID->Next)
    {
        DbgPrint("--> ");
        DumpPnpHwID(PnpHwID->Next);
    }
}

VOID
DumpDetectedDevice(
    IN PDETECTED_DEVICE DetectedDevice)
{
    DbgPrint("DETECTED_DEVICE 0x%p\n"
             "    Next = 0x%p\n"
             "    IdString = 0x%p '%s'\n"
             "    Ordinal = %lu\n"
             "    Description = 0x%p '%s'\n"
             "    ThirdPartyOptionSelected = %s\n"
             "    FileTypeBits = %lu\n"
             "    Files = 0x%p\n"
             "    BaseDllName = 0x%p '%s'\n"
             "    MigratedDriver = %s\n"
             "    HardwareIds = 0x%p\n",
        DetectedDevice,
        DetectedDevice->Next,
        DetectedDevice->IdString, DetectedDevice->IdString,
        DetectedDevice->Ordinal,
        DetectedDevice->Description, DetectedDevice->Description,
        DetectedDevice->ThirdPartyOptionSelected ? "TRUE" : "FALSE",
        DetectedDevice->FileTypeBits,
        DetectedDevice->Files,
        DetectedDevice->BaseDllName, DetectedDevice->BaseDllName,
        DetectedDevice->MigratedDriver ? "TRUE" : "FALSE",
        DetectedDevice->HardwareIds);
    /*DbgPrint("\n");*/

    if (DetectedDevice->Files)
    {
        DbgPrint("Files ");
        DumpDetectedDeviceFile(DetectedDevice->Files);
        /*DbgPrint("\n");*/
    }

    if (DetectedDevice->HardwareIds)
    {
        DbgPrint("HardwareIds ");
        DumpPnpHwID(DetectedDevice->HardwareIds);
        /*DbgPrint("\n");*/
    }

    if (DetectedDevice->Next)
    {
        DbgPrint("--> ");
        DumpDetectedDevice(DetectedDevice->Next);
    }
}

VOID
DumpDetectedOemSrcDevice(
    IN PDETECTED_OEM_SOURCE_DEVICE OemSrcDevice)
{
    DbgPrint("DETECTED_OEM_SOURCE_DEVICE 0x%p\n"
             "    Next = 0x%p\n"
             "    ArcDeviceName = 0x%p '%s'\n"
             "    ImageBase = %lu\n"
             "    ImageSize = %I64u\n",
        OemSrcDevice,
        OemSrcDevice->Next,
        OemSrcDevice->ArcDeviceName, OemSrcDevice->ArcDeviceName,
        OemSrcDevice->ImageBase,
        OemSrcDevice->ImageSize);

    if (OemSrcDevice->Next)
    {
        DbgPrint("--> ");
        DumpDetectedOemSrcDevice(OemSrcDevice->Next);
    }
}

VOID
DumpSetupLoaderBlock(IN PSETUP_LOADER_BLOCK SetupLdrBlock)
{
    DbgPrint("SETUP_LOADER_BLOCK 0x%p\n"
             "    ArcSetupDeviceName = 0x%p '%s'\n"
             "    VideoDevice = 0x%p\n"
             "    KeyboardDevices = 0x%p\n"
             "    ComputerDevice = 0x%p\n"
             "    ScsiDevices = 0x%p\n"
             "    ScalarValues: SetupOperation = %lu ; Flags = %lu\n"
             "    IniFile = 0x%p '%.*s'\n"
             "    WinntSif = 0x%p '%.*s'\n"
             "    MigrateInf = 0x%p '%.*s'\n"
             "    UnsupDriversInf = 0x%p '%.*s'\n"
             "    BootFontFile = 0x%p\n"
             "    BootFontFileLength = %lu\n"
             "    Monitor = 0x%p (TODO!)\n"
             "    MonitorId = 0x%p '%s'\n"
             "    BootBusExtenders = 0x%p\n"
             "    BusExtenders = 0x%p\n"
             "    InputDevicesSupport = 0x%p\n"
             "    HardwareIdDatabase = 0x%p\n"
             "    ComputerName = '%s'\n",
             // "    IpAddress = %02x.%02x.%02x.%02x\n"
             // "    SubnetMask = %02x.%02x.%02x.%02x\n"
             // "    ServerIpAddress = %02x.%02x.%02x.%02x\n"
             // "    DefaultRouter = %02x.%02x.%02x.%02x\n"
             // "    DnsNameServer = %02x.%02x.%02x.%02x\n",

    SetupLdrBlock,
    SetupLdrBlock->ArcSetupDeviceName, SetupLdrBlock->ArcSetupDeviceName,
    &SetupLdrBlock->VideoDevice,
    SetupLdrBlock->KeyboardDevices,
    &SetupLdrBlock->ComputerDevice,
    SetupLdrBlock->ScsiDevices,
    SetupLdrBlock->ScalarValues.SetupOperation,
    SetupLdrBlock->ScalarValues.AsULong,
    SetupLdrBlock->IniFile, SetupLdrBlock->IniFileLength, SetupLdrBlock->IniFile,
    SetupLdrBlock->WinntSifFile, SetupLdrBlock->WinntSifFileLength, SetupLdrBlock->WinntSifFile,
    SetupLdrBlock->MigrateInfFile, SetupLdrBlock->MigrateInfFileLength, SetupLdrBlock->MigrateInfFile,
    SetupLdrBlock->UnsupDriversInfFile, SetupLdrBlock->UnsupDriversInfFileLength, SetupLdrBlock->UnsupDriversInfFile,
    SetupLdrBlock->BootFontFile,
    SetupLdrBlock->BootFontFileLength,
    SetupLdrBlock->Monitor,
    SetupLdrBlock->MonitorId, SetupLdrBlock->MonitorId,
    SetupLdrBlock->BootBusExtenders,
    SetupLdrBlock->BusExtenders,
    SetupLdrBlock->InputDevicesSupport,
    SetupLdrBlock->HardwareIdDatabase,
    SetupLdrBlock->ComputerName // [64]
    // SetupLdrBlock->IpAddress,
    // SetupLdrBlock->SubnetMask,
    // SetupLdrBlock->ServerIpAddress,
    // SetupLdrBlock->DefaultRouter,
    // SetupLdrBlock->DnsNameServer
    );
    // WCHAR NetbootCardHardwareId[64];
    // WCHAR NetbootCardDriverName[24];
    // WCHAR NetbootCardServiceName[24];
    // PCHAR NetbootCardRegistry;
    // ULONG NetbootCardRegistryLength;
    // PCHAR NetbootCardInfo;
    // ULONG NetbootCardInfoLength;
    // ULONG Flags;
    // PCHAR MachineDirectoryPath;
    // PCHAR NetBootSifPath;
    // PVOID NetBootSecret;
    // CHAR NetBootIMirrorFilePath[260];
    // PCHAR ASRPnPSifFile;
    // ULONG ASRPnPSifFileLength;
    // CHAR NetBootAdministratorPassword[64];
    /*DbgPrint("\n");*/

    DbgPrint("VideoDevice ");
    DumpDetectedDevice(&SetupLdrBlock->VideoDevice);
    DbgPrint("\n");

    if (SetupLdrBlock->KeyboardDevices)
    {
        DbgPrint("KeyboardDevices ");
        DumpDetectedDevice(SetupLdrBlock->KeyboardDevices);
        DbgPrint("\n");
    }

    DbgPrint("ComputerDevice ");
    DumpDetectedDevice(&SetupLdrBlock->ComputerDevice);
    DbgPrint("\n");

    if (SetupLdrBlock->ScsiDevices)
    {
        DbgPrint("ScsiDevices ");
        DumpDetectedDevice(SetupLdrBlock->ScsiDevices);
        DbgPrint("\n");
    }

    // PMONITOR_CONFIGURATION_DATA Monitor; // TODO!!

    if (SetupLdrBlock->BootBusExtenders)
    {
        DbgPrint("BootBusExtenders ");
        DumpDetectedDevice(SetupLdrBlock->BootBusExtenders);
        DbgPrint("\n");
    }

    if (SetupLdrBlock->BusExtenders)
    {
        DbgPrint("BusExtenders ");
        DumpDetectedDevice(SetupLdrBlock->BusExtenders);
        DbgPrint("\n");
    }

    if (SetupLdrBlock->InputDevicesSupport)
    {
        DbgPrint("InputDevicesSupport ");
        DumpDetectedDevice(SetupLdrBlock->InputDevicesSupport);
        DbgPrint("\n");
    }

    if (SetupLdrBlock->HardwareIdDatabase)
    {
        DbgPrint("HardwareIdDatabase ");
        DumpPnpHwID(SetupLdrBlock->HardwareIdDatabase);
        DbgPrint("\n");
    }
}


/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI
CompleteRequest(IN PIRP      Irp,
                IN NTSTATUS  Status,
                IN ULONG_PTR Information);

/*
 * Callback functions prototypes
 */
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     SpDdUnload;

DRIVER_DISPATCH SpDdDispatch;
// DRIVER_DISPATCH ConDrvIoCtl;

DRIVER_DISPATCH SpDdCreate;
DRIVER_DISPATCH SpDdClose;
// DRIVER_DISPATCH ConDrvCleanup;


NTSTATUS
NTAPI
SpDdCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return CompleteRequest(Irp, STATUS_SUCCESS, FILE_OPENED);
}

NTSTATUS
NTAPI
SpDdClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}


NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    USHORT i;

    DPRINT1("Loading ReactOS Setup Driver v0.0.1...\n");

    /* Check whether we have a loader block */
    if (!KeLoaderBlock)
    {
        DPRINT1("No NT loader block present, quit!\n");
        return STATUS_SUCCESS;
    }
    DbgPrint("\n"
             "Command Line: %s\n"
             "ARC Paths: %s %s %s %s\n\n",
             KeLoaderBlock->LoadOptions,
             KeLoaderBlock->ArcBootDeviceName,
             KeLoaderBlock->NtHalPathName,
             KeLoaderBlock->ArcHalDeviceName,
             KeLoaderBlock->NtBootPathName);

    /* Check whether we have a setup loader block */
    if (!KeLoaderBlock->SetupLdrBlock)
    {
        DPRINT1("No NT setup loader block present, quit!\n");
        return STATUS_SUCCESS;
    }
    DumpSetupLoaderBlock(KeLoaderBlock->SetupLdrBlock);
    DbgPrint("\n");

    DriverObject->DriverUnload = SpDdUnload;

    /* Initialize the different callback function pointers */
    for (i = 0 ; i <= IRP_MJ_MAXIMUM_FUNCTION ; ++i)
        DriverObject->MajorFunction[i] = SpDdDispatch;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SpDdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SpDdClose;

    // DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ConDrvCleanup;
    // DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ConDrvIoCtl;

    Status = STATUS_SUCCESS;

    DPRINT1("Done, Status = 0x%08lx\n", Status);
    return Status;
}

VOID
NTAPI
SpDdUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("Unloading ReactOS Setup Driver v0.0.1...\n");

    /* Sanity check: No devices must exist at this point */
    ASSERT(DriverObject->DeviceObject == NULL);

    DPRINT1("Done\n");
    return;
}

/* EOF */
