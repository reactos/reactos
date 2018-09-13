/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescriptparse.hxx"

#if DBG == 1
Class * _comimport<struct IActiveScriptParse,class ActiveScriptParse>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif

void ActiveScriptParseWrapper::initNew()
{
	checkhr(_wrapped->InitNew());
}

void ActiveScriptParseWrapper::close()
{
    _wrapped = null;
}

String * ActiveScriptParseWrapper::addScriptlet(
	String * pstrDefaultName, 
	String * pstrCode, 
	String * pstrItemName, 
	String * pstrSubItemName, 
	String * pstrEventName, 
	String * pstrDelimiter, 
	DWORD dwSourceContextCookie, 
	ULONG ulStartingLineNumber, 
	DWORD dwFlags)
{
	BSTR bstrDefaultName = pstrDefaultName ? pstrDefaultName->getBSTR() : null;
	BSTR bstrCode = pstrCode ? pstrCode->getBSTR() : null;
	BSTR bstrItemName = pstrItemName ? pstrItemName->getBSTR() : null;
	BSTR bstrSubItemName = pstrSubItemName ? pstrSubItemName->getBSTR() : null;
	BSTR bstrEventName = pstrEventName ? pstrEventName->getBSTR() : null;
	BSTR bstrDelimiter = pstrDelimiter ? pstrDelimiter->getBSTR() : null;
	BSTR bstrName;
  	EXCEPINFO excepinfo;
	HRESULT hr = _wrapped->AddScriptlet(
		bstrDefaultName, 
		bstrCode, 
		bstrItemName, 
		bstrSubItemName, 
		bstrEventName, 
		bstrDelimiter, 
		dwSourceContextCookie,
		ulStartingLineNumber,
		dwFlags,
		&bstrName,
		&excepinfo);
	SysFreeString(bstrDefaultName);
	SysFreeString(bstrCode);
	SysFreeString(bstrItemName);
	SysFreeString(bstrSubItemName);
	SysFreeString(bstrEventName);
	SysFreeString(bstrDelimiter);
	checkhr(hr);
    return String::newString(bstrName);
}

HRESULT ActiveScriptParseWrapper::parseScriptText(
	String * pstrCode, 
	String * pstrItemName, 
	IUnknown * punkContext, 
	String * pstrDelimiter, 
	DWORD dwSourceContextCookie, 
	ULONG ulStartingLineNumber, 
	DWORD dwFlags, 
	VARIANT * pvarResult) 
{
	BSTR bstrCode = pstrCode ? pstrCode->getBSTR() : null;
	BSTR bstrItemName = pstrItemName ? pstrItemName->getBSTR() : null;
	BSTR bstrDelimiter = pstrDelimiter ? pstrDelimiter->getBSTR() : null;
    EXCEPINFO excepinfo;
	HRESULT hr = _wrapped->ParseScriptText(
		bstrCode,
		bstrItemName,
		punkContext, 
		bstrDelimiter, 
		dwSourceContextCookie, 
		ulStartingLineNumber, 
		dwFlags, 
		pvarResult, 
		&excepinfo);
	SysFreeString(bstrCode);
	SysFreeString(bstrItemName);
	SysFreeString(bstrDelimiter);
	return hr;
}

