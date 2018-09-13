/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XTL_SCRIPT_IACTIVESCRIPTPARSE
#define _XTL_SCRIPT_IACTIVESCRIPTPARSE

#ifndef __activscp_h__
#include "activscp.h"
#endif

#ifndef _VARIANT_HXX
#include "core/com/variant.hxx"
#endif


DEFINE_CLASS(ActiveScriptParse);

class NOVTABLE ActiveScriptParse : public Object
{
    public: virtual void initNew() = 0;

    public: virtual String * addScriptlet(String * pstrDefaultName, String * pstrCode, String * pstrItemName, String * pstrSubItemName, String * pstrEventName, String * pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags) = 0;

    public: virtual HRESULT parseScriptText(String * pstrCode, String * pstrItemName, IUnknown * punkContext, String * pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, VARIANT * pvarResult) = 0;

    public: virtual void close() = 0;

};

interface ActiveScriptParseWrapper : public _comimport<IActiveScriptParse, ActiveScriptParse>
{

    public: ActiveScriptParseWrapper(IActiveScriptParse * p) :
	    _comimport<IActiveScriptParse, ActiveScriptParse>(p)
	    {}

    public: virtual void initNew();

    public: virtual String * addScriptlet(String * pstrDefaultName, String * pstrCode, String * pstrItemName, String * pstrSubItemName, String * pstrEventName, String * pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags);

    public: virtual HRESULT parseScriptText(String * pstrCode, String * pstrItemName, IUnknown * punkContext, String * pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, VARIANT * pvarResult);

    public: virtual void close();
};


#endif _XTL_SCRIPT_IACTIVESCRIPTPARSE

