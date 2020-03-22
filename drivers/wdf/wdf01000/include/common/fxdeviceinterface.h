#ifndef _FXDEVICEINTERFACE_H_
#define _FXDEVICEINTERFACE_H_

#include "common/fxstump.h"

class FxDevice;
class FxDeviceInterface : public FxStump
{

public:
    GUID m_InterfaceClassGUID;

    UNICODE_STRING m_ReferenceString;

    UNICODE_STRING m_SymbolicLinkName;

    SINGLE_LIST_ENTRY m_Entry;

    BOOLEAN m_State;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // This is needed in UM to get hold of host interface 
    //
    MdDeviceObject m_Device;

#endif

public:
    FxDeviceInterface(
        VOID
        );

    ~FxDeviceInterface(
        VOID
        );

    static
    FxDeviceInterface*
    _FromEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxDeviceInterface, m_Entry);
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CONST GUID* InterfaceGUID,
        __in_opt PCUNICODE_STRING ReferenceString
        );

    _Must_inspect_result_
    NTSTATUS
    Register(
        __in MdDeviceObject Pdo
        );

    _Must_inspect_result_
    NTSTATUS
    Register(
        _In_ FxDevice* Device
        );

    VOID
    SetState(
        __in BOOLEAN State
        );

};

#endif //_FXDEVICEINTERFACE_H_
