// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#ifndef __cplusplus
    #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4512) // can't generate assignment operator (so what?)
#pragma warning(disable: 4514) // unreferenced inlines are common
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4237) // bool
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif //!_ATL_NO_PRAGMA_WARNINGS

#include <windows.h>
#include <winnls.h>
#include <ole2.h>

#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the ASSERT(x) macro
// in order to compile ATL
    #include <crtdbg.h>
#endif

#include <stddef.h>
#include <tchar.h>
//#include <olectl.h>
//#include <winreg.h>

#define _ATL_PACKING 8
#pragma pack(push, _ATL_PACKING)

#if defined (_CPPUNWIND) & (defined(_ATL_EXCEPTIONS) | defined(_AFX))
#define ATLTRY(x) try{x;} catch(...) {}
#else
#define ATLTRY(x) x;
#endif


/////////////////////////////////////////////////////////////////////////////
// Master version numbers

#define _ATL     1      // ActiveX Template Library
#define _ATL_VER 0x0110 // ActiveX Template Library version 1.10

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _ATL_NO_FORCE_LIBS
    #pragma comment(lib, "kernel32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "oleaut32.lib")
    #pragma comment(lib, "olepro32.lib")
    #pragma comment(lib, "uuid.lib")
    #pragma comment(lib, "advapi32.lib")
#endif // _ATL_NO_FORCE_LIBS


/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComCriticalSection
{
public:
    void Lock() {EnterCriticalSection(&m_sec);}
    void Unlock() {LeaveCriticalSection(&m_sec);}
    void Init() {InitializeCriticalSection(&m_sec);}
    void Term() {DeleteCriticalSection(&m_sec);}
    CRITICAL_SECTION m_sec;
};

class CComAutoCriticalSection
{
public:
    void Lock() {EnterCriticalSection(&m_sec);}
    void Unlock() {LeaveCriticalSection(&m_sec);}
    CComAutoCriticalSection() {InitializeCriticalSection(&m_sec);}
    ~CComAutoCriticalSection() {DeleteCriticalSection(&m_sec);}
    CRITICAL_SECTION m_sec;
};

class CComFakeCriticalSection
{
public:
    void Lock() {}
    void Unlock() {}
    void Init() {}
    void Term() {}
};

class CComMultiThreadModel
{
public:
    static ULONG WINAPI Increment(LPLONG p) {return InterlockedIncrement(p);}
    static ULONG WINAPI Decrement(LPLONG p) {return InterlockedDecrement(p);}
    typedef CComAutoCriticalSection AutoCriticalSection;
    typedef CComCriticalSection CriticalSection;
};

class CComSingleThreadModel
{
public:
    static ULONG WINAPI Increment(LPLONG p) {return ++(*p);}
    static ULONG WINAPI Decrement(LPLONG p) {return --(*p);}
    typedef CComFakeCriticalSection AutoCriticalSection;
    typedef CComFakeCriticalSection CriticalSection;
};

#ifndef _ATL_SINGLE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_FREE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#if defined(_ATL_SINGLE_THREADED)
    typedef CComSingleThreadModel CComObjectThreadModel;
    typedef CComSingleThreadModel CComGlobalsThreadModel;
#elif defined(_ATL_APARTMENT_THREADED)
    typedef CComSingleThreadModel CComObjectThreadModel;
    typedef CComMultiThreadModel CComGlobalsThreadModel;
#else
    typedef CComMultiThreadModel CComObjectThreadModel;
    typedef CComMultiThreadModel CComGlobalsThreadModel;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComModule

struct _ATL_OBJMAP_ENTRY; // fwd decl

struct _ATL_REGMAP_ENTRY
{
    LPCTSTR     szKey;
    LPCTSTR     szData;
};

class CComModule
{
// Operations
public:
    void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h);
    void Term();

    LONG Lock() {return CComGlobalsThreadModel::Increment(&m_nLockCnt);}
    LONG Unlock() {return CComGlobalsThreadModel::Decrement(&m_nLockCnt);}
    LONG GetLockCount() {return m_nLockCnt;}

    HINSTANCE GetModuleInstance() {return m_hInst;}
    HINSTANCE GetResourceInstance() {return m_hInst;}
    HINSTANCE GetTypeLibInstance() {return m_hInst;}
    HINSTANCE GetRegistryResourceInstance() {return m_hInst;}

    // Registry support (helpers)
    HRESULT RegisterTypeLib(LPCTSTR lpszIndex = NULL);
    HRESULT RegisterServer(BOOL bRegTypeLib = FALSE);
    HRESULT UnregisterServer();

    // Resource-based Registration
    HRESULT WINAPI UpdateRegistryFromResource(LPCTSTR lpszRes, BOOL bRegister,
        struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
    HRESULT WINAPI UpdateRegistryFromResource(UINT nResID, BOOL bRegister,
        struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
    #ifdef _ATL_STATIC_REGISTRY
    // Statically linking to Registry Ponent
    HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
        struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
    HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
        struct _ATL_REGMAP_ENTRY* pMapEntries = NULL);
    #endif //_ATL_STATIC_REGISTRY

    // Standard Registration
    HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
        LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister);
    HRESULT WINAPI RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
        LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags);
    HRESULT WINAPI UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
        LPCTSTR lpszVerIndProgID);

    // Register/Revoke All Class Factories with the OS (EXE only)
    HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags);
    HRESULT RevokeClassObjects();

    // Obtain a Class Factory (DLL only)
    HRESULT GetClassObject(REFCLSID rclsid, REFIID riid,
        LPVOID* ppv);

// Attributes
public:
    HINSTANCE m_hInst;
    _ATL_OBJMAP_ENTRY* m_pObjMap;
    LONG m_nLockCnt;

    CComGlobalsThreadModel::CriticalSection m_csTypeInfoHolder;
    CComGlobalsThreadModel::CriticalSection m_csObjMap;
};

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
    CRegKey();
    ~CRegKey();

// Attributes
public:
    operator HKEY() const;
    HKEY m_hKey;

// Operations
public:
    LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
    LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
    LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

    LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
    static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
        LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

    LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
        LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
        REGSAM samDesired = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
        LPDWORD lpdwDisposition = NULL);
    LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
        REGSAM samDesired = KEY_ALL_ACCESS);
    LONG Close();
    HKEY Detach();
    void Attach(HKEY hKey);
    LONG DeleteSubKey(LPCTSTR lpszSubKey);
    LONG RecurseDeleteKey(LPCTSTR lpszKey);
    LONG DeleteValue(LPCTSTR lpszValue);
};

inline CRegKey::CRegKey()
{m_hKey = NULL;}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey::operator HKEY() const
{return m_hKey;}

inline LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
    ASSERT(m_hKey != NULL);
    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
        (BYTE * const)&dwValue, sizeof(DWORD));
}

inline HRESULT CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    ASSERT(lpszValue != NULL);
    ASSERT(m_hKey != NULL);
    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_SZ,
        (BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
}

inline HKEY CRegKey::Detach()
{
    HKEY hKey = m_hKey;
    m_hKey = NULL;
    return hKey;
}

inline void CRegKey::Attach(HKEY hKey)
{
    ASSERT(m_hKey == NULL);
    m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
    ASSERT(m_hKey != NULL);
    return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
    ASSERT(m_hKey != NULL);
    return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}


//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes)
    {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : malloc(cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }

    ~TempBuffer()
    { if (m_pBuf && m_fHeapAlloc) free(m_pBuf); }

    void *GetBuffer()
    { return m_pBuf; }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

//
// Note: allocate lstrlenW(widestr) * 2 because its possible for a UNICODE 
// character to map to 2 ansi characters this is a quick guarantee that enough
// space will be allocated.
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

#define STR_BSTR   0
#define STR_OLESTR 1
#define BSTRFROMANSI(x)    (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPSTR)(x), STR_OLESTR)
#define BSTRFROMRESID(x)   (BSTR)MakeWideStrFromResourceId(x, STR_BSTR)
#define OLESTRFROMRESID(x) (LPOLESTR)MakeWideStrFromResourceId(x, STR_OLESTR)
#define COPYOLESTR(x)      (LPOLESTR)MakeWideStrFromWide(x, STR_OLESTR)
#define COPYBSTR(x)        (BSTR)MakeWideStrFromWide(x, STR_BSTR)

LPWSTR MakeWideStrFromAnsi(LPSTR, BYTE bType);
LPWSTR MakeWideStrFromResourceId(WORD, BYTE bType);
LPWSTR MakeWideStrFromWide(LPWSTR, BYTE bType);

// REVIEW: All macros aren't implemented correctly yet! [jm]

#define A2W(lpa, lpw)   MAKE_WIDEPTR_FROMANSI(lpw, lpa)
#define W2A(lpw, lpa)   MAKE_ANSIPTR_FROMWIDE(lpa, lpw)

#define A2CW(lpa, lpcw) A2W(lpa, lpcw)
#define W2CA(lpw, lpca) W2A(lpw, lpca)

#ifdef _UNICODE
    #define T2A W2A
    #define A2T A2W
    inline LPWSTR T2W(LPTSTR lp) { return lp; }
    inline LPTSTR W2T(LPWSTR lp) { return lp; }
    #define T2CA W2CA
    #define A2CT A2CW
    inline LPCWSTR T2CW(LPCTSTR lp) { return lp; }
    inline LPCTSTR W2CT(LPCWSTR lp) { return lp; }
#else
    #define T2W A2W
    #define W2T W2A
    inline LPSTR T2A(LPTSTR lp) { return lp; }
    inline LPTSTR A2T(LPSTR lp) { return lp; }
    #define T2CW A2CW
    #define W2CT W2CA
    inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
    inline LPCTSTR A2CT(LPCSTR lp) { return lp; }
#endif

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
    inline size_t ocslen(LPCOLESTR x) { return lstrlenW(x); }
    inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return lstrcpyW(dest, src); }
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNextW(lp);}
    inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
    inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
    inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
    inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
    inline size_t ocslen(LPCOLESTR x) { return lstrlen(x); }
    inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return lstrcpy(dest, src); }
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNext(lp);}
    inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
    inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
    inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
    inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
#else
    inline size_t ocslen(LPCOLESTR x) { return lstrlenW(x); }
    //lstrcpyW doesn't work on Win95, so we do this
    inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src)
    {return (LPOLESTR) memcpy(dest, src, (lstrlenW(src)+1)*sizeof(WCHAR));}
    //CharNextW doesn't work on Win95 so we use this
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return (LPOLESTR)(lp+1);}
    #define T2COLE(lpa, lpco)   A2CW(lpa, lpco)
    #define T2OLE(lpa, lpo)     A2W(lpa, lpo)
    #define OLE2CT(lpo, lpct)   W2CA(lpo, lpct)
    #define OLE2T(lpo, lpt)     W2A(lpo, lpt)
#endif

#ifdef OLE2ANSI
    inline LPOLESTR A2OLE(LPSTR lp) { return lp;}
    inline LPSTR OLE2A(LPOLESTR lp) { return lp;}
    #define W2OLE W2A
    #define OLE2W A2W
    inline LPCOLESTR A2COLE(LPCSTR lp) { return lp;}
    inline LPCSTR OLE2CA(LPCOLESTR lp) { return lp;}
    #define W2COLE W2CA
    #define OLE2CW A2CW
#else
    inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
    inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
    #define A2OLE A2W
    #define OLE2A W2A
    inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
    inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
    #define A2COLE A2CW
    #define OLE2CA W2CA
#endif

inline BSTR OLE2BSTR(LPCOLESTR lp) {return ::SysAllocString(lp);}
#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
    inline BSTR T2BSTR(LPCTSTR lp)  { return ::SysAllocString(lp); }
    inline BSTR A2BSTR(LPCSTR lp)   { A2COLE(lp, lpo); return ::SysAllocString(lpo); }
    inline BSTR W2BSTR(LPCWSTR lp)  { return ::SysAllocString(lp); }
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
    inline BSTR T2BSTR(LPCTSTR lp)  { return ::SysAllocString(lp); }
    inline BSTR A2BSTR(LPCSTR lp)   { return ::SysAllocString(lp); }
    inline BSTR W2BSTR(LPCWSTR lp)  { W2COLE(lp, lpo); return ::SysAllocString(lpo); }
#else
    inline BSTR T2BSTR(LPCTSTR lp)  { T2COLE(lp, lpo); return ::SysAllocString(lpo); }
    inline BSTR A2BSTR(LPCSTR lp)   { A2COLE(lp, lpo); return ::SysAllocString(lpo);}  
    inline BSTR W2BSTR(LPCWSTR lp)  { return ::SysAllocString(lp); }
#endif

#pragma pack(pop)

#endif // __ATLBASE_H__

/////////////////////////////////////////////////////////////////////////////
