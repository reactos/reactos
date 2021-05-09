/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxIoTargetRemoteUm.hpp

Abstract:

    User-mode specific definitions of FxIoTargetRemote

Author:


Environment:

    User mode only

Revision History:

--*/

#pragma once

class FxIoTargetRemoteNotificationCallback :
                    public FxGlobalsStump,
                    public IWudfTargetCallbackDeviceChange
{
private:

    LONG m_cRefs;

    FxIoTargetRemote* m_RemoteTarget;

public:

    FxIoTargetRemoteNotificationCallback(
        PFX_DRIVER_GLOBALS FxDriverGlobals,
        FxIoTargetRemote* Target
        ) :
        FxGlobalsStump(FxDriverGlobals),
        m_RemoteTarget(Target),
        m_cRefs(1)
    {
    }

    ~FxIoTargetRemoteNotificationCallback() {};

    WUDF_TARGET_CONTEXT
    GetRegistrationId(
        VOID
        )
    {
        return m_RemoteTarget->m_TargetNotifyHandle;
    }

    BOOL
    __stdcall
    OnQueryRemove(
        _In_ WUDF_TARGET_CONTEXT RegistrationID
        );

    VOID
    __stdcall
    OnRemoveCanceled(
        _In_ WUDF_TARGET_CONTEXT RegistrationID
        );

    VOID
    __stdcall
    OnRemoveComplete(
        _In_ WUDF_TARGET_CONTEXT RegistrationID
        );

    VOID
    __stdcall
    OnCustomEvent(
        _In_ WUDF_TARGET_CONTEXT  RegistrationID,
        _In_ REFGUID Event,
        _In_reads_bytes_(DataSize) BYTE * Data,
        _In_ DWORD DataSize,
        _In_ DWORD NameBufferOffset
        );

    HRESULT
    __stdcall
    QueryInterface(
        __in  const IID& iid,
        __out void ** ppv
        )
    {
        if (NULL == ppv) {
            return E_INVALIDARG;
        }

        *ppv = NULL;

        if ( iid == IID_IUnknown) {
            *ppv = static_cast<IUnknown *> (this);
        }
        else if ( iid == IID_IWudfTargetCallbackDeviceChange) {
            *ppv = static_cast<IWudfTargetCallbackDeviceChange *> (this);
        }
        else {
            return E_INVALIDARG;
        }

        this->AddRef();
        return S_OK;
    }

    ULONG
    __stdcall
    AddRef(
        )
    {
        LONG cRefs = InterlockedIncrement( &m_cRefs );
        return cRefs;
    }

    ULONG
    __stdcall
    Release(
        )
    {
        LONG cRefs = InterlockedDecrement( &m_cRefs );
        if (0 == cRefs) {
            //
            // The lifetime of this object is controlled by FxIoTargetRemote
            // object (the container object), and not by this ref count. This
            // method is implemented just to satisfy the interface implemetation
            // requirement.
            //
            DO_NOTHING();
        }

        return cRefs;
    }
};

