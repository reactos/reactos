#ifndef _PNPPRIV_H_
#define _PNPPRIV_H_

#include "common/fxglobals.h"
#include "common/mxdeviceobject.h"

_Must_inspect_result_
NTSTATUS
GetStackCapabilities(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in MxDeviceObject* DeviceInStack,
    __out PDEVICE_CAPABILITIES Capabilities
    );

#endif //_PNPPRIV_H_    