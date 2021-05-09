//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXCXDEVICEINFO_H_
#define _FXCXDEVICEINFO_H_

#include "fxdevicecallbacks.hpp"

struct FxCxDeviceInfo : public FxStump {
    FxCxDeviceInfo(PFX_DRIVER_GLOBALS FxDriverGlobals) :
        Driver(NULL),
        IoInCallerContextCallback(FxDriverGlobals),
        Index(0)
    {
        InitializeListHead(&ListEntry);
        RtlZeroMemory(&RequestAttributes, sizeof(RequestAttributes));
    }

    ~FxCxDeviceInfo()
    {
        ASSERT(IsListEmpty(&ListEntry));
    }

    LIST_ENTRY                  ListEntry;
    FxDriver*                   Driver;
    FxIoInCallerContext         IoInCallerContextCallback;
    WDF_OBJECT_ATTRIBUTES       RequestAttributes;
    CCHAR                       Index;
};

#endif // _FXCXDEVICEINFO_H_
