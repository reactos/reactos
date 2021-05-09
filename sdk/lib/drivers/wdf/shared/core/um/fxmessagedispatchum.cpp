/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxMessageDispatchUm.cpp

Abstract:

    Implements the host process dispatcher object.  See header file for
    details.

Author:



Environment:

    User mode only

Revision History:



--*/

#include "fxmin.hpp"
#include "fxldrum.h"

extern "C"
{
#include "FxMessageDispatchUm.tmh"
}

// {cba20727-0910-4a8a-aada-c31d5cf5bf20}
extern const GUID IID_FxMessageDispatch =
{0xcba20727, 0x0910, 0x4a8a, { 0xaa, 0xda, 0xc3, 0x1d, 0x5c, 0xf5, 0xbf, 0x20 }};

//
// Manager functions.
//
NTSTATUS
FxMessageDispatch::_CreateAndInitialize(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_ FxDevice* Device,
    _Out_ FxMessageDispatch ** ppWudfDispatcher
   )
{
    HRESULT             hr = S_OK;
    FxMessageDispatch *   pWudfDispatcher = NULL;
    NTSTATUS status;

    *ppWudfDispatcher = NULL;

    pWudfDispatcher = new (DriverGlobals) FxMessageDispatch(Device);
    if (NULL == pWudfDispatcher) {
        hr = E_OUTOFMEMORY;
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Memory allocation failure. Cannot create Dispatcher object.\n");
        goto Done;
    }

    *ppWudfDispatcher = pWudfDispatcher;

Done:

    if (FAILED(hr)) {
        status = Device->NtStatusFromHr(hr);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

//
// IUnknown
//
ULONG
FxMessageDispatch::AddRef()
{
    LONG cRefs = InterlockedIncrement( &m_cRefs );

    //
    // This allows host to manage the lifetime of FxDevice.
    //
    m_Device->ADDREF(this);

    return cRefs;
}


ULONG
FxMessageDispatch::Release()
{
    LONG cRefs = InterlockedDecrement( &m_cRefs );

    if (0 == cRefs) {
        //
        // The lifetime of this object is controlled by FxDevice
        // object (the container object), and not by this ref count. FxDevice
        // will delete this object in its destructior.
        //
        DO_NOTHING();
    }

    //
    // This allows host to manage the lifetime of FxDevice. If this is the last
    // release on FxDevice, FxDevice will e deleted and this object will be
    // deleted as part of FxDevice destructor.
    //
    m_Device->RELEASE(this);

    return cRefs;
}

HRESULT
FxMessageDispatch::QueryInterface(
    _In_ const IID&     iid,
    _Out_ void **       ppv
    )
{
    if ( NULL == ppv ) {
        return E_INVALIDARG;
    }

    if ( iid == IID_IUnknown) {
        *ppv = static_cast<IUnknown *> (this);
    }
    else if ( iid == IID_IFxMessageDispatch ) {
        *ppv = static_cast<IFxMessageDispatch *> (this);
    }
    else if ( iid == IID_IFxMessageDispatch2 ) {
        *ppv = static_cast<IFxMessageDispatch2 *> (this);
    }
    else if ( iid == IID_FxMessageDispatch ) {
        *ppv = this;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    this->AddRef();
    return S_OK;
}

//
// IFxMessageDispatch
//
void
FxMessageDispatch::DispatchPnP(
    _In_ IWudfIrp *  pIrp
    )
{
    IWudfPnpIrp * pIPnpIrp = NULL;

    HRESULT hrQI = pIrp->QueryInterface(IID_IWudfPnpIrp, (PVOID*)&pIPnpIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIPnpIrp));

    if (IRP_MJ_POWER == pIPnpIrp->GetMajorFunction()) {
        GetDriverObject()->MajorFunction[IRP_MJ_POWER](GetDeviceObject(),
                                                       pIrp,
                                                       NULL);
    }
    else {
        GetDriverObject()->MajorFunction[IRP_MJ_PNP](GetDeviceObject(),
                                                     pIrp,
                                                     NULL);
    }

    SAFE_RELEASE(pIPnpIrp);
}

void
FxMessageDispatch::CreateFile(
    _In_ IWudfIoIrp *  pCreateIrp
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_CREATE](GetDeviceObject(),
                                                    pCreateIrp,
                                                    NULL);
}

void
FxMessageDispatch::DeviceControl(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_DEVICE_CONTROL](GetDeviceObject(),
                                                            pIrp,
                                                            pFxContext);
}

void
FxMessageDispatch::ReadFile(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_READ](GetDeviceObject(), pIrp, pFxContext);
}

void
FxMessageDispatch::WriteFile(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_WRITE](GetDeviceObject(), pIrp, pFxContext);
}

void
FxMessageDispatch::CleanupFile(
    _In_ IWudfIoIrp * pIrp,
    _In_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_CLEANUP](GetDeviceObject(), pIrp, pFxContext);
}

void
FxMessageDispatch::CloseFile(
    _In_ IWudfIoIrp * pIrp,
    _In_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_CLOSE](GetDeviceObject(), pIrp, pFxContext);
}

void
FxMessageDispatch::FlushBuffers(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_FLUSH_BUFFERS](GetDeviceObject(),
                                                           pIrp,
                                                           pFxContext);
}

void
FxMessageDispatch::QueryInformationFile(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_QUERY_INFORMATION](
                                                    GetDeviceObject(),
                                                    pIrp,
                                                    pFxContext
                                                    );
}

void
FxMessageDispatch::SetInformationFile(
    _In_ IWudfIoIrp * pIrp,
    _In_opt_ IUnknown * pFxContext
    )
{
    GetDriverObject()->MajorFunction[IRP_MJ_SET_INFORMATION](GetDeviceObject(),
                                                             pIrp,
                                                             pFxContext);
}

VOID
FxMessageDispatch::GetPreferredTransferMode(
    _Out_ UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL *RetrievalMode,
    _Out_ UMINT::WDF_DEVICE_IO_TYPE *RWPreference,
    _Out_ UMINT::WDF_DEVICE_IO_TYPE *IoctlPreference
    )
{
    FxDevice::GetPreferredTransferMode(GetDeviceObject(),
                                       RetrievalMode,
                                       (WDF_DEVICE_IO_TYPE*)RWPreference,
                                       (WDF_DEVICE_IO_TYPE*)IoctlPreference);
}

ULONG
FxMessageDispatch::GetDirectTransferThreshold(
    VOID
    )
{
    return m_Device->GetDirectTransferThreshold();
}

NTSTATUS
FxMessageDispatch::ProcessWmiPowerQueryOrSetData(
    _In_ RdWmiPowerAction Action,
    _Out_ BOOLEAN *QueryResult
    )
{
    return m_Device->ProcessWmiPowerQueryOrSetData(Action, QueryResult);
}

WUDF_INTERFACE_CONTEXT
FxMessageDispatch::RemoteInterfaceArrival(
    _In_    LPCGUID pDeviceInterfaceGuid,
    _In_    PCWSTR  pSymbolicLink
    )
{
    return FxDevice::RemoteInterfaceArrival(GetDeviceObject(),
                                            pDeviceInterfaceGuid,
                                            pSymbolicLink);
}

void
FxMessageDispatch::RemoteInterfaceRemoval(
    _In_    WUDF_INTERFACE_CONTEXT RemoteInterfaceID
    )
{
    FxDevice::RemoteInterfaceRemoval(GetDeviceObject(),
                                     RemoteInterfaceID);
}

BOOL
FxMessageDispatch::TransportQueryID(
    _In_    DWORD  Id,
    _In_    PVOID  DataBuffer,
    _In_    SIZE_T cbDataBufferSize
    )
{
    return FxDevice::TransportQueryId(GetDeviceObject(),
                                      Id,
                                      DataBuffer,
                                      cbDataBufferSize);
}

void
FxMessageDispatch::PoFxDevicePowerRequired(
    void
    )
{
    FxDevice::PoFxDevicePowerRequired(GetDeviceObject());
}

void
FxMessageDispatch::PoFxDevicePowerNotRequired(
    void
    )
{
    FxDevice::PoFxDevicePowerNotRequired(GetDeviceObject());
}

//
// Additional public functions.
//

//
// Returns the Dispatcher object from the given interface without
// incrementing the refcount.
//
FxMessageDispatch*
FxMessageDispatch::_GetObjFromItf(
    _In_ IFxMessageDispatch* pIFxMessageDispatch
    )
{
    FxMessageDispatch * pWudfDispatcher = NULL;
    HRESULT hrQI = pIFxMessageDispatch->QueryInterface(
                        IID_FxMessageDispatch,
                        reinterpret_cast<void**>(&pWudfDispatcher)
                        );
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pWudfDispatcher));
    pWudfDispatcher->Release(); //release the reference taken by QI
    return pWudfDispatcher;
}

//
// Returns the specified interface from the given object without
// incrementing the refcount.
//
IFxMessageDispatch*
FxMessageDispatch::_GetDispatcherItf(
    _In_ FxMessageDispatch* pWudfDispatcher
    )
{
    IFxMessageDispatch * pIFxMessageDispatch = NULL;
    HRESULT hrQI = pWudfDispatcher->QueryInterface(IID_IFxMessageDispatch,
                                                  (PVOID*)&pIFxMessageDispatch);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIFxMessageDispatch));
    pIFxMessageDispatch->Release(); //release the reference taken by QI
    return pIFxMessageDispatch;
}
