/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxIoTargetSelf.hpp

Abstract:

    Encapsulation of the Self target to which FxRequest are sent to.
    FxSelfTarget represents the client itself and is used to send IO to
    itself.

    Unlike the the local and remote targets, the IO sent to an Self IO
    target is routed to the sender's own top level queues. A dedicated queue
    may also be configured as the target for requests dispatched to the Self
    Io targets.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXIOTARGETSELF_H_
#define _FXIOTARGETSELF_H_

class FxIoTargetSelf : public FxIoTarget {

public:

    FxIoTargetSelf(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ USHORT ObjectSize
        );

    virtual
    _Must_inspect_result_
    MdDeviceObject
    GetTargetDeviceObject(
        _In_ CfxDeviceBase* Device
        )
    /*++
    Routine Description:
        Returns the target device object of the Device. In case of an Self
        Io Target it is the device itself.

    Arguments:

        Device - Handle to the Device Object

    Returns:

        MdDeviceObject for the Device.

    --*/
    {
        return Device->GetDeviceObject();
    }

    FxIoQueue*
    GetDispatchQueue(
        _In_ UCHAR MajorFunction
        );

    VOID
    SetDispatchQueue(
        _In_ FxIoQueue* DispatchQueue
        )
    /*++
    Routine Description:
        Sets a disapatch queue for the IO send to the Self IO Target.
    --*/
    {
        ASSERT(m_DispatchQueue == NULL);
        m_DispatchQueue =  DispatchQueue;
    }

    virtual
    VOID
    Send(
        _In_ MdIrp Irp
        );

protected:
    //
    // Hide destructor since we are reference counted object
    //
    ~FxIoTargetSelf();

private:

    //
    // A Queue configured to dispatch IO sent to the Self IO Target.
    //
    FxIoQueue* m_DispatchQueue;

};

#endif //_FXIOTARGETSELF_H_
