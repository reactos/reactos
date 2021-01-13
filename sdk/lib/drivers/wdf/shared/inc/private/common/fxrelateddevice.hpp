/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRelatedDevice.hpp

Abstract:

    This module defines the "related device" class.  These objects are used
    to handle device relations queries.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXRELATEDDEVICE_H_
#define _FXRELATEDDEVICE_H_

enum FxRelatedDeviceState {
    RelatedDeviceStateUnspecified = 0,
    RelatedDeviceStateNeedsReportPresent,
    RelatedDeviceStateReportedPresent,
    RelatedDeviceStateNeedsReportMissing,
};

class FxRelatedDevice : public FxObject {
    friend FxRelatedDeviceList;

protected:
    FxTransactionedEntry m_TransactionedEntry;

    MdDeviceObject m_DeviceObject;

public:
    FxRelatedDeviceState m_State;

public:
    FxRelatedDevice(
        __in MdDeviceObject DeviceObject,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxRelatedDevice(
        VOID
        );

    MdDeviceObject
    GetDevice(
        VOID
        )
    {
        return m_DeviceObject;
    }

    DECLARE_INTERNAL_NEW_OPERATOR();

#ifdef INLINE_WRAPPER_ALLOCATION
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    FORCEINLINE
    PVOID
    GetCOMWrapper(
        )
    {
        PBYTE ptr = (PBYTE) this;
        return (ptr + (USHORT) WDF_ALIGN_SIZE_UP(sizeof(*this), MEMORY_ALLOCATION_ALIGNMENT));
    }
#endif
#endif
};

#endif // _FXRELATEDDEVICE_H_
