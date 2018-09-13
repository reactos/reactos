/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XTL_SCRIPT_IACTIVESCRIPT
#define _XTL_SCRIPT_IACTIVESCRIPT

#ifndef __activscp_h__
#include "activscp.h"
#endif


DEFINE_CLASS(ActiveScript);
class ActiveScriptSite;

class NOVTABLE ActiveScript : public Object
{
	public: virtual void setScriptSite(ActiveScriptSite * pass) = 0;

	public: virtual Object * getScriptSite(REFIID iid) = 0;

	public: virtual void setScriptState(SCRIPTSTATE ss) = 0;

	public: virtual SCRIPTSTATE getScriptState() = 0;

	public: virtual void close() = 0;

	public: virtual void addNamedItem(String * pstrName, DWORD dwFlags) = 0;

	public: virtual void addTypeLib(REFGUID rguidTypeLib, DWORD dwMajor, DWORD dwMinor, DWORD dwFlags) = 0;

	public: virtual Object * getScriptDispatch(String * pstrItemName) = 0;

	public: virtual SCRIPTTHREADID getCurrentScriptThreadID() = 0;

	public: virtual SCRIPTTHREADID getScriptThreadID(DWORD dwWin32ThreadId) = 0;

	public: virtual SCRIPTTHREADSTATE getScriptThreadState(SCRIPTTHREADID stidThread) = 0;

	public: virtual void interruptScriptThread(SCRIPTTHREADID stidThread, DWORD dwFlags) = 0;

	public: virtual Object * clone() = 0;

};

class ActiveScriptWrapper : public _comimport<IActiveScript, ActiveScript>
{
	public: ActiveScriptWrapper(IActiveScript * p) :
		_comimport<IActiveScript, ActiveScript>(p)
		{}

	public: virtual void setScriptSite(ActiveScriptSite * pass);

	public: virtual Object * getScriptSite(REFIID iid);

	public: virtual void setScriptState(SCRIPTSTATE ss);

	public: virtual SCRIPTSTATE getScriptState();

	public: virtual void close();

	public: virtual void addNamedItem(String * pstrName, DWORD dwFlags);

	public: virtual void addTypeLib(REFGUID rguidTypeLib, DWORD dwMajor, DWORD dwMinor, DWORD dwFlags);

	public: virtual Object * getScriptDispatch(String * pstrItemName);

	public: virtual SCRIPTTHREADID getCurrentScriptThreadID();

	public: virtual SCRIPTTHREADID getScriptThreadID(DWORD dwWin32ThreadId);

	public: virtual SCRIPTTHREADSTATE getScriptThreadState(SCRIPTTHREADID stidThread);

	public: virtual void interruptScriptThread(SCRIPTTHREADID stidThread, DWORD dwFlags);

	public: virtual Object * clone();

};


#endif _XTL_SCRIPT_IACTIVESCRIPT

