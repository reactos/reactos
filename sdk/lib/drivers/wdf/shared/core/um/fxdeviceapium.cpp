/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceApiUm.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:


Environment:

    user mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"
#include <intsafe.h>

extern "C" {
#include "FxDeviceApiUm.tmh"
}

//
// extern "C" the entire file
//
extern "C" {

//
// Verifier Functions
//
// Do not specify argument names
FX_DECLARE_VF_FUNCTION_P4(
NTSTATUS,
VerifyWdfDeviceWdmDispatchIrpToIoQueue,
    _In_ FxDevice*,
    _In_ MdIrp,
    _In_ FxIoQueue*,
    _In_ ULONG
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDevicePostEvent)(
    _In_ PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ REFGUID EventGuid,
    _In_ WDF_EVENT_TYPE WdfEventType,
    _In_reads_bytes_(DataSizeCb) BYTE * Data,
    _In_ ULONG DataSizeCb
    )
/*++

Routine Description:

    This method asynchronously notifies applications that are waiting for the
    specified event from a driver.

Arguments:
    Device - WDF Device handle.

    EventGuid: The GUID for the event. The GUID is determined by the application
           and the driver and is opaque to the framework.

    EventType: A WDF_EVENT_TYPE-typed value that identifies the type of
           event. In the current version of UMDF, the driver must set EventType
           to WdfEventBroadcast (1). WdfEventBroadcast indicates that the event
           is broadcast. Applications can subscribe to WdfEventBroadcast-type
           events. To receive broadcast events, the application must register
           for notification through the Microsoft Win32
           RegisterDeviceNotification function. WdfEventBroadcast-type events
           are exposed as DBT_CUSTOMEVENT-type events to applications.

    Data: A pointer to a buffer that contains data that is associated with the
           event. NULL is a valid value.

    DataSizeCb : The size, in bytes, of data that Data points to. Zero is a
           valid size value if Data is set to NULL.

Return Value:
    An NTSTATUS value that denotes success or failure of the DDI

--*/
{
    DDI_ENTRY();

    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    HRESULT hr;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (DataSizeCb > 0) {
        FxPointerNotNull(pFxDriverGlobals, Data);
    }

    //
    // Currently only broadcast events are supported.
    //
    if (WdfEventType != WdfEventBroadcast) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p WdfEventType %d not expected %!STATUS!",
            Device, WdfEventType, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // post the event
    //
    hr = pDevice->GetDeviceStack()->PostEvent(EventGuid,
                                              WdfEventType,
                                              Data,
                                              DataSizeCb);

    if (FAILED(hr)) {
        status = FxDevice::NtStatusFromHr(pDevice->GetDeviceStack(), hr);
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p Failed to post event %!STATUS!",
            Device, status);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceMapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PHYSICAL_ADDRESS PhysicalAddress,
    _In_
    SIZE_T NumberOfBytes,
    _In_
    MEMORY_CACHING_TYPE CacheType,
    _Out_
    PVOID* PseudoBaseAddress
    )
/*++

Routine Description:

    This routine maps given physical address of a device register into
    system address space and optionally into user-mode address space.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PhysicalAddress - Address of MMIO register to be mapped

    NumberOfBytes - Length of resource to be mapped in bytes.

    CacheType - supplies type of caching desired

    PseudoBaseAddress - Pseudo base address (opaque base address) of the
        mapped resource.

Return Value:

    HRESULT

--*/
{
    DDI_ENTRY();

    FxCmResList* transResources;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    HRESULT hr;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Is direct hardware access allowed?
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsDirectHardwareAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // Validate input parameters.
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(PhysicalAddress.QuadPart),
        DriverGlobals->DriverName);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("NumberOfBytes should be > 0",
                                         (NumberOfBytes > 0)), DriverGlobals->DriverName);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("CacheType incorrect",
                                         (CacheType >= MmNonCached &&
                                          CacheType < MmMaximumCacheType)),
                                          DriverGlobals->DriverName);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(PseudoBaseAddress),
        DriverGlobals->DriverName);

    *PseudoBaseAddress = NULL;
    transResources = pDevice->GetTranslatedResources();
    hr = transResources->MapIoSpaceWorker(PhysicalAddress,
                                          NumberOfBytes,
                                          CacheType,
                                          PseudoBaseAddress);
    if (FAILED(hr)) {
        status = pDevice->NtStatusFromHr(hr);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceUnmapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress,
    _In_
    SIZE_T NumberOfBytes
    )
/*++

Routine Description:

    This routine unmaps a previously mapped register resource.

Arguments:
    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PseudoBaseAddress - Address to be unmapped

    NumberOfBytes - Length of resource to be mapped in bytes.

Return Value:

    VOID

--*/
{
    DDI_ENTRY();

    IWudfDeviceStack *deviceStack;
    HRESULT hr;
    FxCmResList* resources;
    PVOID systemBaseAddress;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Is direct hardware access allowed?
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsDirectHardwareAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // Validate input parameters.
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(PseudoBaseAddress),
        DriverGlobals->DriverName);
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("NumberOfBytes should be > 0",
                                         (NumberOfBytes > 0)), DriverGlobals->DriverName);
    //
    // Get system address.
    //
    systemBaseAddress = pDevice->GetSystemAddressFromPseudoAddress(PseudoBaseAddress);

    //
    // Validate that caller has given correct base address and length, and if
    // so clear the mapping from resource table
    //
    resources = pDevice->GetTranslatedResources();
    hr = resources->ValidateAndClearMapping(systemBaseAddress,
                                            NumberOfBytes);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Driver attempted to unmap "
        "incorrect register address, or provided incorrect size",
        SUCCEEDED(hr)), DriverGlobals->DriverName);

    //
    // call host
    //
    if SUCCEEDED(hr) {
        deviceStack = pDevice->GetDeviceStack();

        deviceStack->UnmapIoSpace(systemBaseAddress, NumberOfBytes);
    }

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PVOID
WDFEXPORT(WdfDeviceGetHardwareRegisterMappedAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress
    )
/*++

Routine Description:

    This routine returns user-mode base address where registers corresponing
    to supplied pseudo base address have been mapped..

Arguments:
    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PseudoBaseAddress - Pseudo base address for which the caller needs to get
         user-mode mapped address.

Return Value:

    User-mode mapped base address.

--*/
{
    DDI_ENTRY();

    HRESULT hr;
    FxCmResList* transResources;
    PVOID usermodeBaseAddress;
    PVOID systemBaseAddress;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Is direct hardware access allowed?
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsDirectHardwareAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // Is user-mode mapping of registers enabled
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Incorrect register access mode."
        " Register mapping to user-mode is not enabled. Set the INF directive"
        " UmdfRegisterAccessMode to RegisterAccessUsingUserModeMapping"
        " in driver's INF file to enable Register mapping to user-mode",
        (pDevice->AreRegistersMappedToUsermode() == TRUE)), DriverGlobals->DriverName);

    //
    // Validate input parameters.
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(PseudoBaseAddress),
        DriverGlobals->DriverName);

    //
    // check if this base address is valid.
    //
    usermodeBaseAddress = NULL;
    transResources = pDevice->GetTranslatedResources();
    systemBaseAddress = pDevice->GetSystemAddressFromPseudoAddress(PseudoBaseAddress);

    hr = transResources->ValidateRegisterSystemBaseAddress(systemBaseAddress,
                                                           &usermodeBaseAddress);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Driver provided incorrect base "
        "address", SUCCEEDED(hr)), DriverGlobals->DriverName);

    return usermodeBaseAddress;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
SIZE_T
WDFAPI
WDFEXPORT(WdfDeviceReadFromHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _Out_writes_all_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    )
/*++

Routine Description:

    This routine does read from a device port or register.

Arguments:
    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    Type - Specified whether it is port or register

    Size - Supplies size of read in bytes.

    TargetAddress - Supplies address of port or register to read from

    Buffer - Supplies optionally a buffer to receive the read data

    Count - Size of buffer in bytes

Return Value:

    Returns the read value if it is non-buffered port or register.
    Otherwise zero.

--*/
{
    DDI_ENTRY();

    SIZE_T value = 0;
    PVOID systemAddress = NULL;
    HRESULT hr = S_OK;
    SIZE_T length = 0;
    FxCmResList* resources;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    //
    // ETW event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_DDI_READ_FROM_HARDWARE_START(Type, Size, Count);

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // See if direct hwaccess is allowed
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsDirectHardwareAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // validate parameters
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("Incorrect Type parameter",
            (Type > WdfDeviceHwAccessTargetTypeInvalid &&
             Type < WdfDeviceHwAccessTargetTypeMaximum)),
             DriverGlobals->DriverName);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("Incorrect Size parameter",
            (Size > WdfDeviceHwAccessTargetSizeInvalid &&
             Size < WdfDeviceHwAccessTargetSizeMaximum)),
             DriverGlobals->DriverName);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(TargetAddress),
        DriverGlobals->DriverName);

    if (pDevice->IsBufferType(Type)) {
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(Buffer), DriverGlobals->DriverName);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Count should be > 0", (Count > 0)),
            DriverGlobals->DriverName);
    }
    else {
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NULL(Buffer), DriverGlobals->DriverName);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Count should be 0", (Count == 0)),
            DriverGlobals->DriverName);
    }

#if !defined(_WIN64)
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("ULONG64 write is allowed only"
            "on 64-bit platform", (Size != WdfDeviceHwAccessTargetSizeUlong64)),
            DriverGlobals->DriverName);
#endif

    //
    // get system address from pseudo address for registers
    //
    if (pDevice->IsRegister(Type)){
        systemAddress = pDevice->GetSystemAddressFromPseudoAddress(TargetAddress);
    }

    length = pDevice->GetLength(Size);

    //
    // For buffer access for registers, compute the length of buffer using Count.
    // Count is the element count in the buffer (of UCHAR/USHORT/ULONG/ULONG64).
    // Note that Port is accessed differently than registers - the buffer is
    // written into the port address of specified size (UCHAR/USHORT/ULONG).
    //
    if (pDevice->IsBufferType(Type) && pDevice->IsRegister(Type)) {
        size_t tmp;

        hr = SizeTMult(Count, length, &tmp);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Integer overflow occurred "
        "when computing length of read access", (SUCCEEDED(hr))),
        DriverGlobals->DriverName);

        length = tmp;
    }

    resources = pDevice->GetTranslatedResources();

    //
    // Port access is always handled through system call.
    // Register access can be handled in usermode if driver enabled
    // mapping registers to usermode.
    //
    if (pDevice->AreRegistersMappedToUsermode() && pDevice->IsRegister(Type)) {
        PVOID umAddress = NULL;

        //
        // Acquire the resource validation table lock for read/write as well
        // since a driver's thread accessing hardware register/port and
        // race with Map/Unmap operations.
        //
        resources->LockResourceTable();

        hr = resources->ValidateRegisterSystemAddressRange(systemAddress,
                                                           length,
                                                           &umAddress);

        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
            CHECK("Driver attempted to read from invalid register address or "
            "address range", (SUCCEEDED(hr))),
            DriverGlobals->DriverName);

        if (pDevice->IsBufferType(Type)) {
            pDevice->ReadRegisterBuffer(Size, umAddress, Buffer, Count);
        }
        else {
            value = pDevice->ReadRegister(Size, umAddress);
        }

        resources->UnlockResourceTable();
    }
    else {
        //
        // Registers are not mapped to user-mode address space so send
        // message to reflector to use system HAL routines to access register.
        // Acquire validation table lock here as well since some read/write
        // thread might race with PrepareHardware/ReleaseHardware that may be
        // building/deleting the table.
        //
        IWudfDeviceStack *deviceStack;

        resources->LockResourceTable();

        if (pDevice->IsRegister(Type)) {
            hr = resources->ValidateRegisterSystemAddressRange(systemAddress,
                                                               length,
                                                               NULL);
            FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                CHECK("Driver attempted to read from invalid register address "
                "or address range", (SUCCEEDED(hr))),
                DriverGlobals->DriverName);
        }
        else {
            hr = resources->ValidatePortAddressRange(TargetAddress,
                                                     length);
            FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                CHECK("Driver attempted to read from invalid port address or "
                "address range", (SUCCEEDED(hr))),
                DriverGlobals->DriverName);
        }

        resources->UnlockResourceTable();

        deviceStack = pDevice->GetDeviceStack();
        deviceStack->ReadFromHardware((UMINT::WDF_DEVICE_HWACCESS_TARGET_TYPE)Type,
                                      (UMINT::WDF_DEVICE_HWACCESS_TARGET_SIZE)Size,
                                      TargetAddress,
                                      &value,
                                      Buffer,
                                      Count);
    }

    //
    // ETW event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_DDI_READ_FROM_HARDWARE_END(Type, Size, Count);

    return value;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfDeviceWriteToHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _In_
    SIZE_T Value,
    _In_reads_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    )
/*++

Routine Description:

    This routine does writes to a device port or register.

Arguments:
    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    Type - Specified whether it is port or register

    Size - Supplies size of read in bytes.

    TargetAddress - Supplies address of port or register to read from

    Value - value to write

    Buffer - Supplies optionally a buffer that has data that needs to be
            written.

    Count - Size of buffer in bytes

Return Value:

    void

--*/
{
    DDI_ENTRY();

    PVOID systemAddress = NULL;
    FxCmResList* resources;
    HRESULT hr = S_OK;
    SIZE_T length = 0;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    //
    // ETW event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_DDI_WRITE_TO_HARDWARE_START(Type, Size, Count);

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // See if direct hwaccess is allowed
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK(ERROR_STRING_HW_ACCESS_NOT_ALLOWED,
        (pDevice->IsDirectHardwareAccessAllowed() == TRUE)),
        DriverGlobals->DriverName);

    //
    // validate parameters
    //
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("Incorrect Type parameter",
            (Type > WdfDeviceHwAccessTargetTypeInvalid &&
             Type < WdfDeviceHwAccessTargetTypeMaximum)),
             DriverGlobals->DriverName);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        CHECK("Incorrect Size parameter",
            (Size > WdfDeviceHwAccessTargetSizeInvalid &&
             Size < WdfDeviceHwAccessTargetSizeMaximum)),
             DriverGlobals->DriverName);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(TargetAddress),
        DriverGlobals->DriverName);

    if (pDevice->IsBufferType(Type)) {
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(Buffer),
            DriverGlobals->DriverName);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Count should be > 0", (Count > 0)),
            DriverGlobals->DriverName);
    }
    else {
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NULL(Buffer), DriverGlobals->DriverName);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Count should be 0", (Count == 0)),
            DriverGlobals->DriverName);
    }

#if !defined(_WIN64)
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("ULONG64 write is allowed only"
        "on 64-bit platform", (Size != WdfDeviceHwAccessTargetSizeUlong64)),
        DriverGlobals->DriverName);
#endif

    //
    // get system address from pseudo address for registers
    //
    if (pDevice->IsRegister(Type)){
        systemAddress = pDevice->GetSystemAddressFromPseudoAddress(TargetAddress);
    }

    length = pDevice->GetLength(Size);

    //
    // For buffer access for registers, compute the length of buffer using Count.
    // Count is the element count in the buffer (of UCHAR/USHORT/ULONG/ULONG64).
    // Note that Port is accessed differently than registers - the buffer is
    // written into the port address of specified size (UCHAR/USHORT/ULONG).
    //
    if (pDevice->IsBufferType(Type) && pDevice->IsRegister(Type)) {
        size_t tmp;

        hr = SizeTMult(Count, length, &tmp);
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK("Integer overflow occurred "
        "when computing length of write access", (SUCCEEDED(hr))),
        DriverGlobals->DriverName);

        length = tmp;
    }

    resources = pDevice->GetTranslatedResources();

    //
    // Port access is always handled through system call.
    // Register access can be handled in usermode if driver enabled
    // mapping registers to usermode.
    //
    if (pDevice->AreRegistersMappedToUsermode() && pDevice->IsRegister(Type)) {
        PVOID umAddress = NULL;

        //
        // Acquire the resource validation table lock for read/write as well
        // since a driver's thread accessing hardware register/port and
        // race with Map/Unmap operations.
        //
        resources->LockResourceTable();


        hr = resources->ValidateRegisterSystemAddressRange(systemAddress,
                                                           length,
                                                           &umAddress);

        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
            CHECK("Driver attempted to write to invalid register address or "
            "address range", (SUCCEEDED(hr))),
            DriverGlobals->DriverName);

        if (pDevice->IsBufferType(Type)) {
            pDevice->WriteRegisterBuffer(Size, umAddress, Buffer, Count);
        }
        else {
            pDevice->WriteRegister(Size, umAddress, Value);
        }

        resources->UnlockResourceTable();
    }
    else {
        //
        // Registers are not mapped to user-mode address space so send
        // message to reflector to use system HAL routines to access register.
        // Acquire validation table lock here as well since some read/write
        // thread might race with PrepareHardware/ReleaseHardware that may be
        // building/deleting the table.
        //
        IWudfDeviceStack *deviceStack;

        resources->LockResourceTable();

        if (pDevice->IsRegister(Type)) {
            hr = resources->ValidateRegisterSystemAddressRange(systemAddress,
                                                               length,
                                                               NULL);
            FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                CHECK("Driver attempted to write to invalid register address "
                "or address range", (SUCCEEDED(hr))),
                DriverGlobals->DriverName);

        }
        else {
            hr = resources->ValidatePortAddressRange(TargetAddress,
                                                     length);
            FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                CHECK("Driver attempted to write to invalid port address or "
                "address range", (SUCCEEDED(hr))),
                DriverGlobals->DriverName);
        }

        resources->UnlockResourceTable();

        deviceStack = pDevice->GetDeviceStack();
        deviceStack->WriteToHardware((UMINT::WDF_DEVICE_HWACCESS_TARGET_TYPE)Type,
                                     (UMINT::WDF_DEVICE_HWACCESS_TARGET_SIZE)Size,
                                     TargetAddress,
                                     Value,
                                     Buffer,
                                     Count);
    }

    //
    // ETW event for perf measurement
    //
    EventWriteEVENT_UMDF_FX_DDI_WRITE_TO_HARDWARE_END(Type, Size, Count);

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignInterfaceProperty) (
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG BufferLength,
    _In_opt_
    PVOID PropertyBuffer
    )
/*++

Routine Description:

    This routine assigns interface property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PropertyData - A pointer to WDF_DEVICE_INTERFACE_PROPERTY_ DATA structure.

    Type - Set this parameter to the DEVPROPTYPE value that specifies the type
           of the data that is supplied in the Data buffer.

    BufferLength - Specifies the length, in bytes, of the buffer that
           PropertyBuffer points to.

    PropertyBuffer - optional, A pointer to the device interface property data.
           Set this parameter to NULL to delete the specified property.

Return Value:

    Mthod returns an NTSTATUS value. This routine might return one of the
    following values. It might return other NTSTATUS-codes as well.

    STATUS_SUCCESS - The operation succeeded.
    STATUS_INVALID_PARAMETER - One of the parameters is incorrect.

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    NTSTATUS status;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Validate PropertyData
    //
    status = pDevice->FxValidateInterfacePropertyData(PropertyData);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (BufferLength == 0 && PropertyBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is zero, while the buffer is non-NULL"
                    ", %!STATUS!", status);
        return status;
    }

    status = pDevice->AssignProperty(PropertyData,
                                     FxInterfaceProperty,
                                     Type,
                                     BufferLength,
                                     PropertyBuffer
                                     );
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryInterfaceProperty) (
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    )
/*++

Routine Description:

    This routine queries interface property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PropertyData - A pointer to WDF_DEVICE_INTERFACE_PROPERTY_ DATA structure.

    PoolType - A POOL_TYPE-typed enumerator that specifies the type of memory
               to be allocated.

    PropertyMemoryAttributes - optional, A pointer to a caller-allocated
               WDF_OBJECT_ATTRIBUTES structure that describes object attributes
               for the memory object that the function will allocate. This
               parameter is optional and can be WDF_NO_OBJECT_ATTRIBUTES.

    PropertyMemory - A pointer to a WDFMEMORY-typed location that receives a
               handle to a framework memory object.

    Type - A pointer to a DEVPROPTYPE variable. If method successfully retrieves
               the property data, the routine writes the property type value to
               this variable. This value indicates the type of property data
               that is in the Data buffer.


Return Value:

    Method returns an NTSTATUS value. This routine might return one of the
    following values. It might return other NTSTATUS-codes as well.

    STATUS_SUCCESS  The operation succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is incorrect.

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Validate PropertyData
    //
    status = pDevice->FxValidateInterfacePropertyData(PropertyData);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, PropertyMemory);
    FxPointerNotNull(pFxDriverGlobals, Type);

    *PropertyMemory = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, PropertyMemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxDevice::_AllocAndQueryPropertyEx(pFxDriverGlobals,
                                                NULL,
                                                pDevice,
                                                PropertyData,
                                                FxInterfaceProperty,
                                                PoolType,
                                                PropertyMemoryAttributes,
                                                PropertyMemory,
                                                Type);
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceQueryInterfaceProperty) (
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE PropertyType
    )
/*++

Routine Description:

    This routine queries interface property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PropertyData - A pointer to WDF_DEVICE_INTERFACE_PROPERTY_ DATA structure.

    BufferLength - The size, in bytes, of the buffer that is pointed to by
                   PropertyBuffer.

    PropertyBuffer - A caller-supplied pointer to a caller-allocated buffer that
                  receives the requested information. The pointer can be NULL
                  if the BufferLength parameter is zero.

    ResultLength - A caller-supplied location that, on return, contains the
                  size, in bytes, of the information that the method stored in
                  PropertyBuffer. If the function's return value is
                  STATUS_BUFFER_TOO_SMALL, this location receives the required
                  buffer size.

    Type - A pointer to a DEVPROPTYPE variable. If method successfully retrieves
                  the property data, the routine writes the property type value
                  to this variable. This value indicates the type of property
                  data that is in the Data buffer.

Return Value:

    Method returns an NTSTATUS value. This routine might return one of the
    following values.

    STATUS_BUFFER_TOO_SMALL - The supplied buffer is too small to receive the
                            information. The ResultLength member receives the
                            size of buffer required.
    STATUS_SUCCESS  - The operation succeeded.
    STATUS_INVALID_PARAMETER - One of the parameters is incorrect.

    The method might return other NTSTATUS values.

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Validate PropertyData
    //
    status = pDevice->FxValidateInterfacePropertyData(PropertyData);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, ResultLength);
    FxPointerNotNull(pFxDriverGlobals, PropertyType);

    if (BufferLength != 0 && PropertyBuffer == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is non-zero, while the buffer is NULL"
                    ", %!STATUS!", status);
        return status;
    }

    if (BufferLength == 0 && PropertyBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is zero, while the buffer is non-NULL"
                    ", %!STATUS!", status);
        return status;
    }

    status = FxDevice::_QueryPropertyEx(pFxDriverGlobals,
                                        NULL,
                                        pDevice,
                                        PropertyData,
                                        FxInterfaceProperty,
                                        BufferLength,
                                        PropertyBuffer,
                                        ResultLength,
                                        PropertyType);
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceGetDeviceStackIoType) (
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Out_
    WDF_DEVICE_IO_TYPE* ReadWriteIoType,
    _Out_
    WDF_DEVICE_IO_TYPE* IoControlIoType
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ReadWriteIoType);
    FxPointerNotNull(pFxDriverGlobals, IoControlIoType);

    pDevice->GetDeviceStackIoType(
                    ReadWriteIoType,
                    IoControlIoType
                    );
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceHidNotifyPresence)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN IsPresent
    )
{
    DDI_ENTRY();

    NTSTATUS status;
    HRESULT hr;
    FxDevice* pDevice;
    IWudfDeviceStack* pDevStack;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *)&pDevice,
                                   &pFxDriverGlobals);

    pDevStack = pDevice->m_DevStack;

    hr = pDevStack->HidNotifyPresence(IsPresent);
    status = FxDevice::NtStatusFromHr(pDevStack, hr);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "HidNotifyPresence(%s) failed, %!STATUS! - Make sure to call "
            "WdfDeviceInitEnableHidInterface in EvtDriverDeviceAdd "
            "before calling WdfDeviceHidNotifyPresence.",
            IsPresent ? "TRUE" : "FALSE", status);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFFILEOBJECT
WDFEXPORT(WdfDeviceGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdFileObject FileObject
    )
/*++

Routine Description:

    This functions returns the WDFFILEOBJECT corresponding to the WDM fileobject.

Arguments:

    Device - Handle to the device to which the WDM fileobject is related to.

    FileObject - WDM FILE_OBJECT structure.

Return Value:

--*/

{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(FileObject);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
        TRAPMSG("The DDI WdfDeviceGetFileObject is not supported for UMDF"),
        DriverGlobals->DriverName);

    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdIrp Irp,
    __in
    WDFQUEUE Queue,
    __in
    ULONG Flags
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxDevice* pDevice;
    FxIoQueue* pQueue;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Queue,
                         FX_TYPE_QUEUE,
                         (PVOID*)&pQueue);

    FxPointerNotNull(pFxDriverGlobals, Irp);

    FxIrp fxIrp(Irp);

    //
    // Unlike in KMDF, It's not possible for UMDF to forward to a parent queue.
    //
    ASSERT(pDevice->m_ParentDevice != pQueue->GetDevice());

    status = VerifyWdfDeviceWdmDispatchIrpToIoQueue(pFxDriverGlobals,
                                                    pDevice,
                                                    Irp,
                                                    pQueue,
                                                    Flags);
    if (!NT_SUCCESS(status)) {

        fxIrp.SetStatus(status);
        fxIrp.SetInformation(0x0);
        fxIrp.CompleteRequest(IO_NO_INCREMENT);

        return status;
    }

    //
    // DispatchStep2 will convert the IRP to a WDFRequest and queue it, dispatching
    // the request to the driver if possible.
    //
    return pDevice->m_PkgIo->DispatchStep2(reinterpret_cast<MdIrp>(Irp),
                                                                   NULL,
                                                                   pQueue);
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP pIrp,
    _In_
    WDFCONTEXT DispatchContext
    )

/*++

Routine Description:

    This DDI returns control of the IRP to the framework.
    This must only be called from the dispatch callback passed to
    WdfDeviceConfigureWdmIrpDispatchCallback


Arguments:

    Device - Handle to the I/O device.

    pIrp - Opaque handle to a _WUDF_IRP_WITH_VALIDATION structure.

    DispatchContext - Framework dispatch context passed as a parameter to the
                      dispatch callback.

Returns:

    IRP's status.

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Validate parameters and dispatch state. DispatchContext has already been
    // validated in DispatchStep1.
    //
    FxPointerNotNull(pFxDriverGlobals, pIrp);
    FxPointerNotNull(pFxDriverGlobals, DispatchContext);

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO),
                        CHECK("This function must be called from within a "
                        "EVT_WDFDEVICE_WDM_IRP_DISPATCH callback",
                        ((UCHAR)DispatchContext & FX_IN_DISPATCH_CALLBACK)),
                        DriverGlobals->DriverName);

    //
    // Adjust this context
    //
    DispatchContext = (WDFCONTEXT)((ULONG_PTR)DispatchContext & ~FX_IN_DISPATCH_CALLBACK);

    //
    // Cast this pIrp back to its composite parts and dispatch it again
    //
    return pDevice->m_PkgIo->DispatchStep1(reinterpret_cast<MdIrp>(pIrp),
                                           DispatchContext);
}

} // extern "C"
