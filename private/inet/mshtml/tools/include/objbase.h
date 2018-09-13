//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       objbase.h
//
//  Contents:   Component object model defintions.
//
//----------------------------------------------------------------------------

#include <rpc.h>
#include <rpcndr.h>

#if !defined( _OBJBASE_H_ )
#define _OBJBASE_H_

#include <pshpack8.h>

#ifdef _MAC
#ifndef _WLM_NOFORCE_LIBS

#ifdef _WLMDLL
        #ifdef _DEBUG
                #pragma comment(lib, "oledlgd.lib")
                #pragma comment(lib, "msvcoled.lib")
        #else
                #pragma comment(lib, "oledlg.lib")
                #pragma comment(lib, "msvcole.lib")
        #endif
#else
        #ifdef _DEBUG
                #pragma comment(lib, "wlmoled.lib")
                #pragma comment(lib, "ole2uid.lib")
        #else
                #pragma comment(lib, "wlmole.lib")
                #pragma comment(lib, "ole2ui.lib")
        #endif
        #pragma data_seg(".drectve")
        static char _gszWlmOLEUIResourceDirective[] = "/macres:ole2ui.rsc";
        #pragma data_seg()
#endif

#pragma comment(lib, "uuid.lib")

#ifdef _DEBUG
    #pragma comment(lib, "ole2d.lib")
    #pragma comment(lib, "ole2autd.lib")
#else
    #pragma comment(lib, "ole2.lib")
    #pragma comment(lib, "ole2auto.lib")
#endif

#endif // !_WLM_NOFORCE_LIBS
#endif // _MAC

// Component Object Model defines, and macros

#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#if defined(_WIN32) || defined(_MPPC_)

// Win32 doesn't support __export

#ifdef _68K_
#define STDMETHODCALLTYPE       __cdecl
#else
#define STDMETHODCALLTYPE       __stdcall
#endif
#define STDMETHODVCALLTYPE      __cdecl

#define STDAPICALLTYPE          __stdcall
#define STDAPIVCALLTYPE         __cdecl

#else

#define STDMETHODCALLTYPE       __export __stdcall
#define STDMETHODVCALLTYPE      __export __cdecl

#define STDAPICALLTYPE          __export __stdcall
#define STDAPIVCALLTYPE         __export __cdecl

#endif


#define STDAPI                  EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE

#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE

// The 'V' versions allow Variable Argument lists.

#define STDAPIV                 EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(type)          EXTERN_C type STDAPIVCALLTYPE

#define STDMETHODIMPV           HRESULT STDMETHODVCALLTYPE
#define STDMETHODIMPV_(type)    type STDMETHODVCALLTYPE

#ifdef _OLE32_
#define WINOLEAPI        STDAPI
#define WINOLEAPI_(type) STDAPI_(type)
#else

#ifdef _68K_
#ifndef REQUIRESAPPLEPASCAL
#define WINOLEAPI        EXTERN_C DECLSPEC_IMPORT HRESULT PASCAL
#define WINOLEAPI_(type) EXTERN_C DECLSPEC_IMPORT type PASCAL
#else
#define WINOLEAPI        EXTERN_C DECLSPEC_IMPORT PASCAL HRESULT
#define WINOLEAPI_(type) EXTERN_C DECLSPEC_IMPORT PASCAL type
#endif
#else
#define WINOLEAPI        EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define WINOLEAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#endif

#endif

/****** Interface Declaration ***********************************************/

/*
 *      These are macros for declaring interfaces.  They exist so that
 *      a single definition of the interface is simulataneously a proper
 *      declaration of the interface structures (C++ abstract classes)
 *      for both C and C++.
 *
 *      DECLARE_INTERFACE(iface) is used to declare an interface that does
 *      not derive from a base interface.
 *      DECLARE_INTERFACE_(iface, baseiface) is used to declare an interface
 *      that does derive from a base interface.
 *
 *      By default if the source file has a .c extension the C version of
 *      the interface declaratations will be expanded; if it has a .cpp
 *      extension the C++ version will be expanded. if you want to force
 *      the C version expansion even though the source file has a .cpp
 *      extension, then define the macro "CINTERFACE".
 *      eg.     cl -DCINTERFACE file.cpp
 *
 *      Example Interface declaration:
 *
 *          #undef  INTERFACE
 *          #define INTERFACE   IClassFactory
 *
 *          DECLARE_INTERFACE_(IClassFactory, IUnknown)
 *          {
 *              // *** IUnknown methods ***
 *              STDMETHOD(QueryInterface) (THIS_
 *                                        REFIID riid,
 *                                        LPVOID FAR* ppvObj) PURE;
 *              STDMETHOD_(ULONG,AddRef) (THIS) PURE;
 *              STDMETHOD_(ULONG,Release) (THIS) PURE;
 *
 *              // *** IClassFactory methods ***
 *              STDMETHOD(CreateInstance) (THIS_
 *                                        LPUNKNOWN pUnkOuter,
 *                                        REFIID riid,
 *                                        LPVOID FAR* ppvObject) PURE;
 *          };
 *
 *      Example C++ expansion:
 *
 *          struct FAR IClassFactory : public IUnknown
 *          {
 *              virtual HRESULT STDMETHODCALLTYPE QueryInterface(
 *                                                  IID FAR& riid,
 *                                                  LPVOID FAR* ppvObj) = 0;
 *              virtual HRESULT STDMETHODCALLTYPE AddRef(void) = 0;
 *              virtual HRESULT STDMETHODCALLTYPE Release(void) = 0;
 *              virtual HRESULT STDMETHODCALLTYPE CreateInstance(
 *                                              LPUNKNOWN pUnkOuter,
 *                                              IID FAR& riid,
 *                                              LPVOID FAR* ppvObject) = 0;
 *          };
 *
 *          NOTE: Our documentation says '#define interface class' but we use
 *          'struct' instead of 'class' to keep a lot of 'public:' lines
 *          out of the interfaces.  The 'FAR' forces the 'this' pointers to
 *          be far, which is what we need.
 *
 *      Example C expansion:
 *
 *          typedef struct IClassFactory
 *          {
 *              const struct IClassFactoryVtbl FAR* lpVtbl;
 *          } IClassFactory;
 *
 *          typedef struct IClassFactoryVtbl IClassFactoryVtbl;
 *
 *          struct IClassFactoryVtbl
 *          {
 *              HRESULT (STDMETHODCALLTYPE * QueryInterface) (
 *                                                  IClassFactory FAR* This,
 *                                                  IID FAR* riid,
 *                                                  LPVOID FAR* ppvObj) ;
 *              HRESULT (STDMETHODCALLTYPE * AddRef) (IClassFactory FAR* This) ;
 *              HRESULT (STDMETHODCALLTYPE * Release) (IClassFactory FAR* This) ;
 *              HRESULT (STDMETHODCALLTYPE * CreateInstance) (
 *                                                  IClassFactory FAR* This,
 *                                                  LPUNKNOWN pUnkOuter,
 *                                                  IID FAR* riid,
 *                                                  LPVOID FAR* ppvObject);
 *              HRESULT (STDMETHODCALLTYPE * LockServer) (
 *                                                  IClassFactory FAR* This,
 *                                                  BOOL fLock);
 *          };
 */

#if defined(__cplusplus) && !defined(CINTERFACE)
//#define interface               struct FAR
#define interface struct
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define PURE                    = 0
#define THIS_
#define THIS                    void
#define DECLARE_INTERFACE(iface)    interface iface
#define DECLARE_INTERFACE_(iface, baseiface)    interface iface : public baseiface


#if !defined(BEGIN_INTERFACE)
#if defined(_MPPC_)  && \
    ( (defined(_MSC_VER) || defined(__SC__) || defined(__MWERKS__)) && \
    !defined(NO_NULL_VTABLE_ENTRY) )
   #define BEGIN_INTERFACE virtual void a() {}
   #define END_INTERFACE
#else
   #define BEGIN_INTERFACE
   #define END_INTERFACE
#endif
#endif

#else

#define interface               struct

#define STDMETHOD(method)       HRESULT (STDMETHODCALLTYPE * method)
#define STDMETHOD_(type,method) type (STDMETHODCALLTYPE * method)

#if !defined(BEGIN_INTERFACE)
#if defined(_MPPC_)
    #define BEGIN_INTERFACE       void    *b;
    #define END_INTERFACE
#else
    #define BEGIN_INTERFACE
    #define END_INTERFACE
#endif
#endif


#define PURE
#define THIS_                   INTERFACE FAR* This,
#define THIS                    INTERFACE FAR* This
#ifdef CONST_VTABLE
#undef CONST_VTBL
#define CONST_VTBL const
#define DECLARE_INTERFACE(iface)    typedef interface iface { \
                                    const struct iface##Vtbl FAR* lpVtbl; \
                                } iface; \
                                typedef const struct iface##Vtbl iface##Vtbl; \
                                const struct iface##Vtbl
#else
#undef CONST_VTBL
#define CONST_VTBL
#define DECLARE_INTERFACE(iface)    typedef interface iface { \
                                    struct iface##Vtbl FAR* lpVtbl; \
                                } iface; \
                                typedef struct iface##Vtbl iface##Vtbl; \
                                struct iface##Vtbl
#endif
#define DECLARE_INTERFACE_(iface, baseiface)    DECLARE_INTERFACE(iface)

#endif




/****** Additional basic types **********************************************/


#ifndef FARSTRUCT
#ifdef __cplusplus
#define FARSTRUCT   FAR
#else
#define FARSTRUCT
#endif  // __cplusplus
#endif  // FARSTRUCT



#ifndef HUGEP
#if defined(_WIN32) || defined(_MPPC_)
#define HUGEP
#else
#define HUGEP __huge
#endif // WIN32
#endif // HUGEP


#ifdef _MAC
#if !defined(OLE2ANSI)
#define OLE2ANSI
#endif
#endif

#include <stdlib.h>

#define LISet32(li, v) ((li).HighPart = (v) < 0 ? -1 : 0, (li).LowPart = (v))

#define ULISet32(li, v) ((li).HighPart = 0, (li).LowPart = (v))






#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)

// With DCOM, CLSCTX_REMOTE_SERVER should be included
#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER| \
                                 CLSCTX_REMOTE_SERVER)

#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)
#else
#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER )

#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER)
#endif


// class registration flags; passed to CoRegisterClassObject
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,       // class object only generates one instance
    REGCLS_MULTIPLEUSE = 1,     // same class object genereates multiple inst.
                                // and local automatically goes into inproc tbl.
    REGCLS_MULTI_SEPARATE = 2,  // multiple use, but separate control over each
                                // context.
    REGCLS_SUSPENDED      = 4   // register is as suspended, will be activated
                                // when app calls CoResumeClassObjects
} REGCLS;

// interface marshaling definitions
#define MARSHALINTERFACE_MIN 500 // minimum number of bytes for interface marshl


//
// Common typedefs for paramaters used in Storage API's, gleamed from storage.h
// Also contains Storage error codes, which should be moved into the storage
// idl files.
//


#define CWCSTORAGENAME 32

/* Storage instantiation modes */
#define STGM_DIRECT             0x00000000L
#define STGM_TRANSACTED         0x00010000L
#define STGM_SIMPLE             0x08000000L

#define STGM_READ               0x00000000L
#define STGM_WRITE              0x00000001L
#define STGM_READWRITE          0x00000002L

#define STGM_SHARE_DENY_NONE    0x00000040L
#define STGM_SHARE_DENY_READ    0x00000030L
#define STGM_SHARE_DENY_WRITE   0x00000020L
#define STGM_SHARE_EXCLUSIVE    0x00000010L

#define STGM_PRIORITY           0x00040000L
#define STGM_DELETEONRELEASE    0x04000000L
#if (WINVER >= 400)
#define STGM_NOSCRATCH          0x00100000L
#endif /* WINVER */

#define STGM_CREATE             0x00001000L
#define STGM_CONVERT            0x00020000L
#define STGM_FAILIFTHERE        0x00000000L

#define STGM_NOSNAPSHOT         0x00200000L

/*  flags for internet asyncronous and layout docfile */
#define ASYNC_MODE_COMPATIBILITY    0x00000001L
#define ASYNC_MODE_DEFAULT          0x00000000L

#define STGTY_REPEAT                0x00000100L
#define STG_TOEND                   0xFFFFFFFFL

#define STG_LAYOUT_SEQUENTIAL       0x00000000L
#define STG_LAYOUT_INTERLEAVED      0x00000001L




/* here is where we pull in the MIDL generated headers for the interfaces */
typedef interface    IRpcStubBuffer     IRpcStubBuffer;
typedef interface    IRpcChannelBuffer  IRpcChannelBuffer;

#include <wtypes.h>
#include <unknwn.h>
#include <objidl.h>


// macros to define byte pattern for a GUID.
//      Example: DEFINE_GUID(GUID_XXX, a, b, c, ...);
//
// Each dll/exe must initialize the GUIDs once.  This is done in one of
// two ways.  If you are not using precompiled headers for the file(s) which
// initializes the GUIDs, define INITGUID before including objbase.h.  This
// is how OLE builds the initialized versions of the GUIDs which are included
// in ole2.lib.  The GUIDs in ole2.lib are all defined in the same text
// segment GUID_TEXT.
//
// The alternative (which some versions of the compiler don't handle properly;
// they wind up with the initialized GUIDs in a data, not a text segment),
// is to use a precompiled version of objbase.h and then include initguid.h
// after objbase.h followed by one or more of the guid defintion files.

#ifndef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID FAR name
#else

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif // INITGUID

#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#ifdef _OLE32_


// Faster (but makes code fatter) inline version...use sparingly
#ifdef __cplusplus
inline BOOL  InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}
#else   // ! __cplusplus
#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])
#endif  // __cplusplus

#ifdef _OLE32PRIV_
BOOL _fastcall wIsEqualGUID(REFGUID rguid1, REFGUID rguid2);
#define IsEqualGUID(rguid1, rguid2) wIsEqualGUID(rguid1, rguid2)
#else
#define IsEqualGUID(rguid1, rguid2) InlineIsEqualGUID(rguid1, rguid2)
#endif  // _OLE32PRIV_

#else   // ! _OLE32_
#ifdef __cplusplus
inline BOOL IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#else   //  ! __cplusplus
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif  //  __cplusplus
#endif  //  _OLE32_

#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#ifdef __cplusplus

// because GUID is defined elsewhere in WIN32 land, the operator == and !=
// are moved outside the class to global scope.

#ifdef _OLE32_
inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
    return IsEqualGUID(guidOne,guidOther);
}
#else   // !_OLE32_
inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
#ifdef _WIN32
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
#else
    return !_fmemcmp(&guidOne,&guidOther,sizeof(GUID)); }
#endif
}
#endif // _OLE32_

inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}

#endif // __cpluscplus


#ifndef INITGUID
#include <cguid.h>
#endif

// COM initialization flags; passed to CoInitialize.
typedef enum tagCOINIT
{
  COINIT_APARTMENTTHREADED  = 0x2,      // Apartment model

#if  (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
  // These constants are only valid on Windows NT 4.0
  COINIT_MULTITHREADED      = 0x0,      // OLE calls objects on any thread.
  COINIT_DISABLE_OLE1DDE    = 0x4,      // Don't use DDE for Ole1 support.
  COINIT_SPEED_OVER_MEMORY  = 0x8,      // Trade memory for speed.
#endif // DCOM
} COINIT;





/****** STD Object API Prototypes *****************************************/

WINOLEAPI_(DWORD) CoBuildVersion( VOID );

/* init/uninit */

WINOLEAPI  CoInitialize(LPVOID pvReserved);
WINOLEAPI_(void)  CoUninitialize(void);
WINOLEAPI  CoGetMalloc(DWORD dwMemContext, LPMALLOC FAR* ppMalloc);
WINOLEAPI_(DWORD) CoGetCurrentProcess(void);
WINOLEAPI  CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy);
WINOLEAPI  CoRevokeMallocSpy(void);
WINOLEAPI  CoCreateStandardMalloc(DWORD memctx, IMalloc FAR* FAR* ppMalloc);

#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
#endif // DCOM

#if DBG == 1
WINOLEAPI_(ULONG) DebugCoGetRpcFault( void );
WINOLEAPI_(void) DebugCoSetRpcFault( ULONG );
#endif

/* register/revoke/get class objects */

WINOLEAPI  CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved,
                    REFIID riid, LPVOID FAR* ppv);
WINOLEAPI  CoRegisterClassObject(REFCLSID rclsid, LPUNKNOWN pUnk,
                    DWORD dwClsContext, DWORD flags, LPDWORD lpdwRegister);
WINOLEAPI  CoRevokeClassObject(DWORD dwRegister);
WINOLEAPI  CoResumeClassObjects(void);
WINOLEAPI  CoSuspendClassObjects(void);
WINOLEAPI_(ULONG) CoAddRefServerProcess(void);
WINOLEAPI_(ULONG) CoReleaseServerProcess(void);
WINOLEAPI  CoGetPSClsid(REFIID riid, CLSID *pClsid);
WINOLEAPI  CoRegisterPSClsid(REFIID riid, REFCLSID rclsid);

/* marshaling interface pointers */

WINOLEAPI CoGetMarshalSizeMax(ULONG *pulSize, REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
WINOLEAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
WINOLEAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv);
WINOLEAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult);
WINOLEAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult);
WINOLEAPI CoReleaseMarshalData(LPSTREAM pStm);
WINOLEAPI CoDisconnectObject(LPUNKNOWN pUnk, DWORD dwReserved);
WINOLEAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases);
WINOLEAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,
                    LPMARSHAL FAR* ppMarshal);

WINOLEAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk);
WINOLEAPI_(BOOL) CoHasStrongExternalConnections(LPUNKNOWN pUnk);

// Apartment model inter-thread interface passing helpers
WINOLEAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, LPUNKNOWN pUnk,
                    LPSTREAM *ppStm);

WINOLEAPI CoGetInterfaceAndReleaseStream(LPSTREAM pStm, REFIID iid,
                    LPVOID FAR* ppv);

WINOLEAPI CoCreateFreeThreadedMarshaler(LPUNKNOWN  punkOuter,
                    LPUNKNOWN *ppunkMarshal);

/* dll loading helpers; keeps track of ref counts and unloads all on exit */

WINOLEAPI_(HINSTANCE) CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree);
WINOLEAPI_(void) CoFreeLibrary(HINSTANCE hInst);
WINOLEAPI_(void) CoFreeAllLibraries(void);
WINOLEAPI_(void) CoFreeUnusedLibraries(void);


#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM

/* Call Security. */

WINOLEAPI CoInitializeSecurity(
                    PSECURITY_DESCRIPTOR         pSecDesc,
                    LONG                         cAuthSvc,
                    SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                    void                        *pReserved1,
                    DWORD                        dwAuthnLevel,
                    DWORD                        dwImpLevel,
                    void                        *pReserved2,
                    DWORD                        dwCapabilities,
                    void                        *pReserved3 );
WINOLEAPI CoGetCallContext( REFIID riid, void **ppInterface );
WINOLEAPI CoQueryProxyBlanket(
    IUnknown                  *pProxy,
    DWORD                     *pwAuthnSvc,
    DWORD                     *pAuthzSvc,
    OLECHAR                  **pServerPrincName,
    DWORD                     *pAuthnLevel,
    DWORD                     *pImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,
    DWORD                     *pCapabilites );
WINOLEAPI CoSetProxyBlanket(
    IUnknown                 *pProxy,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthnLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities );
WINOLEAPI CoCopyProxy(
    IUnknown    *pProxy,
    IUnknown   **ppCopy );
WINOLEAPI CoQueryClientBlanket(
    DWORD             *pAuthnSvc,
    DWORD             *pAuthzSvc,
    OLECHAR          **pServerPrincName,
    DWORD             *pAuthnLevel,
    DWORD             *pImpLevel,
    RPC_AUTHZ_HANDLE  *pPrivs,
    DWORD             *pCapabilities );
WINOLEAPI CoImpersonateClient();
WINOLEAPI CoRevertToSelf();
WINOLEAPI CoQueryAuthenticationServices(
    DWORD *pcAuthSvc,
    SOLE_AUTHENTICATION_SERVICE **asAuthSvc );
WINOLEAPI CoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject );

#define COM_RIGHTS_EXECUTE 1

#endif // DCOM

/* helper for creating instances */

WINOLEAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);


#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM

WINOLEAPI CoGetInstanceFromFile(
    COSERVERINFO *              pServerInfo,
    CLSID       *               pClsid,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    DWORD                       grfMode,
    OLECHAR *                   pwszName,
    DWORD                       dwCount,
    MULTI_QI        *           pResults );

WINOLEAPI CoGetInstanceFromIStorage(
    COSERVERINFO *              pServerInfo,
    CLSID       *               pClsid,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    struct IStorage *           pstg,
    DWORD                       dwCount,
    MULTI_QI        *           pResults );

WINOLEAPI CoCreateInstanceEx(
    REFCLSID                    Clsid,
    IUnknown    *               punkOuter, // only relevant locally
    DWORD                       dwClsCtx,
    COSERVERINFO *              pServerInfo,
    DWORD                       dwCount,
    MULTI_QI        *           pResults );

#endif // DCOM


/* other helpers */

WINOLEAPI StringFromCLSID(REFCLSID rclsid, LPOLESTR FAR* lplpsz);
WINOLEAPI CLSIDFromString(LPOLESTR lpsz, LPCLSID pclsid);
WINOLEAPI StringFromIID(REFIID rclsid, LPOLESTR FAR* lplpsz);
WINOLEAPI IIDFromString(LPOLESTR lpsz, LPIID lpiid);
WINOLEAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid);
WINOLEAPI ProgIDFromCLSID (REFCLSID clsid, LPOLESTR FAR* lplpszProgID);
WINOLEAPI CLSIDFromProgID (LPCOLESTR lpszProgID, LPCLSID lpclsid);
WINOLEAPI_(int) StringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cbMax);

WINOLEAPI CoCreateGuid(GUID FAR *pguid);

WINOLEAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpDosDate, LPWORD lpDosTime);
WINOLEAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime);
WINOLEAPI  CoFileTimeNow( FILETIME FAR* lpFileTime );


WINOLEAPI CoRegisterMessageFilter( LPMESSAGEFILTER lpMessageFilter,
                                LPMESSAGEFILTER FAR* lplpMessageFilter );

#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) // DCOM
WINOLEAPI CoRegisterChannelHook( REFGUID ExtensionUuid, IChannelHook *pChannelHook );
#endif // DCOM


/* TreatAs APIS */

WINOLEAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew);
WINOLEAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);


/* the server dlls must define their DllGetClassObject and DllCanUnloadNow
 * to match these; the typedefs are located here to ensure all are changed at
 * the same time.
 */

//#ifdef _MAC
//typedef STDAPICALLTYPE HRESULT (* LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID *);
//#else
typedef HRESULT (STDAPICALLTYPE * LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID *);
//#endif

//#ifdef _MAC
//typedef STDAPICALLTYPE HRESULT (* LPFNCANUNLOADNOW)(void);
//#else
typedef HRESULT (STDAPICALLTYPE * LPFNCANUNLOADNOW)(void);
//#endif

STDAPI  DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);

STDAPI  DllCanUnloadNow(void);


/****** Default Memory Allocation ******************************************/
WINOLEAPI_(LPVOID) CoTaskMemAlloc(ULONG cb);
WINOLEAPI_(LPVOID) CoTaskMemRealloc(LPVOID pv, ULONG cb);
WINOLEAPI_(void)   CoTaskMemFree(LPVOID pv);

/****** DV APIs ***********************************************************/


WINOLEAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder);

WINOLEAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid,
                                        REFIID iid, LPVOID FAR* ppv);




/****** Storage API Prototypes ********************************************/


WINOLEAPI StgCreateDocfile(const OLECHAR FAR* pwcsName,
            DWORD grfMode,
            DWORD reserved,
            IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgCreateDocfileOnILockBytes(ILockBytes FAR *plkbyt,
                    DWORD grfMode,
                    DWORD reserved,
                    IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgOpenStorage(const OLECHAR FAR* pwcsName,
              IStorage FAR *pstgPriority,
              DWORD grfMode,
              SNB snbExclude,
              DWORD reserved,
              IStorage FAR * FAR *ppstgOpen);
WINOLEAPI StgOpenStorageOnILockBytes(ILockBytes FAR *plkbyt,
                  IStorage FAR *pstgPriority,
                  DWORD grfMode,
                  SNB snbExclude,
                  DWORD reserved,
                  IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgIsStorageFile(const OLECHAR FAR* pwcsName);
WINOLEAPI StgIsStorageILockBytes(ILockBytes FAR* plkbyt);

WINOLEAPI StgSetTimes(OLECHAR const FAR* lpszName,
                   FILETIME const FAR* pctime,
                   FILETIME const FAR* patime,
                   FILETIME const FAR* pmtime);

WINOLEAPI StgOpenAsyncDocfileOnIFillLockBytes( IFillLockBytes *pflb,
             DWORD grfMode,
             DWORD asyncFlags,
             IStorage **ppstgOpen);

WINOLEAPI StgGetIFillLockBytesOnILockBytes( ILockBytes *pilb,
             IFillLockBytes **ppflb);

WINOLEAPI StgGetIFillLockBytesOnFile(OLECHAR const *pwcsName,
             IFillLockBytes **ppflb);


WINOLEAPI StgOpenLayoutDocfile(OLECHAR const *pwcsDfName,
             DWORD grfMode,
             DWORD reserved,
             IStorage **ppstgOpen);



//
//  Moniker APIs
//

WINOLEAPI  BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID FAR* ppvResult);
WINOLEAPI  CoGetObject(LPCWSTR pszName, BIND_OPTS *pBindOptions, REFIID riid, void **ppv);
WINOLEAPI  MkParseDisplayName(LPBC pbc, LPCOLESTR szUserName,
                ULONG FAR * pchEaten, LPMONIKER FAR * ppmk);
WINOLEAPI  MonikerRelativePathTo(LPMONIKER pmkSrc, LPMONIKER pmkDest, LPMONIKER
                FAR* ppmkRelPath, BOOL dwReserved);
WINOLEAPI  MonikerCommonPrefixWith(LPMONIKER pmkThis, LPMONIKER pmkOther,
                LPMONIKER FAR* ppmkCommon);
WINOLEAPI  CreateBindCtx(DWORD reserved, LPBC FAR* ppbc);
WINOLEAPI  CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest,
    LPMONIKER FAR* ppmkComposite);
WINOLEAPI  GetClassFile (LPCOLESTR szFilename, CLSID FAR* pclsid);

WINOLEAPI  CreateClassMoniker(REFCLSID rclsid, LPMONIKER FAR* ppmk);

WINOLEAPI  CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER FAR* ppmk);

WINOLEAPI  CreateItemMoniker(LPCOLESTR lpszDelim, LPCOLESTR lpszItem,
    LPMONIKER FAR* ppmk);
WINOLEAPI  CreateAntiMoniker(LPMONIKER FAR* ppmk);
WINOLEAPI  CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER FAR* ppmk);

WINOLEAPI  GetRunningObjectTable( DWORD reserved, LPRUNNINGOBJECTTABLE FAR* pprot);

#ifndef RC_INVOKED
#include <poppack.h>
#endif // RC_INVOKED

#endif     // __OBJBASE_H__
