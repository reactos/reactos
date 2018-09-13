//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdutil.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_CDUTIL_HXX_
#define I_CDUTIL_HXX_
#pragma INCMSG("--- Beg 'cdutil.hxx'")

//
// Platform dependent MACROS.
//
//
// these are the features we are turning off - we should coordinate with
// the windows CE folks to get this right, for now I am putting these here.
// - vamshi - 1/14/97.
#ifdef WIN16
#define NO_IME
#define NO_AVI
#define NO_RTF
#define NO_ART          // no ART Image stuff - might have to enable for AOL
#define NO_SCRIPT_DEBUGGER
#define NO_OFFSCREEN
#define NO_METAFILE
#define NO_DATABINDING
#endif // WIN16

// The Watcom Library functions are not cdecl so we use a define RunTimeCalling
// Convention to declare functions/callbacks that involve the C Run time libraries.
// used construct here
#ifdef WIN16

#define RTCCONV
typedef int (WINAPI * STRINGCOMPAREFN)(const TCHAR *, const TCHAR *);

#else

#define RTCCONV __cdecl
typedef int (__stdcall * STRINGCOMPAREFN)(const TCHAR *, const TCHAR *);
#endif

// Win16 has no DirectDraw, so use device-dependent bitmaps
#if defined(WIN16) || defined(WINCE) || defined(UNIX)
#define NODD
#endif // WIN16 || WINCE || UNIX

// HEVENT is the 'return type' of CreateEvent.
// It's always a 32-bit value.
#ifndef WIN16

typedef HANDLE EVENT_HANDLE;
typedef HANDLE THREAD_HANDLE;
#define CloseEvent CloseHandle
#define CloseThread CloseHandle

#define INPROCSERVER "InProcServer32\0"

#else

#define INPROCSERVER "InProcServer\0"

typedef W16HANDLE EVENT_HANDLE;
typedef W16HANDLE THREAD_HANDLE;

#define MB_SETFOREGROUND    0

#undef InitializeCriticalSection
inline InitializeCriticalSection(LPCRITICAL_SECTION pcs) {}
#undef EnterCriticalSection
inline EnterCriticalSection(LPCRITICAL_SECTION pcs) {}
#undef LeaveCriticalSection
inline LeaveCriticalSection(LPCRITICAL_SECTION pcs) {}
#undef DeleteCriticalSection
inline DeleteCriticalSection(LPCRITICAL_SECTION pcs) {}


// Use this class in wndprocs when you need to impersonate a thread.
// One good way to use it is like this:
//  myinfo = (CMyClass*)GetWindowLong(hDlg, GWL_USER);
//  CThreadImpersonate impersonater(myinfo ? myinfo->_dwTID : GetCurrentThreadId());
// Init _dwTID either in the constructor or in the WM_CREATE message.

class CThreadImpersonate
{
public:
    CThreadImpersonate(THREAD_HANDLE dwTID)
    { _dwPrevTID = w16ImpersonateThread(dwTID); }

    ~CThreadImpersonate()
    { w16ImpersonateThread(_dwPrevTID); }
private:
    THREAD_HANDLE _dwPrevTID;
};

#endif

// Some one liners can be wrapped in IF_WIN16 or IF_WIN32 so that the
// code is more readable.
#ifdef WIN16
#define IF_WIN16(x) x
#define IF_NOT_WIN16(x)
#define IF_WIN32(x)
#else
#define IF_WIN16(x)
#define IF_NOT_WIN16(x) x
#define IF_WIN32(x) x
#endif


// At various places in the code (TCHAR *ptrA - TCHAR *ptrB) is done.
// The Ptr. difference is not calculated correctly in Win16 as the
// compiler takes into account only the offset & not the segment. Looks
// like it needs HUGE ptrs to do that. But using huge ptrs is bad for performance
// so we resort to this macro. As usual this is a no-op for Win 32.
#ifdef WIN16
#define PTR_DIFF(x, y)   ((DWORD)(x) - (DWORD)(y))
#else
#define PTR_DIFF(x, y)   ((x) - (y))
#endif

#ifdef _M_IA64
#define SPEED_OPTIMIZE_FLAGS "tg"       // flags used for local speed optimisation in #pragma optimize
#else
#define SPEED_OPTIMIZE_FLAGS "tyg"      // flags used for local speed optimisation in #pragma optimize
#endif

class CDoc;
class CServer;                      // forward declaration
class CDocInfo;
class CFormDrawInfo;
class CSite;
class CElement;

//+------------------------------------------------------------------------
// Performance tags and metering
//-------------------------------------------------------------------------

#ifndef X_MSHTMDBG_H_
#define X_MSHTMDBG_H_
#pragma INCMSG("--- Beg <mshtmdbg.h>")
#include <mshtmdbg.h>
#pragma INCMSG("--- End <mshtmdbg.h>")
#endif

extern HTMPERFCTL * g_pHtmPerfCtl;

//+------------------------------------------------------------------------
// Memory allocation
//-------------------------------------------------------------------------

MtExtern(Mem)

EXTERN_C void *  _MemAlloc(ULONG cb);
EXTERN_C void *  _MemAllocClear(ULONG cb);
EXTERN_C HRESULT _MemRealloc(void ** ppv, ULONG cb);
EXTERN_C ULONG   _MemGetSize(void * pv);
EXTERN_C void    _MemFree(void * pv);
HRESULT          _MemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemAllocString(ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemReplaceString(LPCTSTR pchSrc, LPTSTR * ppchDest);
#define          _MemFreeString(pch) _MemFree(pch)
void __cdecl     _MemSetName(void * pv, char * szFmt, ...);
char *           _MemGetName(void * pv);

EXTERN_C void *  _MtMemAlloc(PERFMETERTAG mt, ULONG cb);
EXTERN_C void *  _MtMemAllocClear(PERFMETERTAG mt, ULONG cb);
EXTERN_C HRESULT _MtMemRealloc(PERFMETERTAG mt, void ** ppv, ULONG cb);
EXTERN_C ULONG   _MtMemGetSize(void * pv);
EXTERN_C void    _MtMemFree(void * pv);
HRESULT          _MtMemAllocString(PERFMETERTAG mt, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MtMemAllocString(PERFMETERTAG mt, ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MtMemReplaceString(PERFMETERTAG mt, LPCTSTR pchSrc, LPTSTR * ppchDst);
#define          _MtMemFreeString(pch) _MtMemFree(pch)
void __cdecl     _MtMemSetName(void * pv, char * szFmt, ...);
char *           _MtMemGetName(void * pv);
int              _MtMemGetMeter(void * pv);
void             _MtMemSetMeter(void * pv, PERFMETERTAG mt);

EXTERN_C void *  _MgMemAlloc(ULONG cb);
EXTERN_C void *  _MgMemAllocClear(ULONG cb);
EXTERN_C HRESULT _MgMemRealloc(void ** ppv, ULONG cb);
EXTERN_C ULONG   _MgMemGetSize(void * pv);
EXTERN_C void    _MgMemFree(void * pv);
HRESULT          _MgMemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MgMemAllocString(ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MgMemReplaceString(LPCTSTR pchSrc, LPTSTR * ppchDst);
#define          _MgMemFreeString(pch) _MgMemFree(pch)

#ifdef PERFMETER

#define MemAlloc(mt, cb)                            _MtMemAlloc(mt, cb)
#define MemAllocClear(mt, cb)                       _MtMemAllocClear(mt, cb)
#define MemRealloc(mt, ppv, cb)                     _MtMemRealloc(mt, ppv, cb)
#define MemGetSize(pv)                              _MtMemGetSize(pv)
#define MemFree(pv)                                 _MtMemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MtMemAllocString(mt, pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MtMemAllocString(mt, cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MtMemReplaceString(mt, pch, ppch)
#define MemFreeString(pch)                          _MtMemFreeString(pch)
#define MemGetMeter(pv)                             _MtMemGetMeter(pv)
#define MemSetMeter(pv, mt)                         _MtMemSetMeter(pv, mt)

#elif defined(MEMGUARD)

#define MemAlloc(mt, cb)                            _MgMemAlloc(cb)
#define MemAllocClear(mt, cb)                       _MgMemAllocClear(cb)
#define MemRealloc(mt, ppv, cb)                     _MgMemRealloc(ppv, cb)
#define MemGetSize(pv)                              _MgMemGetSize(pv)
#define MemFree(pv)                                 _MgMemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MgMemAllocString(pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MgMemAllocString(cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MgMemReplaceString(pch, ppch)
#define MemFreeString(pch)                          _MgMemFreeString(pch)
#define MemGetMeter(pv)                             0
#define MemSetMeter(pv, mt)

#else

#define MemAlloc(mt, cb)                            _MemAlloc(cb)
#define MemAllocClear(mt, cb)                       _MemAllocClear(cb)
#define MemRealloc(mt, ppv, cb)                     _MemRealloc(ppv, cb)
#define MemGetSize(pv)                              _MemGetSize(pv)
#define MemFree(pv)                                 _MemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MemAllocString(pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MemAllocString(cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MemReplaceString(pch, ppch)
#define MemFreeString(pch)                          _MemFreeString(pch)
#define MemGetMeter(pv)                             0
#define MemSetMeter(pv, mt)

#endif

#if DBG==1
    #ifdef PERFMETER
        #define MemGetName(pv)              _MtMemGetName(pv)
        #define MemSetName(x)               _MtMemSetName x
    #else
        #define MemGetName(pv)              _MemGetName(pv)
        #define MemSetName(x)               _MemSetName x
    #endif
#else
    #define MemGetName(pv)
    #define MemSetName(x)
#endif

HRESULT TaskAllocString(const TCHAR *pstrSrc, TCHAR **ppstrDest);
HRESULT TaskReplaceString(const TCHAR * pstrSrc, TCHAR **ppstrDest);

MtExtern(OpNew)

#ifdef PERFMETER
       void * __cdecl UseOperatorNewWithMemoryMeterInstead(size_t cb);
inline void * __cdecl operator new(size_t cb)           { return UseOperatorNewWithMemoryMeterInstead(cb); }
inline void * __cdecl operator new[](size_t cb)         { return UseOperatorNewWithMemoryMeterInstead(cb); }
#else
inline void * __cdecl operator new(size_t cb)           { return MemAlloc(Mt(OpNew), cb); }
#ifndef UNIX // UNIX can't take new[] and delete[]
inline void * __cdecl operator new[](size_t cb)         { return MemAlloc(Mt(OpNew), cb); }
#endif // UNIX
#endif

inline void * __cdecl operator new(size_t cb, PERFMETERTAG mt)   { return MemAlloc(mt, cb); }
#ifndef UNIX
inline void * __cdecl operator new[](size_t cb, PERFMETERTAG mt) { return MemAlloc(mt, cb); }
#endif
inline void * __cdecl operator new(size_t cb, void * pv){ return pv; }
inline void   __cdecl operator delete(void *pv)         { MemFree(pv); }
#ifndef UNIX
inline void   __cdecl operator delete[](void *pv)       { MemFree(pv); }
#endif

inline void TaskFreeString(LPVOID pstr)
        { CoTaskMemFree(pstr); }

#ifndef UNIX
#define DECLARE_MEMALLOC_NEW_DELETE(mt) \
    inline void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAlloc(mt, cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE(mt) \
    inline void * __cdecl operator new(size_t cb) { return(MemAllocClear(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAllocClear(mt, cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMMETER_NEW \
    inline void * __cdecl operator new(size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); }
#else
#define DECLARE_MEMALLOC_NEW_DELETE(mt) \
    void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); } \
    void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE(mt) \
    void * __cdecl operator new(size_t cb) { return(MemAllocClear(mt, cb)); } \
    void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMMETER_NEW \
    void * __cdecl operator new(size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); }
#endif //UNIX
// Unicode signatures

#ifdef BIG_ENDIAN
#  define NATIVE_UNICODE_SIGNATURE    UNICODE_SIGNATURE_BIGENDIAN
#  define NONNATIVE_UNICODE_SIGNATURE UNICODE_SIGNATURE_LITTLEENDIAN
#else
#  define NATIVE_UNICODE_SIGNATURE    UNICODE_SIGNATURE_LITTLEENDIAN
#  define NONNATIVE_UNICODE_SIGNATURE UNICODE_SIGNATURE_BIGENDIAN
#endif

#ifdef UNIX
#  define NATIVE_UNICODE_CODEPAGE               CP_UCS_4
#  define NONNATIVE_UNICODE_CODEPAGE            CP_UCS_2
#  define NATIVE_UNICODE_CODEPAGE_BIGENDIAN     CP_UCS_4_BIGENDIAN
#  define NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN  CP_UCS_2_BIGENDIAN
#else
#  define NATIVE_UNICODE_CODEPAGE               CP_UCS_2
#  define NONNATIVE_UNICODE_CODEPAGE            CP_UCS_4
#  define NATIVE_UNICODE_CODEPAGE_BIGENDIAN     CP_UCS_2_BIGENDIAN
#  define NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN  CP_UCS_4_BIGENDIAN
#endif

#ifdef UNIX

#define UNICODE_SIGNATURE_LITTLEENDIAN 0xfefffeff
#define UNICODE_SIGNATURE_BIGENDIAN    0xfffefffe

#else

#define UNICODE_SIGNATURE_LITTLEENDIAN 0xfeff
#define UNICODE_SIGNATURE_BIGENDIAN    0xfffe

#endif
//+------------------------------------------------------------------------
// Useful Macros
//-------------------------------------------------------------------------

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

//+------------------------------------------------------------------------
//
// DYNCAST macro
//
// Use to cast objects from one class type to another. This should be used
// rather than using standard casts.
//
// Example:
//         CBodyElement *pBody = (CBodyElement*)_pElement;
//
//      is replaced by:
//
//         CBodyElement *pBody = DYNCAST(CBodyElement, _pElement);
//
// The dyncast macro will assert if _pElement is not really a CBodyElement.
//
// For ship builds the DYNCAST macro expands to a standard cast.
//
//-------------------------------------------------------------------------

#if DBG != 1 || defined(WIN16) || defined(WINCE) || defined(NO_RTTI)

#ifdef UNIX
#define DYNCAST(Dest_type, Source_Value) ((Dest_type*)(Source_Value))
#else
#define DYNCAST(Dest_type, Source_Value) (static_cast<Dest_type*>(Source_Value))
#endif

#else // DBG == 1

#ifndef X_TYPEINFO_H_
#define X_TYPEINFO_H_
#pragma INCMSG("--- Beg <typeinfo.h>")
#include <typeinfo.h>
#pragma INCMSG("--- End <typeinfo.h>")
#pragma warning(disable:4244)
#endif

extern char g_achDynCastMsg[];
extern char *g_pszDynMsg;
extern char *g_pszDynMsg2;

template <class TS, class TD>
TD * DYNCAST_IMPL (TS * source, TD &, char* pszType)
{
    if (!source) return NULL;

    TD * dest  = dynamic_cast <TD *> (source);
    TD * dest2 = static_cast <TD *> (source);
    if (!dest)
    {
        wsprintfA(g_achDynCastMsg, g_pszDynMsg, typeid(*source).name(), pszType);
        AssertSz(FALSE, g_achDynCastMsg);
    }
    else if (dest != dest2)
    {
        wsprintfA(g_achDynCastMsg, g_pszDynMsg2, typeid(*source).name(), pszType);
        AssertSz(FALSE, g_achDynCastMsg);
    }

    return dest2;
}

#define DYNCAST(Dest_type, Source_value) \
    DYNCAST_IMPL(Source_value,(Dest_type &)*(Dest_type*)NULL, #Dest_type)

#endif // ifdef DBG != 1


//+------------------------------------------------------------------------
//
// Min and max templates
//
// Warning, Arguments must be cast to same types for template instantiation
//
//-------------------------------------------------------------------------

#ifndef WIN16

#ifdef min
#undef min
#endif

template < class T > inline T min ( T a, T b ) { return a < b ? a : b; }

#ifdef max
#undef max
#endif

template < class T > inline T max ( T a, T b ) { return a > b ? a : b; }

#endif

int BSearch(const BYTE * pb, const int c, const unsigned long l, const int cb=4,
            const int ob=0);

HRESULT GetBStrFromStream(IStream * pIStream, BSTR * pbstr,
                          BOOL fStripTrailingCRLF);

BOOL    FormsRectInRegion(HRGN hrgn, RECT * prc);

//+------------------------------------------------------------------------
//
//  Locale-correct implementations for the string compare functions
//  Added benefit is getting rid of the C runtime baggage.
//
//  Implementation lives in strcmp.c
//
//-------------------------------------------------------------------------

//  Include tchar.h so that someone including it later again
//  do not clobber the replacements.

#ifndef X_TCHAR_H_
#define X_TCHAR_H_
#pragma INCMSG("--- Beg <tchar.h>")
#include <tchar.h>
#pragma INCMSG("--- End <tchar.h>")
#endif

#ifdef WIN16
#undef _tcsncmp
#define _tcsncmp(s1, l1, s2, l2)        strncmp(s1, s2, (unsigned)l1 > (unsigned)l2 ? l2 : l1)

#undef _tcsnicmp
#define _tcsnicmp(s1, l1, s2, l2)       _strnicmp(s1, s2, (unsigned)l1 > (unsigned)l2 ? l2 : l1)

#else

#undef _tcscmp
#undef _tcsicmp
#ifndef WINCE
#undef _wcsicmp
#endif
#undef _tcsncmp
#undef _tcsnicmp

#undef _istspace
#undef _istdigit
#undef _istalpha
#undef _istalnum
#undef _istxdigit
#undef _istprint

#undef isdigit
#undef isalpha
#undef isspace
#undef iswspace

extern "C" int __cdecl isdigit(int ch);
extern "C" int __cdecl isalpha(int ch);
extern "C" int __cdecl isspace(int ch);

// UNIX: Error: Only one of a set of overloaded functions can be "C" or "Pascal". See /usr/include/wchar.h
#ifndef UNIX
extern "C" int __cdecl iswspace(wchar_t ch);
#endif

#if defined(_MAC) && !defined(_MACUNICODE)
extern "C" int _tcscmp  (const TCHAR *string1, const TCHAR *string2);
extern "C" int _tcsicmp (const TCHAR *string1, const TCHAR *string2);
extern "C" int _wcsicmp (const TCHAR *string1, const TCHAR *string2);
extern "C" int _tcsncmp (const TCHAR *string1, int cch1, const TCHAR *string2, int cch2);
extern "C" int _tcsnicmp(const TCHAR *string1, int cch1, const TCHAR *string2, int cch2);
extern "C" int _istspace  (TCHAR ch);
extern "C" int _istdigit  (TCHAR ch);
extern "C" int _istalpha  (TCHAR ch);
extern "C" int _istalnum  (TCHAR ch);
extern "C" int _istxdigit  (TCHAR ch);
extern "C" int _istprint  (TCHAR ch);
#else
int _cdecl _tcscmp  (const TCHAR *string1, const TCHAR *string2);
int _cdecl _tcsicmp (const TCHAR *string1, const TCHAR *string2);
const TCHAR * __cdecl _tcsistr (const TCHAR * wcs1,const TCHAR * wcs2);
#ifndef WINCE
int _cdecl _wcsicmp (const TCHAR *string1, const TCHAR *string2);
#endif
int _cdecl _tcsncmp (const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);
int _cdecl _tcsnicmp(const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);
int _cdecl _istspace  (TCHAR ch);
int _cdecl _istdigit  (TCHAR ch);
int _cdecl _istalpha  (TCHAR ch);
int _cdecl _istalnum  (TCHAR ch);
int _cdecl _istxdigit  (TCHAR ch);
int _cdecl _istprint  (TCHAR ch);
#endif // defined(_MAC)
#endif // !WIN16

BOOL _tcsequal(const TCHAR *string1, const TCHAR *string2);
BOOL _tcsiequal(const TCHAR *string1, const TCHAR *string2);
BOOL _tcsnpre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);
BOOL _tcsnipre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);
BOOL _7csnipre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);

// Special implementation _ttol except it returns an error if the string to
// convert isn't a number.
HRESULT ttol_with_error ( LPCTSTR pStr, long *plValue );


//+------------------------------------------------------------------------
//
//  Class:      CVoid
//
//  Synopsis:   A class for declaring pointers to member functions.
//
//-------------------------------------------------------------------------

class CVoid
{
};

//
// WIN16:
//
// See dev handbook for an explanation of this.
// BUGWIN16 - need to writeup the explanation in dev handbook
// vamshi - 3/15/97.
//#ifdef WIN16
//#define CVOID_CAST(pDerObject)  ((CVoid *)pDerObject)
//#else
#define CVOID_CAST(pDerObject)  ((CVoid *)(void *)pDerObject)
//#endif


// BUGBUG - Delete BUGCALL when we switch to VC3.
// The compiler generates the same names for stdcall and thiscall
// virtual thunks.  Because of this bug, we must make all pointer
// to members be of the same type (thiscall or stdcall).  Since
// we need stdcall for tearoff interfaces, we standardize on
// stdcall.  The BUGCALL macro is used to declare functions as
// stdcall when the function should really be thiscall.  This
// compiler bug has been reported and will be fixed in vc3.
#define BUGCALL STDMETHODCALLTYPE


//+------------------------------------------------------------------------
//
//  misc.cxx
//
//-------------------------------------------------------------------------

HRESULT GetLastWin32Error(void);

// This used to call an inline routine on i386 platform but the OS already
// does a good job on this routine and is more robust, so we will always
// call the OS...

// N.B. The OS MulDiv call returns -1 if we divide by zero.  This is *always*
// wrong for our code.  For now, return zero on a divide by zero.  Coded this
// way so that the ship build does not complain.

#ifdef WIN16
// This is defined in win16x.h
//long WINAPI MulDivQuick(long nMultiplicand, long nMultiplier, long nDivisor);
#else
inline int MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor)
        { Assert(nDivisor); return (!nDivisor-1) & MulDiv(nMultiplicand, nMultiplier, nDivisor); }
#endif

//+------------------------------------------------------------------------
//
//  Dispatch utilites in disputil.cxx/disputl2.cxx
//
//-------------------------------------------------------------------------

HRESULT GetTypeInfoFromCoClass(ITypeInfo * pTICoClass, BOOL fSource,
                               ITypeInfo ** ppTI, IID * piid);
void    GetFormsTypeLibPath(TCHAR * pch);
HRESULT GetFormsTypeLib(ITypeLib **ppTL, BOOL fNoCache = FALSE);

HRESULT ValidateInvoke(
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr);

HRESULT ReadyStateInvoke(DISPID dispid,
                         WORD wFlags,
                         long lReadyState,
                         VARIANT * pvarResult);

inline void
InitEXCEPINFO(EXCEPINFO * pEI)
{
    if (pEI)
        memset(pEI, 0, sizeof(*pEI));
}

void FreeEXCEPINFO(EXCEPINFO * pEI);

HRESULT LoadF3TypeInfo(REFCLSID clsid, ITypeInfo ** ppTI);
HRESULT DispatchGetTypeInfo(
                REFIID riidInterface,
                UINT itinfo,
                LCID lcid,
                ITypeInfo ** ppTI);
HRESULT DispatchGetTypeInfoCount(UINT * pctinfo);
HRESULT DispatchGetIDsOfNames(
                REFIID riidInterface,
                REFIID riid,
                OLECHAR ** rgszNames,
                UINT cNames,
                LCID lcid,
                DISPID * rgdispid);

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper
//
//---------------------------------------------------------------------------

class CCreateTypeInfoHelper
{
public:

    //
    // methods
    //

    CCreateTypeInfoHelper();
    ~CCreateTypeInfoHelper();
    HRESULT Start(REFIID riid);
    HRESULT Finalize(LONG lImplTypeFlags);

    //
    // data
    //

#ifdef WIN16
    ICreateTypeLib *    pTypLib;
#else
    ICreateTypeLib2 *   pTypLib;
#endif
    ITypeLib *          pTypLibStdOLE;
    ITypeInfo *         pTypInfoStdOLE;

    ICreateTypeInfo *   pTypInfoCoClass;
    ICreateTypeInfo *   pTypInfoCreate;

    ITypeInfo *         pTIOut;
    ITypeInfo *         pTICoClassOut;

    HREFTYPE            hreftype;
};

class CVariant : public VARIANT
{
public:
    CVariant()  { ZeroVariant(); }
    CVariant(VARTYPE vt) { ZeroVariant(); V_VT(this) = vt; }
    ~CVariant() { Clear(); }

    void ZeroVariant()
    {
        memset(this, 0, sizeof(CVariant));
    }

    HRESULT Copy(VARIANT *pVar)
    {
        Assert(pVar);
        return VariantCopy(this, pVar);
    }

    HRESULT Clear()
    {
        return VariantClear(this);
    }

    // Coerce from an arbitrary variant into this. (Copes with VATIANT BYREF's within VARIANTS).
    HRESULT CoerceVariantArg (VARIANT *pArgFrom, WORD wCoerceToType);
    // Coerce current variant into itself
    HRESULT CoerceVariantArg (WORD wCoerceToType);
    BOOL CoerceNumericToI4 ();
    BOOL IsEmpty() { return (vt == VT_NULL || vt == VT_EMPTY);}
    BOOL IsOptional() { return (IsEmpty() || vt == VT_ERROR);}
};

class CExcepInfo : public EXCEPINFO
{
public:
    CExcepInfo()  { InitEXCEPINFO(this); }
    ~CExcepInfo() { FreeEXCEPINFO(this); }
};

//+--------------------------------------------------------------------------
//
//  Class:      CInvoke
//
//  Synopsis:   helper class to invoke IDispatch, IDispatchEx, CBase
//
//---------------------------------------------------------------------------

interface IDispatchEx;
interface IDispatch;
class CBase;

class CInvoke
{
public:

    CInvoke();
    CInvoke(IDispatchEx * pdispex);
    CInvoke(IDispatch * pdisp);
    CInvoke(IUnknown * punk);
    CInvoke(CBase * pBase);

    ~CInvoke();

    HRESULT Init(IDispatchEx * pdispex);
    HRESULT Init(IDispatch * pdisp);
    HRESULT Init(IUnknown * punk);
    HRESULT Init(CBase * pBase);

    void Clear();
    void ClearArgs();
    void ClearRes();

    HRESULT Invoke (DISPID dispid, WORD wFlags);

    HRESULT AddArg();
    HRESULT AddArg(VARIANT * pvarArg);

    HRESULT AddNamedArg(DISPID dispid);

    inline VARIANT * Arg(UINT i)
    {
        Assert (i <= _dispParams.cArgs);
        return &_aryvarArg[i];
    }

    inline VARIANT * Res()
    {
        return &_varRes;
    }

    //
    // data
    //

    IDispatchEx *   _pdispex;
    IDispatch *     _pdisp;

    VARIANT         _aryvarArg[3];              // CONSIDER making these two growable CStackArray-s
    DISPID          _arydispidNamedArg[2];
    VARIANT         _varRes;
    DISPPARAMS      _dispParams;
    EXCEPINFO       _excepInfo;
};


//+------------------------------------------------------------------------
// OLE Controls stuff
//-------------------------------------------------------------------------

#define TRISTATE_FALSE triUnchecked
#define TRISTATE_TRUE triChecked
#define TRISTATE_MIXED triGray


//+------------------------------------------------------------------------
// Winuser.h stuff   Defined only in later releases of winuser.h
// BUGBUG(MohanB) We may want to formalize this (a la shell's apithk stuff)
//-------------------------------------------------------------------------
#if (_WIN32_WINNT < 0x0500)

#define WM_APPCOMMAND                   0x0319

#define WM_GETOBJECT                    0x003D

#define WM_CHANGEUISTATE                0x0127
#define WM_UISTATEUPDATE                0x0128
#define WM_QUERYUISTATE                 0x0129

/*
 * LOWORD(wParam) values in WM_*UISTATE*
 */

#define UIS_SET         1
#define UIS_CLEAR       2
#define UIS_INITIALIZE  3

/*
 * HIWORD(wParam) values in WM_*UISTATE*
 */

#define UISF_HIDEFOCUS   0x1
#define UISF_HIDEACCEL   0x2

#endif  // (_WIN32_WINNT >= 0x0500)

//+------------------------------------------------------------------------
//
//  AddPages -- adds property pages to those provided by an
//              implementation of ISpecifyPropertyPages::GetPages.
//
//-------------------------------------------------------------------------

typedef struct tagCAUUID CAUUID;

STDAPI AddPages(
        IUnknown *      pUnk,
        const UUID *    apUUID[],
        CAUUID *        pcauuid);


//+---------------------------------------------------------------------
//
//  VB helpers
//
//----------------------------------------------------------------------

#define VB_LBUTTON 1
#define VB_RBUTTON 2
#define VB_MBUTTON 4

#define VB_SHIFT   1
#define VB_CONTROL 2
#define VB_ALT     4

short  VBButtonState(WPARAM);
short  VBShiftState();
short  VBShiftState(DWORD grfKeyState);


//+------------------------------------------------------------------------
//
//  Class:      CCurs (Curs)
//
//  Purpose:    System cursor stack wrapper class.  Creating one of these
//              objects pushes a new system cursor (the wait cursor, the
//              arrow, etc) on top of a stack; destroying the object
//              restores the old cursor.
//
//  Interface:  Constructor     Pushes a new cursor
//              Destructor      Pops the old cursor back
//
//-------------------------------------------------------------------------
class CCurs
{
public:
    CCurs(LPCTSTR idr);
    ~CCurs(void);

private:
    HCURSOR     _hcrs;
    HCURSOR     _hcrsOld;
};


//+------------------------------------------------------------------------
//
//  Miscellaneous time helper API's
//
//-------------------------------------------------------------------------

ULONG NextEventTime(ULONG ulDelta);
BOOL  IsTimePassed(ULONG ulTime);


//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))

#define CTL_E_METHODNOTAPPLICABLE  STD_CTL_SCODE(444)
#define CTL_E_CANTMOVEFOCUSTOCTRL  STD_CTL_SCODE(2110)
#define CTL_E_CONTROLNEEDSFOCUS    STD_CTL_SCODE(2185)
#define CTL_E_INVALIDPICTURETYPE   STD_CTL_SCODE(485)
#define CTL_E_INVALIDPASTETARGET   CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 0 )
#define CTL_E_INVALIDPASTESOURCE   CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 1 )
#define CTL_E_MISMATCHEDTAG        CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 2 )
#define CTL_E_INCOMPATIBLEPOINTERS CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 3 )
#define CTL_E_UNPOSITIONEDPOINTER  CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 4 )
#define CTL_E_UNPOSITIONEDELEMENT  CUSTOM_CTL_SCODE( CTL_E_CUSTOM_FIRST + 5 )

//-------------------------------------------------------------------------
//  Macros for declaring property get/set methods
//-------------------------------------------------------------------------

#define DECLARE_PROPERTY_GETSETMETHODS(_PropType_, _PropName_)              \
                                                                            \
STDMETHOD(Get##_PropName_) (_PropType_ *);                                  \
STDMETHOD(Set##_PropName_) (_PropType_);


//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------

#define ULREF_IN_DESTRUCTOR 256

#define DECLARE_FORMS_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);


#define DECLARE_FORMS_STANDARD_IUNKNOWN(cls)                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            return ++_ulRefs;                                       \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (--_ulRefs == 0)                                     \
            {                                                       \
                _ulRefs = ULREF_IN_DESTRUCTOR;                      \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }                                                           \
    ULONG GetRefs(void)                                             \
        { return _ulRefs; }


#define DECLARE_AGGREGATED_IUNKNOWN(cls)                            \
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)            \
        { return _pUnkOuter->QueryInterface(iid, ppv); }            \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return _pUnkOuter->AddRef(); }                            \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return _pUnkOuter->Release(); }

#define DECLARE_PLAIN_IUNKNOWN(cls)                                 \
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)            \
        { return PrivateQueryInterface(iid, ppv); }                 \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return PrivateAddRef(); }                                 \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return PrivateRelease(); }

interface IPrivateUnknown
{
public:
#ifdef _MAC
   STDMETHOD(DummyMethodForMacInterface) ( void);
#endif
   STDMETHOD(PrivateQueryInterface) (REFIID riid, void ** ppv) = 0;
   STDMETHOD_(ULONG, PrivateAddRef) () = 0;
   STDMETHOD_(ULONG, PrivateRelease) () = 0;
};
#ifdef _MAC
inline STDMETHODIMP IPrivateUnknown::DummyMethodForMacInterface(void)
{
    return S_OK;
}
#endif

#define DECLARE_SUBOBJECT_IUNKNOWN(parent_cls, parent_mth)          \
    DECLARE_FORMS_IUNKNOWN_METHODS                                  \
    parent_cls * parent_mth();                                      \
    BOOL IsMyParentAlive();

#define DECLARE_SUBOBJECT_IUNKNOWN_NOQI(parent_cls, parent_mth)     \
    STDMETHOD_(ULONG, AddRef) ();                                   \
    STDMETHOD_(ULONG, Release) ();                                  \
    parent_cls * parent_mth();                                      \
    BOOL IsMyParentAlive();

#define IMPLEMENT_SUBOBJECT_IUNKNOWN(cls, parent_cls, parent_mth, member) \
    inline parent_cls * cls::parent_mth()                           \
    {                                                               \
        return CONTAINING_RECORD(this, parent_cls, member);         \
    }                                                               \
    inline BOOL cls::IsMyParentAlive(void)                          \
        { return parent_mth()->GetObjectRefs() != 0; }              \
    STDMETHODIMP_(ULONG) cls::AddRef( )                             \
        { return parent_mth()->SubAddRef(); }                       \
    STDMETHODIMP_(ULONG) cls::Release( )                            \
        { return parent_mth()->SubRelease(); }


#define DECLARE_FORMS_SUBOBJECT_IUNKNOWN(parent_cls)\
    DECLARE_SUBOBJECT_IUNKNOWN(parent_cls, My##parent_cls)
#define DECLARE_FORMS_SUBOBJECT_IUNKNOWN_NOQI(parent_cls)\
    DECLARE_SUBOBJECT_IUNKNOWN_NOQI(parent_cls, My##parent_cls)
#define IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(cls, parent_cls, member) \
    IMPLEMENT_SUBOBJECT_IUNKNOWN(cls, parent_cls, My##parent_cls, member)

//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------

#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&)

#if defined(_MSC_VER) && (_MSC_VER>=1100)
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif

//+---------------------------------------------------------------------
//
//  SIZEF
//
//----------------------------------------------------------------------

typedef struct tagSIZEF
{
    float cx;
    float cy;

} SIZEF;

//+---------------------------------------------------------------------
//
//  Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

#define HIMETRIC_PER_INCH   2540L
#define POINT_PER_INCH      72L
#define TWIPS_PER_POINT     20L
#define TWIPS_PER_INCH      (POINT_PER_INCH * TWIPS_PER_POINT)
#define TWIPS_FROM_POINTS(points) ((TWIPS_PER_INCH*(points))/POINT_PER_INCH)

long HimetricFromHPix(int iPix);
long HimetricFromVPix(int iPix);
int HPixFromHimetric(long lHi);
int VPixFromHimetric(long lHi);
float UserFromHimetric(long lValue);
long  HimetricFromUser(float flt);

//+---------------------------------------------------------------------------
//
//  Useful rectangle utilities.
//
//----------------------------------------------------------------------------

// OLERECTs are for standard OLE interfaces. And are synonymous to RECT.
// the one with ints as members.
// Private interfaces should use RECTL.

// GDIRECTs are also 'short' on win16, 'long' on win32.

// Size and point work the same way.

#ifndef WIN16
typedef RECT GDIRECT, OLERECT, *POLERECT, *LPOLERECT, *LPGDIRECT;
typedef const RECT COLERECT;
typedef COLERECT *PCOLERECT, *LPCOLERECT;
typedef SIZE OLESIZE;
typedef POINT GDIPOINT;

#define SIZES SIZE

#define ENSUREOLERECT(x) (x)
#define ENSUREOLESIZE(x) (x)

#else
typedef RECTword GDIRECT, OLERECT, *POLERECT, *LPOLERECT, *LPGDIRECT;
typedef const RECTword COLERECT;
typedef COLERECT *PCOLERECT, *LPCOLERECT;
typedef SIZEword OLESIZE;
typedef POINTword GDIPOINT;

#define ENSUREOLERECT(x) (CRectAutoconvert) (x)
#define ENSUREOLESIZE(x) (CSizeAutoconvert) (x)

#endif

inline long RectArea(long top, long bottom, long left, long right)
{
    return (bottom - top + 1) * (right - left  + 1);
}

inline long RectArea(RECT * prc)
{
    return RectArea(prc->top, prc->bottom, prc->left, prc->right);
}

#define MAX_INVAL_RECTS   50      // max number of rects we will bother combine before just doing one big one.
void CombineRectsAggressive(int * pcRects, RECT * arc);
void CombineRects(int * pcRects, RECT * arc);

#if !defined(WIN16) && !defined(UNIX)
inline BOOL WINAPI SetRect(LPRECT prc, int xLeft, int yTop,
    int xRight, int yBottom)
{
    prc->left = xLeft;
    prc->top = yTop;
    prc->right = xRight;
    prc->bottom = yBottom;
    return TRUE;
}
#endif // !WIN16

inline BOOL SetRectl(LPRECTL prcl, int xLeft, int yTop,
    int xRight, int yBottom)
{
    prcl->left = xLeft;
    prcl->top = yTop;
    prcl->right = xRight;
    prcl->bottom = yBottom;
    return TRUE;
}

inline BOOL SetRectlEmpty(LPRECTL prcl)
{
#ifdef WIN16
    SetRectEmpty((RECT *)prcl);
    return TRUE;
#else
    return SetRectEmpty((RECT *)prcl);
#endif// !WIN16
}

inline BOOL IntersectRectl(LPRECTL prcDst,
        CONST RECTL *prcSrc1, CONST RECTL *prcSrc2)
{
    return IntersectRect((LPRECT)prcDst,
        (RECT*)prcSrc1, (RECT*)prcSrc2);
}

inline BOOL PtlInRectl(RECTL *prc, POINTL ptl)
{
    return PtInRect((RECT*)prc, *((POINT*)&ptl));
}

inline BOOL InflateRectl(RECTL *prcl, long xAmt, long yAmt)
{
#ifdef WIN16
    InflateRect((RECT *)prcl, (int)xAmt, (int)yAmt);
    return TRUE;
#else
    return InflateRect((RECT *)prcl, (int)xAmt, (int)yAmt);
#endif// !WIN16
}

inline BOOL OffsetRectl(RECTL *prcl, long xAmt, long yAmt)
{
#ifdef WIN16
    OffsetRect((RECT *)prcl, (int)xAmt, (int)yAmt);
    return TRUE;
#else
    return OffsetRect((RECT *)prcl, (int)xAmt, (int)yAmt);
#endif// !WIN16
}

inline BOOL IsRectlEmpty(RECTL *prcl)
{
    return IsRectEmpty((RECT *)prcl);
}

inline BOOL UnionRectl(RECTL *prclDst, const RECTL *prclSrc1, const RECTL *prclSrc2)
{
    return UnionRect((RECT *)prclDst, (RECT *)prclSrc1, (RECT *)prclSrc2);
}

BOOL BoundingRectl(RECTL *prclDst, const RECTL *prclSrc1, const RECTL *prclSrc2);

inline BOOL BoundingRect(RECT *prcDst, const RECT *prcSrc1, const RECT *prcSrc2)
{
    return BoundingRectl((RECTL *)prcDst, (RECTL *)prcSrc1, (RECTL *)prcSrc2);
}

inline BOOL EqualRectl(const RECTL *prcl1, const RECTL *prcl2)
{
    return EqualRect((RECT *)prcl1, (RECT *)prcl2);
}

inline int ExcludeClipRect(HDC hdc, RECT * prc)
{
    return ::ExcludeClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
}

inline BOOL ContainsRect(const RECT * prcOuter, const RECT * prcInner)
{
    return ((prcOuter)->left   <= (prcInner)->left   &&
            (prcOuter)->right  >= (prcInner)->right  &&
            (prcOuter)->top    <= (prcInner)->top    &&
            (prcOuter)->bottom >= (prcInner)->bottom);
}

//+---------------------------------------------------------------------
//
//  Windows helper functions
//
//----------------------------------------------------------------------

typedef struct tagFONTDESC * LPFONTDESC;

HRESULT LoadString(HINSTANCE hinst, UINT ids, int *pcch, TCHAR **ppsz);

STDAPI FormsCreateFontIndirect(LPFONTDESC lpFontDesc,
            REFIID riid, LPVOID FAR* lplpvObj);

STDAPI FormsUpdateRegistration(void);

void GetChildWindowRect(HWND hwndChild, RECT *prc);
void UpdateChildTree(HWND hWnd);

BOOL InClientArea(POINTL ptScreen, HWND hwnd);
BOOL IsWindowActive(HWND hwnd);
BOOL IsWindowPopup(HWND hwnd);
HWND GetOwningMDIChild(HWND hwnd);
HWND GetOwnerOfParent(HWND hwnd);

void InvalidateProcessWindows();

enum SCROLLPIN
{
    SP_TOPLEFT = 1,     // Pin inner RECT to the top or left of the outer RECT
    SP_BOTTOMRIGHT,     // Pin inner RECT to the bottom or right of the outer RECT
    SP_MINIMAL,         // Calculate minimal scroll necessary to move the inner RECT into the outer RECT
    SP_MAX,
    SP_FORCE_LONG = (ULONG)-1       // Force this to be long for win16 world.
};

BOOL    CalcScrollDelta(
                RECT *prcOuter,
                RECT *prcInner,
                SIZE sizeGutter,
                SIZE *psizeScroll,
                SIZE *psizeOffset = NULL,
                SCROLLPIN spVert = SP_MINIMAL,
                SCROLLPIN spHorz = SP_MINIMAL);
BOOL    CalcInsetScroll(
                RECT *prc,
                SIZE sizeGutter,
                POINT pt,
                SIZES *psizeScroll);

HCURSOR SetCursorIDC(LPCTSTR pstrIDCCursor);

struct DYNLIB
{
    HINSTANCE hinst;
    DYNLIB *  pdynlibNext;
#if (!defined(_MAC) && !defined(UNIX))
    char achName[];
#else
    char *achName;
#endif
};

struct DYNPROC
{
    void *pfn;
    DYNLIB *pdynlib;
    char *achName;
};

HRESULT LoadProcedure(DYNPROC *pdynproc);
HRESULT FreeDynlib(DYNLIB *pdynlib);

HRESULT RegisterWindowClass(
        TCHAR *   pstrClass,
        LRESULT   (CALLBACK *pfnWndProc)(HWND, UINT, WPARAM, LPARAM),
        UINT      style,
        TCHAR *   pstrBase,
        WNDPROC * ppfnBase,
        ATOM    * patom,
        HICON     hIconSm = NULL);


// This macro is used for SetBrushOrgEx to change value to an offset in range 0..7 that is
//  accepted by the function, taking into account the negative values
#define POSITIVE_MOD(lValue, lDiv) ((lValue >= 0) ? (lValue % lDiv) : (lDiv - (-lValue) % lDiv))

//---------------------------------------------------------------
//  IOleObject
//---------------------------------------------------------------

enum OLE_SERVER_STATE
{
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.I.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.I.
    OS_OPEN                             // server running, open-edited
};

#if DBG==1 || defined(PERFTAGS)

#define STRINGIFY(constant) { if (v == constant) return (#constant); }

inline char *DebugOleStateName(OLE_SERVER_STATE v)
{
    STRINGIFY(OS_PASSIVE);
    STRINGIFY(OS_LOADED);
    STRINGIFY(OS_RUNNING);
    STRINGIFY(OS_INPLACE);
    STRINGIFY(OS_UIACTIVE);
    STRINGIFY(OS_OPEN);
    return "UNKNOWN STATE";
}

#undef STRINGIFY

#endif


//+---------------------------------------------------------------------------
//
//  Enumeration: HTC
//
//  Synopsis:    Success values for hit testing.
//
//----------------------------------------------------------------------------

enum HTC
{
    HTC_NO                = 0,
    HTC_MAYBE             = 1,
    HTC_YES               = 2,

    HTC_HSCROLLBAR        = 3,
    HTC_VSCROLLBAR        = 4,

    HTC_LEFTGRID          = 5,
    HTC_TOPGRID           = 6,
    HTC_RIGHTGRID         = 7,
    HTC_BOTTOMGRID        = 8,

    HTC_NONCLIENT         = 9,

    HTC_EDGE              = 10,

    HTC_GRPTOPBORDER      = 11,
    HTC_GRPLEFTBORDER     = 12,
    HTC_GRPBOTTOMBORDER   = 13,
    HTC_GRPRIGHTBORDER    = 14,
    HTC_GRPTOPLEFTHANDLE  = 15,
    HTC_GRPLEFTHANDLE     = 16,
    HTC_GRPTOPHANDLE      = 17,
    HTC_GRPBOTTOMLEFTHANDLE = 18,
    HTC_GRPTOPRIGHTHANDLE = 19,
    HTC_GRPBOTTOMHANDLE   = 20,
    HTC_GRPRIGHTHANDLE    = 21,
    HTC_GRPBOTTOMRIGHTHANDLE = 22,

    HTC_TOPBORDER         = 23,
    HTC_LEFTBORDER        = 24,
    HTC_BOTTOMBORDER      = 25,
    HTC_RIGHTBORDER       = 26,

    HTC_TOPLEFTHANDLE     = 27,
    HTC_LEFTHANDLE        = 28,
    HTC_TOPHANDLE         = 29,
    HTC_BOTTOMLEFTHANDLE  = 30,
    HTC_TOPRIGHTHANDLE    = 31,
    HTC_BOTTOMHANDLE      = 32,
    HTC_RIGHTHANDLE       = 33,
    HTC_BOTTOMRIGHTHANDLE = 34,
    HTC_ADORNMENT         = 35
};

//
// More ROP codes
//
#define DST_PAT_NOT_OR  0x00af0229
#define DST_PAT_AND     0x00a000c9
#define DST_PAT_OR      0x00fa0089

DWORD ColorDiff (COLORREF c1, COLORREF c2);

//+---------------------------------------------------------------------
//
//  Helper functions for drawing feedback and ghosting rects.
//
//----------------------------------------------------------------------
void PatBltRect(HDC hDC, RECT * prc, int cThick, DWORD dwRop) ;
void DrawDefaultFeedbackRect(HDC hDC, RECT * prc);


//+---------------------------------------------------------------------
//
//  Helper functions for implementing IDataObject and IViewObject
//
//----------------------------------------------------------------------

enum
{
    ICF_EMBEDDEDOBJECT,
    ICF_EMBEDSOURCE,
    ICF_LINKSOURCE,
    ICF_LINKSRCDESCRIPTOR,
    ICF_OBJECTDESCRIPTOR,
    ICF_FORMSCLSID,
    ICF_FORMSTEXT
};

// Array of common clip formats, indexed by ICF_xxx enum
extern CLIPFORMAT g_acfCommon[];

// Initialize g_cfStandard.
void RegisterClipFormats();

// Replace CF_COMMON(icf) with true clip format.
void SetCommonClipFormats(FORMATETC *pfmtetc, int cfmtetc);
#ifndef _MAC
#define CF_COMMON(icf) (CLIPFORMAT)(CF_PRIVATEFIRST + icf)
#else
#define CF_COMMON(icf) g_acfCommon[icf]
#endif

//
//  FORMATETC helpers
//

HRESULT CloneStgMedium(const STGMEDIUM * pcstgmedSrc, STGMEDIUM * pstgmedDest);
BOOL DVTARGETDEVICEMatchesRequest(const DVTARGETDEVICE * pcdvtdRequest,
                                  const DVTARGETDEVICE * pcdvtdActual);
BOOL TYMEDMatchesRequest(TYMED tymedRequest, TYMED tymedActual);
BOOL FORMATETCMatchesRequest(const FORMATETC *   pcfmtetcRequest,
                             const FORMATETC *   pcfmtetcActual);

int FindCompatibleFormat(FORMATETC FmtTable[], int iSize, FORMATETC& formatetc);

//
//  OBJECTDESCRIPTOR clipboard format helpers
//

HRESULT GetObjectDescriptor(LPDATAOBJECT pDataObj, LPOBJECTDESCRIPTOR pDescOut);
HRESULT UpdateObjectDescriptor(LPDATAOBJECT pDataObj, POINTL& ptl, DWORD dwAspect);

//
//  Other helper functions
//

class ThreeDColors;

enum BORDER_FLAGS
{
    BRFLAGS_BUTTON      = 0x01,
    BRFLAGS_ADJUSTRECT  = 0x02,
    BRFLAGS_DEFAULT     = 0x04,
    BRFLAGS_MONO        = 0x08  // Inner border only (for flat scrollbars)
};


//+---------------------------------------------------------------------
//
//  Standard implementations of common enumerators
//
//----------------------------------------------------------------------

HRESULT CreateOLEVERBEnum(LPOLEVERB pVerbs, ULONG cVerbs,
                                                      LPENUMOLEVERB FAR* ppenum);

HRESULT CreateFORMATETCEnum(LPFORMATETC pFormats, ULONG cFormats,
                                                      LPENUMFORMATETC FAR* ppenum, BOOL fDeleteOnExit=FALSE);

//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Class:      CClassFactory
//
//  Purpose:    Base class for CStaticCF, CDynamicCF
//
//+---------------------------------------------------------------------------

class CClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(LockServer)(BOOL fLock);
};

//+---------------------------------------------------------------------------
//
//  Class:      CStaticCF
//
//  Purpose:    Standard implementation a class factory object declared
//              as a static variable.  The implementation of Release does
//              not call delete.
//
//              To use this class, declare a variable of type CStaticCF
//              and initialize it with an instance creation function and
//              and optional DWORD context.  The instance creation function
//              is of type FNCREATE defined below.
//
//+---------------------------------------------------------------------------

class CStaticCF : public CClassFactory
{
public:
    typedef HRESULT (FNCREATE)(
            IUnknown *pUnkOuter,    // pUnkOuter for aggregation
            IUnknown **ppUnkObj);   // the created object.

    CStaticCF(FNCREATE *pfnCreate)
            { _pfnCreate = pfnCreate; }

    // IClassFactory methods
    STDMETHOD(CreateInstance)(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);

protected:
    FNCREATE *_pfnCreate;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDynamicCF (DYNCF)
//
//  Purpose:    Class factory which exists on the heap, and whose Release
//              method deletes the class factory.
//
//  Interface:  DECLARE_FORMS_STANDARD_IUNKNOWN -- IUnknown methods
//
//              LockServer             -- Per IClassFactory.
//              CDynamicCF             -- ctor.
//              ~CDynamicCF            -- dtor.
//
//----------------------------------------------------------------------------

class CDynamicCF: public CClassFactory
{
public:
    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory::CreateInstance is left pure virtual.

protected:
            CDynamicCF();
    virtual ~CDynamicCF();

    ULONG _ulRefs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CBaseEventSink (bes)
//
//  Purpose:    Provides null implementations of methods not used on an event
//              sink.
//
//----------------------------------------------------------------------------

class CBaseEventSink : public IDispatch
{
public:
    // IDispatch methods
    STDMETHOD(GetTypeInfoCount) ( UINT FAR* pctinfo );
    STDMETHOD(GetTypeInfo)(
            UINT                  itinfo,
            LCID                  lcid,
            ITypeInfo FAR* FAR*   pptinfo);

    STDMETHOD(GetIDsOfNames)(
            REFIID                riid,
            LPOLESTR *            rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID FAR*           rgdispid);
};



//+---------------------------------------------------------------------
//
//  IStorage and IStream Helper functions
//
//----------------------------------------------------------------------

// LARGE_INTEGER sign conversions are a pain without this

union LARGEINT
{
    LONGLONG       i64;
    ULONGLONG      ui64;
    LARGE_INTEGER  li;
    ULARGE_INTEGER uli;
};

const LARGEINT LI_ZERO = { 0 };

#define STGM_DFRALL (STGM_READWRITE|STGM_TRANSACTED|STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL  (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)
#define STGM_SRO  (STGM_READ | STGM_SHARE_EXCLUSIVE)

HRESULT GetMonikerDisplayName(LPMONIKER pmk, LPTSTR FAR* ppstr);
HRESULT CreateStorageOnHGlobal(HGLOBAL hgbl, LPSTORAGE FAR* ppStg);


//+---------------------------------------------------------------------
//
//  Registration Helper functions.
//
//----------------------------------------------------------------------

void RegDbDeleteKey(HKEY hkeyParent, const TCHAR *szDelete);
HRESULT RegDbSetValues(HKEY hkeyParent, TCHAR *szFmt, DWORD_PTR *adwArgs);
HRESULT RegDbOpenCLSIDKey(HKEY *phkeyCLSID);


//+------------------------------------------------------------------------
//
//  Class:      COffScreenContext
//
//  Synopsis:   Manages off-screen drawing context.
//
//-------------------------------------------------------------------------

MtExtern(COffScreenContext)

class COffScreenContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(COffScreenContext))
    COffScreenContext(HDC hdcWnd, long width, long height, HPALETTE hpal, DWORD dwFlags);
    ~COffScreenContext();

    HDC     GetDC(RECT *prc);
    HDC     ReleaseDC(HWND hwnd, BOOL fDraw = TRUE);

    // actual dimensions
    long    _widthActual;
    long    _heightActual;

private:
    BOOL    CreateDDB(long width, long height);
    BOOL    GetDDB(HPALETTE hpal);
    void    ReleaseDDB();
#if !defined(NODD)
    BOOL    CreateDDSurface(long width, long height, HPALETTE hpal);
    BOOL    GetDDSurface(HPALETTE hpal);
    void    ReleaseDDSurface();
#endif

    RECT    _rc;            // requested position information (for viewport)
    HDC     _hdcMem;
    HDC     _hdcWnd;
    BOOL    _fOffScreen;
    long    _cBitsPixel;
    int     _nSavedDC;
    HBITMAP _hbmMem;
    HBITMAP _hbmOld;
    BOOL    _fCaret;
#if !defined(NODD)
    BOOL    _fUseDD;
    BOOL    _fUse3D;
    interface IDirectDrawSurface  *_pDDSurface;
#endif // !defined(NODD)
};

enum
{
    OFFSCR_SURFACE  = 0x80000000,   // Use DD surface for offscreen buffer
    OFFSCR_3DSURFACE= 0x40000000,   // Use 3D DD surface for offscreen buffer
    OFFSCR_CARET    = 0x20000000,   // Manage caret around BitBlt's
    OFFSCR_BPP      = 0x000000FF    // bits-per-pixel mask
};

HRESULT InitSurface();

//+------------------------------------------------------------------------
//
//  Tear off interfaces.
//
//-------------------------------------------------------------------------

#ifndef X_VTABLE_HXX_
#define X_VTABLE_HXX_
#include "vtable.hxx"
#endif

#if defined(_M_IX86) && !defined(WIN16) && !defined(UNIX) && DBG == 1
// BUGBUG ReinerF, BryanT
// compiler busted doing debugtearoffs
// #define DEBUG_TEAROFFS 1
#undef DEBUG_TEAROFFS
#else
#undef DEBUG_TEAROFFS
#endif

#ifdef UNIX

#ifndef X_UNIXTOFF_HXX_
#define X_UNIXTOFF_HXX_
#include "unixtoff.hxx"
#endif

#else // UNIX

#ifdef WIN16

#ifndef X_W16TOFF_HXX_
#define X_W16TOFF_HXX_
#include "w16toff.hxx"
#endif

#else // Win32

#ifndef X_WINTOFF_HXX_
#define X_WINTOFF_HXX_
#include "wintoff.hxx"
#endif

#endif // Win32
#endif // !UNIX

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      appropdescsInVtblOrder = NULL);
//
// Indices to the IUnknown methods.
//

#define METHOD_QI      0
#define METHOD_ADDREF  1
#define METHOD_RELEASE 2

#define METHOD_MASK(index) (1 << (index))

// Well known method dwMask values

#define QI_MASK      METHOD_MASK( METHOD_QI )
#define ADDREF_MASK  METHOD_MASK( METHOD_ADDREF )
#define RELEASE_MASK METHOD_MASK( METHOD_RELEASE )

HRESULT
CreateTearOffThunk(
        void*       pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID,
        void *      appropdescsInVtblOrder = NULL);


// Installs pvObject2 after the tearoff is created
HRESULT
InstallTearOffObject(void * pvthunk,
                     void * pvObject,
                     void * apfn,
                     DWORD dwMask);

// Usuage if IID_TEAROFF(xxxx, xxxx, xxxx)
#define IID_TEAROFF(pObj, itf, pUnkOuter)       \
        (iid == IID_##itf)                      \
        {                                       \
            hr = CreateTearOffThunk(            \
                pObj,                           \
                s_apfn##itf,                    \
                pUnkOuter,                      \
                ppv);                           \
            if (hr)                             \
                RRETURN(hr);                    \
        }                                       \

// Usuage if IID_HTML_TEAROFF(xxxx, xxxx, xxxx)
#define IID_HTML_TEAROFF(pObj, itf, pUnkOuter)       \
        (iid == IID_##itf)                      \
        {                                       \
            hr = CreateTearOffThunk(            \
                pObj,                           \
                s_apfnpd##itf,                  \
                pUnkOuter,                      \
                ppv,                            \
                (void *)s_ppropdescsInVtblOrder##itf);        \
            if (hr)                             \
                RRETURN(hr);                    \
        }                                       \


//-------------------------------------------------------------------------
//  helper method to create an DataObject for IDispatch
//      implemented in cdatadsp.cxx
//-------------------------------------------------------------------------
HRESULT FormSetClipboard(IDataObject *pdo);

//+------------------------------------------------------------------------
//
//  Brush stuff.
//
//-------------------------------------------------------------------------

HBRUSH  GetCachedBrush(COLORREF color);
void    ReleaseCachedBrush(HBRUSH hbr);
void    SelectCachedBrush(HDC hdc, COLORREF crNew, HBRUSH * phbrNew, HBRUSH * phbrOld, COLORREF * pcrNow);

// Bitmap brush cache managers. Do not use the Release (above) with these.
HBRUSH  GetCachedBmpBrush(int resId);

void    PatBltBrush(HDC hdc, LONG x, LONG y, LONG xWid, LONG yHei, DWORD dwRop, COLORREF cr);
void    PatBltBrush(HDC hdc, RECT * prc, DWORD dwRop, COLORREF cr);

//+------------------------------------------------------------------------
//
//  Color stuff.
//
//-------------------------------------------------------------------------

#ifdef UNIX
extern COLORREF g_acrSysColor[25];
extern BOOL     g_fSysColorInit;
void InitColorTranslation();
#endif

typedef DWORD OLE_COLOR;
#define OLECOLOR_FROM_SYSCOLOR(__c) ((__c) | 0x80000000)

BOOL        IsOleColorValid (OLE_COLOR clr);
COLORREF    ColorRefFromOleColor(OLE_COLOR clr);
HPALETTE    GetDefaultPalette(HDC hdc = NULL);
BOOL        IsHalftonePalette(HPALETTE hpal);
inline HPALETTE    GetHalftonePalette() { extern HPALETTE g_hpalHalftone; return g_hpalHalftone; }
void        CopyColorsFromPaletteEntries(RGBQUAD *prgb, const PALETTEENTRY *ppe, UINT uCount);
void        CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb, UINT uCount);
HDC         GetMemoryDC();
void        ReleaseMemoryDC(HDC hdc);
COLORREF    GetSysColorQuick(int i);

inline unsigned GetPaletteSize(LOGPALETTE *pColors)
{
    Assert(pColors);
    Assert(pColors->palVersion == 0x300);

    return (sizeof(LOGPALETTE) - sizeof(PALETTEENTRY) + pColors->palNumEntries * sizeof(PALETTEENTRY));
}

inline int ComparePalettes(LOGPALETTE *pColorsLeft, LOGPALETTE *pColorsRight)
{
    //
    // This counts on the fact that the second member of LOGPALETTE is the size
    // so if the sizes don't match, we'll stop long before either one ends.  If
    // the sizes are equal then GetPaletteSize(pColorsLeft) is the maximum size
    //
    return memcmp(pColorsLeft, pColorsRight, GetPaletteSize(pColorsLeft));
}

struct LOGPAL256
{
    WORD            wVer;
    WORD            wCnt;
    PALETTEENTRY    ape[256];
};

extern HPALETTE  g_hpalHalftone;
extern LOGPAL256 g_lpHalftone;
extern RGBQUAD   g_rgbHalftone[];

extern BYTE * g_pInvCMAP;

//+------------------------------------------------------------------------
//
//  Formatting Swiss Army Knife
//
//-------------------------------------------------------------------------

enum FMT_OPTIONS // tag fopt
{
    FMT_EXTRA_NULL_MASK= 0x0F,
    FMT_OUT_ALLOC      = 0x10,
    FMT_OUT_XTRATERM   = 0x20,
    FMT_ARG_ARRAY      = 0x40
};

HRESULT VFormat(DWORD dwOptions,
    void *pvOutput, int cchOutput,
    TCHAR *pchFmt,
    void *pvArgs);

HRESULT CDECL Format(DWORD dwOptions,
    void *pvOutput, int cchOutput,
    TCHAR *pchFmt,
    ...);

//+---------------------------------------------------------------------------
//
//  Persistence Structures and API
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
// PROP_DESC - This structure is used to tell the persistence code what
// properties an object wants persisted.
//
//      Mac Note: the pfnPrep callback is used to byteswap a userdefined
//                  structure. See Win2Mac.hxx for macro defines useful here.
//
//----------------------------------------------------------------------------
#ifdef _MAC
typedef void *(*PFNVPREP)(void *);
#endif
struct PROP_DESC
{
    BYTE       wpif;          //  Type (WPI_LONG, WPI_BSTRING, etc.)
    BYTE       cbSize;        //  Size of data.
    USHORT     cbOffset;      //  Offset into class for member variable.
    ULONG      ulDefault;     //  Default value for the property cast to
                              //    the type ULONG. Can't use a union
                              //    and still statically initialize.
    LPCSTR     pszName;       //  Name of property for Save As Text (always ANSI)
#ifdef _MAC
    PFNVPREP   pfnPrep;       // pointer to a function returning void pointer that
                              // will pre/post process (byteswap) properties
#endif
};

#define SIZE_OF(s, m) sizeof(((s *)0)->m)
#define SIZE_AND_OFFSET_OF(s, m) SIZE_OF(s,m), offsetof(s, m)

//
// The pfnPropDescGet/Set typedefs are defined in cdbase.hxx
//
// DECLARE_GET_SET_METHODS is put inside the class definition.
//
#ifdef WIN16
#define DECLARE_GET_SET_METHODS(cls, prop)            \
            static HRESULT gET##prop (ULONG *plVal);  \
            HRESULT Get##prop (ULONG *plVal);         \
            static HRESULT sET##prop (ULONG lVal);    \
            HRESULT Set##prop (ULONG lVal);           \
            static PROP_DESC_GETSET s_##prop##GetSet;
#else
#define DECLARE_GET_SET_METHODS(cls, prop)            \
            HRESULT Get##prop (ULONG *plVal);         \
            HRESULT Set##prop (ULONG lVal);           \
            static PROP_DESC_GETSET s_##prop##GetSet;
#endif

//
// DEFINE_GET_SET_METHODS is put in a cxx file, usually near the PROP_DESC
// definition
//
#ifdef WIN16
#define DEFINE_GET_SET_METHODS(cls, prop, iid)        \
            PROP_DESC_GETSET cls::s_##prop##GetSet =  \
            {                                         \
                (pfnPropDescGet)&cls::gET##prop,      \
                (pfnPropDescSet)&cls::sET##prop,      \
                &(iid)                                \
            };
#else
#define DEFINE_GET_SET_METHODS(cls, prop, iid)        \
            PROP_DESC_GETSET cls::s_##prop##GetSet =  \
            {                                         \
                (pfnPropDescGet)&cls::Get##prop,      \
                (pfnPropDescSet)&cls::Set##prop,      \
                &(iid)                                \
            };
#endif

//
// DEFINE_DERIVED_GETSET_METHODS is used for classes derived from
// a helper class that support the specified property.  This is
// put in a cxx file, usually near the PROP_DESC definition.  An
// example would be the MouseIcon property which is supported in
// a number for controls classes and has a corresponding helper
// in CServer.
//
#define DEFINE_DERIVED_GETSET_METHODS(cls, basecls, prop, iid)          \
            DEFINE_GET_SET_METHODS(cls, prop, iid)                      \
                                                                        \
            HRESULT cls::Get##prop(ULONG * pulValue)                    \
            { return basecls::Get##prop##ForPersistence(pulValue); }    \
                                                                        \
            HRESULT cls::Set##prop(ULONG ulValue)                       \
            { return basecls::Set##prop##ForPersistence(ulValue); }


//
// You are responsible for providing implementations of the Get and Set
// methods created by the above macros.
//

// -----------
//
// Macros used to fill in PROP_DESC structs.
//
// -----------

#ifdef _MAC

#define PROP_NOPERSIST(type, size, name)         \
    {                                            \
        WPI_##type | WPI_NOPERSIST,              \
        size, 0, 0,                              \
        (LPCSTR)NULL,                            \
        PROP_DESC_NOBYTESWAP                     \
    },

#define PROP_GETSET(type, class, prop, size, name) \
    {                                              \
        WPI_##type | WPI_GETSET,                   \
        size, 0, (ULONG)&class::s_##prop##GetSet,  \
        name,                                      \
        PROP_DESC_NOBYTESWAP                       \
    },

#define PROP_MEMBER(type, class, member, default, name, macbyteswapfn)  \
    {                                                                   \
        WPI_##type,                                                     \
        SIZE_AND_OFFSET_OF(class, member),                              \
        (ULONG)(default),                                               \
        name,                                                           \
        macbyteswapfn                                                   \
    },

#define PROP_VARARG(type, size, default, name, macbyteswapfn)   \
    {                                                           \
        WPI_##type | WPI_VARARG,                                \
        size, 0,                                                \
        (ULONG)(default),                                       \
        name,                                                   \
        macbyteswapfn                                           \
    },

#define PROP_CUSTOM(type, size, offset, default, name, macbyteswapfn)   \
    {                                                                   \
        (type),                                                         \
        size,                                                           \
        offset,                                                         \
        (ULONG)(default),                                               \
        name,                                                           \
        macbyteswapfn                                                   \
    },

#else // _MAC

#define PROP_NOPERSIST(type, size, name)         \
    {                                            \
        WPI_##type | WPI_NOPERSIST,              \
        size, 0, 0,                              \
        (LPCSTR)NULL                             \
    },

#define PROP_GETSET(type, class, prop, size, name) \
    {                                              \
        WPI_##type | WPI_GETSET,                   \
        size, 0, (ULONG)&class::s_##prop##GetSet,  \
        name                                       \
    },

#define PROP_MEMBER(type, class, member, default, name, maconly)    \
    {                                                               \
        WPI_##type,                                                 \
        SIZE_AND_OFFSET_OF(class, member),                          \
        (ULONG)(default),                                           \
        name                                                        \
    },

#define PROP_VARARG(type, size, default, name, maconly) \
    {                                                   \
        WPI_##type | WPI_VARARG,                        \
        size, 0,                                        \
        (ULONG)(default),                               \
        name                                            \
    },

#define PROP_CUSTOM(type, size, offset, default, name, maconly) \
    {                                                           \
        (type),                                                 \
        size,                                                   \
        offset,                                                 \
        (ULONG)(default),                                       \
        name                                                    \
    },


#endif

//+------------------------------------------------------------------------
//
// Version Numbers for objects which use WriteProps and ReadProps.  Objects
// should base their version numbers on this so if the implementation of
// WriteProps changes the object's version number will appropriately
// increment.
//
// The major version indicates an incompatible change in the format.  The
// minor version indicates a change which is readable by previous
// implementations having the same major version.
//
//  Major History:    15-Dec-94   LyleC  1 = Created
//                    07-Feb-95   LyleC  2 = No datablock for VT_USERDEFINED
//
//  Minor History:    15-Dec-94   LyleC  0 = Created
//
//-------------------------------------------------------------------------

const USHORT WRITEPROPS_MINOR_VERSION =   0x00;  // Must be <= 0xFF
const USHORT WRITEPROPS_MAJOR_VERSION = 0x0200;  // Low byte must be zero

const USHORT WRITEPROPS_VERSION = (WRITEPROPS_MAJOR_VERSION + WRITEPROPS_MINOR_VERSION);

#define MAJORVER_MASK  0xFF00
#define MINORVER_MASK  0xFF

HRESULT ParseStringToLongs(OLECHAR * psz, LONG ** ppLongs, LONG * pcLongs);
HRESULT MakeStringOfLongs(LONG * pLongs, LONG cLongs, BSTR * pbstr);

interface IPropertyBag;
interface IErrorLog;

HRESULT CDECL WriteProps(IStream *   pStm,
                   USHORT      usVersion,
                   BOOL        fForceAlign,
                   PROP_DESC * pDesc,
                   UINT        cDesc,
                   void *      pThis,
                   ...);

HRESULT CDECL WriteProps(IPropertyBag *   pBag,
                   BOOL             fSaveAllProperties,
                   PROP_DESC *      pDesc,
                   UINT             cDesc,
                   void *           pThis,
                   ...);

HRESULT VWriteProps(IStream *   pStm,
                   USHORT      usVersion,
                   BOOL        fForceAlign,
                   PROP_DESC * pDesc,
                   UINT        cDesc,
                   void *      pThis,
                   va_list     vaArgs);

HRESULT VWriteProps(IPropertyBag *  pBag,
                   BOOL             fSaveAllProperties,
                   PROP_DESC *      pDesc,
                   UINT             cDesc,
                   void *           pThis,
                   va_list          vaArgs);

HRESULT CDECL ReadProps(USHORT       usVersion,
                  IStream *    pStm,
                  USHORT       cBytes,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  ...);

HRESULT CDECL ReadProps(IPropertyBag *    pBag,
                  IErrorLog *       pErrLog,
                  PROP_DESC *       pDesc,
                  UINT              cDesc,
                  void *            pThis,
                  ...);

HRESULT VReadProps(USHORT       usVersion,
                  IStream *    pStm,
                  USHORT       cBytes,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  va_list      vaArgs);

HRESULT VReadProps(IPropertyBag *   pBag,
                  IErrorLog *       pErrLog,
                  PROP_DESC *       pDesc,
                  UINT              cDesc,
                  void *            pThis,
                  va_list           vaArgs);

HRESULT CDECL ReadProps(USHORT       usVersion,
                  BYTE *       pBuf,
                  ULONG        cBuf,
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis,
                  ...);

HRESULT InitFromPropDesc(
                  PROP_DESC *  pDesc,
                  UINT         cDesc,
                  void *       pThis);

#if DBG == 1
#define AssertPropDescs(strB, pBase, cBase, strD, pDer, cDer, asz) \
          VerifyPropDescDerivation(strB, pBase, cBase, strD, pDer, cDer, asz)

void VerifyPropDescDerivation(LPCSTR     pstrBase,
                              PROP_DESC *ppdBase,
                              UINT       cBase,
                              LPCSTR     pstrDerived,
                              PROP_DESC *ppdDerived,
                              UINT       cDerived,
                              LPTSTR    *aszKnownDiff);
#else
#define AssertPropDescs(strB, pBase, cBase, strD, pDer, cDer, asz)
#endif

//+---------------------------------------------------------------------------
//
// Inline functions that return a data pointer that has been aligned to a
// particular boundary.
//
//----------------------------------------------------------------------------

#ifdef WIN16
#define AlignTo(boundary, t)    \
    (((ULONG)t + (boundary-1)) & ~(boundary - 1))

#define IsAlignedTo(boundary, t)    \
    (((ULONG)t & (boundary -1)) == 0)

#else
template <class T>
inline T
AlignTo(int boundary, T t)
{
    Assert(boundary > 0);

    Assert( "boundary is power of 2" &&
            ((boundary - 1) | boundary) == (boundary + (boundary - 1)));

    return (T)(((ULONG)t + (boundary-1)) & ~(boundary - 1));
}

template <class T>
inline BOOL
IsAlignedTo(int boundary, T t)
{
    Assert(boundary > 0);

    Assert( "boundary is power of 2" &&
            ((boundary - 1) | boundary) == (boundary + (boundary - 1)));

    return ((ULONG)t & (boundary-1)) == 0;
}
#endif // !WIN16

// constant size for stack-based buffers used for LoadString()
#define FORMS_BUFLEN 255

// maxlen for Verb names
#define FORMS_MAXVERBNAMLEN 40

// States of an object which support IPersistStorage
enum PERSIST_STATE
{
    PS_UNINIT,
    PS_NORMAL,
    PS_NOSCRIBBLE_SAMEASLOAD,
    PS_NOSCRIBBLE_NOTSAMEASLOAD,
    PS_HANDSOFFSTORAGE_FROM_NOSCRIBBLE,
    PS_HANDSOFFSTORAGE_FROM_NORMAL
};


//+------------------------------------------------------------------------
//
//  Macros for dealing with BOOLean properties
//
//-------------------------------------------------------------------------

#define ENSURE_BOOL(_x_)    (!!(BOOL)(_x_))
#define EQUAL_BOOL(_x_,_y_) (ENSURE_BOOL(_x_) == ENSURE_BOOL(_y_))
#define ISBOOL(_x_)         (ENSURE_BOOL(_x_) == (BOOL)(_x_))


//+------------------------------------------------------------------------
//
//  Macros for dealing with VARIANT_BOOL properties
//
//-------------------------------------------------------------------------

#define VARIANT_BOOL_FROM_BOOL(_x_)    ((_x_) ? VB_TRUE : VB_FALSE )
#define BOOL_FROM_VARIANT_BOOL(_x_)    ((VB_TRUE == _x_) ? TRUE : FALSE)


//+-------------------------------------------------------------------------
//
//  Transfer: bagged list
//
//--------------------------------------------------------------------------

typedef struct
{
    RECTL rclBounds;
    ULONG ulID;
} CTRLABSTRACT, FAR *LPCTRLABSTRACT;

#define CLSID_STRLEN    38

HRESULT FindLegalCF(IDataObject * pDO);
HRESULT GetcfCLSIDFmt(LPDATAOBJECT pDataObj, TCHAR * tszClsid);


//+-------------------------------------------------------------------------
//
//  Global window stuff
//
//--------------------------------------------------------------------------

#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_VOID_ONTICK)(void *, UINT idTimer);
typedef HRESULT (BUGCALL *PFN_VOID_ONCOMMAND)(void *, int id, HWND hwndCtrl, UINT codeNotify);
typedef LRESULT (BUGCALL *PFN_VOID_ONMESSAGE)(void *, UINT msg, WPARAM wParam, LPARAM lParam);
typedef void    (BUGCALL *PFN_VOID_ONCALL)(void *, DWORD_PTR);
typedef HRESULT (BUGCALL *PFN_VOID_ONSEND)(void *, void *);

#define NV_DECLARE_ONTICK_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args

#define DECLARE_ONTICK_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        virtual HRESULT BUGCALL fn args

#define ONTICK_METHOD(klass, fn, FN)   (PFN_VOID_ONTICK)&klass::FN

#define NV_DECLARE_ONCALL_METHOD(fn, FN, args)\
        static void BUGCALL FN args;\
        void BUGCALL fn args
#define ONCALL_METHOD(klass, fn, FN)    (PFN_VOID_ONCALL)&klass::FN

#define NV_DECLARE_ONMESSAGE_METHOD(fn, FN, args)\
        static LRESULT BUGCALL FN args;\
        LRESULT BUGCALL fn args
#define ONMESSAGE_METHOD(klass, fn, FN)\
            (PFN_VOID_ONMESSAGE)&klass::FN
#else

#ifdef UNIX

// The Unix compiler seems to have problems when callin CVoid::*
// method pointers.  It assumes they're nonvirtual (probably because
// CVoid has no virtual methods.
//
// So, instead of messing with the class hierarchies (I've learned
// my lesson there), I'm putting in a macro and a bit of hack to
// call an assembly thunk to call the method the correct way.  The
// thunk leverages the thunking code used for tearoffs.
//
// The following architecture allows the resulting macro to:
//
//      o Behave like a function
//      o Take any number of arguments and have the compiler
//        put them in the right place on the stack for us
//      o Take the exact same arguments (including argument list) on
//        NT and Unix.
// davidd
class TextContextEvent;

class CMethodThunk
{
private:

    void *_pObject;
    void *_pfnMethod;

public:

    CMethodThunk *Init(const void *pObject, const void *pfnMethod)
    {
        _pObject = (void*) pObject;
        _pfnMethod = (void*) pfnMethod;
        return this;
    }

    HRESULT doThunk ();
    HRESULT doThunk (void *arg1, ...);
    HRESULT doThunk (int   arg1, ...);
    HRESULT doThunk (IID   arg1, ...);
#if defined(ux10)
    HRESULT doThunk (WNDPROC arg1, ...);
#endif
    HRESULT doThunk (TextContextEvent&);
};

#ifndef X_MALLOC_H_
#define X_MALLOC_H_
#include <malloc.h>
#endif

#if defined(ux10)
#define CALL_METHOD(pObj,pfnMethod,args) \
    ((CMethodThunk*) malloc(sizeof(CMethodThunk)))->Init(pObj,&pfnMethod)->doThunk args
#else
#define CALL_METHOD(pObj,pfnMethod,args) \
    ((CMethodThunk*) alloca(sizeof(CMethodThunk)))->Init(pObj,&pfnMethod)->doThunk args
#endif

#else //UNIX

#define CALL_METHOD(pObj,pfnMethod,args) \
    (pObj->*pfnMethod) args

#endif

typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONTICK)(UINT idTimer);
typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONCOMMAND)(int id, HWND hwndCtrl, UINT codeNotify);
typedef LRESULT (BUGCALL CVoid::*PFN_VOID_ONMESSAGE)(UINT msg, WPARAM wParam, LPARAM lParam);
typedef void    (BUGCALL CVoid::*PFN_VOID_ONCALL)(DWORD_PTR);
typedef HRESULT (BUGCALL CVoid::*PFN_VOID_ONSEND)(void *);

#define NV_DECLARE_ONTICK_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define DECLARE_ONTICK_METHOD(fn, FN, args)\
        virtual HRESULT BUGCALL fn args

#define ONTICK_METHOD(klass, fn, FN)   (PFN_VOID_ONTICK)&klass::fn

#define NV_DECLARE_ONCALL_METHOD(fn, FN, args)\
        void BUGCALL fn(DWORD_PTR)

#define ONCALL_METHOD(klass, fn, FN)    (PFN_VOID_ONCALL)&klass::fn

#define NV_DECLARE_ONMESSAGE_METHOD(fn, FN, args)\
        LRESULT BUGCALL fn args
#define ONMESSAGE_METHOD(klass, fn, FN)\
            (PFN_VOID_ONMESSAGE)&klass::fn

#endif

enum TRACKTIPPOS
{
    TRACKTIPPOS_LEFT,
    TRACKTIPPOS_RIGHT,
    TRACKTIPPOS_BOTTOM,
    TRACKTIPPOS_TOP
};

HRESULT FormsSetTimer(
            void *pvObject,
            PFN_VOID_ONTICK pfnOnTick,
            UINT idTimer,
            UINT uTimeout);

HRESULT FormsKillTimer(
            void *pvObject,
            UINT idTimer);

HRESULT FormsTrackPopupMenu(
            HMENU hMenu,
            UINT fuFlags,
            int x,
            int y,
            HWND hwndMessage,
            int *piSelection);

void    FormsShowTooltip(
            TCHAR * szText,
            HWND hwnd,
            MSG msg,
            RECT * prc,
            DWORD_PTR dwCookie,
            BOOL fRTL);     // COMPLEXSCRIPT - text is right-to-left reading
void    FormsHideTooltip(BOOL fReset);
BOOL    FormsTooltipMessage(UINT msg, WPARAM wParam, LPARAM lParam);

#if DBG==1 || defined(PERFTAGS)
#define  GWPostMethodCall(pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
         GWPostMethodCallEx(GetThreadState(), pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
#define  GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
        _GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
HRESULT _GWPostMethodCallEx(struct THREADSTATE *pts, void *pvObject , PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext, BOOL fIgnoreContext, char * pszCallDbg);
#else
#define  GWPostMethodCall(pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
         GWPostMethodCallEx(GetThreadState(), pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg)
#define  GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext, pszCallDbg) \
        _GWPostMethodCallEx(pts, pvObject, pfnCall, dwContext, fIgnoreContext)
HRESULT _GWPostMethodCallEx(struct THREADSTATE *pts, void *pvObject , PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext, BOOL fIgnoreContext);
#endif

void    GWKillMethodCall(void *pvObject, PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext);
void    GWKillMethodCallEx(struct THREADSTATE *pts, void *pvObject, PFN_VOID_ONCALL pfnCall, DWORD_PTR dwContext);

#if DBG==1
BOOL    GWHasPostedMethod(void *pvObject);
#endif

HRESULT GWSetCapture(void *pvObject, PFN_VOID_ONMESSAGE, HWND hwndRef);
void    GWReleaseCapture(void *pvObject);
BOOL    GWGetCapture(void *pvObject);


//+-------------------------------------------------------------------------
//
//  File Open/Save helper
//
//--------------------------------------------------------------------------

typedef BOOL    (*FNOFN)(LPOPENFILENAME);
HRESULT FormsGetFileName(
                BOOL fSaveFile,
                HWND hwndOwner,
                int idFilterRes,
                LPTSTR pstrFile,
                int cch,
                LPARAM lCustData,
                DWORD *pnFilterIndex=NULL);

#ifdef IE5_ZOOM

//+-------------------------------------------------------------------------
//
//  Display units.
//
//--------------------------------------------------------------------------

typedef INT Z;
typedef Z X;
typedef Z Y;

typedef unsigned long ulong;
typedef unsigned char uchar;

//+-------------------------------------------------------------------------
//
//  Comparison utilities
//
//--------------------------------------------------------------------------

inline BOOL FEqualLpb(const uchar *lpch1, const uchar *lpch2, long cch)
{
    return (memcmp(lpch1, lpch2, cch) == 0);
}

#define FEqualStruct(struct1, struct2)\
    FEqualLpb((uchar *)&(struct1), (uchar *)&(struct2), sizeof(struct1))

//+-------------------------------------------------------------------------
//
//  Some Neat little utilities...
//
//--------------------------------------------------------------------------

// this means: "a >= b && a <= c" (useful for [First,Lim] pairs)
#define FBetween(a, b, c)   (((unsigned)((a) - (b))) <= ((unsigned)(c) - (b)))
#define FInRange(a, b, c)   FBetween(a, b, c)

// (useful for [First,Lim] pairs)
#define FRangesOverlap(a, b, c, d)  (((a) < (d)) && ((b) > (c)))

// this means: "a >= b && a < c" (useful for [First,Lim) pairs)
#define FInLimits(a, b, c)  ((unsigned)((a) - (b)) < (c) - (b))

// a = b +/- 5
#define IsWithinFive(a, b)   (FBetween((a), (b) - 5, (b) + 5))

// a = b +/- n
#define IsWithinN(a, b, n)   (FBetween((a), (b) - (n), (b) + (n)))

#define MAX_MEASURE_WIDTH 0x7fffff  // stolen from text\lsm.hxx

//+-------------------------------------------------------------------------
//
//  Device Resolution.
//
//--------------------------------------------------------------------------

typedef struct _DRES
{
    X dxuInch;
    Y dyuInch;
} DRES;

#ifdef IE6_ROTATE

//+-------------------------------------------------------------------------
//
//  Angle of rotation
//
//--------------------------------------------------------------------------

typedef long ANG;           // tenths of degree

const float PI = 3.14159265358979323846f;

#define Abs(x)              ((x)>0?(x):-(x))
#define Sgn(x)              ((x)>=0?1:(-1))

const ANG ang90     = 900;
const ANG ang180    = 1800;
const ANG ang270    = 2700;
const ANG ang360    = 3600;  // ang value for 360 degrees
const ANG angNil    = -99999;

inline float RadFromDeg(float deg) { return deg * PI / 180.0f; }
inline float RadFromAng(ANG ang) { return (float)ang / 10.0f * PI / 180.0f; }
inline ANG AngFromDeg(float deg) { return (ANG)((deg + .05) * 10); }
inline float DegFromRad(float rad) { return rad * 180.0f / PI; }
inline float DegFromAng(ANG ang) { return (float)ang / 10.0f; }
inline ANG AngFromRad(float rad) { return AngFromDeg(DegFromRad(rad)); }

inline ANG AngNormalize(ANG ang) { return (ang360 + (Sgn(ang) * (Abs(ang) % ang360))) % ang360; }

//+-------------------------------------------------------------------------
//
//  Rotation matrix
//
//  This structure corresponds to the following matrix. It is
//  stored row-major.
//
//  t = theta, (x, y) center point
//  [  cos(t)  -sin(t)  |  x - x cos(t) + y sin(t)  ]
//  [  sin(t)   cos(t)  |  y - y cos(t) + x sin(t)  ]
//
//  For derivation of the last column see the quill 98 rotstack.doc
//  (mshtml\handbook\rotstack.doc)
//
//  with indices
//  [  0,0     0,1      | 0,2 ]
//  [  1,0     1,1      | 1,2 ]
//
//--------------------------------------------------------------------------

const float vrgfUnit[2][3] =    { 1.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f };

class MAT
{
public:
    inline void Init(void)
    {
        memcpy(rgf, vrgfUnit, sizeof(vrgfUnit));
    }
    inline MAT::MAT(void)
    {
        Init();
    }
    inline BOOL MAT::FTransforms();

    float rgf[2][3];
};

extern MAT g_matUnit;
inline BOOL MAT::FTransforms()
{
    return (!FEqualStruct(rgf, g_matUnit));
}

#endif  // IE6_ROTATE

//+-------------------------------------------------------------------------
//
//  fraction, used for two dimensional scaling
//
//--------------------------------------------------------------------------

class CFrt
{
public:
    void SetScaleFraction(int wNumer, int wDenom);
    void SetFrt(int wNumer, int wDenom, BOOL fScaleY, BOOL fRestrict);
    void RestrictFrtForPda();
    X GetDxpInchFromFrt();
    Y GetDypInchFromFrt();
    void GetFrt(int *pNumer, int *pDenom);
    inline X GetNumerX() { return _wNumerX; }
    inline Y GetNumerY() { return _wNumerY; }
    inline Z GetDenom() { return _wDenom; }

private:
    void SimplifyFrt();
    void RestrictFrt();

    union
    {
        DRES _dres;
        struct
        {
            X _wNumerX;
            Y _wNumerY;
        };
    };
    Z   _wDenom;
};

//+-------------------------------------------------------------------------
//
//  Unit constants
//
//--------------------------------------------------------------------------

const Z dzlInch         = 294912L;      // number of logical units per inch (Word design units)
const Z dzlPoint        = dzlInch/72;
const Z dzlMm           = dzlInch*10/254;
const Z dzlCm           = dzlMm*10;
const Z dzlFoot         = dzlInch*12;
const Z dzlMeter        = dzlCm*100;
const Z dzlTwip         = dzlPoint/20;

const Z ptsInch         = 72;           // number of points per inch
const Z twpInch         = 1440;         // number of twips per inch
const Z himInch         = 2540;         // number of himetric units per inch

const long lwMaxInt     = ((long)0x7FFFFFFF);
const long lwMinInt     = ((long)0x80000000);
const USHORT uswMaxInt  = ((USHORT)0xFFFF);
const ULONG ulwMaxInt   = ((ULONG)0xFFFFFFFF);

#endif  // IE5_ZOOM

//+-------------------------------------------------------------------------
//
//  CTransform  Tag = Transform
//
//--------------------------------------------------------------------------

class CTransform
{
public:
    void DocumentFromWindow(POINTL *pptlDoc, long xWin, long yWin) const;
    void DocumentFromWindow(SIZEL *psizelDoc, long cxWin, long cyWin) const;
    void DocumentFromWindow(RECTL  *prclDoc, const RECT *prcWin) const;
    void DocumentFromWindow(POINTL *pptlDoc, POINT ptWin) const
                            { DocumentFromWindow(pptlDoc, ptWin.x, ptWin.y); }
    void DocumentFromWindow(SIZEL *psizelDoc, SIZE sizeWin) const
                            { DocumentFromWindow(psizelDoc, sizeWin.cx, sizeWin.cy); }

    long  WindowFromDocumentX(long xlDoc) const;
    long  WindowFromDocumentY(long ylDoc) const;
    long  WindowFromDocumentCX(long xlDoc) const;
    long  WindowFromDocumentCY(long ylDoc) const;

    void WindowFromDocument(POINT *pptWin,  long xlDoc, long ylDoc) const;
    void WindowFromDocument(RECT *prcWin, const RECTL *prclDoc) const;
    void WindowFromDocument(SIZE *psizeWin, long cxlDoc, long cylDoc) const;
    void WindowFromDocument(POINT *pptWin, POINTL ptlDoc) const
                            { WindowFromDocument(pptWin, (int)ptlDoc.x, (int)ptlDoc.y); }
    void WindowFromDocument(SIZE *psizeWin, SIZEL sizelDoc) const
                            { WindowFromDocument(psizeWin, (int)sizelDoc.cx, (int)sizelDoc.cy); }

    void HimetricFromDevice(POINTL *pptlDocOut, int cxWinIn, int cyWinIn)
                            { DocumentFromWindow(pptlDocOut, cxWinIn, cyWinIn); }
    void HimetricFromDevice(POINTL *pptlDocOut, POINT ptWinIn)
                            { DocumentFromWindow(pptlDocOut, ptWinIn); }
    void HimetricFromDevice(SIZEL *psizelDocOut, SIZE sizeWinIn) const
                            { DocumentFromWindow(psizelDocOut, sizeWinIn); }
    void HimetricFromDevice(SIZEL *psizelDocOut, int cxWinin, int cyWinIn) const
                            { DocumentFromWindow(psizelDocOut, cxWinin, cyWinIn); }

    long DeviceFromHimetricCY(long hi) const
            { return WindowFromDocumentCY(hi); }
    long DeviceFromHimetricCX(long hi) const
                            { return (long)WindowFromDocumentCX((int)hi); }

    void DeviceFromHimetric(POINT *pptWinOut, int xlDocIn, int ylDocIn)
                            { WindowFromDocument(pptWinOut, xlDocIn, ylDocIn); }
    void DeviceFromHimetric(POINT *pptWinOut, POINTL ptlDocIn)
                            { WindowFromDocument(pptWinOut, ptlDocIn); }
    void DeviceFromHimetric(SIZE *psizeWinOut, SIZEL sizelDocIn) const
                            { WindowFromDocument(psizeWinOut, sizelDocIn); }
    void DeviceFromHimetric(SIZE *psizeWinOut, int cxlDocIn, int cylDocIn) const
                            { WindowFromDocument(psizeWinOut, cxlDocIn, cylDocIn); }

    void WindowFromDocPixels(POINT *ppt, POINT p, BOOL fRelative = FALSE);
    long WindowXFromDocPixels(long lPixels, BOOL fRelative = FALSE);
    long WindowYFromDocPixels(long lPixels, BOOL fRelative = FALSE);
    long DocPixelsFromWindowX(long lValue, BOOL fRelative = FALSE);
    long DocPixelsFromWindowY(long lValue, BOOL fRelative = FALSE);

    long DeviceFromTwipsCX(long twip) const;
    long DeviceFromTwipsCY(long twip) const;
    long TwipsFromDeviceCX(long pix)  const;
    long TwipsFromDeviceCY(long pix)  const;

    void Init(const CTransform * pTransform);
    void Init(const RECT *prcDst, SIZEL sizelSrc, const SIZE *psizeInch = NULL);
    void Init(SIZEL sizelSrc);

#ifdef  IE5_ZOOM

    //
    // Effective scaled resolution
    //

    void zoom(int wNumerX, int wNumerY, int wDenom);
    void SetScaleFraction(int wNumerX, int wNumerY, int wDenom);
    void SetResolutions(const SIZE * psizeInchTarget, const SIZE * psizeInchRender);
    X GetScaledDxpInch();
    Y GetScaledDypInch();
    inline X GetNumerX() { return _wNumerX; }
    inline Y GetNumerY() { return _wNumerY; }
    inline Z GetDenom() { return _wDenom; }
    inline BOOL IsZoomed() const { return _fScaled; }
    inline BOOL TargetDiffers() { return _fDiff; }

    //
    // Pixel scaling routines
    //
    // Layout units:            dxt
    // Unzoomed render units:   dxr
    // Zoomed render units:     dxz
    //

    X DxrFromDxt(X dxt) const;
    Y DyrFromDyt(Y dyt) const;
    X DxzFromDxr(X dxr, BOOL fRelative = FALSE) const;
    Y DyzFromDyr(Y dyr, BOOL fRelative = FALSE) const;
    X DxzFromDxt(X dxt, BOOL fRelative = FALSE) const;
    Y DyzFromDyt(Y dyt, BOOL fRelative = FALSE) const;
    void RectzFromRectr(LPRECT pRectZ, LPRECT pRectR);

    //
    // Pixel measurement routines
    //
    // Points:      pts
    // Twips:       twp
    // Himetric:    him
    //

    X DxtFromPts(float pts) const;
    Y DytFromPts(float pts) const;
    X DxtFromTwp(long twp) const;
    Y DytFromTwp(long twp) const;
    X DxtFromHim(long him) const;
    Y DytFromHim(long him) const;

    //
    // Reverse scaling routines
    //
    // Note: going from low-resolution to high-resolution units is almost always
    // a bug when used in measuring code.
    //

    X DxtFromDxr(X dxr) const;
    Y DytFromDyr(Y dyr) const;
    X DxrFromDxz(X dxz, BOOL fRelative = FALSE) const;
    Y DyrFromDyz(Y dyz, BOOL fRelative = FALSE) const;
    X DxtFromDxz(X dxz, BOOL fRelative = FALSE) const;
    Y DytFromDyz(Y dyz, BOOL fRelative = FALSE) const;

    X HimFromDxt(long dxt) const;
    Y HimFromDyt(long dyt) const;
    X TwpFromDxt(long dxt) const;
    Y TwpFromDyt(long dyt) const;

#endif  // IE5_ZOOM

    CTransform()
            {   }
    CTransform(const CTransform * ptransform)
            { Init(ptransform); }
    CTransform(const CTransform & transform)
            { Init(&transform); }
    CTransform(const RECT *prcDst, const SIZEL *psizelSrc, const SIZE *psizeInch = NULL)
            { Init(prcDst, *psizelSrc, psizeInch); }

    SIZE    _sizeSrc;
    POINT   _ptDst;
    SIZE    _sizeDst;
    SIZE    _sizeInch;          // device dots per inch

#ifdef IE5_ZOOM

private:
    void SimplifyScaleFraction();

    X       _wNumerX;       // X numerator of scaling fraction
    Y       _wNumerY;       // Y numerator of scaling fraction
    Z       _wDenom;        // denominator of scaling fraction
    BOOL    _fScaled;       // TRUE if non-unity scaling fraction

#ifdef IE6_ROTATE

    // Rotation values: rotation matrix, and angles for convenience
    inline BOOL FTransforms()
        { return (_mat.FTransforms()); }
    inline BOOL FRotates()
    	{ return _ang != 0; }
    inline BOOL FRotatesNonTrivially()
        { return (_ang % AngFromDeg(90) != 0); }

	ANG     _ang;			// angle to use for drawing. Note that you cannot rely
							// on the _ang being 0 to mean you don't need to use
							// the matrix transformation code. (You could have a table
							// rotated 90 degrees and an ILC rotated -90. The text has
							// moved around even though the effective rotation is 0.)
							// Please use FTransforms().

    MAT     _mat;           // rotation matrix to use in rendering
    MAT     _matInverse;    // unrotated matrix used in rendering

#endif  // IE6_ROTATE

    union
    {
        DRES _dresLayout;   // horizontal, vertical layout resolution (may be device-independent)
        struct
        {
            X _dxtInch;     // resolution of target device
            Y _dytInch;     // resolution of target device
        };
    };

    union
    {
        DRES _dresRender;   // horizontal, vertical rendering resolution (for example, display or printer)
        struct
        {
            X _dxrInch;     // resolution of display device
            Y _dyrInch;     // resolution of display device
        };
    };

    BOOL    _fDiff;         // if FALSE, use layout resolution only

#endif  // IE5_ZOOM

};


class CSaveTransform : public CTransform
{
public:
    CSaveTransform(CTransform * ptransform) { Save(ptransform); }
    CSaveTransform(CTransform & transform)  { Save(&transform); }

    ~CSaveTransform()                       { Restore(); }

private:
    CTransform *    _ptransform;

    void Save(CTransform * ptransform)
    {
        _ptransform = ptransform;
        ::memcpy(this, _ptransform, sizeof(CTransform));
    }
    void Restore()
    {
        ::memcpy(_ptransform, this, sizeof(CTransform));
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDocInfo
//
//  Purpose:    Document-associated transform
//
//----------------------------------------------------------------------------

class CDocInfo : public CTransform
{
public:
    CDoc *  _pDoc;          // Associated CDoc

    CDocInfo()                      { Init(); }
    CDocInfo(const CDocInfo * pdci) { Init(pdci); }
    CDocInfo(const CDocInfo & dci)  { Init(&dci); }
    CDocInfo(CElement * pElement)   { Init(pElement); }


    void Init(CElement * pElement);
    void Init(const CDocInfo * pdci)
    {
        ::memcpy(this, pdci, sizeof(CDocInfo));
    }

protected:
    void Init() { _pDoc = NULL; }
};



//+-------------------------------------------------------------------------
//
//  CDrawInfo  Tag = DI
//
//--------------------------------------------------------------------------

class CDrawInfo : public CDocInfo
{
public:
    BOOL            _fInplacePaint;      // True if painting to current inplace location.
    BOOL            _fIsMetafile;        // True if associated HDC is a meta-file
    BOOL            _fIsMemoryDC;        // True if associated HDC is a memory DC
    BOOL            _fHtPalette;         // True if selected palette is the halftone palette
    DWORD           _dwDrawAspect;       // per IViewObject::Draw
    LONG            _lindex;             // per IViewObject::Draw
    void *          _pvAspect;           // per IViewObject::Draw
    DVTARGETDEVICE* _ptd;                // per IViewObject::Draw
    HDC             _hic;                // per IViewObject::Draw
    HDC             _hdc;                // per IViewObject::Draw
    LPCRECTL        _prcWBounds;         // per IViewObject::Draw
    DWORD           _dwContinue;         // per IViewObject::Draw
    BOOL            (CALLBACK * _pfnContinue) (ULONG_PTR); // per IViewObject::Draw
};



//+------------------------------------------------------------------------
//
//  Forms3 part-implementation of the Win32 API DrawFrameControl, which
//  doesn't work for checkboxes on metafiles. Currently function takes
//  checkbox and optionbuttons (and uses DrawFrameControl for Optionbuttons)
//  As soon as the Win32 API is fixed, replace body with system function
//
//-------------------------------------------------------------------------

BOOL FormsDrawGlyph(CDrawInfo *, LPGDIRECT, UINT, UINT);


//+---------------------------------------------------------------------------
//
//  Scrollbar utilities
//
//----------------------------------------------------------------------------

class CLayout;

#ifdef WIN16

typedef HRESULT (BUGCALL *PFN_ONSCROLL)(CLayout *, int, UINT, long, BOOL);

#define NV_DECLARE_ONSCROLL_METHOD(fn, FN, args)\
            static HRESULT BUGCALL FN args;\
            HRESULT BUGCALL fn args
#define DECLARE_ONSCROLL_METHOD(fn, FN, args)\
            static HRESULT BUGCALL FN args;\
            virtual HRESULT BUGCALL fn args
#define ONSCROLL_METHOD(klass, fn, FN)\
        (PFN_ONSCROLL)&klass::FN
#else

typedef HRESULT (BUGCALL CLayout::*PFN_ONSCROLL)(int, UINT, long, BOOL);

#define NV_DECLARE_ONSCROLL_METHOD(fn, FN, args)\
            HRESULT BUGCALL fn args
#define DECLARE_ONSCROLL_METHOD(fn, FN, args)\
            virtual HRESULT BUGCALL fn args
#define ONSCROLL_METHOD(klass, fn, FN)\
        (PFN_ONSCROLL)&klass::fn
#endif

void DrawScrollbar(
        CLayout *   pvOwner,
        int         iDirection,
        CFormDrawInfo * pDI,
        RECT *      prc,
        long        lExtentWidth,
        long        lExtentHeight,
        long        lPosition,
        long        lVisible,
        long        lSize,
        BOOL        fEnabled,
        BOOL        fInvalidate,
        BOOL        fDrawButtons = TRUE);

class CMessage;


int GetScrollbarArea(
        const POINT &   pPt,
        CLayout *       pvOwner,
        int             iDirection,
        const RECT &    rc,
        long            lExtentWidth,
        long            lExtentHeight,
        long            lPosition,
        long            lVisible,
        long            lSize,
        CServer *       pServerHost);


void OnScrollbarMouseMessage(
        CLayout *       pvOwner,
        int             iDirection,
        RECT *          prc,
        long            lExtentWidth,
        long            lExtentHeight,
        long            lPosition,
        long            lVisible,
        long            lSize,
        CServer *       pServerHost,
        CMessage *      pMessage,
        PFN_ONSCROLL    pfnOnScroll);

void UpdateScrollbar(
        CFormDrawInfo * pDI,
        CLayout *       pvOwner,
        int             iDirection,
        RECT *          prc,
        long            lExtentWidth,
        long            lExtentHeight,
        long            lPosition,
        long            lVisible,
        long            lSize,
        CServer *       pServerHost,
        long            lPositionOld,
        BOOL            fEnabled);

void InitScrollbarTiming();

//+-------------------------------------------------------------------------
//
//  String-to-himetric utilities
//
//--------------------------------------------------------------------------

#define UNITS_BUFLEN    10

enum UNITS
{
    UNITS_MIN       = 0,
    UNITS_INCH      = 0,
    UNITS_CM        = 1,
    UNITS_POINT     = 2,
    UNITS_UNKNOWN   = 3
};

HRESULT StringToHimetric(TCHAR * sz, UNITS * pUnits, long * lValue);
HRESULT HimetricToString(long lVal, UNITS units, TCHAR * szRet, int cch);

//+-------------------------------------------------------------------------
//
//  Floating point Points units to long himetric
//
//--------------------------------------------------------------------------

#define USER_HORZ 0x00
#define USER_VERT 0x01
#define USER_POS  0x00
#define USER_SIZE 0x02

#define USER_DIMENSION_MASK 0x01
#define USER_SIZEPOS_MASK   0x02

#define IMPLEMENT_GETUSER(cls, prop, dwUserFlags) \
HRESULT                             \
cls::Get##prop##Single(float * pxf) \
{                                   \
    long  xl;                       \
    RRETURN_NOTRACE(                \
        THR_NOTRACE(GetUserHelper(  \
            Get##prop(&xl),         \
            &xl,                    \
            dwUserFlags,            \
            pxf)));                 \
}

#define IMPLEMENT_SETUSER(cls, prop, dwUserFlags)  \
STDMETHODIMP                        \
cls::Set##prop##Single(float xf)    \
{                                   \
    RRETURN(THR(Set##prop(          \
        SetUserHelper(              \
            xf,                     \
            dwUserFlags))));        \
}




//+---------------------------------------------------------------------------
//
//  DropButton utilities
//
//----------------------------------------------------------------------------

enum BUTTON_GLYPH {
    BG_UP, BG_DOWN, BG_LEFT, BG_RIGHT,
    BG_COMBO, BG_REFEDIT, BG_PLAIN, BG_REDUCE };

void DrawDropButton(
        CServer *    pvOwner,
        CDrawInfo *  pDI,
        RECT *       prc,
        BUTTON_GLYPH bg,
        BOOL         fEnabled);

inline long GetDBCx(BUTTON_GLYPH bg, long defCx)
{
    return BG_REFEDIT == bg ? max (450L, defCx) : defCx;
}

BOOL OnDropButtonLButtonDown(
        CTransform *    pTransform,
        RECT *          prc,
        CServer *       pServerHost,
        BUTTON_GLYPH    bg,
        BOOL            fEnabled,
        BOOL            fUseGW,
        BOOL            fDrawPressed);

BOOL OnDropButtonLButtonUp(void);

BOOL OnDropButtonMouseMove(const POINT & p, WPARAM wParam);

//+-------------------------------------------------------------------------
//
//  Help utilities
//
//--------------------------------------------------------------------------

HRESULT FormsHelp(TCHAR * szHelpFile, UINT uCmd, DWORD dwData);
inline TCHAR * GetFormsHelpFile()
{
    extern TCHAR g_achHelpFileName[];
    return g_achHelpFileName;
}

HRESULT GetTypeInfoForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        ITypeInfo ** ppTI);
HRESULT GetDocumentationForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        BSTR * pbstrName,
        DWORD * pdwHelpContextId,
        BSTR * pbstrHelpFile);
HRESULT GetNameForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        TCHAR * szName,
        int cch);
HRESULT GetHelpForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        DWORD * pdwId,
        TCHAR * szHelpFile,
        int cch);
#ifndef WIN16
HRESULT OnDialogHelp(
        class CBase * pBase,
        HELPINFO * phi,
        DWORD dwHelpContextID);
#endif // !WIN16

//+-------------------------------------------------------------------------
//
//  Enum:       EPART
//
//  Synopsis:   Error message parts. The parts are composed into
//              a message using the following template.  Square
//              brackets are used to indication optional components
//              of the message.
//
//                 [<EPART_ACTION>] <EPART_ERROR>
//
//                 [Solution:
//                 <EPART_SOLUTION>]
//
//              If EPART_ERROR is not specified, it is derived from
//              the HRESULT.
//
//--------------------------------------------------------------------------

enum EPART
{
    EPART_ACTION,
    EPART_ERROR,
    EPART_SOLUTION,
    EPART_LAST
};

//+-------------------------------------------------------------------------
//
//  Class:      CErrorInfo
//
//  Synopsis:   Rich error information object. Use GetErrorInfo() to get
//              the current error information object.  Use ClearErrorInfo()
//              to delete the current error information object.  Use
//              CloseErrorInfo to set the per thread error information object.
//
//--------------------------------------------------------------------------

MtExtern(CErrorInfo)

extern const GUID CLSID_CErrorInfo;

class CErrorInfo : public IErrorInfo
{
public:

    CErrorInfo();
    ~CErrorInfo();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CErrorInfo))

    // IUnknown methods

    DECLARE_FORMS_STANDARD_IUNKNOWN(CErrorInfo);

    // IErrorInfo methods

    STDMETHOD(GetGUID)(GUID *pguid);
    STDMETHOD(GetSource)(BSTR *pbstrSource);
    STDMETHOD(GetDescription)(BSTR *pbstrDescription);
    STDMETHOD(GetHelpFile)(BSTR *pbstrHelpFile);
    STDMETHOD(GetHelpContext)(DWORD *pdwHelpContext);

    // IErrorInfo2 methods

    STDMETHOD(GetDescriptionEx)(BSTR *pbstrDescription, BSTR *pbstrSolution);

    // Methods and members for setting error info

    void         SetTextV(EPART epart, UINT ids, void *pvArgs);
    void __cdecl SetText(EPART epart, UINT ids, ...);

    HRESULT     _hr;
    TCHAR *     _apch[EPART_LAST];

    DWORD       _dwHelpContext;     // Help context in Forms3 help file
    GUID        _clsidSource;       // Used to generate progid for GetSource

    IID         _iidInvoke;         // Used to generate action part of message
    DISPID      _dispidInvoke;      //      if set and _apch[EPART_ACTION]
    INVOKEKIND  _invkind;           //      not set.

    // Private helper functions

private:

    HRESULT     GetMemberName(BSTR *pbstrName);
};

CErrorInfo * GetErrorInfo();
void         ClearErrorInfo(struct THREADSTATE * pts = NULL);
void         CloseErrorInfo(HRESULT hr, REFCLSID clsidSource);
void __cdecl PutErrorInfoText(EPART epart, UINT ids, ...);

HRESULT GetErrorText(HRESULT hrError, LPTSTR pstr, int cch);


//+------------------------------------------------------------------------
//
//  Object extensions helper functions
//
//-------------------------------------------------------------------------

HRESULT ShowUIElement(IUnknown * pUnk, BOOL fShow);
HRESULT IsUIElementVisible(IUnknown * pUnk);

//+------------------------------------------------------------------------
//
//  Cached system metric value and such
//
//  Notes:  The variable g_cMetricChange is guaranteed to be different
//          every time the metrics we cache change.  This way you can
//          track changes to the cookie to know when metrics change, and
//          not each individual metric.
//
//          g_sizeScrollbar:
//
//              Stores the width/height of vert/horz scrollbars
//              in screen device units.
//
//          g_sizelScrollbar:
//
//              Stores the width/height of vert/horz scrollbars
//              in HIMETRICS (based on screen device scaling)
//
//          g_sizelScrollButton
//
//              Stores the width/height of horz/vert scrollbar
//              buttons in HIMETRICS (based on screen device scaling)
//
//          g_sizelScrollThumb
//
//              Stores the width/height of horz/vert scrollbar
//              buttons in HIMETRICS (based on screen device scaling)
//
//
//-------------------------------------------------------------------------

extern LONG     g_cMetricChange;

extern HBRUSH   g_hbr3DShadow;
extern HBRUSH   g_hbr3DFace;
extern HBRUSH   g_hbr3DHilite;
extern HBRUSH   g_hbr3DDkShadow;
extern HBRUSH   g_hbr3DLight;

extern  SIZE    g_sizeScrollbar;
extern  SIZEL   g_sizelScrollbar;
extern  SIZEL   g_sizelScrollButton;
extern  SIZEL   g_sizelScrollThumb;
extern  SIZE    g_sizePixelsPerInch;

#ifdef  IE6_WYSIWYG

extern  SIZE    g_sizeTargetPixelsPerInch;

#endif  // IE6_WYSIWYG

#if NEVER // we should not change the unit in Forms3 96.

  // User specified unit in control panel
extern  UNITS   g_unitsMeasure;

#endif //NEVER

  // Locale Information
extern  BOOL    g_fUSSystem;
extern  BOOL    g_fJapanSystem;
extern  BOOL    g_fKoreaSystem;
extern  BOOL    g_fCHTSystem;
extern  BOOL    g_fCHSSystem;
extern  LCID    g_lcidUserDefault ;

extern  UINT    g_cpDefault;

// hold for number shaping used by system
typedef enum NUMSHAPE
{
    NUMSHAPE_CONTEXT = 0,
    NUMSHAPE_NONE    = 1,
    NUMSHAPE_NATIVE  = 2,
};

extern  NUMSHAPE    g_iNumShape;
extern  DWORD       g_uLangNationalDigits;

void GetSystemNumberSettings(NUMSHAPE * piNumShape, DWORD * plangNationalDigits);

#ifndef NO_IME
// BUGBUG (cthrash) Check interaction with CTxtEdit
void CancelUndeterminedIMEString(HWND);
#endif

//+------------------------------------------------------------------------
//
//  Helper to convert singleline/multiline text
//
//-------------------------------------------------------------------------
HRESULT TextConvert(LPTSTR pszTextIn, BSTR *pBstrOut, BOOL fToGlyph);



//+------------------------------------------------------------------------
//
//  Helper to make the top-level browser window corresponding to the given
//  inplace hwnd, the top of all top-level browser windows, if any --- a
//  BringWindowToTop for windows created by diff threads in the same process
//
//-------------------------------------------------------------------------
BOOL MakeThisTopBrowserInProcess(HWND hwnd);


//+------------------------------------------------------------------------
//
//  Helpers to set/get integer of given size or type at given address
//
//-------------------------------------------------------------------------

void SetNumberOfSize (void * pv, int cb, long i );
long GetNumberOfSize (void * pv, int cb         );

void SetNumberOfType (void * pv, VARENUM vt, long i );
long GetNumberOfType (void * pv, VARENUM vt         );

//+------------------------------------------------------------------------
//
//  Helpers to set bits in a DWORD
//
//-------------------------------------------------------------------------


inline void SetBit( DWORD * pdw, DWORD dwMask, BOOL fBOOL )
{
    *pdw = fBOOL ? (*pdw | dwMask) : (*pdw & ~dwMask) ;
}


IStream *
BufferStream(IStream * pStm);

// Unit value OLE Automation Typedef
typedef long UNITVALUE;

//+------------------------------------------------------------------------
//
//  Debug utilities for faster BitBlt
//
//-------------------------------------------------------------------------

BOOL IsIdentityPalette(HPALETTE hpal);
BOOL IsIdentityBlt(HDC hdcS, HDC hdcD, int xWid);

//+------------------------------------------------------------------------
//
// Useful combinations of flags for IMsoCommandTarget
//
//-------------------------------------------------------------------------

#define MSOCMDSTATE_DISABLED    MSOCMDF_SUPPORTED
#define MSOCMDSTATE_UP          (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED)
#define MSOCMDSTATE_DOWN        (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED | MSOCMDF_LATCHED)
#define MSOCMDSTATE_NINCHED     (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED | MSOCMDF_NINCHED)

typedef struct _tagOLECMD OLECMD;
typedef struct _tagOLECMDTEXT OLECMDTEXT;

HRESULT CTQueryStatus(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext);

HRESULT CTExec(
        IUnknown *pUnk,
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

#ifdef WIN16
// Use VT_BOOL4 when automation type is not VARIANT_BOOL, but BOOL, which is
// 4 bytes, instead of 2

#define VT_BOOL4 0xfe   // 254 is not used for another common VT_*
#endif

//+---------------------------------------------------------------------------
//
// Enums
//
//----------------------------------------------------------------------------

typedef enum _fmBorderStyle
{
    fmBorderStyleNone = 0,
    fmBorderStyleSingle = 1,
    fmBorderStyleRaised = 2,
    fmBorderStyleSunken = 3,
    fmBorderStyleEtched = 4,
    fmBorderStyleBump = 5,
    fmBorderStyleRaisedMono = 6,
    fmBorderStyleSunkenMono = 7,
    fmBorderStyleDouble = 8,
    fmBorderStyleDotted = 9,
    fmBorderStyleDashed = 10
} fmBorderStyle;


MtExtern(CColorInfo)

class CColorInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CColorInfo))
    CColorInfo();
    CColorInfo(DWORD dwDrawAspect, LONG lindex, void FAR *pvAspect, DVTARGETDEVICE FAR *ptd, HDC hicTargetDev, unsigned cColorMax = 256);
    BOOL AddColor(BYTE red, BYTE green, BYTE blue)
    {
        if (_cColors < _cColorsMax)
        {
            _aColors[_cColors].peRed = red;
            _aColors[_cColors].peGreen = green;
            _aColors[_cColors].peBlue = blue;
            _cColors++;

            return TRUE;
        }
        return FALSE;
    }

    HRESULT AddColors(unsigned cColors, COLORREF *pColors);
    HRESULT AddColors(unsigned cColors, RGBQUAD *pColors);
    HRESULT AddColors(unsigned cColors, PALETTEENTRY *pColors);
    HRESULT AddColors(HPALETTE hpal);
    HRESULT AddColors(LOGPALETTE *pLogPalette);
    HRESULT AddColors(IViewObject *pVO);
    HRESULT GetColorSet(LPLOGPALETTE FAR *ppColorSet);
    unsigned GetCount() { return _cColors; };
    BOOL IsFull();

private:
    DWORD _dwDrawAspect;
    LONG _lindex;
    void FAR *_pvAspect;
    DVTARGETDEVICE FAR *_ptd;
    HDC _hicTargetDev;
    unsigned _cColorsMax;
    //
    // Don't move the following three items around
    // We use them as a LOGPALETTE
    //
    WORD _palVersion;
    unsigned _cColors;
    PALETTEENTRY _aColors[256];
};

HRESULT FaultInIEFeatureHelper(HWND hWnd,
            uCLSSPEC *pClassSpec,
            QUERYCONTEXT *pQuery, DWORD dwFlags);

HINSTANCE EnsureMLLoadLibrary();

#ifdef UNIX // Fix a compiler problem
inline long max(int x, long y) 
{
    return max((long)x, y);
}
#endif

#pragma INCMSG("--- End 'cdutil.hxx'")
#else
#pragma INCMSG("*** Dup 'cdutil.hxx'")
#endif

