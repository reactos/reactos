/* This file will be SDK-style public -- handler implementors will be
   expected to include it to get to scriptoid stuff */

#ifndef __HANDIMPL_HXX_INCLUDED__
#define __HANDIMPL_HXX_INCLUDED__

#define DEFAULT_SCRIPTOID_SCOPE     _T("scriptoid")

extern "C" const GUID IID_IScriptletHandlerConstructor;
extern "C" const GUID IID_IScriptletHandler;
extern "C" const GUID IID_IGenericEventSource;

class IGenericEventSource : public IUnknown
{
public:
    virtual STDMETHODIMP FireEvent( DISPID id, DISPPARAMS *pdp ) = 0;
};

#endif
