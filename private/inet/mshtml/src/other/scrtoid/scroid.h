#ifndef SCRIPTOID_H
#define SCRIPTOID_H

// Forward decls.
interface IDispatch;
interface IScriptoidHandler;

// Interface scriptlet uses to get the object to inject into the scriptlet
// namespace. This is an optional interface. If it does not exist then
// no object will be bound to the name. 
#undef INTERFACE
#define INTERFACE IScriptoidHandler

// HACK: Temp hack to get the first script engine's type info. We need to do this right eventually...
//       Waiting for RokYu to fix.  ***TLL***
DEFINE_GUID(SID_HackTypeInfoForFirstEngine, 0xd7b654d0, 0x747b, 0x11d1, 0x8c, 0xac, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0);

DECLARE_INTERFACE_(IScriptoidHandler, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS
	/* IUnknown methods */
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

	/* IScriptoid methods */
	STDMETHOD(GetNameSpaceDispatch)(THIS_ IDispatch **ppdisp) PURE;
};


#endif // SCRIPTOID_H

