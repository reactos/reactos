#ifndef _PNPPRIV_H_
#define _PNPPRIV_H_

#include "fxglobals.h"
#include "primitives/mxdeviceobject.h"


__inline
VOID
FxSetPnpDeviceStateBit(
    __in PNP_DEVICE_STATE* PnpDeviceState,
    __in LONG ExternalState,
    __in LONG InternalState,
    __in LONG BitMask,
    __in LONG TrueValue
    )
{
    LONG bits;

    bits = InternalState & BitMask;

    if (bits == TrueValue)
    {
        *PnpDeviceState |= ExternalState;
    }
    else if (bits == 0)
    {  // 0 is the always false for every bit-set
        *PnpDeviceState &= ~ExternalState;
    }
}

#define SET_PNP_DEVICE_STATE_BIT(State, ExternalState, value, Name) \
    FxSetPnpDeviceStateBit(State,                                   \
                           ExternalState,                           \
                           state,                                   \
                           FxPnpState##Name##Mask,                  \
                           FxPnpState##Name##True)

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