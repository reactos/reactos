/*
 * Defines the COM interfaces and APIs related to client/server aspects.
 */

#ifndef __WINE_WINE_OBJ_CLIENTSERVER_H
#define __WINE_WINE_OBJ_CLIENTSERVER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IClientSecurity,	0x0000013dL, 0, 0);
typedef struct IClientSecurity IClientSecurity,*LPCLIENTSECURITY;

DEFINE_OLEGUID(IID_IExternalConnection,	0x00000019L, 0, 0);
typedef struct IExternalConnection IExternalConnection,*LPEXTERNALCONNECTION;

DEFINE_OLEGUID(IID_IMessageFilter,	0x00000016L, 0, 0);
typedef struct IMessageFilter IMessageFilter,*LPMESSAGEFILTER;

DEFINE_OLEGUID(IID_IServerSecurity,	0x0000013eL, 0, 0);
typedef struct IServerSecurity IServerSecurity,*LPSERVERSECURITY;


/*****************************************************************************
 * IClientSecurity interface
 */
typedef struct tagSOLE_AUTHENTICATION_SERVICE
{
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    OLECHAR* pPrincipalName;
    HRESULT hr;
} SOLE_AUTHENTICATION_SERVICE, *PSOLE_AUTHENTICATION_SERVICE;

typedef enum tagEOLE_AUTHENTICATION_CAPABILITIES
{
     EOAC_NONE           = 0x0,
     EOAC_MUTUAL_AUTH    = 0x1,
     EOAC_SECURE_REFS    = 0x2,
     EOAC_ACCESS_CONTROL = 0x4
} EOLE_AUTHENTICATION_CAPABILITIES;

#define ICOM_INTERFACE IClientSecurity
#define IClientSecurity_METHODS \
     ICOM_METHOD8(HRESULT,QueryBlanket, IUnknown*,pProxy, DWORD*,pAuthnSvc, DWORD*,pAuthzSvc, OLECHAR**,pServerPrincName, DWORD*,pAuthnLevel, DWORD*,pImpLevel, void**,pAuthInfo, DWORD*,pCapabilites) \
    ICOM_METHOD8(HRESULT,SetBlanket,   IUnknown*,pProxy, DWORD,pAuthnSvc, DWORD,pAuthzSvc, OLECHAR*,pServerPrincName, DWORD,pAuthnLevel, DWORD,pImpLevel, void*,pAuthInfo, DWORD,pCapabilites) \
    ICOM_METHOD2(HRESULT,CopyProxy,    IUnknown*,pProxy, IUnknown**,ppCopy)
#define IClientSecurity_IMETHODS \
    IUnknown_IMETHODS \
    IClientSecurity_METHODS
ICOM_DEFINE(IClientSecurity,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IClientSecurity_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IClientSecurity_AddRef(p)             ICOM_CALL (AddRef,p)
#define IClientSecurity_Release(p)            ICOM_CALL (Release,p)
/*** IClientSecurity methods ***/
#define IClientSecurity_QueryBlanket(p,a,b,c,d,e,f,g,h) ICOM_CALL8(QueryBlanket,p,a,b,c,d,e,f,g,h)
#define IClientSecurity_SetBlanket(p,a,b,c,d,e,f,g,h)   ICOM_CALL8(SetBlanket,p,a,b,c,d,e,f,g,h)
#define IClientSecurity_CopyProxy(p,a,b)                ICOM_CALL2(CopyProxy,p,a,b)


/*****************************************************************************
 * IExternalConnection interface
 */
typedef enum tagEXTCONN
{
    EXTCONN_STRONG   = 0x1,
    EXTCONN_WEAK     = 0x2,
    EXTCONN_CALLABLE = 0x4
} EXTCONN;

#define ICOM_INTERFACE IExternalConnection
#define IExternalConnection_METHODS \
    ICOM_METHOD2(DWORD,AddConnection,     DWORD,extconn, DWORD,reserved) \
    ICOM_METHOD3(DWORD,ReleaseConnection, DWORD,extconn, DWORD,reserved, BOOL,fLastReleaseCloses)
#define IExternalConnection_IMETHODS \
    IUnknown_IMETHODS \
    IExternalConnection_METHODS
ICOM_DEFINE(IExternalConnection,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IExternalConnection_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IExternalConnection_AddRef(p)             ICOM_CALL (AddRef,p)
#define IExternalConnection_Release(p)            ICOM_CALL (Release,p)
/*** IExternalConnection methods ***/
#define IExternalConnection_AddConnection(p,a,b)       ICOM_CALL8(AddConnection,p,a,b)
#define IExternalConnection_ReleaseConnection(p,a,b,c) ICOM_CALL8(ReleaseConnection,p,a,b,c)


HRESULT WINAPI CoDisconnectObject(LPUNKNOWN lpUnk, DWORD reserved);


/*****************************************************************************
 * IMessageFilter interface
 */
typedef enum tagCALLTYPE
{
    CALLTYPE_TOPLEVEL             = 1,
    CALLTYPE_NESTED               = 2,
    CALLTYPE_ASYNC                = 3,
    CALLTYPE_TOPLEVEL_CALLPENDING = 4,
    CALLTYPE_ASYNC_CALLPENDING    = 5
} CALLTYPE;

typedef enum tagSERVERCALL
{
    SERVERCALL_ISHANDLED  = 0,
    SERVERCALL_REJECTED   = 1,
    SERVERCALL_RETRYLATER = 2
} SERVERCALL;

typedef enum tagPENDINGTYPE
{
    PENDINGTYPE_TOPLEVEL = 1,
    PENDINGTYPE_NESTED   = 2
} PENDINGTYPE;

typedef enum tagPENDINGMSG
{
    PENDINGMSG_CANCELCALL     = 0,
    PENDINGMSG_WAITNOPROCESS  = 1,
    PENDINGMSG_WAITDEFPROCESS = 2
} PENDINGMSG;

typedef struct tagINTERFACEINFO
{
    IUnknown* pUnk;
    IID iid;
    WORD wMethod;
} INTERFACEINFO,*LPINTERFACEINFO;

#if 0
#define ICOM_INTERFACE IMessageFilter
#define IMessageFilter_METHODS \
    ICOM_METHOD4(DWORD,HandleInComingCall, DWORD,dwCallType, HTASK,htaskCaller, DWORD,dwTickCount, LPINTERFACEINFO,lpInterfaceInfo) \
    ICOM_METHOD3(DWORD,RetryRejectedCall,  HTASK,htaskCallee, DWORD,dwTickCount, DWORD,dwRejectType) \
    ICOM_METHOD3(DWORD,MessagePending,     HTASK,htaskCallee, DWORD,dwTickCount, DWORD,dwRejectType)
#define IMessageFilter_IMETHODS \
    IUnknown_IMETHODS \
    IMessageFilter_METHODS
ICOM_DEFINE(IMessageFilter,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMessageFilter_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMessageFilter_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMessageFilter_Release(p)            ICOM_CALL (Release,p)
/*** IMessageFilter methods ***/
#define IMessageFilter_HandleInComingCall(p,a,b,c,d) ICOM_CALL4(HandleInComingCall,p,a,b,c,d)
#define IMessageFilter_RetryRejectedCall(p,a,b,c)    ICOM_CALL3(RetryRejectedCall,p,a,b,c)
#define IMessageFilter_MessagePending(p,a,b,c)       ICOM_CALL3(MessagePending,p,a,b,c)


HRESULT WINAPI CoRegisterMessageFilter16(LPMESSAGEFILTER lpMessageFilter,LPMESSAGEFILTER *lplpMessageFilter);
HRESULT WINAPI CoRegisterMessageFilter(LPMESSAGEFILTER lpMessageFilter,LPMESSAGEFILTER *lplpMessageFilter);
#endif

/*****************************************************************************
 * IServerSecurity interface
 */
#define ICOM_INTERFACE IServerSecurity
#define IServerSecurity_METHODS \
    ICOM_METHOD7(HRESULT,QueryBlanket,     DWORD*,pAuthnSvc, DWORD*,pAuthzSvc, OLECHAR**,pServerPrincName, DWORD*,pAuthnLevel, DWORD*,pImpLevel, void**,pPrivs, DWORD*,pCapabilities) \
    ICOM_METHOD (HRESULT,ImpersonateClient) \
    ICOM_METHOD (HRESULT,RevertToSelf) \
    ICOM_METHOD (BOOL,   IsImpersonating)
#define IServerSecurity_IMETHODS \
    IUnknown_IMETHODS \
    IServerSecurity_METHODS
ICOM_DEFINE(IServerSecurity,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IServerSecurity_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IServerSecurity_AddRef(p)             ICOM_CALL (AddRef,p)
#define IServerSecurity_Release(p)            ICOM_CALL (Release,p)
/*** IServerSecurity methods ***/
#define IServerSecurity_QueryBlanket(p,a,b,c,d,e,f,g) ICOM_CALL7(QueryBlanket,p,a,b,c,d,e,f,g)
#define IServerSecurity_ImpersonateClient(p)          ICOM_CALL (ImpersonateClient,p)
#define IServerSecurity_RevertToSelf(p)               ICOM_CALL (RevertToSelf,p)
#define IServerSecurity_IsImpersonating(p)            ICOM_CALL (IsImpersonating,p)


/*****************************************************************************
 * Additional client API
 */
#if 0
/* FIXME: not implemented */
HRESULT WINAPI CoCopyProxy(IUnknown* pProxy, IUnknown** ppCopy);

/* FIXME: not implemented */
HRESULT WINAPI CoQueryProxyBlanket(IUnknown* pProxy, DWORD* pwAuthnSvc, DWORD* pAuthzSvc, OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel, RPC_AUTH_IDENTITY_HANDLE* pAuthInfo, DWORD* pCapabilites);

/* FIXME: not implemented */
HRESULT WINAPI CoSetProxyBlanket(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);
#endif

/*****************************************************************************
 * Additional server API
 */

#if 0
/* FIXME: not implemented */
ULONG WINAPI CoAddRefServerProcess(void);

/* FIXME: not implemented */
HRESULT WINAPI CoImpersonateClient(void);

/* FIXME: not implemented */
HRESULT WINAPI CoQueryClientBlanket(DWORD* pAuthnSvc, DWORD* pAuthzSvc, OLECHAR16** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel, RPC_AUTHZ_HANDLE* pPrivs, DWORD* pCapabilities);

/* FIXME: not implemented */
HRESULT WINAPI CoReleaseServerProcess(void);

/* FIXME: not implemented */
HRESULT WINAPI CoRevertToSelf(void);

/* FIXME: not implemented */
HRESULT WINAPI CoSuspendClassObjects(void);
#endif

/*****************************************************************************
 * Additional API
 */

/* FIXME: not implemented */
HRESULT WINAPI CoGetCallContext(REFIID riid, void** ppInterface);

/* FIXME: not implemented */
HRESULT WINAPI CoGetPSClsid(REFIID riid, CLSID* pClsid);

/* FIXME: not implemented */
HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc, SOLE_AUTHENTICATION_SERVICE* asAuthSvc, void* pReserved1, DWORD dwAuthnLevel, DWORD dwImpLevel, void* pReserved2, DWORD dwCapabilities, void* pReserved3);

/* FIXME: not implemented */
BOOL WINAPI CoIsHandlerConnected(LPUNKNOWN pUnk);

/* FIXME: not implemented */
HRESULT WINAPI CoQueryAuthenticationServices(DWORD* pcAuthSvc, SOLE_AUTHENTICATION_SERVICE** asAuthSvc);

/* FIXME: not implemented */
HRESULT WINAPI CoRegisterPSClsid(REFIID riid, REFCLSID rclsid);

/* FIXME: not implemented */
HRESULT WINAPI CoResumeClassObjects(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CLIENTSERVER_H */
