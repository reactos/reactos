/*
 * Defines undocumented Microsoft COM interfaces and APIs seemingly related to some 'channel' notion.
 */

#ifndef __WINE_WINE_OBJ_CHANNEL_H
#define __WINE_WINE_OBJ_CHANNEL_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID   (IID_IChannelHook,	0x1008c4a0L, 0x7613, 0x11cf, 0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4);
typedef struct IChannelHook IChannelHook,*LPCHANNELHOOK;

DEFINE_GUID   (IID_IPSFactoryBuffer,	0xd5f569d0L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IPSFactoryBuffer IPSFactoryBuffer,*LPPSFACTORYBUFFER;

DEFINE_GUID   (IID_IRpcChannelBuffer,	0xd5f56b60L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcChannelBuffer IRpcChannelBuffer,*LPRPCCHANNELBUFFER;

DEFINE_GUID   (IID_IRpcProxyBuffer,	0xd5f56a34L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcProxyBuffer IRpcProxyBuffer,*LPRPCPROXYBUFFER;

DEFINE_GUID   (IID_IRpcStubBuffer,	0xd5f56afcL, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcStubBuffer IRpcStubBuffer,*LPRPCSTUBBUFFER;


/*****************************************************************************
 * IChannelHook interface
 */
#define ICOM_INTERFACE IChannelHook
#define IChannelHook_METHODS \
    ICOM_VMETHOD3(ClientGetSize,    REFGUID,uExtent, REFIID,riid, ULONG*,pDataSize) \
    ICOM_VMETHOD4(ClientFillBuffer, REFGUID,uExtent, REFIID,riid, ULONG*,pDataSize, void*,pDataBuffer) \
    ICOM_VMETHOD6(ClientNotify,     REFGUID,uExtent, REFIID,riid, ULONG,cbDataSize, void*,pDataBuffer, DWORD,lDataRep, HRESULT,hrFault) \
    ICOM_VMETHOD5(ServerNotify,     REFGUID,uExtent, REFIID,riid, ULONG,cbDataSize, void*,pDataBuffer, DWORD,lDataRep) \
    ICOM_VMETHOD4(ServerGetSize,    REFGUID,uExtent, REFIID,riid, HRESULT,hrFault, ULONG*,pDataSize) \
    ICOM_VMETHOD5(ServerFillBuffer, REFGUID,uExtent, REFIID,riid, ULONG*,pDataSize, void*,pDataBuffer, HRESULT,hrFault)
#define IChannelHook_IMETHODS \
    IUnknown_IMETHODS \
    IChannelHook_METHODS
ICOM_DEFINE(IChannelHook,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IChannelHook_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IChannelHook_AddRef(p)             ICOM_CALL (AddRef,p)
#define IChannelHook_Release(p)            ICOM_CALL (Release,p)
/*** IChannelHook methods ***/
#define IChannelHook_ClientGetSize(p,a,b,c)        ICOM_CALL(ClientGetSize,p,a,b,c)
#define IChannelHook_ClientFillBuffer(p,a,b,c,d)   ICOM_CALL(ClientFillBuffer,p,a,b,c,d)
#define IChannelHook_ClientNotify(p,a,b,c,d,e,f)   ICOM_CALL(ClientNotify,p,a,b,c,d,e,f)
#define IChannelHook_ServerNotify(p,a,b,c,d,e)     ICOM_CALL(ServerNotify,p,a,b,c,d,e)
#define IChannelHook_ServerGetSize(p,a,b,c,d)      ICOM_CALL(ServerGetSize,p,a,b,c,d)
#define IChannelHook_ServerFillBuffer(p,a,b,c,d,e) ICOM_CALL(ServerFillBuffer,p,a,b,c,d,e)


/*****************************************************************************
 * IPSFactoryBuffer interface
 */
#define ICOM_INTERFACE IPSFactoryBuffer
#define IPSFactoryBuffer_METHODS \
    ICOM_METHOD4(HRESULT,CreateProxy, IUnknown*,pUnkOuter, REFIID,riid, IRpcProxyBuffer**,ppProxy, void**,ppv) \
    ICOM_METHOD3(HRESULT,CreateStub,  REFIID,riid, IUnknown*,pUnkServer, IRpcStubBuffer**,ppStub)
#define IPSFactoryBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IPSFactoryBuffer_METHODS
ICOM_DEFINE(IPSFactoryBuffer,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPSFactoryBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPSFactoryBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPSFactoryBuffer_Release(p)            ICOM_CALL (Release,p)
/*** IPSFactoryBuffer methods ***/
#define IPSFactoryBuffer_CreateProxy(p,a,b,c,d) ICOM_CALL4(CreateProxy,p,a,b,c,d)
#define IPSFactoryBuffer_CreateStub(p,a,b,c)    ICOM_CALL3(CreateStub,p,a,b,c)


/*****************************************************************************
 * IRpcChannelBuffer interface
 */
typedef unsigned long RPCOLEDATAREP;

typedef struct tagRPCOLEMESSAGE
{
    void* reserved1;
    RPCOLEDATAREP dataRepresentation;
    void* Buffer;
    ULONG cbBuffer;
    ULONG iMethod;
    void* reserved2[5];
    ULONG rpcFlags;
} RPCOLEMESSAGE, *PRPCOLEMESSAGE;

#define ICOM_INTERFACE IRpcChannelBuffer
#define IRpcChannelBuffer_METHODS \
    ICOM_METHOD2(HRESULT,GetBuffer,   RPCOLEMESSAGE*,pMessage, REFIID,riid) \
    ICOM_METHOD2(HRESULT,SendReceive, RPCOLEMESSAGE*,pMessage, ULONG*,pStatus) \
    ICOM_METHOD1(HRESULT,FreeBuffer,  RPCOLEMESSAGE*,pMessage) \
    ICOM_METHOD2(HRESULT,GetDestCtx,  DWORD*,pdwDestContext, void**,ppvDestContext) \
    ICOM_METHOD (HRESULT,IsConnected)
#define IRpcChannelBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IRpcChannelBuffer_METHODS
ICOM_DEFINE(IRpcChannelBuffer,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRpcChannelBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRpcChannelBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRpcChannelBuffer_Release(p)            ICOM_CALL (Release,p)
/*** IRpcChannelBuffer methods ***/
#define IRpcChannelBuffer_GetBuffer(p,a,b)   ICOM_CALL2(GetBuffer,p,a,b)
#define IRpcChannelBuffer_SendReceive(p,a,b) ICOM_CALL2(SendReceive,p,a,b)
#define IRpcChannelBuffer_FreeBuffer(p,a)    ICOM_CALL1(FreeBuffer,p,a)
#define IRpcChannelBuffer_GetDestCtx(p,a,b)  ICOM_CALL2(GetDestCtx,p,a,b)
#define IRpcChannelBuffer_IsConnected(p)     ICOM_CALL (IsConnected,p)


/*****************************************************************************
 * IRpcProxyBuffer interface
 */
#define ICOM_INTERFACE IRpcProxyBuffer
#define IRpcProxyBuffer_METHODS \
    ICOM_METHOD1(HRESULT,Connect,   IRpcChannelBuffer*,pRpcChannelBuffer) \
    ICOM_VMETHOD(        Disconnect)
#define IRpcProxyBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IRpcProxyBuffer_METHODS
ICOM_DEFINE(IRpcProxyBuffer,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRpcProxyBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRpcProxyBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRpcProxyBuffer_Release(p)            ICOM_CALL (Release,p)
/*** IRpcProxyBuffer methods ***/
#define IRpcProxyBuffer_Connect(p,a)  ICOM_CALL2(Connect,p,a)
#define IRpcProxyBuffer_Disconnect(p) ICOM_CALL (Disconnect,p)


/*****************************************************************************
 * IRpcStubBuffer interface
 */
#define ICOM_INTERFACE IRpcStubBuffer
#define IRpcStubBuffer_METHODS \
    ICOM_METHOD1 (HRESULT,        Connect,                   IUnknown*,pUnkServer) \
    ICOM_VMETHOD (                Disconnect) \
    ICOM_METHOD2 (HRESULT,        Invoke,                    RPCOLEMESSAGE*,_prpcmsg, IRpcChannelBuffer*,_pRpcChannelBuffer) \
    ICOM_METHOD1 (LPRPCCHANNELBUFFER,IsIIDSupported,            REFIID,riid) \
    ICOM_METHOD  (ULONG,          CountRefs) \
    ICOM_METHOD1 (HRESULT,        DebugServerQueryInterface, void**,ppv) \
    ICOM_VMETHOD1(                DebugServerRelease,        void*,pv)
#define IRpcStubBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IRpcStubBuffer_METHODS
ICOM_DEFINE(IRpcStubBuffer,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRpcStubBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRpcStubBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRpcStubBuffer_Release(p)            ICOM_CALL (Release,p)
/*** IRpcStubBuffer methods ***/
#define IRpcStubBuffer_Connect(p,a)                   ICOM_CALL1(Connect,p,a)
#define IRpcStubBuffer_Disconnect(p)                  ICOM_CALL (Disconnect,p)
#define IRpcStubBuffer_Invoke(p,a,b)                  ICOM_CALL2(Invoke,p,a,b)
#define IRpcStubBuffer_IsIIDSupported(p,a)            ICOM_CALL1(IsIIDSupported,p,a)
#define IRpcStubBuffer_CountRefs(p)                   ICOM_CALL (CountRefs,p)
#define IRpcStubBuffer_DebugServerQueryInterface(p,a) ICOM_CALL1(DebugServerQueryInterface,p,a)
#define IRpcStubBuffer_DebugServerRelease(p,a)        ICOM_CALL1(DebugServerRelease,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CHANNEL_H */
