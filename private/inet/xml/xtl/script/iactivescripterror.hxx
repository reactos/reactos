/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XTL_SCRIPT_IACTIVESCRIPTERROR
#define _XTL_SCRIPT_IACTIVESCRIPTERROR

#ifndef __activscp_h__
#include "activscp.h"
#endif


class NOVTABLE ActiveScriptError : public Object
{
	public: virtual void getExceptionInfo(EXCEPINFO * excepinfo) = 0;

	public: virtual void getSourcePosition(DWORD * pdwSourceContext, ULONG * pulLineNumber, LONG * plCharacterPosition) = 0;

	public: virtual String * getSourceLineText() = 0;
};

class ActiveScriptErrorWrapper : public _comimport<IActiveScriptError, ActiveScriptError>
{
	public: ActiveScriptErrorWrapper(IActiveScriptError * p) :
		_comimport<IActiveScriptError, ActiveScriptError>(p)
		{}

	public: virtual void getExceptionInfo(EXCEPINFO * excepinfo);

	public: virtual void getSourcePosition(DWORD * pdwSourceContext, ULONG * pulLineNumber, LONG * plCharacterPosition);

	public: virtual String * getSourceLineText();
};


#endif _XTL_SCRIPT_IACTIVESCRIPTERROR

