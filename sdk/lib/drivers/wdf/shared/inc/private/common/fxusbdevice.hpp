/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDevice.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXUSBDEVICE_H_
#define _FXUSBDEVICE_H_

#include "FxUsbRequestContext.hpp"

typedef enum _FX_URB_TYPE : UCHAR {
    FxUrbTypeLegacy,
    FxUrbTypeUsbdAllocated
} FX_URB_TYPE;

struct FxUsbDeviceControlContext : public FxUsbRequestContext {
    FxUsbDeviceControlContext(
        __in FX_URB_TYPE FxUrbType
        );

    ~FxUsbDeviceControlContext(
        VOID
        );

    __checkReturn
    NTSTATUS
    AllocateUrb(
        __in USBD_HANDLE USBDHandle
        );

    virtual
    VOID
    Dispose(
        VOID
        );

    virtual
    VOID
    CopyParameters(
        __in FxRequestBase* Request
        );

    VOID
    StoreAndReferenceMemory(
        __in FxUsbDevice* Device,
        __in FxRequestBuffer* Buffer,
        __in PWDF_USB_CONTROL_SETUP_PACKET SetupPacket
        );

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

    USBD_STATUS
    GetUsbdStatus(
        VOID
        );

private:
    USBD_HANDLE m_USBDHandle;

public:

    _URB_CONTROL_TRANSFER m_UrbLegacy;

    //
    // m_Urb will either point to m_UrbLegacy or one allocated by USBD_UrbAllocate
    //
    _URB_CONTROL_TRANSFER* m_Urb;

    PMDL m_PartialMdl;

    BOOLEAN m_UnlockPages;
};

struct FxUsbDeviceStringContext : public FxUsbRequestContext {
    FxUsbDeviceStringContext(
        __in FX_URB_TYPE FxUrbType
        );

    ~FxUsbDeviceStringContext(
        VOID
        );

    __checkReturn
    NTSTATUS
    AllocateUrb(
        __in USBD_HANDLE USBDHandle
        );

    virtual
    VOID
    Dispose(
        VOID
        );

    virtual
    VOID
    CopyParameters(
        __in FxRequestBase* Request
        );

    VOID
    SetUrbInfo(
        __in  UCHAR StringIndex,
        __in  USHORT LangID
        );

    USBD_STATUS
    GetUsbdStatus(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateDescriptor(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t BufferSize
        );

private:
    USBD_HANDLE m_USBDHandle;

public:

    _URB_CONTROL_DESCRIPTOR_REQUEST m_UrbLegacy;

    //
    // m_Urb will either point to m_UrbLegacy or one allocated by USBD_UrbAllocate
    //
    _URB_CONTROL_DESCRIPTOR_REQUEST* m_Urb;

    PUSB_STRING_DESCRIPTOR m_StringDescriptor;

    ULONG m_StringDescriptorLength;
};

class FxUsbUrb : public FxMemoryBufferPreallocated {
public:

    FxUsbUrb(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USBD_HANDLE USBDHandle,
        __in_bcount(BufferSize) PVOID Buffer,
        __in size_t BufferSize
        );

protected:

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    ~FxUsbUrb();

private:

    USBD_HANDLE m_USBDHandle;
};


#define FX_USB_DEVICE_TAG   'sUfD'

class FxUsbDevice : public FxIoTarget {
public:
    friend FxUsbPipe;
    friend FxUsbInterface;

    FxUsbDevice(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    _Must_inspect_result_
    NTSTATUS
    InitDevice(
        __in ULONG USBDClientContractVersionForWdfClient
        );

    _Must_inspect_result_
    NTSTATUS
    GetConfigDescriptor(
        __out PVOID ConfigDescriptor,
        __inout PUSHORT ConfigDescriptorLength
        );

    _Must_inspect_result_
    NTSTATUS
    GetString(
        __in_ecount(*NumCharacters) PUSHORT String,
        __in PUSHORT NumCharacters,
        __in UCHAR StringIndex,
        __in_opt USHORT LangID,
        __in_opt WDFREQUEST Request = NULL,
        __in_opt PWDF_REQUEST_SEND_OPTIONS Options = NULL
        );

    __inline
    CopyDeviceDescriptor(
        __out PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
        )
    {
        RtlCopyMemory(UsbDeviceDescriptor,
                      &m_DeviceDescriptor,
                      sizeof(m_DeviceDescriptor));
    }

    VOID
    GetInformation(
        __out PWDF_USB_DEVICE_INFORMATION Information
        );

    __inline
    USBD_CONFIGURATION_HANDLE
    GetConfigHandle(
        VOID
        )
    {
        return m_ConfigHandle;
    }

    _Must_inspect_result_
    __inline
    NTSTATUS
    GetCurrentFrameNumber(
        __in PULONG Current
        )
    {
        if (m_QueryBusTime != NULL) {
            return m_QueryBusTime(m_BusInterfaceContext, Current);
        }
        else {
            return STATUS_UNSUCCESSFUL;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    SelectConfigAuto(
        __in PWDF_OBJECT_ATTRIBUTES PipeAttributes
        );

    _Must_inspect_result_
    NTSTATUS
    SelectConfigInterfaces(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
        __in_ecount(NumInterfaces)PUSB_INTERFACE_DESCRIPTOR* InterfaceDescriptors,
        __in ULONG NumInterfaces
        );

    _Must_inspect_result_
    NTSTATUS
    SelectConfig(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PURB Urb,
        __in FX_URB_TYPE FxUrbType,
        __out_opt PUCHAR NumConfiguredInterfaces
        );

    _Must_inspect_result_
    NTSTATUS
    Deconfig(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    SelectInterfaceByInterface(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
        );

    _Must_inspect_result_
    NTSTATUS
    SelectInterface(
        __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
        __in PURB Urb
        );

    UCHAR
    GetNumInterfaces(
        VOID
        )
    {
        return m_NumInterfaces;
    }

    UCHAR
    GetInterfaceNumEndpoints(
        __in UCHAR InterfaceNumber
        );

    WDFUSBPIPE
    GetInterfacePipeReferenced(
        __in UCHAR InterfaceNumber,
        __in UCHAR EndpointNumber
        );

    _Must_inspect_result_
    NTSTATUS
    FormatStringRequest(
        __in FxRequestBase* Request,
        __in FxRequestBuffer *RequestBuffer,
        __in  UCHAR StringIndex,
        __in  USHORT LangID
        );

    _Must_inspect_result_
    NTSTATUS
    FormatControlRequest(
        __in FxRequestBase* Request,
        __in PWDF_USB_CONTROL_SETUP_PACKET Packet,
        __in FxRequestBuffer *RequestBuffer
        );

    _Must_inspect_result_
    NTSTATUS
    IsConnected(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Reset(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    CyclePort(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    FormatCycleRequest(
        __in FxRequestBase* Request
        );

    BOOLEAN
    OnUSBD(
        VOID
        )
    {
        return m_OnUSBD;
    }

    USBD_PIPE_HANDLE
    GetControlPipeHandle(
        VOID
        )
    {
        return m_ControlPipe;
    }

    _Must_inspect_result_
    NTSTATUS
    CreateInterfaces(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    SelectConfigSingle(
        __in PWDF_OBJECT_ATTRIBUTES PipeAttributes,
        __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
        );

    _Must_inspect_result_
    NTSTATUS
    SelectConfigMulti(
        __in PWDF_OBJECT_ATTRIBUTES PipeAttributes,
        __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
        );

    _Must_inspect_result_
    NTSTATUS
    SelectConfigDescriptor(
        __in PWDF_OBJECT_ATTRIBUTES PipeAttributes,
        __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
        );

    FxUsbInterface *
    GetInterfaceFromIndex(
        __in UCHAR InterfaceIndex
        );

    BOOLEAN
    HasMismatchedInterfacesInConfigDescriptor(
        VOID
        )
    {
        return m_MismatchedInterfacesInConfigDescriptor;
    }

    VOID
    CancelSentIo(
        VOID
        );

    BOOLEAN
    IsEnabled(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    QueryUsbCapability(
        __in
        CONST GUID* CapabilityType,
        __in
        ULONG CapabilityBufferLength,
        __drv_when(CapabilityBufferLength == 0, __out_opt)
        __drv_when(CapabilityBufferLength != 0 && ResultLength == NULL, __out_bcount(CapabilityBufferLength))
        __drv_when(CapabilityBufferLength != 0 && ResultLength != NULL, __out_bcount_part_opt(CapabilityBufferLength, *ResultLength))
        PVOID CapabilityBuffer,
        __out_opt
        __drv_when(ResultLength != NULL,__deref_out_range(<=,CapabilityBufferLength))
        PULONG ResultLength
        );

    __checkReturn
    NTSTATUS
    CreateUrb(
        __in_opt
        PWDF_OBJECT_ATTRIBUTES Attributes,
        __out
        WDFMEMORY* UrbMemory,
        __deref_opt_out_bcount(sizeof(URB))
        PURB* Urb
        );

#pragma warning(disable:28285)
    __checkReturn
    NTSTATUS
    CreateIsochUrb(
        __in_opt
        PWDF_OBJECT_ATTRIBUTES Attributes,
        __in
        ULONG NumberOfIsochPackets,
        __out
        WDFMEMORY* UrbMemory,
        __deref_opt_out_bcount(GET_ISOCH_URB_SIZE(NumberOfIsochPackets))
        PURB* Urb
        );

    USBD_HANDLE
    GetUSBDHandle(
        VOID
        )
    {
        return m_USBDHandle;
    }

    FX_URB_TYPE
    GetUrbType(
        VOID
        )
    {
        return m_UrbType;
    }

    FX_URB_TYPE
    GetFxUrbTypeForRequest(
        __in FxRequestBase* Request
        );

    BOOLEAN
    IsObjectDisposedOnRemove(
        __in FxObject* Object
        );

protected:
    ~FxUsbDevice(
        VOID
        );

    VOID
    RemoveDeletedInterface(
        __in FxUsbInterface* Interface
        );

    //
    // FxIoTarget overrides
    //
    virtual
    _Must_inspect_result_
    NTSTATUS
    Start(
        VOID
        );

    virtual
    VOID
    Stop(
        __in WDF_IO_TARGET_SENT_IO_ACTION Action
        );

    virtual
    VOID
    Purge(
        __in WDF_IO_TARGET_PURGE_IO_ACTION Action
        );
    // end FxIoTarget overrides

    VOID
    PipesGotoRemoveState(
        __in BOOLEAN ForceRemovePipes
        );

    static
    VOID
    _CleanupPipesRequests(
        __in PLIST_ENTRY PendHead,
        __in PSINGLE_LIST_ENTRY SentHead
        );

    FxUsbInterface *
    GetInterfaceFromNumber(
        __in UCHAR InterfaceNumber
        );

    _Must_inspect_result_
    NTSTATUS
    GetInterfaceNumberFromInterface(
        __in WDFUSBINTERFACE UsbInterface,
        __out PUCHAR InterfaceNumber
        );

    VOID
    CleanupInterfacePipesAndDelete(
        __in FxUsbInterface * UsbInterface
        );

    _Acquires_lock_(_Global_critical_region_)
    VOID
    AcquireInterfaceIterationLock(
        VOID
        )
    {
        m_InterfaceIterationLock.AcquireLock(GetDriverGlobals());
    }

    _Releases_lock_(_Global_critical_region_)
    VOID
    ReleaseInterfaceIterationLock(
        VOID
        )
    {
        m_InterfaceIterationLock.ReleaseLock(GetDriverGlobals());
    }

    ULONG
    GetDefaultMaxTransferSize(
        VOID
        );

    VOID
    FormatInterfaceSelectSettingUrb(
        __in PURB Urb,
        __in USHORT NumEndpoints,
        __in UCHAR InterfaceNumber,
        __in UCHAR SettingNumber
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    GetPortStatus(
        __out PULONG PortStatus
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    _Must_inspect_result_
    NTSTATUS
    FxUsbDevice::SendSyncRequest(
        __in FxSyncRequest* Request,
        __in ULONGLONG Time
        );

    _Must_inspect_result_
    NTSTATUS
    SendSyncUmUrb(
        __inout PUMURB Urb,
        __in ULONGLONG Time,
        __in_opt WDFREQUEST Request = NULL,
        __in_opt PWDF_REQUEST_SEND_OPTIONS Options = NULL
        );
#endif

protected:
    USBD_HANDLE m_USBDHandle;

    USBD_PIPE_HANDLE m_ControlPipe;

    FxUsbInterface ** m_Interfaces;

    USBD_CONFIGURATION_HANDLE m_ConfigHandle;

    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;

    PUSB_CONFIGURATION_DESCRIPTOR m_ConfigDescriptor;

    USBD_VERSION_INFORMATION m_UsbdVersionInformation;

    PUSB_BUSIFFN_QUERY_BUS_TIME m_QueryBusTime;

    PVOID m_BusInterfaceContext;

    PINTERFACE_DEREFERENCE m_BusInterfaceDereference;

    FxWaitLockInternal m_InterfaceIterationLock;

    ULONG m_HcdPortCapabilities;

    ULONG m_Traits;

    BOOLEAN m_OnUSBD;

    UCHAR m_NumInterfaces;

    BOOLEAN m_MismatchedInterfacesInConfigDescriptor;

    FX_URB_TYPE m_UrbType;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
private:
    //
    // Used to format the IWudfIrp for user-mode requests
    //
    IWudfFile* m_pHostTargetFile;

    //
    // Handle to the default USB interface exposed by WinUsb
    //
    WINUSB_INTERFACE_HANDLE m_WinUsbHandle;
#endif
};

#endif // _FXUSBDEVICE_H_
