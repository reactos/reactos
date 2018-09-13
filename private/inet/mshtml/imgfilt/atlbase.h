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

#ifdef UNIX
#define MWNO_DEF_IN_TEMPLATES
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

#include <ddraw.h>

#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the _ASSERTE(x) macro
// in order to compile ATL
	#include <crtdbg.h>
#endif

#include <stddef.h>
#include <tchar.h>
#include <malloc.h>
#include <olectl.h>
#include <winreg.h>

#define _ATL_PACKING 8
#pragma pack(push, _ATL_PACKING)

#if defined (_CPPUNWIND) & (defined(_ATL_EXCEPTIONS) | defined(_AFX))
#define ATLTRY(x) try{x;} catch(...) {}
#else
#define ATLTRY(x) x;
#endif

#ifdef _DEBUG
void _cdecl AtlTrace(LPCTSTR lpszFormat, ...);
#define ATLTRACE            AtlTrace
#define ATLTRACENOTIMPL(funcname)   AtlTrace(_T("%s not implemented.\n"), funcname); return E_NOTIMPL
#else
inline void _cdecl AtlTrace(LPCTSTR , ...){}
#define ATLTRACE            1 ? (void)0 : AtlTrace
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#endif //_DEBUG

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
	HANDLE m_hHeap;
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
	_ASSERTE(m_hKey != NULL);
	return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
		(BYTE * const)&dwValue, sizeof(DWORD));
}

inline HRESULT CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	_ASSERTE(lpszValue != NULL);
	_ASSERTE(m_hKey != NULL);
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
	_ASSERTE(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	_ASSERTE(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
	_ASSERTE(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

// Make sure MFC's afxconv.h hasn't already been loaded to do this
#ifndef USES_CONVERSION
#ifndef _DEBUG
#define USES_CONVERSION int _convert; _convert
#else
#define USES_CONVERSION int _convert = 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers

inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	_ASSERTE(lpa != NULL);
	_ASSERTE(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	_ASSERTE(lpw != NULL);
	_ASSERTE(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#define A2W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : (\
		_convert = (lstrlenA(lpa)+1),\
		AtlA2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)))

#define W2A(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		_convert = (lstrlenW(lpw)+1)*2,\
		AtlW2AHelper((LPSTR) alloca(_convert), lpw, _convert)))

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

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
	#define T2COLE(lpa) A2CW(lpa)
	#define T2OLE(lpa) A2W(lpa)
	#define OLE2CT(lpo) W2CA(lpo)
	#define OLE2T(lpo) W2A(lpo)
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

#ifndef _ATL_NO_OLEAUT
inline BSTR OLE2BSTR(LPCOLESTR lp) {return ::SysAllocString(lp);}
#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return ::SysAllocString(A2COLE(lp));}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {return ::SysAllocString(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {USES_CONVERSION; return ::SysAllocString(W2COLE(lp));}
#else
	inline BSTR T2BSTR(LPCTSTR lp) {USES_CONVERSION; return ::SysAllocString(T2COLE(lp));}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return ::SysAllocString(A2COLE(lp));}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#endif
#endif  // !_ATL_NO_OLEAUT
#endif //!USES_CONVERSION

#pragma pack(pop)

#endif // __ATLBASE_H__

/////////////////////////////////////////////////////////////////////////////
