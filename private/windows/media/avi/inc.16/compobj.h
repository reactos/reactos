// compobj.h - component object model definitions

#if !defined( _COMPOBJ_H_ )
#define _COMPOBJ_H_


/****** Linkage Definitions *************************************************/

/*
 *      These are macros for declaring methods/functions.  They exist so that
 *      control over the use of keywords (CDECL, PASCAL, __export,
 *      extern "C") resides in one place, and because this is the least
 *      intrusive way of writing function declarations that do not have
 *      to be modified in order to port to the Mac.
 *
 *      The macros without the trailing underscore are for functions/methods
 *      which a return value of type HRESULT; this is by far the most common
 *      case in OLE. The macros with a trailing underscore take a return
 *      type as a parameter.
 *
 * WARNING: STDAPI is hard coded into the LPFNGETCLASSOBJECT typedef below.
 */

#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#ifdef _MAC
#define STDMETHODCALLTYPE
#define STDAPICALLTYPE          pascal

#define STDAPI                  EXTERN_C STDAPICALLTYPE HRESULT
#define STDAPI_(type)           EXTERN_C STDAPICALLTYPE type

#else   //  !_MAC

#ifdef WIN32
#define STDMETHODCALLTYPE       __export __cdecl
#define STDAPICALLTYPE          __export __stdcall

#define STDAPI                  EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE

#else
#define STDMETHODCALLTYPE       __export FAR CDECL
#define STDAPICALLTYPE          __export FAR PASCAL

#define STDAPI                  EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE

#endif

#endif //!_MAC

#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE


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
#ifdef __TURBOC__
#define interface               struct huge
#else
#define interface               struct FAR
#endif
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define PURE                    = 0
#define THIS_
#define THIS                    void
#define DECLARE_INTERFACE(iface)    interface iface
#define DECLARE_INTERFACE_(iface, baseiface)    interface iface : public baseiface

#else
#define interface               struct

#ifdef _MAC

#define STDMETHOD(method)       long    method##pad;\
                                HRESULT (STDMETHODCALLTYPE * method)
#define STDMETHOD_(type,method) long    method##pad;\
                                type (STDMETHODCALLTYPE * method)

#else // _MAC

#define STDMETHOD(method)       HRESULT (STDMETHODCALLTYPE * method)
#define STDMETHOD_(type,method) type (STDMETHODCALLTYPE * method)

#endif // !_MAC

#define PURE
#define THIS_                   INTERFACE FAR* This,
#define THIS                    INTERFACE FAR* This
#ifdef CONST_VTABLE
#define DECLARE_INTERFACE(iface)    typedef interface iface { \
                                    const struct iface##Vtbl FAR* lpVtbl; \
                                } iface; \
                                typedef const struct iface##Vtbl iface##Vtbl; \
                                const struct iface##Vtbl
#else
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


#ifndef WINAPI          /* If not included with 3.1 headers... */

#ifdef WIN32
#define FAR
#define PASCAL          __stdcall
#define CDECL
#else
#define FAR             _far
#define PASCAL          _pascal
#define CDECL           _cdecl
#endif

#define VOID            void
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;

typedef long LONG;
typedef unsigned long DWORD;


typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;

typedef unsigned int HANDLE;
#define DECLARE_HANDLE(name)    typedef UINT name

DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HTASK);

#ifndef NULL
#define NULL 0
#endif


typedef void FAR *      LPVOID;
typedef WORD FAR *      LPWORD;
typedef DWORD FAR *     LPDWORD;
typedef char FAR*       LPSTR;
typedef const char FAR* LPCSTR;
typedef void FAR*       LPLOGPALETTE;
typedef void FAR*       LPMSG;
//typedef struct tagMSG FAR *LPMSG;

typedef HANDLE FAR *LPHANDLE;
typedef struct tagRECT FAR *LPRECT;

typedef struct FARSTRUCT tagSIZE
{
    int cx;
    int cy;
} SIZE;
typedef SIZE*       PSIZE;


#endif  /* WINAPI */


typedef short SHORT;
typedef unsigned short USHORT;
typedef DWORD ULONG;


#ifndef HUGEP
#ifdef WIN32
#define HUGEP
#else
#define HUGEP __huge
#endif // WIN32
#endif // HUGEP

typedef WORD WCHAR;

#ifndef WIN32
typedef struct FARSTRUCT _LARGE_INTEGER {
    DWORD LowPart;
    LONG  HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
#endif
#define LISet32(li, v) ((li).HighPart = ((LONG)(v)) < 0 ? -1 : 0, (li).LowPart = (v))

#ifndef WIN32
typedef struct FARSTRUCT _ULARGE_INTEGER {
    DWORD LowPart;
    DWORD HighPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;
#endif
#define ULISet32(li, v) ((li).HighPart = 0, (li).LowPart = (v))

#ifndef _WINDOWS_
#ifndef _FILETIME_
#define _FILETIME_
typedef struct FARSTRUCT tagFILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;
#endif
#endif

#ifdef WIN32
#define HTASK DWORD
#endif

#include "scode.h"



// *********************** Compobj errors **********************************

#define CO_E_NOTINITIALIZED         (CO_E_FIRST + 0x0)
// CoInitialize has not been called and must be

#define CO_E_ALREADYINITIALIZED     (CO_E_FIRST + 0x1)
// CoInitialize has already been called and cannot be called again (temporary)

#define CO_E_CANTDETERMINECLASS     (CO_E_FIRST + 0x2)
// can't determine clsid (e.g., extension not in reg.dat)

#define CO_E_CLASSSTRING            (CO_E_FIRST + 0x3)
// the string form of the clsid is invalid (including ole1 classes)

#define CO_E_IIDSTRING              (CO_E_FIRST + 0x4)
// the string form of the iid is invalid

#define CO_E_APPNOTFOUND            (CO_E_FIRST + 0x5)
// application not found

#define CO_E_APPSINGLEUSE           (CO_E_FIRST + 0x6)
// application cannot be run more than once

#define CO_E_ERRORINAPP             (CO_E_FIRST + 0x7)
// some error in the app program file

#define CO_E_DLLNOTFOUND            (CO_E_FIRST + 0x8)
// dll not found

#define CO_E_ERRORINDLL             (CO_E_FIRST + 0x9)
// some error in the dll file

#define CO_E_WRONGOSFORAPP          (CO_E_FIRST + 0xa)
// app written for other version of OS or other OS altogether

#define CO_E_OBJNOTREG              (CO_E_FIRST + 0xb)
// object is not registered

#define CO_E_OBJISREG               (CO_E_FIRST + 0xc)
// object is already registered

#define CO_E_OBJNOTCONNECTED        (CO_E_FIRST + 0xd)
// handler is not connected to server

#define CO_E_APPDIDNTREG            (CO_E_FIRST + 0xe)
// app was launched, but didn't registered a class factory


// ********************* ClassObject errors ********************************

#define CLASS_E_NOAGGREGATION       (CLASSFACTORY_E_FIRST + 0x0)
// class does not support aggregation (or class object is remote)


// *********************** Reg.dat errors **********************************

#define REGDB_E_READREGDB           (REGDB_E_FIRST + 0x0)
// some error reading the registration database

#define REGDB_E_WRITEREGDB          (REGDB_E_FIRST + 0x1)
// some error reading the registration database

#define REGDB_E_KEYMISSING          (REGDB_E_FIRST + 0x2)
// some error reading the registration database

#define REGDB_E_INVALIDVALUE        (REGDB_E_FIRST + 0x3)
// some error reading the registration database

#define REGDB_E_CLASSNOTREG         (REGDB_E_FIRST + 0x4)
// some error reading the registration database

#define REGDB_E_IIDNOTREG           (REGDB_E_FIRST + 0x5)
// some error reading the registration database


// *************************** RPC errors **********************************

#define RPC_E_FIRST    MAKE_SCODE(SEVERITY_ERROR, FACILITY_RPC,  0x000)

// call was rejected by callee, either by MF::HandleIncomingCall or
#define RPC_E_CALL_REJECTED             (RPC_E_FIRST + 0x1)         

// call was canceld by call - returned by MessagePending
// this code only occurs if MessagePending return cancel
#define RPC_E_CALL_CANCELED             (RPC_E_FIRST + 0x2)         

// the caller is dispatching an intertask SendMessage call and 
// can NOT call out via PostMessage
#define RPC_E_CANTPOST_INSENDCALL       (RPC_E_FIRST + 0x3)             

// the caller is dispatching an asynchronus call can NOT 
// make an outgoing call on behalf of this call
#define RPC_E_CANTCALLOUT_INASYNCCALL   (RPC_E_FIRST + 0x4)         

// the caller is not in a state where an outgoing call can be made
// this is the case if the caller has an outstandig call and
// another incoming call was excepted by HIC; now the caller is
// not allowed to call out again
#define RPC_E_CANTCALLOUT_INEXTERNALCALL (RPC_E_FIRST + 0x5)                

// the connection terminated or is in a bogus state
// and can not be used any more. Other connections
// are still valid.
#define RPC_E_CONNECTION_TERMINATED     (RPC_E_FIRST + 0x6)         

// the callee (server [not server application]) is not available 
// and disappeared; all connections are invalid
#define RPC_E_SERVER_DIED               (RPC_E_FIRST + 0x7)         

// the caller (client ) disappeared while the callee (server) was 
// processing a call 
#define RPC_E_CLIENT_DIED               (RPC_E_FIRST + 0x8)         

// the date paket with the marshalled parameter data is
// incorrect 
#define RPC_E_INVALID_DATAPACKET        (RPC_E_FIRST + 0x9)         

// the call was not transmitted properly; the message queue 
// was full and was not emptied after yielding
#define RPC_E_CANTTRANSMIT_CALL         (RPC_E_FIRST + 0xa)         

// the client (caller) can not marshall the parameter data 
// or unmarshall the return data - low memory etc.
#define RPC_E_CLIENT_CANTMARSHAL_DATA   (RPC_E_FIRST + 0xb)         
#define RPC_E_CLIENT_CANTUNMARSHAL_DATA (RPC_E_FIRST + 0xc)         

// the server (caller) can not unmarshall the parameter data
// or marshall the return data - low memory
#define RPC_E_SERVER_CANTMARSHAL_DATA   (RPC_E_FIRST + 0xd)         
#define RPC_E_SERVER_CANTUNMARSHAL_DATA (RPC_E_FIRST + 0xe)         

// received data are invalid; can be server or 
// client data
#define RPC_E_INVALID_DATA              (RPC_E_FIRST + 0xf)         

// a particular parameter is invalid and can not be un/marshalled
#define RPC_E_INVALID_PARAMETER         (RPC_E_FIRST + 0x10)

// a internal error occured 
#define RPC_E_UNEXPECTED                (RPC_E_FIRST + 0xFFFF)


/****** Globally Unique Ids *************************************************/
 
#ifdef __cplusplus

struct FAR GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];

    BOOL operator==(const GUID& iidOther) const

#ifdef WIN32
        { return !memcmp(&Data1,&iidOther.Data1,sizeof(GUID)); }
#else        
        { return !_fmemcmp(&Data1,&iidOther.Data1,sizeof(GUID)); }
#endif
    BOOL operator!=(const GUID& iidOther) const
        { return !((*this) == iidOther); }
};

#else
typedef struct GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif

typedef                GUID FAR* LPGUID;


// macros to define byte pattern for a GUID.  
//      Example: DEFINE_GUID(GUID_XXX, a, b, c, ...);
//
// Each dll/exe must initialize the GUIDs once.  This is done in one of
// two ways.  If you are not using precompiled headers for the file(s) which
// initializes the GUIDs, define INITGUID before including compobj.h.  This
// is how OLE builds the initialized versions of the GUIDs which are included
// in ole2.lib.  The GUIDs in ole2.lib are all defined in the same text 
// segment GUID_TEXT.
//
// The alternative (which some versions of the compiler don't handle properly;
// they wind up with the initialized GUIDs in a data, not a text segment),
// is to use a precompiled version of compobj.h and then include initguid.h 
// after compobj.h followed by one or more of the guid defintion files.


#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID CDECL FAR name

#ifdef INITGUID
#include "initguid.h"
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)


// Interface ID are just a kind of GUID
typedef GUID IID;
typedef                IID FAR* LPIID;
#define IID_NULL            GUID_NULL
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)


// Class ID are just a kind of GUID
typedef GUID CLSID;
typedef              CLSID FAR* LPCLSID;
#define CLSID_NULL          GUID_NULL
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)


#if defined(__cplusplus)
#define REFGUID             const GUID FAR&
#define REFIID              const IID FAR&
#define REFCLSID            const CLSID FAR&
#else
#define REFGUID             const GUID FAR* const
#define REFIID              const IID FAR* const
#define REFCLSID            const CLSID FAR* const
#endif


#ifndef INITGUID
#include "coguid.h"
#endif


/****** Other value types ***************************************************/

// memory context values; passed to CoGetMalloc
typedef enum tagMEMCTX
{
    MEMCTX_TASK = 1,            // task (private) memory
    MEMCTX_SHARED = 2,          // shared memory (between processes)
#ifdef _MAC
    MEMCTX_MACSYSTEM = 3,       // on the mac, the system heap
#endif 

    // these are mostly for internal use...
    MEMCTX_UNKNOWN = -1,        // unknown context (when asked about it)
    MEMCTX_SAME = -2,           // same context (as some other pointer)
} MEMCTX;



// class context: used to determine what scope and kind of class object to use
// NOTE: this is a bitwise enum
typedef enum tagCLSCTX
{
    CLSCTX_INPROC_SERVER = 1,   // server dll (runs in same process as caller)
    CLSCTX_INPROC_HANDLER = 2,  // handler dll (runs in same process as caller)
    CLSCTX_LOCAL_SERVER = 4     // server exe (runs on same machine; diff proc)
} CLSCTX;

#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER| \
                                 CLSCTX_INPROC_HANDLER| \
                                 CLSCTX_LOCAL_SERVER)

#define CLSCTX_INPROC           (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)

#define CLSCTX_SERVER           (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER)


// class registration flags; passed to CoRegisterClassObject
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,       // class object only generates one instance
    REGCLS_MULTIPLEUSE = 1      // same class object genereates multiple inst.
} REGCLS;


// interface marshaling definitions
#define MARSHALINTERFACE_MIN 40 // minimum number of bytes for interface marshl

// marshaling flags; passed to CoMarshalInterface
typedef enum tagMSHLFLAGS
{
    MSHLFLAGS_NORMAL = 0,       // normal marshaling via proxy/stub
    MSHLFLAGS_TABLESTRONG = 1,  // keep object alive; must explicitly release
    MSHLFLAGS_TABLEWEAK = 2     // doesn't hold object alive; still must release
} MSHLFLAGS;

// marshal context: determines the destination context of the marshal operation
typedef enum tagMSHCTX
{
    MSHCTX_LOCAL = 0,           // unmarshal context is local (eg.shared memory)
    MSHCTX_NOSHAREDMEM = 1,     // unmarshal context has no shared memory access
} MSHCTX;


// call type used by IMessageFilter::HandleIncommingMessage
typedef enum tagCALLTYPE
{
    CALLTYPE_TOPLEVEL = 1,      // toplevel call - no outgoing call 
    CALLTYPE_NESTED   = 2,      // callback on behalf of previous outgoing call - should always handle
    CALLTYPE_ASYNC    = 3,      // aysnchronous call - can NOT be rejected
    CALLTYPE_TOPLEVEL_CALLPENDING = 4,  // new toplevel call with new LID
    CALLTYPE_ASYNC_CALLPENDING    = 5   // async call - can NOT be rejected
} CALLTYPE;

// status of server call - returned by IMessageFilter::HandleIncommingCall
// and passed to  IMessageFilter::RetryRejectedCall
typedef enum tagSERVERCALL
{
    SERVERCALL_ISHANDLED    = 0,
    SERVERCALL_REJECTED     = 1,
    SERVERCALL_RETRYLATER   = 2         
} SERVERCALL;


// Pending type indicates the level of nesting
typedef enum tagPENDINGTYPE
{   
    PENDINGTYPE_TOPLEVEL = 1,       // toplevel call
    PENDINGTYPE_NESTED   = 2,       // nested call
} PENDINGTYPE;

// return values of MessagePending
typedef enum tagPENDINGMSG
{   
    PENDINGMSG_CANCELCALL  = 0, // cancel the outgoing call
    PENDINGMSG_WAITNOPROCESS  = 1, // wait for the return and don't dispatch the message
    PENDINGMSG_WAITDEFPROCESS = 2  // wait and dispatch the message 
    
} PENDINGMSG;


/****** IUnknown Interface **************************************************/


#undef  INTERFACE
#define INTERFACE   IUnknown

DECLARE_INTERFACE(IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
};
typedef        IUnknown FAR* LPUNKNOWN;


/****** Class Factory Interface *******************************************/


#undef  INTERFACE
#define INTERFACE   IClassFactory

DECLARE_INTERFACE_(IClassFactory, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_ LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              LPVOID FAR* ppvObject) PURE;
    STDMETHOD(LockServer) (THIS_ BOOL fLock) PURE;

};
typedef       IClassFactory FAR* LPCLASSFACTORY;


/****** Memory Allocation Interface ***************************************/


#undef  INTERFACE
#define INTERFACE   IMalloc

DECLARE_INTERFACE_(IMalloc, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IMalloc methods ***
    STDMETHOD_(void FAR*, Alloc) (THIS_ ULONG cb) PURE;
    STDMETHOD_(void FAR*, Realloc) (THIS_ void FAR* pv, ULONG cb) PURE;
    STDMETHOD_(void, Free) (THIS_ void FAR* pv) PURE;
    STDMETHOD_(ULONG, GetSize) (THIS_ void FAR* pv) PURE;
    STDMETHOD_(int, DidAlloc) (THIS_ void FAR* pv) PURE;
    STDMETHOD_(void, HeapMinimize) (THIS) PURE;
};
typedef       IMalloc FAR* LPMALLOC;


/****** IMarshal Interface ************************************************/

// forward declaration for IStream; must include storage.h later to use
#ifdef __cplusplus
interface IStream;
#else
typedef interface IStream IStream;
#endif
typedef         IStream FAR* LPSTREAM;


#undef  INTERFACE
#define INTERFACE   IMarshal

DECLARE_INTERFACE_(IMarshal, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IMarshal methods ***
    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv, 
                        DWORD dwDestContext, LPVOID pvDestContext,
                        DWORD mshlflags, LPCLSID pCid) PURE;
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv, 
                        DWORD dwDestContext, LPVOID pvDestContext,
                        DWORD mshlflags, LPDWORD pSize) PURE;
    STDMETHOD(MarshalInterface)(THIS_ LPSTREAM pStm, REFIID riid,
                        LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
                        DWORD mshlflags) PURE;
    STDMETHOD(UnmarshalInterface)(THIS_ LPSTREAM pStm, REFIID riid,
                        LPVOID FAR* ppv) PURE;
    STDMETHOD(ReleaseMarshalData)(THIS_ LPSTREAM pStm) PURE;
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved) PURE;
};
typedef         IMarshal FAR* LPMARSHAL;


#undef  INTERFACE
#define INTERFACE   IStdMarshalInfo

DECLARE_INTERFACE_(IStdMarshalInfo, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IStdMarshalInfo methods ***
    STDMETHOD(GetClassForHandler)(THIS_ DWORD dwDestContext, 
                        LPVOID pvDestContext, LPCLSID pClsid) PURE;
};
typedef         IStdMarshalInfo FAR* LPSTDMARSHALINFO;


/****** Message Filter Interface *******************************************/


#undef  INTERFACE
#define INTERFACE   IMessageFilter

DECLARE_INTERFACE_(IMessageFilter, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IMessageFilter methods ***
    STDMETHOD_(DWORD, HandleInComingCall) (THIS_ DWORD dwCallType,
                                HTASK htaskCaller, DWORD dwTickCount,
                                DWORD dwReserved ) PURE;
    STDMETHOD_(DWORD, RetryRejectedCall) (THIS_ 
                                HTASK htaskCallee, DWORD dwTickCount,
                                DWORD dwRejectType ) PURE;
    STDMETHOD_(DWORD, MessagePending) (THIS_ 
                                HTASK htaskCallee, DWORD dwTickCount, 
                                DWORD dwPendingType  ) PURE; 
};
typedef       IMessageFilter FAR* LPMESSAGEFILTER;


/****** Enumerator Interfaces *********************************************/

/*
 *  Since we don't use parametrized types, we put in explicit declarations
 *  of the enumerators we need.
 */


#undef  INTERFACE
#define INTERFACE   IEnumString

DECLARE_INTERFACE_(IEnumString, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumString methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, 
                       LPSTR FAR* rgelt, 
                       ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumString FAR* FAR* ppenm) PURE;
};
typedef      IEnumString FAR* LPENUMSTRING;


#undef  INTERFACE
#define INTERFACE   IEnumUnknown

DECLARE_INTERFACE_(IEnumUnknown, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumUnknown methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, LPUNKNOWN FAR* rgelt, ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumUnknown FAR* FAR* ppenm) PURE;
};
typedef         IEnumUnknown FAR* LPENUMUNKNOWN;


/****** STD Object API Prototypes *****************************************/

STDAPI_(DWORD) CoBuildVersion( VOID );

/* init/uninit */

STDAPI  CoInitialize(LPMALLOC pMalloc);
STDAPI_(void)  CoUninitialize(void);
STDAPI  CoGetMalloc(DWORD dwMemContext, LPMALLOC FAR* ppMalloc);
STDAPI_(DWORD) CoGetCurrentProcess(void);


/* register/revoke/get class objects */

STDAPI  CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved,
                    REFIID riid, LPVOID FAR* ppv);
STDAPI  CoRegisterClassObject(REFCLSID rclsid, LPUNKNOWN pUnk,
                    DWORD dwClsContext, DWORD flags, LPDWORD lpdwRegister);
STDAPI  CoRevokeClassObject(DWORD dwRegister);


/* marshaling interface pointers */

STDAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
STDAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv);
STDAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult);
STDAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult);
STDAPI CoReleaseMarshalData(LPSTREAM pStm);
STDAPI CoDisconnectObject(LPUNKNOWN pUnk, DWORD dwReserved);
STDAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases);
STDAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk, 
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,
                    LPMARSHAL FAR* ppMarshal);


/* dll loading helpers; keeps track of ref counts and unloads all on exit */

STDAPI_(HINSTANCE) CoLoadLibrary(LPSTR lpszLibName, BOOL bAutoFree);
STDAPI_(void) CoFreeLibrary(HINSTANCE hInst);
STDAPI_(void) CoFreeAllLibraries(void);
STDAPI_(void) CoFreeUnusedLibraries(void);


/* helper for creating instances */

STDAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);


/* other helpers */

STDAPI_(BOOL) IsEqualGUID(REFGUID rguid1, REFGUID rguid2);
STDAPI StringFromCLSID(REFCLSID rclsid, LPSTR FAR* lplpsz);
STDAPI CLSIDFromString(LPSTR lpsz, LPCLSID pclsid);
STDAPI StringFromIID(REFIID rclsid, LPSTR FAR* lplpsz);
STDAPI IIDFromString(LPSTR lpsz, LPIID lpiid);
STDAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid);
STDAPI ProgIDFromCLSID (REFCLSID clsid, LPSTR FAR* lplpszProgID);
STDAPI CLSIDFromProgID (LPCSTR lpszProgID, LPCLSID lpclsid);


STDAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpDosDate, LPWORD lpDosTime);
STDAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime);
STDAPI  CoFileTimeNow( FILETIME FAR* lpFileTime );


STDAPI CoRegisterMessageFilter( LPMESSAGEFILTER lpMessageFilter,
                                LPMESSAGEFILTER FAR* lplpMessageFilter );


/* TreatAs APIS */

STDAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew);
STDAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);


/* the server dlls must define their DllGetClassObject and DllCanUnloadNow
 * to match these; the typedefs are located here to ensure all are changed at 
 * the same time.
 */

STDAPI  DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);
#ifdef _MAC
typedef STDAPICALLTYPE HRESULT (FAR* LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID FAR*);
#else
typedef HRESULT (STDAPICALLTYPE FAR* LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID FAR*);
#endif


STDAPI  DllCanUnloadNow(void);
#ifdef _MAC
typedef STDAPICALLTYPE HRESULT (FAR* LPFNCANUNLOADNOW)(void);
#else
typedef HRESULT (STDAPICALLTYPE FAR* LPFNCANUNLOADNOW)(void);
#endif


/****** Debugging Helpers *************************************************/

#ifdef _DEBUG
// writes to the debug port and displays a message box
STDAPI FnAssert(LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine);
#endif  //  _DEBUG

#endif // _COMPOBJ_H_
