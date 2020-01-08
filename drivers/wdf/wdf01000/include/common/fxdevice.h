#ifndef _FXDEVICE_H_
#define _FXDEVICE_H_


#include "common/fxnonpagedobject.h"
#include "common/fxdisposelist.h"

//
// Base class for all devices.
//
class FxDeviceBase : public FxNonPagedObject {

protected:
    FxDeviceBase(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxDriver* Driver,
        __in WDFTYPE Type,
        __in USHORT Size
        );

    ~FxDeviceBase(
        VOID
        );

public:

    //
    // This is used to defer items that must be cleaned up at passive
    // level, and FxDevice waits on this list to empty in DeviceRemove.
    //
    FxDisposeList* m_DisposeList;

VOID
    AddToDisposeList(
        __inout FxObject* Object
        )
    {
        m_DisposeList->Add(Object);
    }

};

class FxDevice : public FxDeviceBase {

};

#endif //_FXDEVICE_H_
