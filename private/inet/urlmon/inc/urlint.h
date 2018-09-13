    //+---------------------------------------------------------------------------
    //
    //  Microsoft Windows
    //  Copyright (C) Microsoft Corporation, 1992 - 1995.
    //
    //  File:       urlint.h
    //
    //  Contents:   internal include file for ulrmon project
    //
    //  Classes:
    //
    //  Functions:
    //
    //  History:    10-25-95   JohannP (Johann Posch)   Created
    //
    //----------------------------------------------------------------------------
    #ifndef _URLINT_H_
    #define _URLINT_H_
    #define _WITH_INTERNET_URL_ZONES_
    #ifdef ALPHA    // required for Wx86 support
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #endif
    #include <urlmki.h>
    #include <debug.h>
    #include <valid.h>
    #include <perftags.h>
    #include <crtsubst.h>
    
    #ifndef ARRAYSIZE
    #define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
    #endif
    
    #if DBG==1
    #define XDBG(dbg,nondbg)    dbg
    #else
    #define XDBG(dbg,nondbg)    nondbg
    #endif
    
    #if DBG == 1
    //interface IDebugOut : public IUnknown
    #undef INTERFACE
    #define INTERFACE   IDebugOut
    DECLARE_INTERFACE_(IDebugOut,IUnknown)
    {
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;
    
        // *** IDebugOut methods ***
        STDMETHOD_(void, SendEntry) (THIS_ DWORD ThreadId, DWORD dwFlags, LPCSTR pstr, DWORD dwReserved) PURE;
    };
    
    
    #undef INTERFACE
    #define INTERFACE   IDebugRegister
    DECLARE_INTERFACE_(IDebugRegister,IUnknown)
    {
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;
    
        // *** IDebugRegister methods ***
        STDMETHOD(GetFacilities) (THIS_ LPCWSTR *ppwzNames, DWORD *pcNames, DWORD dwReserved) PURE;
        STDMETHOD(Register) (THIS_ LPCWSTR pwzName, IDebugOut *pDbgOut, DWORD dwFlags, DWORD dwReserved) PURE;
    };
    
    // each thread can have it's own IDebugOut interface
    EXTERN_C HRESULT RegisterDebugOut(LPCWSTR pwzName, DWORD dwOptions, IDebugOut *pDbgOut, DWORD dwReserved);
    void UrlSpySendEntry(IDebugOut *pDbgOut, LPSTR szOutBuffer, DWORD ThreadId = 0, DWORD dwFlags = 0, DWORD dwReserved = 0);
    void UrlSpyFn(int iOption, const char *pscFormat, ...);
    
    #define PerfDbgTag(tag, szOwner, szDescrip, iOption) int tag = iOption;
    #define     DbgTag(tag, szOwner, szDescrip, iOption) int tag = iOption;
    #define PerfDbgExtern(tag) extern int tag;
    #define     DbgExtern(tag) extern int tag;
    #define PerfDbgLog(tag,pv,f) PerfDbgLogFn(tag,pv,f)
    #define PerfDbgLog1(tag,pv,f,a1) PerfDbgLogFn(tag,pv,f,a1)
    #define PerfDbgLog2(tag,pv,f,a1,a2) PerfDbgLogFn(tag,pv,f,a1,a2)
    #define PerfDbgLog3(tag,pv,f,a1,a2,a3) PerfDbgLogFn(tag,pv,f,a1,a2,a3)
    #define PerfDbgLog4(tag,pv,f,a1,a2,a3,a4) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4)
    #define PerfDbgLog5(tag,pv,f,a1,a2,a3,a4,a5) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5)
    #define PerfDbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define PerfDbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define PerfDbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define PerfDbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define PerfDbgLogN(x) PerfDbgLogFn x
    #define     DbgLog(tag,pv,f) PerfDbgLogFn(tag,pv,f)
    #define     DbgLog1(tag,pv,f,a1) PerfDbgLogFn(tag,pv,f,a1)
    #define     DbgLog2(tag,pv,f,a1,a2) PerfDbgLogFn(tag,pv,f,a1,a2)
    #define     DbgLog3(tag,pv,f,a1,a2,a3) PerfDbgLogFn(tag,pv,f,a1,a2,a3)
    #define     DbgLog4(tag,pv,f,a1,a2,a3,a4) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4)
    #define     DbgLog5(tag,pv,f,a1,a2,a3,a4,a5) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5)
    #define     DbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define     DbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define     DbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define     DbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define     DbgLogN(x) PerfDbgLogFn x
    void    PerfDbgLogFn(int tag, void * pvObj, const char * pchFmt, ...);
    
    #   define DEB_LEVEL_SHIFT      28
    #   define DEB_LEVEL_MASK       0x0FFFFFFF
    
        DECLARE_DEBUG(UrlMk)
    #   define UrlMkUrlSpy          UrlSpyFn
    #   define UrlMkDebugOut(x)     UrlMkUrlSpy x
    #   define UrlMkAssert(x)       Win4Assert(x)
    #   define UrlMkVerify(x)       UrlMkAssert(x)
    #   define DEB_URLMK_LEVEL      0x00000000
    #   define DEB_ASYNCAPIS        (DEB_USER1 | DEB_URLMK_LEVEL)
    #   define DEB_URLMON           (DEB_USER2 | DEB_URLMK_LEVEL)
    #   define DEB_ISTREAM          (DEB_USER3 | DEB_URLMK_LEVEL)
    #   define DEB_DLL              (DEB_USER4 | DEB_URLMK_LEVEL)
    #   define DEB_FORMAT           (DEB_USER5 | DEB_URLMK_LEVEL)
    #   define DEB_CODEDL           (DEB_USER6 | DEB_URLMK_LEVEL)
    
        DECLARE_DEBUG(Trans)
    #   define TransUrlSpy          UrlSpyFn
    #   define TransDebugOut(x)     TransUrlSpy x
    #   define TransAssert(x)       Win4Assert(x)
    #   define TransVerify(x)       TransAssert(x)
    #   define DEB_TRANS_LEVEL      0x10000000
    #   define DEB_BINDING          (DEB_USER1 | DEB_TRANS_LEVEL)
    #   define DEB_TRANS            (DEB_USER2 | DEB_TRANS_LEVEL)
    #   define DEB_TRANSPACKET      (DEB_USER3 | DEB_TRANS_LEVEL)
    #   define DEB_DATA             (DEB_USER4 | DEB_TRANS_LEVEL)
    #   define DEB_TRANSMGR         (DEB_USER5 | DEB_TRANS_LEVEL)
    #   define DEB_SESSION          (DEB_USER6 | DEB_TRANS_LEVEL)
    
        DECLARE_DEBUG(PProt)
    #   define PProtUrlSpy          UrlSpyFn
    #   define PProtDebugOut(x)     PProtUrlSpy x
    #   define PProtAssert(x)       Win4Assert(x)
    #   define PProtVerify(x)       PProtAssert(x)
    #   define DEB_PROT_LEVEL       0x20000000
    #   define DEB_PROT             (DEB_USER1 | DEB_PROT_LEVEL)
    #   define DEB_PROTHTTP         (DEB_USER2 | DEB_PROT_LEVEL)
    #   define DEB_PROTFTP          (DEB_USER3 | DEB_PROT_LEVEL)
    #   define DEB_PROTGOPHER       (DEB_USER4 | DEB_PROT_LEVEL)
    #   define DEB_PROTSIMP         (DEB_USER5 | DEB_PROT_LEVEL)
    
        DECLARE_DEBUG(Notf)
    #   define NotfUrlSpy          UrlSpyFn
    #   define NotfDebugOut(x)     NotfUrlSpy x
    #   define NotfAssert(x)       Win4Assert(x)
    #   define NotfVerify(x)       NotfAssert(x)
    #   define DEB_NOTF_LEVEL       0x30000000
    
    #   define DEB_NOTF_1             (DEB_USER1  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_2             (DEB_USER2  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_3             (DEB_USER3  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_4             (DEB_USER4  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_5             (DEB_USER5  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_6             (DEB_USER6  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_7             (DEB_USER7  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_8             (DEB_USER8  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_9             (DEB_USER9  | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_10            (DEB_USER10 | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_11            (DEB_USER11 | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_12            (DEB_USER12 | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_13            (DEB_USER13 | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_14            (DEB_USER14 | DEB_NOTF_LEVEL)
    #   define DEB_NOTF_15            (DEB_USER15 | DEB_NOTF_LEVEL)
    
        DECLARE_DEBUG(EProt)
    #   define EProtUrlSpy          UrlSpyFn
    #   define EProtDebugOut(x)     EProtUrlSpy x
    #   define EProtAssert(x)       Win4Assert(x)
    #   define EProtVerify(x)       EProtAssert(x)
    #   define DEB_EPROT_LEVEL      0x40000000
    #   define DEB_PLUGPROT         (DEB_USER1  | DEB_EPROT_LEVEL)
    #   define DEB_BASE             (DEB_USER2  | DEB_EPROT_LEVEL)
    
        DECLARE_DEBUG(TNotf)
    #   define TNotfUrlSpy         UrlSpyFn
    #   define TNotfDebugOut(x)    TNotfUrlSpy x
    #   define TNotfAssert(x)      Win4Assert(x)
    #   define TNotfVerify(x)      TNotfAssert(x)
    #   define DEB_TNOTF_LEVEL     0x50000000
    
    #   define DEB_TNOTF_1            (DEB_USER1  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_2            (DEB_USER2  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_3            (DEB_USER3  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_4            (DEB_USER4  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_5            (DEB_USER5  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_6            (DEB_USER6  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_7            (DEB_USER7  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_8            (DEB_USER8  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_9            (DEB_USER9  | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_10           (DEB_USER10 | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_11           (DEB_USER11 | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_12           (DEB_USER12 | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_13           (DEB_USER13 | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_14           (DEB_USER14 | DEB_TNOTF_LEVEL)
    #   define DEB_TNOTF_15           (DEB_USER15 | DEB_TNOTF_LEVEL)
    
    #   define PPKG_DUMP(ptr, params)       ptr->Dump params
    #   define PLIST_DUMP(ptr, params)      ptr->Dump params
    #   define LIST_DUMP(obj, params)       obj.Dump params
    #   define SPEW_TIME(params)            SpewTime params
    
    #else
    
    #define PerfDbgTag(tag, szOwner, szDescrip, iOption) PerfTag(tag, szOwner, szDescrip)
    #define     DbgTag(tag, szOwner, szDescrip, iOption)
    #define PerfDbgExtern(tag) PerfExtern(tag)
    #define     DbgExtern(tag)
    #define PerfDbgLog(tag,pv,f) PerfLog(tag,pv,f)
    #define PerfDbgLog1(tag,pv,f,a1) PerfLog1(tag,pv,f,a1)
    #define PerfDbgLog2(tag,pv,f,a1,a2) PerfLog2(tag,pv,f,a1,a2)
    #define PerfDbgLog3(tag,pv,f,a1,a2,a3) PerfLog3(tag,pv,f,a1,a2,a3)
    #define PerfDbgLog4(tag,pv,f,a1,a2,a3,a4) PerfLog4(tag,pv,f,a1,a2,a3,a4)
    #define PerfDbgLog5(tag,pv,f,a1,a2,a3,a4,a5) PerfLog5(tag,pv,f,a1,a2,a3,a4,a5)
    #define PerfDbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) PerfLog6(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define PerfDbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) PerfLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define PerfDbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) PerfLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define PerfDbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) PerfLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define PerfDbgLogN(x) PerfLogFn x
    #define     DbgLog(tag,pv,f)
    #define     DbgLog1(tag,pv,f,a1)
    #define     DbgLog2(tag,pv,f,a1,a2)
    #define     DbgLog3(tag,pv,f,a1,a2,a3)
    #define     DbgLog4(tag,pv,f,a1,a2,a3,a4)
    #define     DbgLog5(tag,pv,f,a1,a2,a3,a4,a5)
    #define     DbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define     DbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define     DbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define     DbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define     DbgLogN(x)
    
    #define PPKG_DUMP(ptr, params)
    #define PLIST_DUMP(ptr, params)
    #define LIST_DUMP(obj, params)
    #define SPEW_TIME(params)
    
    #if DBGASSERT == 1
    
    #   define UrlMkAssert(x)  (void) ((x) || (DebugBreak(),0))
    #   define TransAssert(x)  (void) ((x) || (DebugBreak(),0))
    #   define PProtAssert(x)  (void) ((x) || (DebugBreak(),0))
    #   define EProtAssert(x)  (void) ((x) || (DebugBreak(),0))
    #   define NotfAssert(x)   (void) ((x) || (DebugBreak(),0))
    
    #   define UrlMkDebugOut(x)
    #   define UrlMkVerify(x)         x
    
    #   define TransDebugOut(x)
    #   define TransVerify(x)         x
    
    #   define PProtDebugOut(x)
    #   define PProtVerify(x)         x
    
    #   define NotfDebugOut(x)
    #   define NotfVerify(x)          x
    
    #   define TNotfDebugOut(x)
    #   define TNotfVerify(x)         x
    
    #else
    
    #   define UrlMkDebugOut(x)
    #   define UrlMkAssert(x)
    #   define UrlMkVerify(x)         x
    
    #   define TransDebugOut(x)
    #   define TransAssert(x)
    #   define TransVerify(x)         x
    
    #   define PProtDebugOut(x)
    #   define PProtAssert(x)
    #   define PProtVerify(x)         x
    
    #   define NotfDebugOut(x)
    #   define NotfAssert(x)
    #   define NotfVerify(x)          x
    
    #   define TNotfDebugOut(x)
    #   define TNotfAssert(x)
    #   define TNotfVerify(x)         x
    
    #   define EProtDebugOut(x)
    #   define EProtAssert(x)
    #   define EProtVerify(x)         x
    
    #endif
    #endif
    
    HRESULT GetClassMime(LPSTR pszMime, CLSID *pclsid, BOOL fIgnoreMimeClsid=FALSE);
    STDAPI GetClassFileOrMime2(LPBC pBC, LPCWSTR pwzFilename, LPVOID pBuffer, DWORD cbSize,
        LPCWSTR pwzMimeIn, DWORD dwReserved, CLSID *pclsid, BOOL fIgnoreMimeClsid);
    HWND GetThreadNotificationWnd(BOOL fCreate = TRUE);
    
    // messages for URLMON's private window on client's thread
    #define WM_URLMON_BASE                  WM_USER+100
    #define WM_TRANS_FIRST                  WM_URLMON_BASE+1
    #define WM_TRANS_PACKET                 WM_URLMON_BASE+1
    #define WM_TRANS_NOPACKET               WM_URLMON_BASE+2
    #define WM_TRANS_OUTOFMEMORY            WM_URLMON_BASE+3
    #define WM_TRANS_INTERNAL               WM_URLMON_BASE+4
    #define WM_CODE_DOWNLOAD_SETUP          WM_URLMON_BASE+5
    #define WM_CODE_DOWNLOAD_TRUST_PIECE    WM_URMLON_BASE+6
    #define WM_CODE_DOWNLOAD_PROCESS_PIECE  WM_URLMON_BASE+7
    #define WM_CODE_DOWNLOAD_PROCESS_INF    WM_URLMON_BASE+8
    #define WM_THREADPACKET_POST            WM_URLMON_BASE+9
    #define WM_THREADPACKET_SEND            WM_URLMON_BASE+10
    #define WM_THREADPACKET_NOTIFY          WM_URLMON_BASE+11
    #define WM_THREADPACKET_INPUTSYNC       WM_URLMON_BASE+12
    #define WM_PROCESSPACKET_POST           WM_URLMON_BASE+13
    #define WM_PROCESSPACKET_SEND           WM_URLMON_BASE+14
    #define WM_PROCESSWAKEUP                WM_URLMON_BASE+15
    #define WM_THREADPACKET_PRIVATE3        WM_URLMON_BASE+16
    #define WM_SYNC_DEF_PROC_NOTIFICATIONS  WM_URLMON_BASE+17
    #define WM_TRANS_LAST                   WM_URLMON_BASE+17

    #define NOTF_SCHED_TIMER                0xABC123
    #define NOTF_DELAY_TIMER                0xDEF456
    
    #define IID_IAsyncURLMoniker    IID_IMoniker
    #define E_RETRY                 RPC_E_RETRY
    
    #ifdef UNUSED
    #undef VDATEPTROUT
    #undef VDATEPTRIN
    #undef VDATEIFACE
    #undef VDATEIID
    
    #define VDATEPTROUT(p, n)
    #define VDATEPTRIN(p, n)
    #define VDATETHIS(t)
    #define VDATEIFACE(x)
    #define VDATEIID(x)
    #endif //UNUSED
    
    #ifndef VDATETHIS
    #define VDATETHIS(t) VDATEIFACE(t)
    #endif
    
    // prototypes
    
    EXTERN_C const IID IID_IAsyncBindCtx;
    
    
    // Internal Helper API's
    void DllAddRef(void);
    void DllRelease(void);
    
    #undef  URLMONOFFSETOF
    #define URLMONOFFSETOF(t,f)   ((DWORD_PTR)(&((t*)0)->f))
   
    #define REG_BSCB_HOLDER          OLESTR("_BSCB_Holder_")
    #define REG_ENUMFORMATETC        OLESTR("_EnumFORMATETC_")
    #define REG_MEDIA_HOLDER         OLESTR("_Media_Holder_")
    #define SZ_TRANSACTIONDATA       OLESTR("_ITransData_Object_")
    #define SZ_TRANSACTION           OLESTR("_ITransaction_Object_")
    #define SZ_BINDING               OLESTR("CBinding Context")
    #define SZ_IUNKNOWN_PTR          OLESTR("IUnknown Pointer")
    
    #if DBG==1
    HRESULT DumpIID(REFIID riid);
    #else
    #define DumpIID(x)
    #endif
    
    // Needed for linking with static C runtime LIBCMT.LIB
    // Remove when linking to external C runtime DLL
    //#define strnicmp _strnicmp
    #define wcsnicmp _wcsnicmp
    //#define itoa _itoa
    //#define stricmp _stricmp        // URLBIND uses this one.
    #define wcsicmp _wcsicmp
    
    // old flags used inside urlmon
    typedef enum
    {
        BSCO_ONSTARTBINDING     = 0x00000001,
        BSCO_GETPRIORITY        = 0x00000002,
        BSCO_ONLOWRESOURCE      = 0x00000004,
        BSCO_ONPROGRESS         = 0x00000008,
        BSCO_ONSTOPBINDING      = 0x00000010,
        BSCO_GETBINDINFO        = 0x00000020,
        BSCO_ONDATAAVAILABLE    = 0x00000040,
        BSCO_ONOBJECTAVAILABLE  = 0x00000080,
        BSCO_ALLONIBSC          = 0x000000FF,
        BSCO_ALLONIBDGSITE      = 0x0000001F
    } BSCO_OPTION;
    
    // flags for the reserved parameter dwReserved of GetClassFileOrMime API
    typedef enum
    {
        GETCLASSFILEORMIME_IGNOREPLUGIN      = 0x00000001
    } GETCLASSFILEORMIME_FLAGS;
    
    #endif //_URLINT_H_
    
    
    
    
    
 

