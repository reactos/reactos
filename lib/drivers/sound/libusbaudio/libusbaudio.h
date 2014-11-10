#pragma once

typedef enum
{
    UA_STATUS_SUCCESS = 0,
    UA_STATUS_NOTINITIALIZED,
    UA_STATUS_NOT_IMPLEMENTED,
    UA_STATUS_NO_MORE_DEVICES,
    UA_STATUS_MORE_ENTRIES,
    UA_STATUS_INVALID_PARAMETER,
    UA_STATUS_UNSUCCESSFUL,
    UA_STATUS_NO_MEMORY
}USBAUDIO_STATUS;

typedef PVOID (*PUSBAUDIO_ALLOC)(
    IN ULONG NumberOfBytes);

typedef VOID (*PUSBAUDIO_FREE)(
    IN PVOID Block);

typedef VOID (*PUSBAUDIO_COPY)(
    IN PVOID Dst,
    IN PVOID Src,
    IN ULONG Length);

typedef struct
{
    ULONG Size;
    PVOID Context;

    PUSBAUDIO_ALLOC Alloc;
    PUSBAUDIO_FREE  Free;
    PUSBAUDIO_COPY  Copy;


}USBAUDIO_CONTEXT, *PUSBAUDIO_CONTEXT;

USBAUDIO_STATUS
UsbAudio_InitializeContext(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUSBAUDIO_ALLOC Alloc,
    IN PUSBAUDIO_FREE Free,
    IN PUSBAUDIO_COPY Copy);


USBAUDIO_STATUS
UsbAudio_ParseConfigurationDescriptor(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorSize);

USBAUDIO_STATUS
UsbAudio_GetFilter(
    IN PUSBAUDIO_CONTEXT Context,
    OUT PVOID * OutFilterDescriptor);

