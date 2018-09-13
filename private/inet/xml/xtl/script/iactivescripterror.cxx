/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xtl/script/iactivescripterror.hxx"

#ifdef UNIX
#if DBG == 1
Class * _comimport<struct IActiveScriptError,class ActiveScriptError>::getClass(void) const
{
    Assert(FALSE && "Shouldn't be called"); 
    return null; 
}
#endif
#endif // UNIX

void ActiveScriptErrorWrapper::getExceptionInfo(EXCEPINFO * excepinfo)
{
	checkhr(_wrapped->GetExceptionInfo(excepinfo));
}

void ActiveScriptErrorWrapper::getSourcePosition(DWORD * pdwSourceContext, ULONG * pulLineNumber, LONG * plCharacterPosition)
{
	checkhr(_wrapped->GetSourcePosition(pdwSourceContext, pulLineNumber, plCharacterPosition));
}

String * ActiveScriptErrorWrapper::getSourceLineText()
{
	BSTR bstr;
    // BUGBUG - this BSTR leaks!
	checkhr(_wrapped->GetSourceLineText(&bstr));
    return String::newString(bstr);
}

