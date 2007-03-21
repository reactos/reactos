/*
    ReactOS
    Sound Blaster driver

    Programmers:
        Andrew Greenwood

    Notes:
        Compatible with NT4
*/

#define NDEBUG
#include <sndblst.h>

#define TAG(A, B, C, D) (IN ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

/*
    IRP DISPATCH ROUTINES
*/

NTSTATUS STDCALL
CreateSoundBlaster(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PSOUND_BLASTER_PARAMETERS sb_device = DeviceObject->DeviceExtension;

    DPRINT("CreateSoundBlaster() called - extension 0x%x\n", sb_device);

    EnableSpeaker(sb_device);
    /*SetOutputSampleRate(sb_device, 22*/

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CloseSoundBlaster(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    //PSOUND_BLASTER_PARAMETERS sb_device = DeviceObject->DeviceExtension;

    DPRINT("CloseSoundBlaster() called\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CleanupSoundBlaster(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    //PSOUND_BLASTER_PARAMETERS sb_device = DeviceObject->DeviceExtension;

    DPRINT("CleanupSoundBlaster() called\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
ControlSoundBlaster(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    //PSOUND_BLASTER_PARAMETERS sb_device = DeviceObject->DeviceExtension;

    DPRINT("ControlSoundBlaster() called\n");

    stack = IoGetCurrentIrpStackLocation(Irp);

    switch ( stack->Parameters.DeviceIoControl.IoControlCode)
    {
        /* TODO */
    };

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
WriteSoundBlaster(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    //PSOUND_BLASTER_PARAMETERS sb_device = DeviceObject->DeviceExtension;

    DPRINT("WriteSoundBlaster() called\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID STDCALL
UnloadSoundBlaster(
    PDRIVER_OBJECT DriverObject)
{
    DPRINT("Sound Blaster driver unload\n");
}

NTSTATUS STDCALL
OpenSubkey(
    PUNICODE_STRING RegistryPath,
    PWSTR Subkey,
    ACCESS_MASK DesiredAccess,
    OUT HANDLE* DevicesKeyHandle)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attribs;
    UNICODE_STRING subkey_name;
    HANDLE key_handle;

    /* TODO: Check for NULL ptr in DevicesKeyHandle */

    InitializeObjectAttributes(&attribs,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&key_handle, KEY_READ, &attribs);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT("Couldn't open subkey %wZ\n", Subkey);
        return status;
    }

    RtlInitUnicodeString(&subkey_name, Subkey);

    InitializeObjectAttributes(&attribs,
                               &subkey_name,
                               OBJ_CASE_INSENSITIVE,
                               key_handle,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(*DevicesKeyHandle, DesiredAccess, &attribs);
    ZwClose(key_handle);

    return status;
}


PWSTR STDCALL
AllocateRegistryPathInfo(
    PUNICODE_STRING BasePath,
    PUNICODE_STRING ParametersPath,
    PKEY_BASIC_INFORMATION KeyInfo)
{
    PWSTR name;
    PWSTR pos;

    DPRINT("Allocating memory for path info\n");
    name = ExAllocatePool(PagedPool,
                          BasePath->Length + sizeof(WCHAR) +
                          ParametersPath->Length + sizeof(WCHAR) +
                          KeyInfo->NameLength + sizeof(UNICODE_NULL));

    if ( ! name )
        return NULL;

    DPRINT("Copying info\n");
    pos = name;

    RtlCopyMemory((PVOID)Pos, (PVOID)BasePath->Buffer, BasePath->Length);
    pos += BasePath->Length / sizeof(WCHAR);
    pos[0] = '\\';
    pos ++;

    RtlCopyMemory((PVOID)Pos, (PVOID)ParametersPath->Buffer, ParametersPath->Length);
    pos += ParametersPath->Length / sizeof(WCHAR);
    pos[0] = '\\';
    pos ++;

    RtlCopyMemory((PVOID)Pos, (PVOID)ParametersPath->Buffer, ParametersPath->Length);
    pos += KeyInfo->NameLength / sizeof(WCHAR);
    pos[0] = UNICODE_NULL;

    DPRINT("All OK\n");
    return name;
}

#define FreeRegistryPathInfo(ptr) \
    ExFreePool(ptr)


#define TAG_REG_INFO TAG('I','g','e','R')
#define TAG_REG_NAME TAG('N','g','e','R')

NTSTATUS STDCALL
EnumerateSubkey(
    PUNICODE_STRING RegistryPath,
    PWSTR Subkey,
    PREGISTRY_CALLBACK_ROUTINE Callback,
    PDRIVER_OBJECT DriverObject)
{
    NTSTATUS status;
    UNICODE_STRING subkey_name;
    HANDLE devices_key_handle;

    ULONG key_index = 0;
    ULONG result_length;

    status = OpenSubkey(RegistryPath, Subkey, KEY_ENUMERATE_SUB_KEYS, &devices_key_handle);

    if ( ! NT_SUCCESS(status) )
        return status;

    while ( TRUE )
    {
        KEY_BASIC_INFORMATION test_info;
        PKEY_BASIC_INFORMATION info;
        ULONG size;
        PWSTR name;

        status = ZwEnumerateKey(devices_key_handle,
                                key_index,
                                KeyBasicInformation,
                                &test_info,
                                sizeof(test_info),
                                &result_length);

        if ( status == STATUS_NO_MORE_ENTRIES )
            break;

        size = result_length + FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);

        info = (PKEY_BASIC_INFORMATION) ExAllocatePoolWithTag(PagedPool, size, TAG_REG_INFO);

        if ( ! info )
        {
            DPRINT("Out of memory\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = ZwEnumerateKey(devices_key_handle,
                                key_index,
                                KeyBasicInformation,
                                info,
                                size,
                                &result_length);

        if ( ! NT_SUCCESS(status) )
        {
            DPRINT("Unable to enumerate keys\n");
            ExFreePoolWithTag(info, TAG_REG_INFO);
            status = STATUS_INTERNAL_ERROR;
            break;
        }

        /* Is this ok? */
        RtlInitUnicodeString(&subkey_name, Subkey);

        name = AllocateRegistryPathInfo(RegistryPath, &subkey_name, info);

        if ( ! name )
        {
            DPRINT("Out of memory\n");
            ExFreePoolWithTag(info, TAG_REG_INFO);
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ExFreePoolWithTag(info, TAG_REG_INFO);

        /* Call the callback */
        status = Callback(DriverObject, name);

        FreeRegistryPathInfo(name);

        if ( ! NT_SUCCESS(status) )
        {
            DPRINT("Callback FAILED\n");
            break;
        }

        key_index ++;
    }

    ZwClose(devices_key_handle);

    DPRINT("Found %d subkey entries\n", key_index);

    if ( ( key_index == 0 ) && ( status == STATUS_NO_MORE_ENTRIES ) )
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    if ( status == STATUS_NO_MORE_ENTRIES )
        status = STATUS_SUCCESS;

    return status;
}

#define EnumerateDeviceKeys(path, callback, driver_obj) \
    EnumerateSubkey(path, L"Devices", callback, driver_obj)


NTSTATUS
CreateDeviceName(
    PCWSTR PrePrefix,
    PCWSTR Prefix,
    UCHAR Index,
    PUNICODE_STRING DeviceName)
{
    UNICODE_STRING number;
    WCHAR number_buffer[5];
    UNICODE_STRING unicode_pre_prefix;
    UNICODE_STRING unicode_prefix;
    ULONG size;

    RtlInitUnicodeString(&unicode_pre_prefix, PrePrefix);
    RtlInitUnicodeString(&unicode_prefix, Prefix);

    size = unicode_pre_prefix.Length +
           unicode_prefix.Length +
           sizeof(number_buffer) +
           sizeof(UNICODE_NULL);

    DeviceName->Buffer = ExAllocatePool(PagedPool, size);
    DeviceName->MaximumLength = (USHORT) size;

    if ( ! DeviceName->Buffer )
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyUnicodeString(DeviceName, &unicode_pre_prefix);
    RtlAppendUnicodeStringToString(DeviceName, &unicode_prefix);

    if ( Index != 255 )
    {
        number.Buffer = number_buffer;
        number.MaximumLength = sizeof(number_buffer);

        RtlIntegerToUnicodeString((ULONG) Index, 10, &number);
        RtlAppendUnicodeStringToString(DeviceName, &number);
    }

    DeviceName->Buffer[DeviceName->Length / sizeof(UNICODE_NULL)] = UNICODE_NULL;

    return STATUS_SUCCESS; 
}

NTSTATUS STDCALL
InitializeSoundBlaster(
    PDRIVER_OBJECT DriverObject,
    PWSTR RegistryPath)
{
    NTSTATUS status;
    PDEVICE_OBJECT device_object;
    PSOUND_BLASTER_PARAMETERS parameters = NULL;
    UNICODE_STRING device_name;
    UNICODE_STRING dos_device_name;

    UCHAR device_index = 0;

    DPRINT("Initializing a Sound Blaster device\n");

    /* Change these later */
    status = CreateDeviceName(L"",
                              L"\\Device\\WaveOut",
                              device_index,
                              &device_name);

    if ( ! NT_SUCCESS(status) )
        return status;

    status = CreateDeviceName(L"\\DosDevices\\",
                              L"\\Device\\WaveOut" + wcslen(L"\\Device\\"),
                              device_index,
                              &dos_device_name);

    if ( ! NT_SUCCESS(status) )
    {
        /* TODO */
        return status;
    }

    DPRINT("Device: %wZ\n", device_name);
    DPRINT("Symlink: %wZ\n", dos_device_name);

    /*
        Create the device and DOS symlink
    */

    status = IoCreateDevice(DriverObject,
                            sizeof(SOUND_BLASTER_PARAMETERS),
                            &device_name,
                            FILE_DEVICE_SOUND,
                            0,
                            FALSE,
                            &device_object);

    if ( ! NT_SUCCESS(status) )
        return status;

    DPRINT("Created a device extension at 0x%x\n", device_object->DeviceExtension);
    parameters = device_object->DeviceExtension;

    status = IoCreateSymbolicLink(&dos_device_name, &device_name);

    ExFreePool(dos_device_name.Buffer);
    ExFreePool(device_name.Buffer);

    if ( ! NT_SUCCESS(status) )
    {
        IoDeleteDevice(device_object);
        device_object = NULL;
        return status;
    }

    /* IoRegisterShutdownNotification( */

    /*
        Settings
    */

    device_object->AlignmentRequirement = FILE_BYTE_ALIGNMENT;

    parameters->driver = DriverObject;
    parameters->registry_path = RegistryPath;
    parameters->port = DEFAULT_PORT;
    parameters->irq = DEFAULT_IRQ;
    parameters->dma = DEFAULT_DMA;
    parameters->buffer_size = DEFAULT_BUFFER_SIZE;

    /* TODO: Load the settings from the registry */

    DPRINT("Port %x IRQ %d DMA %d\n", parameters->port, parameters->irq, parameters->dma);

    DPRINT("Resetting the sound card\n");

    if ( ! ResetSoundBlaster(parameters) )
    {
        /* TODO */
        return STATUS_UNSUCCESSFUL;
    }

    /*
    DPRINT("What kind of SB card is this?\n");
    GetSoundBlasterModel(parameters);
    */

    return STATUS_SUCCESS;
}


NTSTATUS STDCALL
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    DPRINT("Sound Blaster driver 0.1 by Silver Blade\n");

    DriverObject->Flags = 0;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanupSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteSoundBlaster;
    DriverObject->DriverUnload = UnloadSoundBlaster;

    DPRINT("Beginning device key enumeration\n");

    status = EnumerateDeviceKeys(RegistryPath, *InitializeSoundBlaster, DriverObject);

    return status;
}
