/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCxDeviceInit.hpp

Abstract:


Author:


Environment:

    kernel mode only

Revision History:

--*/

#ifndef __FXCXDEVICEINIT_HPP__
#define __FXCXDEVICEINIT_HPP__

//
// Holds class extension file object configuration.
//
struct CxFileObjectInit {
    WDF_FILEOBJECT_CLASS Class;

    WDF_OBJECT_ATTRIBUTES Attributes;

    WDFCX_FILEOBJECT_CONFIG Callbacks;

    WDF_TRI_STATE AutoForwardCleanupClose;

    BOOLEAN Set;
};

//
// The typedef for a pointer to this structure is exposed in wdfdevice.h
//
struct WDFCXDEVICE_INIT : public FxStump {
public:
    WDFCXDEVICE_INIT();
    ~WDFCXDEVICE_INIT();

    static
    _Must_inspect_result_
    PWDFCXDEVICE_INIT
    _AllocateCxDeviceInit(
        __in PWDFDEVICE_INIT DeviceInit
        );

public:
    //
    // Class extension init list entry.
    //
    LIST_ENTRY              ListEntry;

    //
    // Client and Cx's globals.
    //
    PFX_DRIVER_GLOBALS      ClientDriverGlobals;
    PFX_DRIVER_GLOBALS      CxDriverGlobals;

    //
    // Pre-proc info.
    //
    FxIrpPreprocessInfo*    PreprocessInfo;

    //
    // In caller context info.
    //
    PFN_WDF_IO_IN_CALLER_CONTEXT IoInCallerContextCallback;

    //
    // Request attributes info.
    //
    WDF_OBJECT_ATTRIBUTES   RequestAttributes;

    //
    // File object info.
    //
    CxFileObjectInit        FileObject;

    //
    // Set during the device create.
    //
    FxCxDeviceInfo*         CxDeviceInfo;
};

#endif // __FXCXDEVICEINIT_HPP__

