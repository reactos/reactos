/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectCallbacks.h

Abstract:

    This module implements the I/O package queue object callbacks

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXFILEOBJECTCALLBACKS_H
#define _FXFILEOBJECTCALLBACKS_H


//
// EvtDeviceFileCreate callback delegate
//
class FxFileObjectFileCreate : public FxLockedCallback {

public:
    PFN_WDF_DEVICE_FILE_CREATE Method;

    FxFileObjectFileCreate(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    VOID
    Invoke(
        __in WDFDEVICE Device,
        __in WDFREQUEST Request,
        __in_opt WDFFILEOBJECT FileObject
        )
    {

        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Device, Request, FileObject);
            CallbackEnd(irql);
        }
    }
};

//
// EvtFileCleanup callback delegate
//
class FxFileObjectFileCleanup : public FxLockedCallback {

public:
    PFN_WDF_FILE_CLEANUP Method;

    FxFileObjectFileCleanup(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in_opt WDFFILEOBJECT FileObject
        )
    {
        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(FileObject);
            CallbackEnd(irql);
        }
    }
};

//
// EvtFileClose callback delegate
//
class FxFileObjectFileClose : public FxLockedCallback {

public:
    PFN_WDF_FILE_CLOSE Method;

    FxFileObjectFileClose(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in_opt WDFFILEOBJECT FileObject
        )
    {
        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(FileObject);
            CallbackEnd(irql);
        }
    }
};

//
// EvtDeviceFileCreate callback delegate
//
class FxCxFileObjectFileCreate : public FxLockedCallback {

public:
    PFN_WDFCX_DEVICE_FILE_CREATE Method;

    FxCxFileObjectFileCreate(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    BOOLEAN
    Invoke(
        __in WDFDEVICE Device,
        __in WDFREQUEST Request,
        __in_opt WDFFILEOBJECT FileObject
        )
    {
        BOOLEAN claimed = FALSE;

        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            claimed = Method(Device, Request, FileObject);
            CallbackEnd(irql);
        }

        return claimed;
    }
};

//
// Collection of file-object callbacks.
//
struct FxFileObjectInfo : public FxStump {

    FxFileObjectInfo() :
        FileObjectClass(WdfFileObjectInvalid),
        AutoForwardCleanupClose(WdfUseDefault),
        ClassExtension(FALSE),
        CxDeviceInfo(NULL)
    {
        InitializeListHead(&ListEntry);
        RtlZeroMemory(&Attributes, sizeof(Attributes));
    }

    ~FxFileObjectInfo()
    {
        ASSERT(IsListEmpty(&ListEntry));
    }

    LIST_ENTRY                      ListEntry;

    FxFileObjectFileCreate          EvtFileCreate;
    FxCxFileObjectFileCreate        EvtCxFileCreate;
    FxFileObjectFileCleanup         EvtFileCleanup;
    FxFileObjectFileClose           EvtFileClose;

    WDF_FILEOBJECT_CLASS            FileObjectClass;
    WDF_OBJECT_ATTRIBUTES           Attributes;
    WDF_TRI_STATE                   AutoForwardCleanupClose;

    BOOLEAN                         ClassExtension;

    FxCxDeviceInfo*                 CxDeviceInfo;
};

#endif // _FXFILEOBJECTCALLBACKS_H

