/*
 * @(#)ScriptEngineInfo.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL ScriptEngineInfo object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescript.hxx"
#include "xtl/script/iactivescriptparse.hxx"

#ifndef __activscp_h__
#include "activscp.h"
#endif

#include "processor.hxx"
#include "scriptsite.hxx"
#include "scriptengine.hxx"

// BUGBUG - The script engine shouldn't be typed to the processor.  It should be attached to the templateaction.
// otherwise, the processor can't be reused by multiple template's.

DEFINE_CLASS_MEMBERS(ScriptEngine, _T("ScriptEngine"), Base);


ScriptEngine *
ScriptEngine::newScriptEngine(Processor * xtl, REFCLSID clsid)
{
    ScriptEngine * se = new ScriptEngine(xtl, clsid);
    return se;
}

#ifdef UNIX
#if DBG == 1
Class * _comimport<struct IActiveScript,class ActiveScript>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif

#if DBG == 1
Class * _comimport<struct IActiveScriptParse,class ActiveScriptParse>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif
#if DBG == 1
Class * _comimport<struct IActiveScriptError,class ActiveScriptError>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif
#endif // UNIX

ScriptEngine::ScriptEngine(Processor * xtl, REFCLSID clsid)
{
    IActiveScript * pias;
    IActiveScriptParse *piasp; 
    IObjectSafety *pos;
    HRESULT hr;

    checkhr(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
        IID_IUnknown, (void **)&_unk));

    checkhr(_unk->QueryInterface(IID_IActiveScript, (void **)&pias));
    _as= new ActiveScriptWrapper(pias);

    // Get the script engine to run only safe objects
    hr = pias->QueryInterface(IID_IObjectSafety, (LPVOID *)&pos);
    if (SUCCEEDED(hr))
    {
        DWORD dwSafetySupported, dwSafetyEnabled;
        
        // Get the interface safety otions
        checkhr(pos->GetInterfaceSafetyOptions(IID_IActiveScript, &dwSafetySupported, &dwSafetyEnabled));
        
        // Only allow objects which say they are safe for untrusted data, and 
        // say that we require the use of a security manager.  This gives us much 
        // more control
        dwSafetyEnabled |= INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                           INTERFACE_USES_DISPEX | INTERFACE_USES_SECURITY_MANAGER;
        checkhr(pos->SetInterfaceSafetyOptions(IID_IActiveScript, dwSafetySupported, dwSafetyEnabled));
        
        pos->Release();
    }
    
    pias->Release();
    
    checkhr(_unk->QueryInterface(IID_IActiveScriptParse, (void **)&piasp));
    _asp = new ActiveScriptParseWrapper(piasp);
    piasp->Release();
    
    _asp->initNew();
    _site = ScriptSite::newScriptSite(xtl);
    _as->setScriptSite(_site);
    _xtl = xtl;
}


void 
ScriptEngine::addScriptText(String * str)
{
    if (!_scriptText)
    {
        _scriptText = StringBuffer::newStringBuffer(DEFAULT_SCRIPTTEXT_SIZE);
    }
    _scriptText->append(str);
}


void
ScriptEngine::parseScriptText()
{
    VARIANT var;
    HRESULT hr;

    
	_as->addNamedItem(String::newString(_T("Context")), (SCRIPTITEM_GLOBALMEMBERS | 
				    SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISSOURCE));

    _as->setScriptState(SCRIPTSTATE_CONNECTED);

    if (_scriptText)
    {
        var.vt = VT_EMPTY;

        _xtl->markReadOnly();
	    hr = _asp->parseScriptText(
		    	_scriptText->toString(),
			    null, 
			    null, 
			    null, 
			    0, 
			    1,
			    SCRIPTTEXT_ISVISIBLE,
			    &var);

        VariantClear(&var);

        if (hr)
        {
            throwError(hr);
        }
    }
}


void 
ScriptEngine::parseScriptText(Processor * xtl, String * text, VARTYPE vt, VARIANT * pvar)
{
    HRESULT hr;

    if (text)
    {
        xtl->initRuntimeObject();
        xtl->markReadOnly();

        hr = _asp->parseScriptText(
		    text,  
		    null, 
		    null, 
		    null, 
		    0, 
		    1,
		    SCRIPTTEXT_ISEXPRESSION,
		    pvar);

        if (hr)
        {
            throwError(hr);
        }

        if (V_VT(pvar) != vt)
        {
            hr = VariantChangeTypeEx(pvar, pvar, GetThreadLocale(), VARIANT_NOVALUEPROP, vt);
            if (hr)
            {
                VariantClear(pvar);
                Exception::throwE(hr);
            }
        }        

    }
}


void
ScriptEngine::close()
{
    _as->close();
    _asp->close();
    _site = null;
    _unk = null;
    _as = null;
    _asp = null;
    _scriptText = null;
    _xtl = null;
}


void
ScriptEngine::finalize()
{
    Assert(!_as && "ScriptEngine not closed properly - must close before finalize");
    super::finalize();
}


void 
ScriptEngine::throwError(HRESULT hr)
{
    Exception::throwE(_site->getLastError(), hr); // , Exception::ComException, hr);
}

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
