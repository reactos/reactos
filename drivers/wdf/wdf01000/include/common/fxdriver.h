#ifndef _FXDRIVER_H_
#define _FXDRIVER_H_

#include "common/fxnonpagedobject.h"
#include "common/mxgeneral.h"

//
// The following are support classes for FxDriver
//
class FxDriver : public FxNonPagedObject {

public:

    virtual
    VOID
    DeleteObject(
        VOID
        )
    {
        //
        // If diposed at > PASSIVE, we will cause a deadlock in FxDriver::Dispose
        // when we call into  the dispose list to wait for empty when we are in
        // the context of the dispose list's work item.
        //
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        __super::DeleteObject();
    }
    
};

#endif //_FXDRIVER_H_