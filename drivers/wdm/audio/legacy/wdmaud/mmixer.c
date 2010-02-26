/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/mmixer.c
 * PURPOSE:         WDM Legacy Mixer
 * PROGRAMMER:      Johannes Anderwald
 */

#include "wdmaud.h"


PVOID Alloc(ULONG NumBytes);
MIXER_STATUS Close(HANDLE hDevice);
VOID Free(PVOID Block);
VOID Copy(PVOID Src, PVOID Dst, ULONG NumBytes);
MIXER_STATUS Open(IN LPWSTR DevicePath, OUT PHANDLE hDevice);
MIXER_STATUS Control(IN HANDLE hMixer, IN ULONG dwIoControlCode, IN PVOID lpInBuffer, IN ULONG nInBufferSize, OUT PVOID lpOutBuffer, ULONG nOutBufferSize, PULONG lpBytesReturned);
MIXER_STATUS Enum(IN  PVOID EnumContext, IN  ULONG DeviceIndex, OUT LPWSTR * DeviceName, OUT PHANDLE OutHandle, OUT PHANDLE OutKey);
MIXER_STATUS OpenKey(IN HANDLE hKey, IN LPWSTR SubKey, IN ULONG DesiredAccess, OUT PHANDLE OutKey);
MIXER_STATUS CloseKey(IN HANDLE hKey);
MIXER_STATUS QueryKeyValue(IN HANDLE hKey, IN LPWSTR KeyName, OUT PVOID * ResultBuffer, OUT PULONG ResultLength, OUT PULONG KeyType);
PVOID AllocEventData(IN ULONG ExtraSize);
VOID FreeEventData(IN PVOID EventData);

MIXER_CONTEXT MixerContext =
{
    sizeof(MIXER_CONTEXT),
    NULL,
    Alloc,
    Control,
    Free,
    Open,
    Close,
    Copy,
    OpenKey,
    QueryKeyValue,
    CloseKey,
    AllocEventData,
    FreeEventData
};

GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

MIXER_STATUS
QueryKeyValue(
    IN HANDLE hKey,
    IN LPWSTR lpKeyName,
    OUT PVOID * ResultBuffer,
    OUT PULONG ResultLength,
    OUT PULONG KeyType)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    ULONG Length;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;

    /* initialize key name */
    RtlInitUnicodeString(&KeyName, lpKeyName);

    /* now query MatchingDeviceId key */
    Status = ZwQueryValueKey(hKey, &KeyName, KeyValuePartialInformation, NULL, 0, &Length);

    /* check for success */
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return MM_STATUS_UNSUCCESSFUL;

    /* allocate a buffer for key data */
    PartialInformation = ExAllocatePool(NonPagedPool, Length);

    if (!PartialInformation)
        return MM_STATUS_NO_MEMORY;


    /* now query MatchingDeviceId key */
    Status = ZwQueryValueKey(hKey, &KeyName, KeyValuePartialInformation, PartialInformation, Length, &Length);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(PartialInformation);
        return MM_STATUS_UNSUCCESSFUL;
    }

    if (KeyType)
    {
        /* return key type */
        *KeyType = PartialInformation->Type;
    }

    if (ResultLength)
    {
        /* return data length */
        *ResultLength = PartialInformation->DataLength;
    }

    *ResultBuffer = ExAllocatePool(NonPagedPool, PartialInformation->DataLength);
    if (!*ResultBuffer)
    {
        /* not enough memory */
        ExFreePool(PartialInformation);
        return MM_STATUS_NO_MEMORY;
    }

    /* copy key value */
    RtlMoveMemory(*ResultBuffer, PartialInformation->Data, PartialInformation->DataLength);

    /* free key info */
    ExFreePool(PartialInformation);

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
OpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKeyName,
    IN ULONG DesiredAccess,
    OUT PHANDLE OutKey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyName;
    NTSTATUS Status;

    /* initialize sub key name */
    RtlInitUnicodeString(&SubKeyName, lpSubKeyName);

    /* initialize key attributes */
    InitializeObjectAttributes(&ObjectAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE | OBJ_OPENIF, hKey, NULL);

    /* open the key */
    Status = ZwOpenKey(OutKey, DesiredAccess, &ObjectAttributes);

    if (NT_SUCCESS(Status))
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

MIXER_STATUS
CloseKey(
    IN HANDLE hKey)
{
    if (ZwClose(hKey) == STATUS_SUCCESS)
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}


PVOID Alloc(ULONG NumBytes)
{
    PVOID Mem = ExAllocatePool(NonPagedPool, NumBytes);
    if (!Mem)
        return Mem;

    RtlZeroMemory(Mem, NumBytes);
    return Mem;
}

MIXER_STATUS
Close(HANDLE hDevice)
{
    if (ZwClose(hDevice) == STATUS_SUCCESS)
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

VOID
Free(PVOID Block)
{
    ExFreePool(Block);
}

VOID
Copy(PVOID Src, PVOID Dst, ULONG NumBytes)
{
    RtlMoveMemory(Src, Dst, NumBytes);
}

MIXER_STATUS
Open(
    IN LPWSTR DevicePath,
    OUT PHANDLE hDevice)
{
    if (WdmAudOpenSysAudioDevice(DevicePath, hDevice) == STATUS_SUCCESS)
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

MIXER_STATUS
Control(
    IN HANDLE hMixer,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    ULONG nOutBufferSize,
    PULONG lpBytesReturned)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;

    /* get file object */
    Status = ObReferenceObjectByHandle(hMixer, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("failed to reference %p with %lx\n", hMixer, Status);
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* perform request */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned);

    /* release object reference */
    ObDereferenceObject(FileObject);

    if (Status == STATUS_MORE_ENTRIES || Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* more data is available */
        return MM_STATUS_MORE_ENTRIES;
    }
    else if (Status == STATUS_SUCCESS)
    {
        /* operation succeeded */
        return MM_STATUS_SUCCESS;
    }
    else
    {
        DPRINT("Failed with %lx\n", Status);
        return MM_STATUS_UNSUCCESSFUL;
    }
}

MIXER_STATUS
Enum(
    IN  PVOID EnumContext,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * DeviceName,
    OUT PHANDLE OutHandle,
    OUT PHANDLE OutKey)
{
    PDEVICE_OBJECT DeviceObject;
    ULONG DeviceCount;
    NTSTATUS Status;
    UNICODE_STRING KeyName;

    /* get enumeration context */
    DeviceObject = (PDEVICE_OBJECT)EnumContext;

    /* get device count */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    if (DeviceIndex >= DeviceCount)
    {
        /* no more devices */
        return MM_STATUS_NO_MORE_DEVICES;
    }

    /* get device name */
    Status = GetSysAudioDevicePnpName(DeviceObject, DeviceIndex, DeviceName);

    if (!NT_SUCCESS(Status))
    {
        /* failed to retrieve device name */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* intialize key name */
    RtlInitUnicodeString(&KeyName, *DeviceName);

    /* open device interface key */
    Status = IoOpenDeviceInterfaceRegistryKey(&KeyName, GENERIC_READ | GENERIC_WRITE, OutKey);
#if 0
    if (!NT_SUCCESS(Status))
    {
        /* failed to open key */
        DPRINT("IoOpenDeviceInterfaceRegistryKey failed with %lx\n", Status);
        ExFreePool(*DeviceName);
        return MM_STATUS_UNSUCCESSFUL;
    }
#endif

    /* open device handle */
    Status = OpenDevice(*DeviceName, OutHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* failed to open device */
        return MM_STATUS_UNSUCCESSFUL;
    }

    return MM_STATUS_SUCCESS;
}

PVOID
AllocEventData(
    IN ULONG ExtraSize)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)ExAllocatePool(NonPagedPool, sizeof(KSEVENTDATA) + ExtraSize);
    if (!Data)
        return NULL;

    Data->EventObject.Event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
    if (!Data->EventHandle.Event)
    {
        ExFreePool(Data);
        return NULL;
    }

    KeInitializeEvent(Data->EventObject.Event, NotificationEvent, FALSE);

    Data->NotificationType = KSEVENTF_EVENT_HANDLE;
    return Data;
}

VOID
FreeEventData(IN PVOID EventData)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)EventData;

    ExFreePool(Data->EventHandle.Event);
    ExFreePool(Data);
}

NTSTATUS
WdmAudMixerInitialize(
    IN PDEVICE_OBJECT DeviceObject)
{
    MIXER_STATUS Status;

    /* initialize the mixer library */
    Status = MMixerInitialize(&MixerContext, Enum, (PVOID)DeviceObject);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to initialize mmixer library */
        DPRINT("MMixerInitialize failed with %lx\n", Status);
    }

    return Status;
}

NTSTATUS
WdmAudMixerCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    if (MMixerGetCapabilities(&MixerContext, DeviceInfo->DeviceIndex, &DeviceInfo->u.MixCaps) == MM_STATUS_SUCCESS)
        return STATUS_SUCCESS;

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
WdmAudControlOpenMixer(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    HANDLE hMixer;
    PWDMAUD_HANDLE Handles;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PKEVENT EventObject = NULL;

    DPRINT("WdmAudControlOpenMixer\n");

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->u.hNotifyEvent)
    {
        Status = ObReferenceObjectByHandle(DeviceInfo->u.hNotifyEvent, EVENT_MODIFY_STATE, ExEventObjectType, UserMode, (LPVOID*)&EventObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Invalid notify event passed %p from client %p\n", DeviceInfo->u.hNotifyEvent, ClientInfo);
            DbgBreakPoint();
            return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
        }
    }

    if (MMixerOpen(&MixerContext, DeviceInfo->DeviceIndex, EventObject, NULL /* FIXME */, &hMixer) != MM_STATUS_SUCCESS)
    {
        ObDereferenceObject(EventObject);
        DPRINT1("Failed to open mixer\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }


    Handles = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

    if (Handles)
    {
        if (ClientInfo->NumPins)
        {
            RtlMoveMemory(Handles, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
            ExFreePool(ClientInfo->hPins);
        }

        ClientInfo->hPins = Handles;
        ClientInfo->hPins[ClientInfo->NumPins].Handle = hMixer;
        ClientInfo->hPins[ClientInfo->NumPins].Type = MIXER_DEVICE_TYPE;
        ClientInfo->hPins[ClientInfo->NumPins].NotifyEvent = EventObject;
        ClientInfo->NumPins++;
    }
    else
    {
        ObDereferenceObject(EventObject);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
    }

    DeviceInfo->hDevice = hMixer;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    MIXER_STATUS Status;

    /* clear hmixer type flag */
    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    /* query mmixer library */
    Status = MMixerGetControlDetails(&MixerContext, DeviceInfo->hDevice, DeviceInfo->Flags, &DeviceInfo->u.MixDetails);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetLineInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    MIXER_STATUS Status;

    /* clear hmixer type flag */
    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    /* query mixer library */
    Status = MMixerGetLineInfo(&MixerContext, DeviceInfo->hDevice, DeviceInfo->Flags, &DeviceInfo->u.MixLine);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetLineControls(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    MIXER_STATUS Status;

    /* clear hmixer type flag */
    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    /* query mixer library */
    Status = MMixerGetLineControls(&MixerContext, DeviceInfo->hDevice, DeviceInfo->Flags, &DeviceInfo->u.MixControls);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));


}

NTSTATUS
NTAPI
WdmAudSetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    MIXER_STATUS Status;

    /* clear hmixer type flag */
    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    /* query mixer library */
    Status = MMixerSetControlDetails(&MixerContext, DeviceInfo->hDevice, DeviceInfo->Flags, &DeviceInfo->u.MixDetails);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetMixerEvent(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED
    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
}

ULONG
WdmAudGetMixerDeviceCount()
{
    return MMixerGetCount(&MixerContext);
}

ULONG
WdmAudGetWaveInDeviceCount()
{
    return MMixerGetWaveInCount(&MixerContext);
}

ULONG
WdmAudGetWaveOutDeviceCount()
{
    return MMixerGetWaveOutCount(&MixerContext);
}

NTSTATUS
WdmAudGetMixerPnpNameByIndex(
    IN  ULONG DeviceIndex,
    OUT LPWSTR * Device)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
WdmAudGetPnpNameByIndexAndType(
    IN ULONG DeviceIndex, 
    IN SOUND_DEVICE_TYPE DeviceType, 
    OUT LPWSTR *DevicePath)
{
    if (MMixerGetWaveDevicePath(&MixerContext, DeviceType == WAVE_IN_DEVICE_TYPE, DeviceIndex, DevicePath) == MM_STATUS_SUCCESS)
        return STATUS_SUCCESS;
    else
        return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WdmAudWaveCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    MIXER_STATUS Status;

    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        /* get capabilities */
        Status = MMixerWaveInCapabilities(&MixerContext, DeviceInfo->DeviceIndex, &DeviceInfo->u.WaveInCaps);
    }
    else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        /* get capabilities */
        Status = MMixerWaveOutCapabilities(&MixerContext, DeviceInfo->DeviceIndex, &DeviceInfo->u.WaveOutCaps);
    }
    else
    {
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }

    if (Status == MM_STATUS_SUCCESS)
        return STATUS_SUCCESS;
    else
        return STATUS_UNSUCCESSFUL;
}


MIXER_STATUS
CreatePinCallback(
    IN PVOID Ctx,
    IN ULONG VirtualDeviceId,
    IN ULONG PinId,
    IN HANDLE hFilter,
    IN PKSPIN_CONNECT PinConnect,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE PinHandle)
{
    ULONG BytesReturned;
    SYSAUDIO_INSTANCE_INFO InstanceInfo;
    NTSTATUS Status;
    ULONG FreeIndex;
    PPIN_CREATE_CONTEXT Context = (PPIN_CREATE_CONTEXT)Ctx;

    /* setup property request */
    InstanceInfo.Property.Set = KSPROPSETID_Sysaudio;
    InstanceInfo.Property.Id = KSPROPERTY_SYSAUDIO_INSTANCE_INFO;
    InstanceInfo.Property.Flags = KSPROPERTY_TYPE_SET;
    InstanceInfo.Flags = 0;
    InstanceInfo.DeviceNumber = VirtualDeviceId;

    /* attach to virtual device */
    Status = KsSynchronousIoControlDevice(Context->DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&InstanceInfo, sizeof(SYSAUDIO_INSTANCE_INFO), NULL, 0, &BytesReturned);

    if (!NT_SUCCESS(Status))
        return MM_STATUS_UNSUCCESSFUL;

    /* close existing pin */
    FreeIndex = ClosePin(Context->ClientInfo, VirtualDeviceId, PinId, Context->DeviceType);

    /* now create the pin */
    Status = KsCreatePin(Context->DeviceExtension->hSysAudio, PinConnect, DesiredAccess, PinHandle);

    /* check for success */
    if (!NT_SUCCESS(Status))
        return MM_STATUS_UNSUCCESSFUL;

    /* store the handle */
    Status = InsertPinHandle(Context->ClientInfo, VirtualDeviceId, PinId, Context->DeviceType, *PinHandle, FreeIndex);
    if (!NT_SUCCESS(Status))
    {
        /* failed to insert handle */
        ZwClose(*PinHandle);
        return MM_STATUS_UNSUCCESSFUL;
    }

    return MM_STATUS_SUCCESS;
}

NTSTATUS
WdmAudControlOpenWave(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    MIXER_STATUS Status;
    PIN_CREATE_CONTEXT Context;

    Context.ClientInfo = ClientInfo;
    Context.DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Context.DeviceType = DeviceInfo->DeviceType;

    Status = MMixerOpenWave(&MixerContext, DeviceInfo->DeviceIndex, DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE, &DeviceInfo->u.WaveFormatEx, CreatePinCallback, &Context, &DeviceInfo->hDevice);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_NOT_SUPPORTED, sizeof(WDMAUD_DEVICE_INFO));
}
