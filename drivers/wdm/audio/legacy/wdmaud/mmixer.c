/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/mmixer.c
 * PURPOSE:         WDM Legacy Mixer
 * PROGRAMMER:      Johannes Anderwald
 */

#include "wdmaud.h"

#include <mmixer.h>

#define NDEBUG
#include <debug.h>

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
    PartialInformation = AllocateItem(NonPagedPool, Length);

    if (!PartialInformation)
        return MM_STATUS_NO_MEMORY;


    /* now query MatchingDeviceId key */
    Status = ZwQueryValueKey(hKey, &KeyName, KeyValuePartialInformation, PartialInformation, Length, &Length);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        FreeItem(PartialInformation);
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

    *ResultBuffer = AllocateItem(NonPagedPool, PartialInformation->DataLength);
    if (!*ResultBuffer)
    {
        /* not enough memory */
        FreeItem(PartialInformation);
        return MM_STATUS_NO_MEMORY;
    }

    /* copy key value */
    RtlMoveMemory(*ResultBuffer, PartialInformation->Data, PartialInformation->DataLength);

    /* free key info */
    FreeItem(PartialInformation);

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
    InitializeObjectAttributes(&ObjectAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF, hKey, NULL);

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
    return AllocateItem(NonPagedPool, NumBytes);
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
    FreeItem(Block);
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
    Status = ObReferenceObjectByHandle(hMixer, GENERIC_READ | GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
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

    /* initialize key name */
    RtlInitUnicodeString(&KeyName, *DeviceName);

    /* open device interface key */
    Status = IoOpenDeviceInterfaceRegistryKey(&KeyName, GENERIC_READ | GENERIC_WRITE, OutKey);

    if (!NT_SUCCESS(Status))
    {
        *OutKey = NULL;
    }

#if 0
    if (!NT_SUCCESS(Status))
    {
        /* failed to open key */
        DPRINT("IoOpenDeviceInterfaceRegistryKey failed with %lx\n", Status);
        FreeItem(*DeviceName);
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
    PKSEVENTDATA Data = (PKSEVENTDATA)AllocateItem(NonPagedPool, sizeof(KSEVENTDATA) + ExtraSize);
    if (!Data)
        return NULL;

    Data->EventObject.Event = AllocateItem(NonPagedPool, sizeof(KEVENT));
    if (!Data->EventHandle.Event)
    {
        FreeItem(Data);
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

    FreeItem(Data->EventHandle.Event);
    FreeItem(Data);
}

VOID
CALLBACK
EventCallback(
    IN PVOID MixerEventContext,
    IN HANDLE hMixer,
    IN ULONG NotificationType,
    IN ULONG Value)
{
    PWDMAUD_CLIENT ClientInfo;
    PEVENT_ENTRY Entry;
    ULONG Index;

    /* get client context */
    ClientInfo = (PWDMAUD_CLIENT)MixerEventContext;

    /* now search for the mixer which originated the request */
    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].Handle == hMixer && ClientInfo->hPins[Index].Type == MIXER_DEVICE_TYPE)
        {
            if (ClientInfo->hPins[Index].NotifyEvent)
            {
                /* allocate event entry */
                Entry = AllocateItem(NonPagedPool, sizeof(EVENT_ENTRY));
                if (!Entry)
                {
                    /* no memory */
                    break;
                }

                /* setup event entry */
                Entry->NotificationType = NotificationType;
                Entry->Value = Value;
                Entry->hMixer = hMixer;

                /* insert entry */
                InsertTailList(&ClientInfo->MixerEventList, &Entry->Entry);

                /* now notify the client */
                KeSetEvent(ClientInfo->hPins[Index].NotifyEvent, 0, FALSE);
            }
            /* done */
            break;
        }
    }
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
    //PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PKEVENT EventObject = NULL;

    DPRINT("WdmAudControlOpenMixer\n");

    //DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->u.hNotifyEvent)
    {
        Status = ObReferenceObjectByHandle(DeviceInfo->u.hNotifyEvent, EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, (LPVOID*)&EventObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Invalid notify event passed %p from client %p\n", DeviceInfo->u.hNotifyEvent, ClientInfo);
            DbgBreakPoint();
            return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
        }
    }

    if (MMixerOpen(&MixerContext, DeviceInfo->DeviceIndex, ClientInfo, EventCallback, &hMixer) != MM_STATUS_SUCCESS)
    {
        ObDereferenceObject(EventObject);
        DPRINT1("Failed to open mixer\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }


    Handles = AllocateItem(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

    if (Handles)
    {
        if (ClientInfo->NumPins)
        {
            RtlMoveMemory(Handles, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
            FreeItem(ClientInfo->hPins);
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
WdmAudControlCloseMixer(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN  ULONG Index)
{
    /* Remove event associated to this client */
    if (MMixerClose(&MixerContext, DeviceInfo->DeviceIndex, ClientInfo, EventCallback) != MM_STATUS_SUCCESS)
    {
        DPRINT1("Failed to close mixer\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
    }

    /* Dereference event */
    if (ClientInfo->hPins[Index].NotifyEvent)
    {
        ObDereferenceObject(ClientInfo->hPins[Index].NotifyEvent);
        ClientInfo->hPins[Index].NotifyEvent = NULL;
    }

    /* FIXME: do we need to free ClientInfo->hPins ? */
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

VOID
WdmAudCloseAllMixers(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_CLIENT ClientInfo,
    IN ULONG Index)
{
    ULONG DeviceCount, DeviceIndex;

    /* Get all mixers */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    /* Close every mixer attached to the device */
    for (DeviceIndex = 0; DeviceIndex < DeviceCount; DeviceIndex++)
    {
        if (MMixerClose(&MixerContext, DeviceIndex, ClientInfo, EventCallback) != MM_STATUS_SUCCESS)
        {
            DPRINT1("Failed to close mixer for device %lu\n", DeviceIndex);
        }
    }

    /* Dereference event */
    if (ClientInfo->hPins[Index].NotifyEvent)
    {
        ObDereferenceObject(ClientInfo->hPins[Index].NotifyEvent);
        ClientInfo->hPins[Index].NotifyEvent = NULL;
    }
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
    Status = MMixerGetControlDetails(&MixerContext, DeviceInfo->hDevice, DeviceInfo->DeviceIndex, DeviceInfo->Flags, &DeviceInfo->u.MixDetails);

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
    Status = MMixerGetLineInfo(&MixerContext, DeviceInfo->hDevice, DeviceInfo->DeviceIndex, DeviceInfo->Flags, &DeviceInfo->u.MixLine);

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
    Status = MMixerGetLineControls(&MixerContext, DeviceInfo->hDevice, DeviceInfo->DeviceIndex, DeviceInfo->Flags, &DeviceInfo->u.MixControls);

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
    Status = MMixerSetControlDetails(&MixerContext, DeviceInfo->hDevice, DeviceInfo->DeviceIndex, DeviceInfo->Flags, &DeviceInfo->u.MixDetails);

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
    PLIST_ENTRY Entry;
    PEVENT_ENTRY EventEntry;

    /* enumerate event list and check if there is a new event */
    Entry = ClientInfo->MixerEventList.Flink;

    while(Entry != &ClientInfo->MixerEventList)
    {
        /* grab event entry */
        EventEntry = (PEVENT_ENTRY)CONTAINING_RECORD(Entry, EVENT_ENTRY, Entry);

        if (EventEntry->hMixer == DeviceInfo->hDevice)
        {
            /* found an entry */
            DeviceInfo->u.MixerEvent.hMixer = EventEntry->hMixer;
            DeviceInfo->u.MixerEvent.NotificationType = EventEntry->NotificationType;
            DeviceInfo->u.MixerEvent.Value = EventEntry->Value;

            /* remove entry from list */
            RemoveEntryList(&EventEntry->Entry);

            /* free event entry */
            FreeItem(EventEntry);

            /* done */
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
        }

        /* move to next */
        Entry = Entry->Flink;
    }

    /* no event entry available */
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

ULONG
WdmAudGetMidiInDeviceCount()
{
    return MMixerGetMidiInCount(&MixerContext);
}

ULONG
WdmAudGetMidiOutDeviceCount()
{
    return MMixerGetWaveOutCount(&MixerContext);
}

NTSTATUS
WdmAudGetPnpNameByIndexAndType(
    IN ULONG DeviceIndex,
    IN SOUND_DEVICE_TYPE DeviceType,
    OUT LPWSTR *DevicePath)
{
    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        if (MMixerGetWaveDevicePath(&MixerContext, DeviceType == WAVE_IN_DEVICE_TYPE, DeviceIndex, DevicePath) == MM_STATUS_SUCCESS)
            return STATUS_SUCCESS;
        else
            return STATUS_UNSUCCESSFUL;
    }
    else if (DeviceType == MIDI_IN_DEVICE_TYPE || DeviceType == MIDI_OUT_DEVICE_TYPE)
    {
        if (MMixerGetMidiDevicePath(&MixerContext, DeviceType == MIDI_IN_DEVICE_TYPE, DeviceIndex, DevicePath) == MM_STATUS_SUCCESS)
            return STATUS_SUCCESS;
        else
            return STATUS_UNSUCCESSFUL;
    }
    else if (DeviceType == MIXER_DEVICE_TYPE)
    {
        UNIMPLEMENTED;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WdmAudWaveCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    MIXER_STATUS Status = MM_STATUS_UNSUCCESSFUL;

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

    if (Status == MM_STATUS_SUCCESS)
        return STATUS_SUCCESS;
    else
        return Status;
}

NTSTATUS
WdmAudMidiCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    MIXER_STATUS Status = MM_STATUS_UNSUCCESSFUL;

    if (DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE)
    {
        /* get capabilities */
        Status = MMixerMidiInCapabilities(&MixerContext, DeviceInfo->DeviceIndex, &DeviceInfo->u.MidiInCaps);
    }
    else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        /* get capabilities */
        Status = MMixerMidiOutCapabilities(&MixerContext, DeviceInfo->DeviceIndex, &DeviceInfo->u.MidiOutCaps);
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

NTSTATUS
WdmAudControlOpenMidi(
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

    Status = MMixerOpenMidi(&MixerContext, DeviceInfo->DeviceIndex, DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE, CreatePinCallback, &Context, &DeviceInfo->hDevice);

    if (Status == MM_STATUS_SUCCESS)
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    else
        return SetIrpIoStatus(Irp, STATUS_NOT_SUPPORTED, sizeof(WDMAUD_DEVICE_INFO));
}
