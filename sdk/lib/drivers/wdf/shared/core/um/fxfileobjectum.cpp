/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectUm.hpp

Abstract:

    This module implements a frameworks managed FileObject

Author:



Environment:

    User mode only

Revision History:



--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxFileObjectUm.tmh"
}

VOID
FxFileObject::SetFileObjectContext(
    _In_ MdFileObject WdmFileObject,
    _In_ WDF_FILEOBJECT_CLASS NormalizedFileClass,
    _In_ MdIrp Irp,
    _In_ FxDevice* Device
    )
{
    IWudfIoIrp* pIoIrp;
    HRESULT hrQi;

    UNREFERENCED_PARAMETER(WdmFileObject);
    UNREFERENCED_PARAMETER(NormalizedFileClass);

    ASSERT(NormalizedFileClass == WdfFileObjectWdfCannotUseFsContexts);

    //
    // In UMDF, fx file object is stored by host. Host passes it back to
    // framework as a parameter to irp dispatch routine.
    //
    hrQi = Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQi, pIoIrp));
    pIoIrp->Release();

    pIoIrp->SetFrameworkFileObjectContext(Device->GetDeviceObject(),
                                          (IUnknown *) this);
}

VOID
FxFileObject::Initialize(
    _In_ MdIrp CreateIrp
    )
{
    FxIrp irp(CreateIrp);
    IWudfIoIrp* ioIrp = irp.GetIoIrp();
    IUnknown* pCxtFramework = NULL;
    FxFileObject* pRelatedFileObj = NULL;

    //
    // Get framework related file object.
    //
    ioIrp->GetFrameworkRelatedFileObjectContext(
                                    m_Device->GetDeviceObject(),
                                    &pCxtFramework
                                    );
    if (pCxtFramework != NULL) {
        pRelatedFileObj = (FxFileObject*) pCxtFramework;

        pRelatedFileObj->AddRef();
        this->m_RelatedFileObject = pRelatedFileObj;
    }

    return;
}

_Must_inspect_result_
NTSTATUS
FxFileObject::UpdateProcessKeepAliveCount(
    _In_ BOOLEAN Increment
    )
{
    MdFileObject pWdmFO = GetWdmFileObject();
    FxDevice* pDevice = GetDevice();

    if (pWdmFO == NULL) {
        FX_VERIFY(DRIVER(BadArgument, TODO), TRAPMSG("Cannot increment "
            "process keep alive count from a file object that doesn't "
            "have a WDM file object"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Validate that driver set UmdfFsContextUsePolicy = CannotUseFsContexts
    //
    if (pDevice->m_FsContextUsePolicy != WdfCannotUseFsContexts) {
        FX_VERIFY(DRIVER(BadArgument, TODO), TRAPMSG("Cannot increment "
            "process keep alive count unless UmdfFsContextUsePolicy INF "
            "directive is set to WdfCannotUseFsContexts"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return pDevice->NtStatusFromHr(
        pDevice->GetDeviceStack2()->
            UpdateProcessKeepAliveCount(pWdmFO, Increment));
}

