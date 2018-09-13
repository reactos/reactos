/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescriptsite.hxx"

#if DBG == 1
Class * _comimport<struct IActiveScriptError,class ActiveScriptError>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::GetLCID( 
    /* [retval][out] */ LCID __RPC_FAR *plcid)
{
    HRESULT hr;
    if (!plcid)
        return E_INVALIDARG;

    STACK_ENTRY_WRAPPED;

    TRY
    {
        *plcid = getWrapped()->getLCID();
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr; 
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::GetItemInfo( 
    /* [string][in] */ LPCOLESTR pstrName,
    /* [in] */ DWORD dwReturnMask,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkItem,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti)
{
    HRESULT hr;

    STACK_ENTRY_WRAPPED;

    TRY
    {
        Object * item = null;
        Object * info = null;
        String * s = String::newString((WCHAR *)pstrName);
        getWrapped()->getItemInfo(s, dwReturnMask, ppunkItem, ppti);
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::GetDocVersionString( 
    /* [retval][out] */ BSTR __RPC_FAR *pbstrVersion)
{
    HRESULT hr;
    if (!pbstrVersion)
        return E_INVALIDARG;

    STACK_ENTRY_WRAPPED;

    TRY
    {
        String * s = getWrapped()->getDocVersionString();
        *pbstrVersion = s->getBSTR();
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::OnScriptTerminate( 
    /* [in] */ const VARIANT __RPC_FAR *pvarResult,
    /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)
{
    HRESULT hr;
    STACK_ENTRY_WRAPPED;

    TRY
    {
        // BUGBUG fix variant
        getWrapped()->onScriptTerminate((Variant *)pvarResult, pexcepinfo);
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::OnStateChange( 
    /* [in] */ SCRIPTSTATE ssScriptState)
{
    HRESULT hr;
    STACK_ENTRY_WRAPPED;

    TRY
    {
        getWrapped()->onStateChange(ssScriptState);
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::OnScriptError( 
    /* [in] */ IActiveScriptError __RPC_FAR *pscripterror)
{
    HRESULT hr;
    STACK_ENTRY_WRAPPED;

    TRY
    {
        getWrapped()->onScriptError(new ActiveScriptErrorWrapper(pscripterror));
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::OnEnterScript( void)
{
    HRESULT hr;
    STACK_ENTRY_WRAPPED;

    TRY
    {
        getWrapped()->onEnterScript();
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}

HRESULT STDMETHODCALLTYPE IActiveScriptSiteWrapper::OnLeaveScript( void)
{        
    HRESULT hr;
    STACK_ENTRY_WRAPPED;

    TRY
    {
        getWrapped()->onLeaveScript();
        hr = S_OK;
    }
    CATCH
    {
        hr = GETEXCEPTION()->getHRESULT();
    }
    ENDTRY
    return hr;
}


