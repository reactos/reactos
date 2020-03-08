#ifndef _PNPPRIV_H_
#define _PNPPRIV_H_

#include "common/fxglobals.h"
#include "common/mxdeviceobject.h"


#define SET_PNP_CAP_IF_TRUE(caps, pCaps, FieldName)                         \
{                                                                           \
    if ((caps & FxPnpCap##FieldName##Mask) == FxPnpCap##FieldName##True) {  \
        pCaps->FieldName = TRUE;                                            \
    }                                                                       \
}

#define SET_PNP_CAP_IF_FALSE(caps, pCaps, FieldName)                        \
{                                                                           \
    if ((caps & FxPnpCap##FieldName##Mask) == FxPnpCap##FieldName##False) { \
        pCaps->FieldName = FALSE;                                           \
    }                                                                       \
}

#define SET_PNP_CAP(caps, pCaps, FieldName)                                 \
{                                                                           \
    if ((caps & FxPnpCap##FieldName##Mask) == FxPnpCap##FieldName##False) { \
        pCaps->FieldName = FALSE;                                           \
    }                                                                       \
    else if ((caps & FxPnpCap##FieldName##Mask) == FxPnpCap##FieldName##True) {  \
        pCaps->FieldName = TRUE;                                            \
    }                                                                       \
}

#define SET_POWER_CAP(caps, pCaps, FieldName)                               \
{                                                                           \
    if ((caps & FxPowerCap##FieldName##Mask) == FxPowerCap##FieldName##False) { \
        pCaps->FieldName = FALSE;                                           \
    }                                                                       \
    else if ((caps & FxPowerCap##FieldName##Mask) == FxPowerCap##FieldName##True) {  \
        pCaps->FieldName = TRUE;                                            \
    }                                                                       \
}


_Must_inspect_result_
NTSTATUS
GetStackCapabilities(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in MxDeviceObject* DeviceInStack,
    __out PDEVICE_CAPABILITIES Capabilities
    );

#endif //_PNPPRIV_H_    