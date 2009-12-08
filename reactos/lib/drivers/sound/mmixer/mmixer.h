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
    OUT PHANDLE OutHandle);

typedef MIXER_STATUS(*PMIXER_DEVICE_CONTROL)(
    IN HANDLE hMixer,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    ULONG nOutBufferSize,
    PULONG lpBytesReturned);

typedef MIXER_STATUS(*PMIXER_OPEN)(
    IN LPCWSTR DevicePath,
    OUT PHANDLE hDevice);

typedef MIXER_STATUS(*PMIXER_CLOSE)(
    IN HANDLE hDevice);

typedef VOID (*PMIXER_EVENT)(
    IN PVOID MixerEvent);


typedef struct
{
     ULONG SizeOfStruct;
     PVOID MixerContext;

     PMIXER_ALLOC Alloc;
     PMIXER_DEVICE_CONTROL Control;
     PMIXER_FREE  Free;
     PMIXER_OPEN Open;
     PMIXER_CLOSE Close;
}MIXER_CONTEXT, *PMIXER_CONTEXT;





MIXER_STATUS
MMixerInitialize(
    IN PMIXER_CONTEXT MixerContext, 
    IN PMIXER_ENUM EnumFunction,
    IN PVOID EnumContext);


ULONG
MMixerGetCount(
    IN PMIXER_CONTEXT MixerContext);

MIXER_STATUS
MMixerGetCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerIndex,
    OUT MIXERCAPSW MixerCaps);

MIXER_STATUS
MMixerOpen(
    IN PMIXER_CONTEXT MixerContext,
    IN PVOID MixerEvent,
    IN PMIXER_EVENT MixerEventRoutine,
    OUT PHANDLE MixerHandle);

MIXER_STATUS
MMixerGetLineInfo(
    IN  HANDLE MixerHandle,
    IN  ULONG Flags,
    OUT LPMIXERLINEW MixerLine);

MIXER_STATUS
MMixerGetLineControls(
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERLINECONTROLS MixerLineControls);

MIXER_STATUS
MMixerSetControlDetails(
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails);

MIXER_STATUS
MMixerGetControlDetails(
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails);

#endif
