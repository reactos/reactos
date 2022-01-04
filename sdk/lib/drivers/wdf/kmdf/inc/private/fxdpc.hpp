/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDpc.hpp

Abstract:

    This module implements a frameworks managed DPC that
    can synchrononize with driver frameworks object locks.

Author:



Environment:

    kernel mode only

Revision History:


--*/

#ifndef _FXDPC_H_
#define _FXDPC_H_

//
// Driver Frameworks DPC Design:
//
// The driver frameworks provides an optional DPC wrapper object that allows
// the creation of a reference counted DPC object that can synchronize
// automatically with certain frameworks objects.
//
// This provides automatic synchronization between the DPC's execution, and the
// frameworks objects' event callbacks into the device driver.
//
// The WDFDPC object is designed to be re-useable, in which it can be re-linked
// into the DPC queue after firing.
//
// In many cases, the KDPC struct is embedded inside another structure that
// represents a device command block. These device command blocks are typically
// submitted to another device driver. So the calling driver, which is utilizing
// the driver frameworks would not likely have an opportunity to make
// changes to this. In order to support this, the caller can optionally supply
// a DPC object pointer to Initialize, and the WDFDPC object will use this
// embedded user supplied DPC object, and pass its address as the RawDpc
// parameter to the callback function.
//
// If the user does not supply a DPC pointer by passing NULL, then the
// internal DPC object is used, and RawDPC is NULL.
//
// Calling GetDpcPtr returns the DPC to be used, and could be
// the caller supplied DPC, or the embedded one depending on
// whether the caller supplied a user DPC pointer to Initialize.
//
// The GetDpcPtr allows linking of the WDFDPC object into various DPC
// lists by the driver.
//

class FxDpc : public FxNonPagedObject {

private:

    KDPC               m_Dpc;

    //
    // This is the Framework object who is associated with the DPC
    // if supplied
    //
    FxObject*          m_Object;

    //
    // This is the callback lock for the object this DPC will
    // synchronize with
    //
    FxCallbackLock*    m_CallbackLock;

    //
    // This is the object whose reference count actually controls
    // the lifetime of the m_CallbackLock
    //
    FxObject*          m_CallbackLockObject;

    //
    // This is the user supplied callback function
    //
    PFN_WDF_DPC        m_Callback;

    // Ensures only one of either Delete or Cleanup runs down the object
    BOOLEAN            m_RunningDown;

public:
    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_DPC_CONFIG Config,
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in FxObject* ParentObject,
        __out WDFDPC* Dpc
        );

    FxDpc(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxDpc(
        VOID
        );

    KDPC*
    GetDpcPtr(
        VOID
        )
    {
         return &m_Dpc;
    }

    WDFOBJECT
    GetObject(
        VOID
        )
    {
        if (m_Object != NULL) {
            return m_Object->GetObjectHandle();
        }
        else {
            return NULL;
        }
    }

/*++

Routine Description:

    Initialize the DPC using either the caller supplied DPC
    struct, or if NULL, our own internal one.

Arguments:

Returns:

    NTSTATUS

--*/
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in PWDF_DPC_CONFIG Config,
        __in FxObject* ParentObject,
        __out WDFDPC* Dpc
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    BOOLEAN
    Cancel(
        __in BOOLEAN Wait
        );

    VOID
    DpcHandler(
        __in PKDPC Dpc,
        __in PVOID SystemArgument1,
        __in PVOID SystemArgument2
        );

private:

    //
    // Called from Dispose, or cleanup list to perform final flushing of any
    // outstanding DPC's and dereferencing of objects.
    //
    VOID
    FlushAndRundown(
        );

    static
    KDEFERRED_ROUTINE
    FxDpcThunk;

    static
    VOID
    WorkItemThunk(
        PDEVICE_OBJECT DeviceObject,
        PVOID          Context
        );
};

#endif // _FXDPC_H_

