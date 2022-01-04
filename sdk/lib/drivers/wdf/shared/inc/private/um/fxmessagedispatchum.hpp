/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    WudfDispatcher.h

Abstract:

    This file contains the class definition of the WUDF dispatcher object.

Author:



Environment:

    User mode only

Revision History:



--*/
#pragma once

extern const GUID IID_FxMessageDispatch;
extern const GUID IID_FxMessageDispatch2;

class FxMessageDispatch;

class FxMessageDispatch :
    public FxStump,
    public IFxMessageDispatch2
{
    //
    // Manager functions.
    //
private:
    FxMessageDispatch(
        _In_ FxDevice* Device
        ) :
        m_cRefs(1),
        m_Device(Device)
    {
    }

public:
    ~FxMessageDispatch()
    {
    //    SAFE_RELEASE(m_Device);
    }

    static
    NTSTATUS
    _CreateAndInitialize(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _In_ FxDevice* Device,
        _Out_ FxMessageDispatch ** ppWudfDispatcher
       );

    //
    // IUnknown
    //
public:
    HRESULT
    __stdcall
    QueryInterface(
        _In_ REFIID     riid,
        _Out_ LPVOID*   ppvObject
        );

    ULONG
    __stdcall
    AddRef();

    ULONG
    __stdcall
    Release();

    //
    // IFxMessageDispatch
    //
public:
    virtual void __stdcall
    DispatchPnP(
        _In_ IWudfIrp *  pIrp
        );

    virtual void __stdcall
    CreateFile(
        _In_ IWudfIoIrp *  pCreateIrp
        );

    virtual void __stdcall
    DeviceControl(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual void __stdcall
    ReadFile(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual void __stdcall
    WriteFile(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual void __stdcall
    CleanupFile(
        _In_ IWudfIoIrp * pIrp,
        _In_ IUnknown * pFxContext
        );

    virtual void __stdcall
    CloseFile(
        _In_ IWudfIoIrp * pIrp,
        _In_ IUnknown * pFxContext
        );

    virtual
    VOID
    __stdcall
    GetPreferredTransferMode(
        _Out_ UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL *RetrievalMode,
        _Out_ UMINT::WDF_DEVICE_IO_TYPE *RWPreference,
        _Out_ UMINT::WDF_DEVICE_IO_TYPE *IoctlPreference
        );

    virtual void __stdcall
    FlushBuffers(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual void __stdcall
    QueryInformationFile(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual void __stdcall
    SetInformationFile(
        _In_ IWudfIoIrp * pIrp,
        _In_opt_ IUnknown * pFxContext
        );

    virtual NTSTATUS __stdcall
    ProcessWmiPowerQueryOrSetData(
        _In_ RdWmiPowerAction Action,
        _Out_ BOOLEAN *QueryResult
        );

    //
    // We should remove these methods from this interface.
    //
    virtual WUDF_INTERFACE_CONTEXT __stdcall
    RemoteInterfaceArrival(
        _In_    LPCGUID pDeviceInterfaceGuid,
        _In_    PCWSTR  pSymbolicLink
        );

    virtual void __stdcall
    RemoteInterfaceRemoval(
        _In_    WUDF_INTERFACE_CONTEXT RemoteInterfaceID
        );

    virtual BOOL __stdcall
    TransportQueryID(
        _In_    DWORD  Id,
        _In_    PVOID  DataBuffer,
        _In_    SIZE_T cbDataBufferSize
        );

    virtual ULONG __stdcall
    GetDirectTransferThreshold(
        VOID
        );

    virtual void __stdcall
    PoFxDevicePowerRequired(
        VOID
        );

    virtual void __stdcall
    PoFxDevicePowerNotRequired(
        VOID
        );

    //
    // Additional public functions.
    //
public:
    //
    // Returns the Dispatcher object from the given interface without
    // incrementing the refcount.
    //
    static
    FxMessageDispatch*
    _GetObjFromItf(
        _In_ IFxMessageDispatch* pIFxMessageDispatch
        );

    //
    // Returns the specified interface from the given object without
    // incrementing the refcount.
    //
    static
    IFxMessageDispatch*
    _GetDispatcherItf(
        _In_ FxMessageDispatch* pWudfDispatcher
        );

    //
    // Returns a weak ref to the embedded user-mode driver object.
    //
    PDRIVER_OBJECT_UM
    GetDriverObject(
        VOID
        )
    {
        return m_Device->GetDriver()->GetDriverObject();
    }

    MdDeviceObject
    GetDeviceObject(
        VOID
        )
    {
        return m_Device->GetDeviceObject();
    }

    //
    // Data members.
    //
private:
    //
    // Reference count for debugging purposes. The lifetime is managed by
    // FxDevice.
    //
    LONG                    m_cRefs;

    //
    // Device object associated with this dispatcher object.
    //
    FxDevice *           m_Device;
};

