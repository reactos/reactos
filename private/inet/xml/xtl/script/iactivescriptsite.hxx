/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XTL_SCRIPT_IACTIVESCRIPTSITE
#define _XTL_SCRIPT_IACTIVESCRIPTSITE

#ifndef __activscp_h__
#include "activscp.h"
#endif

#ifndef _XTL_SCRIPT_IACTIVESCRIPTERROR
#include "xtl/script/iactivescripterror.hxx"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif


class NOVTABLE ActiveScriptSite : public Object
{
  public: virtual LCID getLCID() = 0;

  public: virtual void getItemInfo(String * pstrName, DWORD dwReturnMask, IUnknown ** ppiunkItem, ITypeInfo ** ppti) = 0;

  public: virtual String * getDocVersionString() = 0;

  public: virtual void onScriptTerminate(Variant * pvarResult, const EXCEPINFO * pexcepinfo) = 0;

  public: virtual void onStateChange(SCRIPTSTATE ssScriptState) = 0;

  public: virtual void onScriptError(ActiveScriptError * pscripterror) = 0;

  public: virtual void onEnterScript() = 0;

  public: virtual void onLeaveScript() = 0;
};

class IActiveScriptSiteWrapper : public _comexport<ActiveScriptSite, IActiveScriptSite, &IID_IActiveScriptSite>
{
	public: IActiveScriptSiteWrapper(ActiveScriptSite * p)
				: _comexport<ActiveScriptSite, IActiveScriptSite, &IID_IActiveScriptSite>(p)
			{}
    public: virtual HRESULT STDMETHODCALLTYPE GetLCID( 
        /* [retval][out] */ LCID __RPC_FAR *plcid);
    
    public: virtual HRESULT STDMETHODCALLTYPE GetItemInfo( 
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti);
    
    public: virtual HRESULT STDMETHODCALLTYPE GetDocVersionString( 
        /* [retval][out] */ BSTR __RPC_FAR *pbstrVersion);
    
    public: virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate( 
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo);
    
    public: virtual HRESULT STDMETHODCALLTYPE OnStateChange( 
        /* [in] */ SCRIPTSTATE ssScriptState);
    
    public: virtual HRESULT STDMETHODCALLTYPE OnScriptError( 
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror);
    
    public: virtual HRESULT STDMETHODCALLTYPE OnEnterScript( void);
    
    public: virtual HRESULT STDMETHODCALLTYPE OnLeaveScript( void);
};


#endif _XTL_SCRIPT_IACTIVESCRIPTSITE

