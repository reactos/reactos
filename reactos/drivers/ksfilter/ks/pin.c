/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/worker.c
 * PURPOSE:         KS pin functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

/*
    @unimplemented
*/
VOID
NTAPI
KsPinAcquireProcessingMutex(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinAttachAndGate(
    IN PKSPIN Pin,
    IN PKSGATE AndGate OPTIONAL)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinAttachOrGate(
    IN PKSPIN Pin,
    IN PKSGATE OrGate OPTIONAL)
{
    UNIMPLEMENTED
}
/*
    @unimplemented
*/
VOID
NTAPI
KsPinAttemptProcessing(
    IN PKSPIN  Pin,
    IN BOOLEAN  Asynchronous)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetAvailableByteCount(
    IN PKSPIN  Pin,
    OUT PLONG  InputDataBytes OPTIONAL,
    OUT PLONG  OutputBufferBytes OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetConnectedFilterInterface(
    IN PKSPIN  Pin,
    IN const GUID*  InterfaceId,
    OUT PVOID*  Interface)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
PKSGATE
NTAPI
KsPinGetAndGate(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
PDEVICE_OBJECT
NTAPI
KsPinGetConnectedPinDeviceObject(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
PFILE_OBJECT
NTAPI
KsPinGetConnectedPinFileObject(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetConnectedPinInterface(
    IN PKSPIN  Pin,
    IN const GUID*  InterfaceId,
    OUT PVOID*  Interface)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinGetCopyRelationships(
    IN PKSPIN Pin,
    OUT PKSPIN* CopySource,
    OUT PKSPIN* DelegateBranch)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
PKSPIN
NTAPI
KsPinGetNextSiblingPin(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
PKSFILTER
NTAPI
KsPinGetParentFilter(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
  KsPinGetReferenceClockInterface(
    IN PKSPIN  Pin,
    OUT PIKSREFERENCECLOCK*  Interface)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinRegisterFrameReturnCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINFRAMERETURN  FrameReturn)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinRegisterHandshakeCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINHANDSHAKE  Handshake)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinRegisterIrpCompletionCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINIRPCOMPLETION  IrpCompletion)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinRegisterPowerCallbacks(
    IN PKSPIN  Pin,
    IN PFNKSPINPOWER  Sleep OPTIONAL,
    IN PFNKSPINPOWER  Wake OPTIONAL)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinReleaseProcessingMutex(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinSetPinClockTime(
    IN PKSPIN  Pin,
    IN LONGLONG  Time)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinSubmitFrame(
    IN PKSPIN  Pin,
    IN PVOID  Data  OPTIONAL,
    IN ULONG  Size  OPTIONAL,
    IN PKSSTREAM_HEADER  StreamHeader  OPTIONAL,
    IN PVOID  Context  OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsPinSubmitFrameMdl(
    IN PKSPIN  Pin,
    IN PMDL  Mdl  OPTIONAL,
    IN PKSSTREAM_HEADER  StreamHeader  OPTIONAL,
    IN PVOID  Context  OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsProcessPinUpdate(
    IN PKSPROCESSPIN  ProcessPin)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetLeadingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetTrailingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerSetStatusCode(
    IN PKSSTREAM_POINTER StreamPointer,
    IN NTSTATUS Status)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerLock(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN BOOLEAN Eject)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerAdvanceOffsetsAndUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerDelete(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerClone(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
    IN ULONG ContextSize,
    OUT PKSSTREAM_POINTER* CloneStreamPointer)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvanceOffsets(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvance(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
PMDL
NTAPI
KsStreamPointerGetMdl(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
PIRP
NTAPI
KsStreamPointerGetIrp(
    IN PKSSTREAM_POINTER StreamPointer,
    OUT PBOOLEAN FirstFrameInIrp OPTIONAL,
    OUT PBOOLEAN LastFrameInIrp OPTIONAL)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerScheduleTimeout(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER Callback,
    IN ULONGLONG Interval)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerCancelTimeout(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetFirstCloneStreamPointer(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsStreamPointerGetNextClone(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return NULL;
}
