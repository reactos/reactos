#ifndef SCRPTLET_H
#define SCRPTLET_H


// If an error is reported successfully using IScriptletSite, then this
// error code is propagated up the call stack.
#define E_REPORTED	0x80004100L

DEFINE_GUID(CLSID_ScriptletConstructor, 0x21617250, 0xa071, 0x11d1, 0x89, 0xb6, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
DEFINE_GUID(SID_ScriptletSite, 0x22a98050, 0xa65d, 0x11d1, 0x89, 0xbe, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);


/***************************************************************************
	IScriptletConstructor
	
	We need to move these definitions into an idl so we can generate the
	appropriate proxy/stub code.
***************************************************************************/
DEFINE_GUID(IID_IScriptletConstructor, 0xc265fb00, 0x9fa4, 0x11d1, 0x89, 0xb6, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletConstructor : public IUnknown
	{
	STDMETHOD(Load)(LPCOLESTR pstrSource) PURE;
	STDMETHOD(Create)(LPCOLESTR pstrId, IUnknown *punkContext,
			IUnknown *punkOuter, REFIID riid, void **ppv) PURE;
	STDMETHOD(Register)(LPCOLESTR pstrSourceFileName) PURE;
	STDMETHOD(Unregister)(void) PURE;
	STDMETHOD(AddCoclassTypeInfo)(ICreateTypeLib *ptclib) PURE;
	STDMETHOD(IsDefined)(LPCOLESTR pstrId) PURE;
	};


/***************************************************************************
	IScriptletError
***************************************************************************/
DEFINE_GUID(IID_IScriptletError, 0xdf9f3d20, 0xa670, 0x11d1, 0x89, 0xbe, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletError : public IUnknown
    {
	STDMETHOD(GetExceptionInfo)(EXCEPINFO *pexcepinfo) PURE;
	STDMETHOD(GetSourcePosition)(ULONG *pline, ULONG *pcolumn) PURE;
	STDMETHOD(GetSourceLineText)(BSTR *pbstrSourceLine) PURE;
	};


/***************************************************************************
	IScriptletSite
	
	In addition to the dispids defined below, the site may also choose to 
	handle the follwing dispids.
		DISPID_ERROREVENT
		DISPID_AMBIENT_LOCALEID
***************************************************************************/
#define DISPID_SCRIPTLET_ALLOWDEBUG		1

DEFINE_GUID(IID_IScriptletSite, 0xc5f21c30, 0xa7df, 0x11d1, 0x89, 0xbe, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletSite : public IUnknown
	{
	STDMETHOD(OnEvent)(DISPID dispid, int cArg, VARIANT *prgvarArg,
			VARIANT *pvarRes) PURE;
	STDMETHOD(GetProperty)(DISPID dispid, VARIANT *pvarRes) PURE;
	};


		
#endif // SCRPTLET_H

