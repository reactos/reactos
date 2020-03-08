#ifndef _FXQUERYINTERFACE_H_
#define _FXQUERYINTERFACE_H_

#include "common/fxstump.h"
#include "common/fxtypedefs.h"


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

    //FxDeviceProcessQueryInterfaceRequest m_ProcessRequest;

    SINGLE_LIST_ENTRY m_Entry;

    BOOLEAN m_ImportInterface;

    BOOLEAN m_SendQueryToParentStack;

    BOOLEAN m_EmbeddedInterface;
};

#endif //_FXQUERYINTERFACE_H_
