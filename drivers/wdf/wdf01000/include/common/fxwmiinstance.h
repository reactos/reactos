#ifndef _FXWMIINSTANCE_H_
#define _FXWMIINSTANCE_H_

#include "common/fxnonpagedobject.h"
#include "common/fxwmiprovider.h"

class FxWmiProvider;
class FxWmiIrpHandler;

class FxWmiInstance : public FxNonPagedObject {

    friend FxWmiProvider;
    friend FxWmiIrpHandler;

public:
    FxWmiInstance(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in FxWmiProvider* Provider
        );

    ~FxWmiInstance();

    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Provider->GetDevice();
    }

    FxWmiProvider*
    GetProvider(
        VOID
        )
    {
        return m_Provider;
    }

    BOOLEAN
    IsEnabled(
        __in WDF_WMI_PROVIDER_CONTROL Control
        )
    {
        return m_Provider->IsEnabled(Control);
    }

    WDFWMIINSTANCE
    GetHandle(
        VOID
        )
    {
        return (WDFWMIINSTANCE) GetObjectHandle();
    }

protected:
    //
    // List entry used by FxWmiProvider to hold the list of WMI instances
    //
    LIST_ENTRY m_ListEntry;

    //
    // Pointer to the parent provider
    //
    FxWmiProvider* m_Provider;
};

#endif //_FXWMIINSTANCE_H_
