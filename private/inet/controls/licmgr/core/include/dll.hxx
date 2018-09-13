//+----------------------------------------------------------------------------
//  File:       dll.hxx
//
//  Synopsis:   This file contains the core routines and globals for creating
//              DLLs
//
//-----------------------------------------------------------------------------


#ifndef _DLL_HXX
#define _DLL_HXX

extern HANDLE   g_hinst;            // Module instance HANDLE

// Types ----------------------------------------------------------------------
struct THREADSTATE
{
    THREADSTATE *   ptsNext;

    struct
    {
        DWORD       idThread;
        LONG        cUsage;
        IMalloc *   pmalloc;
    }   dll;
};

typedef HRESULT (*PFN_PATTACH)();
typedef HRESULT (*PFN_TATTACH)(THREADSTATE * pts);
typedef void    (*PFN_PDETACH)();
typedef void    (*PFN_TDETACH)(THREADSTATE * pts);

extern PFN_PATTACH  g_apfnPAttach[];
extern PFN_TATTACH  g_apfnTAttach[];
extern PFN_PDETACH  g_apfnPDetach[];
extern PFN_TDETACH  g_apfnTDetach[];

#define BEGIN_PROCESS_ATTACH            \
extern PFN_PATTACH g_apfnPAttach[] =    \
                {

#define END_PROCESS_ATTACH              \
                NULL                    \
                };

#define BEGIN_PROCESS_DETACH            \
extern PFN_PDETACH g_apfnPDetach[] =    \
                {

#define END_PROCESS_DETACH              \
                NULL                    \
                };

#define BEGIN_THREAD_ATTACH             \
extern PFN_TATTACH g_apfnTAttach[] =    \
                {

#define END_THREAD_ATTACH               \
                NULL                    \
                };

#define BEGIN_THREAD_DETACH             \
extern PFN_TDETACH g_apfnTDetach[] =    \
                {

#define END_THREAD_DETACH               \
                NULL                    \
                };

#define ATTACH_METHOD(x)                \
                x,

#define DETACH_METHOD(x)                \
                x,

typedef void    (*PFN_PPASSIVATE)();
typedef void    (*PFN_TPASSIVATE)(THREADSTATE * pts);

extern PFN_PPASSIVATE   g_apfnPPassivate[];
extern PFN_TPASSIVATE   g_apfnTPassivate[];

#define BEGIN_PROCESS_PASSIVATE             \
extern PFN_PPASSIVATE g_apfnPPassivate[] =  \
                {

#define END_PROCESS_PASSIVATE               \
                NULL                        \
                };

#define BEGIN_THREAD_PASSIVATE              \
extern PFN_TPASSIVATE g_apfnTPassivate[] =  \
                {

#define END_THREAD_PASSIVATE                \
                NULL                        \
                };

#define PASSIVATE_METHOD(x)                 \
                x,

enum LICREQUEST
{
    LICREQUEST_VALIDATE = 1,
    LICREQUEST_OBTAIN,
    LICREQUEST_INFO,
};

typedef HRESULT (*PFN_CLASSFACTORY)(IUnknown * pUnkOuter, REFIID riid, void ** ppvObj);
typedef HRESULT (*PFN_CLASSLICENSE)(LICREQUEST lr, void * pv);
struct CLASSFACTORY
{
    const CLSID *       pclsid;
    PFN_CLASSFACTORY    pfnFactory;
    PFN_CLASSLICENSE    pfnLicense;
};

extern CLASSFACTORY g_acf[];

#define BEGIN_CLASS_FACTORIES   \
extern CLASSFACTORY g_acf[] =   \
                {

#define FACTORY(clsid, pfnFactory, pfnLicense)  \
                { &clsid, pfnFactory, pfnLicense },

#define END_CLASS_FACTORIES             \
                { NULL, NULL, NULL }    \
                };


const DWORD NULL_TLS  = (DWORD)(-1);
const LONG  REF_GUARD = 255;

extern struct GINFO
{
    union
    {
        DWORD   dwFlags;    // All flags
        struct
        {
            DWORD   fUnicode:1;     // System supports Unicode
            DWORD   fWinNT:1;       // System is WinNT
        };
    };
}               g_ginfo;
extern HANDLE   g_heap;     // Process heap


// Classes --------------------------------------------------------------------
//+----------------------------------------------------------------------------
//  Template:   TLock
//
//  Synopsis:   This class provides Enter/LeaveCriticalSection behavior
//              through its constructor/destructor
//
//-----------------------------------------------------------------------------
template <class TYPE> class TLock
{
public:
    TLock()
    {
        EnterCriticalSection(&s_cs);
#ifdef  _DEBUG
        if (!s_cNesting)
            s_tidOwner = GetCurrentThreadId();
        else
            Assert(s_tidOwner == GetCurrentThreadId());
        s_cNesting++;
#endif
    }
    ~TLock()
    {
#ifdef  _DEBUG
        Assert(s_tidOwner == GetCurrentThreadId());
        Assert(s_cNesting > 0);
        s_cNesting--;
#endif
        LeaveCriticalSection(&s_cs);
    }

    static void Init()
    {
        InitializeCriticalSection(&s_cs);
    }
    static void Deinit()
    {
        Assert(!s_cNesting);
        DeleteCriticalSection(&s_cs);
    }

protected:
    static CRITICAL_SECTION s_cs;
#ifdef  _DEBUG
    static LONG             s_cNesting;
    static DWORD            s_tidOwner;
#endif
};

#define DEFINE_LOCK(x)  typedef struct { int i; } x
#define DECLARE_LOCK(x) CRITICAL_SECTION TLock<x>::s_cs;        \
                        Debug(LONG  TLock<x>::s_cNesting = 0);  \
                        Debug(DWORD TLock<x>::s_tidOwner = 0);
#define INIT_LOCK(x)    TLock<x>::Init();
#define DEINIT_LOCK(x)  TLock<x>::Deinit();
#define LOCK(x)         TLock<x>    lock##x;

DEFINE_LOCK(DLL);


// Prototypes ----------------------------------------------------------------
HRESULT GetWin32Hresult();

HRESULT AllocateThreadState(THREADSTATE ** ppts);
HRESULT EnsureThreadState();

inline THREADSTATE * GetThreadState()
{
    extern DWORD g_tlsThreadState;
    Assert(g_tlsThreadState != NULL_TLS);
    Assert(TlsGetValue(g_tlsThreadState));
    return (THREADSTATE *)TlsGetValue(g_tlsThreadState);
}

#define TLS(x)  GetThreadState()->x

void IncrementProcessUsage();
void IncrementThreadUsage();
void DecrementProcessUsage();
void DecrementThreadUsage();

#endif  // _DLL_HXX
