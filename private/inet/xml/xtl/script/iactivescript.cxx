/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescript.hxx"
#include "xtl/script/iactivescriptsite.hxx"

#if DBG == 1
Class * _comimport<struct IActiveScript,class ActiveScript>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif

void ActiveScriptWrapper::setScriptSite(ActiveScriptSite * pass)
{
	IActiveScriptSiteWrapper * p = new IActiveScriptSiteWrapper(pass);
	HRESULT hr = _wrapped->SetScriptSite(p);
	p->Release();
	checkhr(hr);
}

Object * ActiveScriptWrapper::getScriptSite(REFIID iid)
{
	IUnknown * p;
	checkhr(_wrapped->GetScriptSite(iid, (void **) &p));
	// BUGBUG ignoring iid...
	Object * o;
	checkhr(p->QueryInterface(IID_Object, (void **)&o));
	return o;
}

void ActiveScriptWrapper::setScriptState(SCRIPTSTATE ss)
{
	checkhr(_wrapped->SetScriptState(ss));
}

SCRIPTSTATE ActiveScriptWrapper::getScriptState()
{
	SCRIPTSTATE ss;
	checkhr(_wrapped->GetScriptState(&ss));
	return ss;
}

void ActiveScriptWrapper::close()
{
    if (_wrapped)
    {
        HRESULT hr = _wrapped->Close();
        _wrapped = null;
        checkhr(hr);
    }
}

void ActiveScriptWrapper::addNamedItem(String * pstrName, DWORD dwFlags)
{
	// BUGBUG remove this extra allocation when switched to Unicode...
	BSTR bstr = pstrName->getBSTR();
	HRESULT hr = _wrapped->AddNamedItem(bstr, dwFlags);
	SysFreeString(bstr);
	checkhr(hr);
}

void ActiveScriptWrapper::addTypeLib(REFGUID rguidTypeLib, DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
	checkhr(_wrapped->AddTypeLib(rguidTypeLib, dwMajor, dwMinor, dwFlags));
}

Object * ActiveScriptWrapper::getScriptDispatch(String * pstrItemName)
{
    Exception::throwE(E_NOTIMPL);
	return null;
}

SCRIPTTHREADID ActiveScriptWrapper::getCurrentScriptThreadID()
{
	SCRIPTTHREADID s;
	checkhr(_wrapped->GetCurrentScriptThreadID(&s));
	return s;
}

SCRIPTTHREADID ActiveScriptWrapper::getScriptThreadID(DWORD dwWin32ThreadId)
{
	SCRIPTTHREADID s;
	checkhr(_wrapped->GetScriptThreadID(dwWin32ThreadId, &s));
	return s;
}

SCRIPTTHREADSTATE ActiveScriptWrapper::getScriptThreadState(SCRIPTTHREADID stidThread)
{
	SCRIPTTHREADSTATE s;
	checkhr(_wrapped->GetScriptThreadState(stidThread, &s));
	return s;
}

void ActiveScriptWrapper::interruptScriptThread(SCRIPTTHREADID stidThread, DWORD dwFlags)
{
    EXCEPINFO excepinfo;
	checkhr(_wrapped->InterruptScriptThread(stidThread, &excepinfo, dwFlags));
}

Object * ActiveScriptWrapper::clone()
{
	IActiveScript * p;
	checkhr(_wrapped->Clone(&p));
	return (Base *)new ActiveScriptWrapper(p);
}
