/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxQueryInterface.hpp

Abstract:

    This module implements the "query" interface object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXQUERYINTERFACE_H_
#define _FXQUERYINTERFACE_H_

class FxDeviceProcessQueryInterfaceRequest : public FxCallback {

public:

    FxDeviceProcessQueryInterfaceRequest(
        VOID
        ) :
        m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in LPGUID InterfacType,
        __out PINTERFACE ExposedInterface,
        __in_opt PVOID ExposedInterfaceSpecificData
        )
    {
        if (m_Method != NULL) {
            NTSTATUS status;

            CallbackStart();
            status = m_Method(Device,
                              InterfacType,
                              ExposedInterface,
                              ExposedInterfaceSpecificData);
            CallbackEnd();

            return status;
        }
        else {
            return STATUS_SUCCESS;
        }
    }

public:
    PFN_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST m_Method;
};

struct FxQueryInterface : public FxStump {

public:
    FxQueryInterface(
        __in CfxDevice* Device,
        __in PWDF_QUERY_INTERFACE_CONFIG Config
        );

    ~FxQueryInterface(
        VOID
        );

    VOID
    SetEmbedded(
        __in PWDF_QUERY_INTERFACE_CONFIG Config,
        __in PINTERFACE Interface
        );

    static
    FxQueryInterface*
    _FromEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxQueryInterface, m_Entry);
    }

    static
    VOID
    _FormatIrp(
        __in PIRP Irp,
        __in const GUID* InterfaceGuid,
        __out PINTERFACE Interface,
        __in USHORT InterfaceSize,
        __in USHORT InterfaceVersion,
        __in_opt PVOID InterfaceSpecificData = NULL
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _QueryForInterface(
        __in PDEVICE_OBJECT TopOfStack,
        __in const GUID* InterfaceType,
        __out PINTERFACE Interface,
        __in USHORT Size,
        __in USHORT Version,
        __in_opt PVOID InterfaceSpecificData
        );

public:
    GUID m_InterfaceType;

    PINTERFACE m_Interface;

    CfxDevice *m_Device;

    FxDeviceProcessQueryInterfaceRequest m_ProcessRequest;

    SINGLE_LIST_ENTRY m_Entry;

    BOOLEAN m_ImportInterface;

    BOOLEAN m_SendQueryToParentStack;

    BOOLEAN m_EmbeddedInterface;
};

#endif // _FXQUERYINTERFACE_H_
