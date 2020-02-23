#ifndef _FXREQUESTCALLBACKS_H_
#define _FXREQUESTCALLBACKS_H_

#include "common/fxcallback.h"

//
// Delegate which contains EvtRequestCompletion 
//
class FxRequestCompletionCallback : public FxCallback {

public:
    PFN_WDF_REQUEST_COMPLETION_ROUTINE m_Completion;

    FxRequestCompletionCallback(
        VOID
        )
    {
        m_Completion = NULL;
    }
};

#endif // _FXREQUESTCALLBACKS_H_
