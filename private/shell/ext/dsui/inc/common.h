#ifndef __inc_common_h
#define __inc_common_h


/*-----------------------------------------------------------------------------
/ Avoid the C runtimes for new, delete etc.  Instead lets use LocalAlloc and
/ its goodness.
/----------------------------------------------------------------------------*/

#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus


/*-----------------------------------------------------------------------------
/ Debugging APIs (use the Macros, they make it easier and cope with correctly
/ removing debugging when it is disabled at built time).
/----------------------------------------------------------------------------*/

#define TRACE_COMMON_ASSERT    0x80000000
#define TRACE_COMMON_MISC      0x40000000

#ifdef DBG
EXTERN_C void DoTraceSetMask(DWORD dwMask);
EXTERN_C void DoTraceSetMaskFromCLSID(REFCLSID rCLSID);
EXTERN_C void DoTraceEnter(DWORD dwMask, LPCTSTR pName);
EXTERN_C void DoTraceLeave(void);
EXTERN_C LPCTSTR DoTraceGetCurrentFn(VOID);
EXTERN_C void DoTrace(LPCTSTR pFormat, ...);
EXTERN_C void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
EXTERN_C void DoTraceAssert(int iLine, LPTSTR pFilename);

#else // DBG not defined (e.g. retail build)

#define DoTraceMask(mask)
#define DoTraceSetMaskFromCLSID(rCLSID)
#define DoTraceEnter(dwMask, pName)
#define DoTraceLeave()
#define DoTraceGetCurrentFn() ("")
#define DoTrace 1 ? (void) 0: (void)
#define DoTraceGUID(pPrefix, rGUID)
#define DoTraceAssert( iLine , pFilename)

#endif // DBG



/*-----------------------------------------------------------------------------
/ Macros to ease the use of the debugging APIS.
/----------------------------------------------------------------------------*/

#if DBG
#define DSUI_DEBUG 1
#define debug if ( TRUE )
#else
#undef  DSUI_DEBUG
#define debug
#endif

#define TraceSetMask(dwMask)          debug DoTraceSetMask(dwMask)
#define TraceSetMaskFromCLSID(rCLSID) debug DoTraceSetMaskFromCLSID(rCLSID)
#define TraceEnter(dwMask, fn)        debug DoTraceEnter(dwMask, TEXT(fn))
#define TraceLeave                    debug DoTraceLeave

#define Trace                         debug DoTrace
#define TraceMsg(s)                   debug DoTrace(TEXT(s))
#define TraceGUID(s, rGUID)           debug DoTraceGUID(TEXT(s), rGUID)

#ifdef DSUI_DEBUG

#define TraceAssert(x) \
                { if ( !(x) ) DoTraceAssert(__LINE__, TEXT(__FILE__)); }

#define TraceLeaveResult(hr) \
                { HRESULT __hr = hr; if (FAILED(__hr)) Trace(TEXT("Failed (%08x)"), hr); TraceLeave(); return __hr; }

#define TraceLeaveVoid() \
                { TraceLeave(); return; }

#define TraceLeaveValue(value) \
                { TraceLeave(); return(value); }

#else
#define TraceAssert(x)
#define TraceLeaveResult(hr)    { return hr; }
#define TraceLeaveVoid()	{ return; }
#define TraceLeaveValue(value)  { return(value); }
#endif


//
// flow control helpers, these expect you to have a exit_gracefully: label
// defined in your function which is called to exit the body of the
// routine.
//

#define ExitGracefully(hr, result, text)            \
            { TraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
	    { if ( FAILED(hr) ) { TraceMsg(text); goto exit_gracefully; } }


//
// Some atomic free macros (should be replaced with calls to the shell ones)
//

#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }

#define DoILFree(pidl)                              \
        { ILFree(pidl); pidl = NULL; }


/*-----------------------------------------------------------------------------
/ String/byte helper macros
/----------------------------------------------------------------------------*/

#define StringByteSizeA(sz)         ((sz) ? ((lstrlenA(sz)+1)*SIZEOF(CHAR)):0)
#define StringByteSizeW(sz)         ((sz) ? ((lstrlenW(sz)+1)*SIZEOF(WCHAR)):0)

#define StringByteCopyA(pDest, iOffset, sz)         \
        { CopyMemory(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSizeA(sz)); }

#define StringByteCopyW(pDest, iOffset, sz)         \
        { CopyMemory(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSizeW(sz)); }

#ifndef UNICODE
#define StringByteSize              StringByteSizeA
#define StringByteCopy              StringByteCopyA
#else
#define StringByteSize              StringByteSizeW
#define StringByteCopy              StringByteCopyW
#endif

#define ByteOffset(base, offset)   (((LPBYTE)base)+offset)

//
// Lifted from ccstock.h
//

#ifndef InRange
#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))
#endif

#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))

//-----------------------------------------------------------------------------
// CUnknown - a base class for building objects.  (__cplusplus only)
//-----------------------------------------------------------------------------

#ifdef __cplusplus

extern LONG g_cRefCount;
#define GLOBAL_REFCOUNT     (g_cRefCount)

// This is used if we are building C++ COM objects, it wraps
// up the entire base class for a COM object (addref, release
// and QI) into a single manageable class that other objects
// end up referencing.

typedef struct
{
    const IID* piid;            // interface ID
    LPVOID  pvObject;           // pointer to the object
} INTERFACES, * LPINTERFACES;

class CUnknown 
{
    protected:
        LONG m_cRefCount;

    public:
        CUnknown();
        virtual ~CUnknown();
        
        STDMETHODIMP         HandleQueryInterface(REFIID riid, LPVOID* ppvObject, LPINTERFACES aInterfaces, int cif);
        STDMETHODIMP_(ULONG) HandleAddRef();
        STDMETHODIMP_(ULONG) HandleRelease();
};

#endif // __cplusplus


/*-----------------------------------------------------------------------------
/ Helper functions (misc.cpp)
/----------------------------------------------------------------------------*/

EXTERN_C HRESULT GetKeyForCLSID(REFCLSID clsid, LPCTSTR pSubKey, HKEY* phkey);
EXTERN_C HRESULT PutRegistryString(HINSTANCE hInstance, UINT uID, HKEY hKey, LPCTSTR pSubKey, LPCTSTR pValue);

EXTERN_C HRESULT GetRealWindowInfo(HWND hwnd, LPRECT pRect, LPSIZE pSize);
EXTERN_C VOID OffsetWindow(HWND hwnd, INT dx, INT dy);

EXTERN_C HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection);
EXTERN_C VOID SetDefButton(HWND hwndDlg, int idButton);

EXTERN_C HRESULT AllocStorageMedium(FORMATETC* pFmt, STGMEDIUM* pMedium, SIZE_T cbStruct, LPVOID* ppAlloc);
EXTERN_C HRESULT CopyStorageMedium(FORMATETC* pFmt, STGMEDIUM* pMediumDst, STGMEDIUM* pMediumSrc);

//
// The shell defines these on newer platforms, but for downlevel clients we will use our
// own home grown (stollen versions)
//

EXTERN_C BOOL GetGUIDFromString(LPCTSTR psz, GUID* pguid);
EXTERN_C INT  GetStringFromGUID(UNALIGNED REFGUID rguid, LPTSTR psz, INT cchMax);

#ifndef UNICODE
EXTERN_C BOOL GetGUIDFromStringW(LPCWSTR psz, GUID* pguid);
#else
#define GetGUIDFromStringW(a, b) GetGUIDFromString(a, b)
#endif

//
// replace LoadStringW with our own version that will work on the downlevel platforms
//

#ifndef UNICODE
EXTERN_C INT MyLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR pszBuffer, INT cchBuffer);
#define LoadStringW MyLoadStringW
#endif

#endif
