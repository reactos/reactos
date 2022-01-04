/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceUM.hpp

Abstract:

    This is the definition of the FxDevice object UM specific

Author:



Environment:

    User mode only

Revision History:

--*/

#ifndef _FXDEVICEUM_H_
#define _FXDEVICEUM_H_

#define WDF_PATH_SEPARATOR              L"\\"
#define WUDF_SUB_KEY                    L"WUDF"
#define WUDF_ADDITIONAL_SUB_KEY         L"WDF"

#define FX_DIRECT_HARDWARE_ACCESS           L"DirectHardwareAccess"
#define FX_DIRECT_HARDWARE_ACCESS_DEFAULT   (WdfRejectDirectHardwareAccess)

#define FX_REGISTER_ACCESS_MODE             L"RegisterAccessMode"
#define FX_REGISTER_ACCESS_MODE_DEFAULT     (WdfRegisterAccessUsingSystemCall)

#define FX_FILE_OBJECT_POLICY               L"FileObjectPolicy"
#define FX_FILE_OBJECT_POLICY_DEFAULT       (WdfRejectNullAndUnknownFileObjects)

#define FX_FS_CONTEXT_USE_POLICY            L"FsContextUsePolicy"
#define FX_FS_CONTEXT_USE_POLICY_DEFAULT    (WdfDefaultFsContextUsePolicy)

#define FX_KERNEL_MODE_CLIENT_POLICY        L"KernelModeClientPolicy"
#define FX_METHOD_NEITHER_ACTION            L"MethodNeitherAction"
#define FX_PROCESS_SHARING_ENABLED          L"HostProcessSharingEnabled"
#define FX_DEVICE_GROUP_ID                  L"DeviceGroupId"
#define FX_FILE_OBJECT_POLICY               L"FileObjectPolicy"

//
// READ/WRITE_REGISTER_Xxx macros need compiler and memory barrier. Each
// platform has a different set of compiler intrinsics to support that.
// In kernel, x86 READ/WRITE macros are implemented in assembly and use
// lock prefix to force real access. For amd64, _ReadWriteBarrier
// (compiler barrier) is used for reads and __faststorefence (CPU barrier) for
// writes. For ARM, _ReadWriteBarrier (compiler barrier) is used for reads and
// both _ReadWriteBarrier and __emit(Value) for writes.
//
// Because of this variation, UMDF will use the macros directly from wdm.h
// by autogenerating those macros from wdm.h into a private UMDF header.
// For x86, there are no macros so either UMDF could directly link to the
// ntosrtl.lib (that has these macros implemented in assembly), or implement
// its own macros that use MemoryBarrier() macro from winnt.h.
//
// Below is UMDF's implementation for x86. The macros for other platforms are
// in WudfWdm_private.h.
//
#if defined(_X86_)

#define READ_REGISTER_UCHAR(x) \
    (MemoryBarrier(), *(volatile UCHAR * const)(x))

#define READ_REGISTER_USHORT(x) \
    (MemoryBarrier(), *(volatile USHORT * const)(x))

#define READ_REGISTER_ULONG(x) \
    (MemoryBarrier(), *(volatile ULONG * const)(x))

#define READ_REGISTER_ULONG64(x) \
    (MemoryBarrier(), *(volatile ULONG64 * const)(x))

#define READ_REGISTER_BUFFER_UCHAR(x, y, z) {                           \
    PUCHAR registerBuffer = x;                                          \
    PUCHAR readBuffer = y;                                              \
    ULONG readCount;                                                    \
    MemoryBarrier();                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile UCHAR * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_USHORT(x, y, z) {                          \
    PUSHORT registerBuffer = x;                                         \
    PUSHORT readBuffer = y;                                             \
    ULONG readCount;                                                    \
    MemoryBarrier();                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile USHORT * const)(registerBuffer);       \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG(x, y, z) {                           \
    PULONG registerBuffer = x;                                          \
    PULONG readBuffer = y;                                              \
    ULONG readCount;                                                    \
    MemoryBarrier();                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile ULONG * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG64(x, y, z) {                         \
    PULONG64 registerBuffer = x;                                        \
    PULONG64 readBuffer = y;                                            \
    ULONG readCount;                                                    \
    MemoryBarrier();                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile ULONG64 * const)(registerBuffer);      \
    }                                                                   \
}

#define WRITE_REGISTER_UCHAR(x, y) {    \
    *(volatile UCHAR * const)(x) = y;   \
    MemoryBarrier();                    \
}

#define WRITE_REGISTER_USHORT(x, y) {   \
    *(volatile USHORT * const)(x) = y;  \
    MemoryBarrier();               \
}

#define WRITE_REGISTER_ULONG(x, y) {    \
    *(volatile ULONG * const)(x) = y;   \
    MemoryBarrier();                    \
}

#define WRITE_REGISTER_ULONG64(x, y) {  \
    *(volatile ULONG64 * const)(x) = y; \
    MemoryBarrier();                    \
}

#define WRITE_REGISTER_BUFFER_UCHAR(x, y, z) {                            \
    PUCHAR registerBuffer = x;                                            \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile UCHAR * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    MemoryBarrier();                                                      \
}

#define WRITE_REGISTER_BUFFER_USHORT(x, y, z) {                           \
    PUSHORT registerBuffer = x;                                           \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile USHORT * const)(registerBuffer) = *writeBuffer;        \
    }                                                                     \
    MemoryBarrier();                                                      \
}

#define WRITE_REGISTER_BUFFER_ULONG(x, y, z) {                            \
    PULONG registerBuffer = x;                                            \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile ULONG * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    MemoryBarrier();                                                      \
}

#define WRITE_REGISTER_BUFFER_ULONG64(x, y, z) {                          \
    PULONG64 registerBuffer = x;                                          \
    PULONG64 writeBuffer = y;                                             \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile ULONG64 * const)(registerBuffer) = *writeBuffer;       \
    }                                                                     \
    MemoryBarrier();                                                      \
}

#endif // _X86_

__inline
SIZE_T
FxDevice::ReadRegister(
    __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    __in PVOID Register
    )
{
    SIZE_T value = 0;

    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_READ_FROM_HARDWARE_START(
        WdfDeviceHwAccessTargetTypeRegister, Size, 0);

    switch(Size) {
    case WdfDeviceHwAccessTargetSizeUchar:
        value = READ_REGISTER_UCHAR((PUCHAR)Register);
        break;
    case WdfDeviceHwAccessTargetSizeUshort:
        value = READ_REGISTER_USHORT((PUSHORT)Register);
        break;
    case WdfDeviceHwAccessTargetSizeUlong:
        value = READ_REGISTER_ULONG((PULONG)Register);
        break;
    case WdfDeviceHwAccessTargetSizeUlong64:
#if defined(_WIN64)
        value = READ_REGISTER_ULONG64((PULONG64)Register);
#else
        FX_VERIFY(DRIVER(BadArgument, TODO), CHECK("Invalid call to ULONG64 "
            "hardware access function", FALSE));
#endif
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Unexpected"));
        break;
    }

    //
    // ETW end event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_READ_FROM_HARDWARE_END(
        WdfDeviceHwAccessTargetTypeRegister, Size, 0);

    return value;
}

__inline
VOID
FxDevice::ReadRegisterBuffer(
    __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    __in PVOID Register,
    __out_ecount_full(Count) PVOID Buffer,
    __in ULONG Count
    )
{
    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_READ_FROM_HARDWARE_START(
        WdfDeviceHwAccessTargetTypeRegisterBuffer, Size, Count);

    switch(Size) {
    case WdfDeviceHwAccessTargetSizeUchar:
        READ_REGISTER_BUFFER_UCHAR(((PUCHAR)Register), (PUCHAR)Buffer, Count );
        break;
    case WdfDeviceHwAccessTargetSizeUshort:
#pragma prefast(suppress:26000, "The Size parameter dictates the buffer size")
        READ_REGISTER_BUFFER_USHORT(((PUSHORT)Register), (PUSHORT)Buffer, Count );
        break;
    case WdfDeviceHwAccessTargetSizeUlong:
        READ_REGISTER_BUFFER_ULONG(((PULONG)Register), (PULONG)Buffer, Count );
        break;
    case WdfDeviceHwAccessTargetSizeUlong64:
#if defined(_WIN64)
        READ_REGISTER_BUFFER_ULONG64(((PULONG64)Register), (PULONG64)Buffer, Count );
#else
        FX_VERIFY(DRIVER(BadArgument, TODO), CHECK("Invalid call to ULONG64 "
            "hardware access function", FALSE));
#endif
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Unexpected"));
        break;
    }

    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_READ_FROM_HARDWARE_END(
        WdfDeviceHwAccessTargetTypeRegisterBuffer, Size, Count);
}

__inline
VOID
FxDevice::WriteRegister(
    __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    __in PVOID Register,
    __in SIZE_T Value
    )
{
    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_WRITE_TO_HARDWARE_START(
        WdfDeviceHwAccessTargetTypeRegister, Size, 0);

    switch(Size) {
    case WdfDeviceHwAccessTargetSizeUchar:
        WRITE_REGISTER_UCHAR((PUCHAR)Register, (UCHAR)Value);
        break;
    case WdfDeviceHwAccessTargetSizeUshort:
        WRITE_REGISTER_USHORT((PUSHORT)Register, (USHORT)Value);
        break;
    case WdfDeviceHwAccessTargetSizeUlong:
        WRITE_REGISTER_ULONG((PULONG)Register, (ULONG)Value);
        break;
    case WdfDeviceHwAccessTargetSizeUlong64:
#if defined(_WIN64)
        WRITE_REGISTER_ULONG64((PULONG64)Register, (ULONG64)Value);
#else
        FX_VERIFY(DRIVER(BadArgument, TODO), CHECK("Invalid call to ULONG64 "
            "hardware access function", FALSE));
#endif
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Unexpected"));
        break;
    }

    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_WRITE_TO_HARDWARE_END(
        WdfDeviceHwAccessTargetTypeRegister, Size, 0);
}

__inline
VOID
FxDevice::WriteRegisterBuffer(
    __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    __in PVOID Register,
    __in_ecount(Count) PVOID Buffer,
    __in ULONG Count
    )
{
    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_WRITE_TO_HARDWARE_START(
        WdfDeviceHwAccessTargetTypeRegisterBuffer, Size, Count);

    switch(Size) {
    case WdfDeviceHwAccessTargetSizeUchar:
        WRITE_REGISTER_BUFFER_UCHAR(((PUCHAR)Register), (PUCHAR)Buffer, Count);
        break;
    case WdfDeviceHwAccessTargetSizeUshort:
#pragma prefast(suppress:26000, "The Size parameter dictates the buffer size")
        WRITE_REGISTER_BUFFER_USHORT(((PUSHORT)Register), (PUSHORT)Buffer, Count);
        break;
    case WdfDeviceHwAccessTargetSizeUlong:
        WRITE_REGISTER_BUFFER_ULONG(((PULONG)Register), (PULONG)Buffer, Count);
        break;
    case WdfDeviceHwAccessTargetSizeUlong64:
#if defined(_WIN64)
        WRITE_REGISTER_BUFFER_ULONG64(((PULONG64)Register), (PULONG64)Buffer, Count);
#else
        FX_VERIFY(DRIVER(BadArgument, TODO), CHECK("Invalid call to ULONG64 "
            "hardware access function", FALSE));
#endif
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Unexpected"));
        break;
    }

    //
    // ETW start event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_WRITE_TO_HARDWARE_END(
        WdfDeviceHwAccessTargetTypeRegisterBuffer, Size, Count);
}

__inline
FxWdmDeviceExtension*
FxDevice::_GetFxWdmExtension(
    __in MdDeviceObject DeviceObject
    )
{
    return (FxWdmDeviceExtension*)
        ((static_cast<IWudfDevice2*> (DeviceObject))->GetDeviceExtension());
}

__inline
BOOLEAN
FxDevice::IsRemoveLockEnabledForIo(
    VOID
    )
{
   return FALSE;
}

__inline
MdRemoveLock
FxDevice::GetRemoveLock(
    VOID
    )
{
    return &FxDevice::_GetFxWdmExtension(
        GetDeviceObject())->IoRemoveLock;
}

__inline
NTSTATUS
FxDevice::WmiPkgRegister(
    VOID
    )
{
    //
    // WMI doesn't apply for UMDF
    //
    DO_NOTHING();
    return STATUS_SUCCESS;
}

__inline
VOID
FxDevice::WmiPkgDeregister(
    VOID
    )
{
    //
    // WMI doesn't apply for UMDF
    //
    DO_NOTHING();
}

__inline
VOID
FxDevice::WmiPkgCleanup(
    VOID
    )
{
    //
    // WMI doesn't apply for UMDF
    //
    DO_NOTHING();
}

__inline
IWudfDeviceStack*
FxDevice::GetDeviceStack(
    VOID
    )
{
    return m_DevStack;
}

__inline
IWudfDeviceStack2 *
FxDevice::GetDeviceStack2(
    )
{
    IWudfDeviceStack2 *pDeviceStack2;
    HRESULT hrQI;

    hrQI = m_DevStack->QueryInterface(IID_IWudfDeviceStack2,
                                      (PVOID*)&pDeviceStack2);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pDeviceStack2));

    m_DevStack->Release();

    return pDeviceStack2;
}

#endif //_FXDEVICEUM_H_

