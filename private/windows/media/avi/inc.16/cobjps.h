// cobjps.h - definitions for writing standard proxies and stubs

#if !defined( _COBJPS_H_ )
#define _COBJPS_H_


/****** IRpcChannel Interface ***********************************************/

interface IRpcChannel : IUnknown 
{
    STDMETHOD(GetStream)(REFIID iid, int iMethod, BOOL fSend,
                     BOOL fNoWait, DWORD size, IStream FAR* FAR* ppIStream) = 0;
    STDMETHOD(Call)(IStream FAR* pIStream) = 0;
    STDMETHOD(GetDestCtx)(DWORD FAR* lpdwDestCtx, LPVOID FAR* lplpvDestCtx) = 0;
    STDMETHOD(IsConnected)(void) = 0;
};


/****** IRpcProxy Interface *************************************************/

// IRpcProxy is an interface implemented by proxy objects.  A proxy object has
// exactly the same interfaces as the real object in addition to IRpcProxy.
//

interface IRpcProxy : IUnknown 
{
    STDMETHOD(Connect)(IRpcChannel FAR* pRpcChannel) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
};


/****** IRpcStub Interface **************************************************/

// IRpcStub is an interface implemented by stub objects.  
//

interface IRpcStub : IUnknown
{
    STDMETHOD(Connect)(IUnknown FAR* pUnk) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
    STDMETHOD(Invoke)(REFIID iid, int iMethod, IStream FAR* pIStream,
            DWORD dwDestCtx, LPVOID lpvDestCtx) = 0;
    STDMETHOD_(BOOL, IsIIDSupported)(REFIID iid) = 0;
    STDMETHOD_(ULONG, CountRefs)(void) = 0;
};


/****** IPSFactory Interface ************************************************/

// IPSFactory - creates proxies and stubs
//

interface IPSFactory : IUnknown
{
    STDMETHOD(CreateProxy)(IUnknown FAR* pUnkOuter, REFIID riid, 
        IRpcProxy FAR* FAR* ppProxy, void FAR* FAR* ppv) = 0;
    STDMETHOD(CreateStub)(REFIID riid, IUnknown FAR* pUnkServer,
        IRpcStub FAR* FAR* ppStub) = 0;
};

#endif // _COBJPS_H_
