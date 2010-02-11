#ifndef MIXER_H__
#define MIXER_H__

typedef enum
{
    MM_STATUS_SUCCESS = 0,
    MM_STATUS_NOTINITIALIZED,
    MM_STATUS_NOT_IMPLEMENTED,
    MM_STATUS_NO_MORE_DEVICES,
    MM_STATUS_MORE_ENTRIES,
    MM_STATUS_INVALID_PARAMETER,
    MM_STATUS_UNSUCCESSFUL,
    MM_STATUS_NO_MEMORY


}MIXER_STATUS;


typedef PVOID (*PMIXER_ALLOC)(
    IN ULONG NumberOfBytes);

typedef VOID (*PMIXER_FREE)(
    IN PVOID Block);

typedef MIXER_STATUS (*PMIXER_ENUM)(
    IN  PVOID EnumContext,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * DeviceName,
    OUT PHANDLE OutHandle,
    OUT PHANDLE OutDevInterfaceKey);

typedef MIXER_STATUS(*PMIXER_DEVICE_CONTROL)(
    IN HANDLE hMixer,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    ULONG nOutBufferSize,
    PULONG lpBytesReturned);

typedef MIXER_STATUS(*PMIXER_OPEN)(
    IN LPWSTR DevicePath,
    OUT PHANDLE hDevice);

typedef MIXER_STATUS(*PMIXER_CLOSE)(
    IN HANDLE hDevice);

typedef MIXER_STATUS(*PMIXER_CLOSEKEY)(
    IN HANDLE hKey);

typedef VOID (*PMIXER_EVENT)(
    IN PVOID MixerEvent);

typedef VOID (*PMIXER_COPY)(
    IN PVOID Dst,
    IN PVOID Src,
    IN ULONG Length);

typedef MIXER_STATUS(*PMIXER_QUERY_KEY_VALUE)(
    IN HANDLE hKey,
    IN LPWSTR KeyName,
    OUT PVOID * ResultBuffer,
    OUT PULONG ResultLength,
    OUT PULONG KeyType);

typedef MIXER_STATUS(*PMIXER_OPEN_KEY)(
    IN HANDLE hKey,
    IN LPWSTR SubKey,
    IN ULONG DesiredAccess,
    OUT PHANDLE OutKey);

typedef PVOID (*PMIXER_ALLOC_EVENT_DATA)(
    IN ULONG ExtraBytes);

typedef VOID (*PMIXER_FREE_EVENT_DATA)(
    IN PVOID EventData);

typedef MIXER_STATUS (*PIN_CREATE_CALLBACK)(
    IN PVOID Context,
    IN ULONG DeviceId,
    IN ULONG PinId,
    IN HANDLE hFilter,
    IN PKSPIN_CONNECT PinConnect,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE PinHandle);

typedef struct
{
     ULONG SizeOfStruct;
     PVOID MixerContext;

     PMIXER_ALLOC Alloc;
     PMIXER_DEVICE_CONTROL Control;
     PMIXER_FREE  Free;
     PMIXER_OPEN Open;
     PMIXER_CLOSE Close;
     PMIXER_COPY Copy;
     PMIXER_OPEN_KEY OpenKey;
     PMIXER_QUERY_KEY_VALUE QueryKeyValue;
     PMIXER_CLOSEKEY CloseKey;
     PMIXER_ALLOC_EVENT_DATA AllocEventData;
     PMIXER_FREE_EVENT_DATA FreeEventData;
}MIXER_CONTEXT, *PMIXER_CONTEXT;

MIXER_STATUS
MMixerInitialize(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_ENUM EnumFunction,
    IN PVOID EnumContext);

ULONG
MMixerGetCount(
    IN PMIXER_CONTEXT MixerContext);

ULONG
MMixerGetWaveInCount(
    IN PMIXER_CONTEXT MixerContext);

ULONG
MMixerGetWaveOutCount(
    IN PMIXER_CONTEXT MixerContext);


MIXER_STATUS
MMixerGetCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerIndex,
    OUT LPMIXERCAPSW MixerCaps);

MIXER_STATUS
MMixerOpen(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerId,
    IN PVOID MixerEvent,
    IN PMIXER_EVENT MixerEventRoutine,
    OUT PHANDLE MixerHandle);

MIXER_STATUS
MMixerGetLineInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN  HANDLE MixerHandle,
    IN  ULONG Flags,
    OUT LPMIXERLINEW MixerLine);

MIXER_STATUS
MMixerGetLineControls(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERLINECONTROLSW MixerLineControls);

MIXER_STATUS
MMixerSetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails);

MIXER_STATUS
MMixerGetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails);

MIXER_STATUS
MMixerWaveOutCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPWAVEOUTCAPSW Caps);

MIXER_STATUS
MMixerWaveInCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPWAVEINCAPSW Caps);

MIXER_STATUS
MMixerOpenWave(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    IN ULONG bWaveIn,
    IN LPWAVEFORMATEX WaveFormat,
    IN PIN_CREATE_CALLBACK CreateCallback,
    IN PVOID Context,
    OUT PHANDLE PinHandle);

MIXER_STATUS
MMixerSetWaveStatus(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN KSSTATE State);

#endif
