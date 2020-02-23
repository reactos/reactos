/*
Abstract:

    Encapsulation of the Self target to which FxRequest are sent to.
    FxSelfTarget represents the client itself and is used to send IO to
    itself.
 
    Unlike the the local and remote targets, the IO sent to an Self IO
    target is routed to the sender's own top level queues. A dedicated queue
    may also be configured as the target for requests dispatched to the Self
    Io targets. 
*/

#ifndef _FXIOTARGETSELF_H_
#define _FXIOTARGETSELF_H_

#include "common/fxiotarget.h"
#include "common/fxioqueue.h"

class FxIoTargetSelf : public FxIoTarget {

    //
    // A Queue configured to dispatch IO sent to the Self IO Target. 
    //
    FxIoQueue* m_DispatchQueue;

public:

    FxIoTargetSelf(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ USHORT ObjectSize
        );


protected:
    //
    // Hide destructor since we are reference counted object
    //
    ~FxIoTargetSelf();

};

#endif //_FXIOTARGETSELF_H_
