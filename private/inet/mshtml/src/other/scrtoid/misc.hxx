#ifndef __MISC_HXX__
#define __MISC_HXX__

//+------------------------------------------------------------------------
//
//  automation helpers
//
//-------------------------------------------------------------------------

HRESULT GetProperty_Dispatch(IDispatch * pDisp, LPTSTR pchName, IDispatch ** ppDispRes);
HRESULT PutProperty_Dispatch(IDispatch * pDisp, LPTSTR pchName, IDispatch * pDispArg);

HRESULT GetDocument(IDispatch * pElement, IDispatch   ** ppDispDocument);
HRESULT GetDocument(IDispatch * pElement, IDispatchEx ** ppDispDocument);
HRESULT GetWindow  (IDispatch * pElement, IDispatch   ** ppDispWindow);
HRESULT GetWindow  (IDispatch * pElement, IDispatchEx ** ppDispWindow);

#endif  // __MISC_HXX__
