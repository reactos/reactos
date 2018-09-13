/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4275 )
#define STRICT 1
#include <windows.h>
#ifdef UNIX
// This is apparently not needed. 
// When included it induces a lot of compile time errors, due to uuidof!
// When it isn't everything compiles just fine!
#else
#include <comdef.h>
#endif /* UNIX */

/// #define STRUCTURED_EXCEPTION 1  --- moved to common.inc.
#define USE_SHLWAPI 1
#define NO_OLE32_WRAPS 1
#ifdef UNIX
#define NOVTABLE 
#else
#define NOVTABLE __declspec(novtable)
#endif
#ifdef MSXML_EXPORT
#define DLLEXPORT   __declspec( dllexport )
#else
#define DLLEXPORT
#endif
#ifdef _DEBUG
#    define DBG 1
#endif
#include <tchar.h>
#include <shlwapi.h>
#include <shlwapip.h>
#ifdef UNIX
// Not needed under UNIX
#else
#ifndef _WIN64
#include <w95wraps.h>
#endif // _WIN64
#endif /* UNIX */
#ifdef UNIX
//   note: there is no builtin way to check the class of an object, so we just assume it is the correct class since
//	typeid is only used for debug Asserts
#define CHECKTYPEID(x,y) (1==1)
#else
#define CHECKTYPEID(x,y) (&typeid(x)==&typeid(y))
#endif

#define AssertPMATCH(p,c) Assert(p == null || CHECKTYPEID(*p, c))

#undef _tcslen
#define _tcslen lstrlen
#undef _tcsnicmp
#define _tcsnicmp StrCmpNI
extern void
IntToStr(int i, TCHAR * buf, int radix);
#undef _itot
#define _itot IntToStr
#undef _ttoi
#define _ttoi StrToInt
#define null 0
#define transient
#define SYNCHRONIZED(O)
typedef __int64 int64;
typedef unsigned char byte;

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))
/*
typedef win32::BOOL BOOL;
typedef win32::DWORD DWORD;
typedef win32::CHAR CHAR;
typedef win32::USHORT USHORT;
typedef win32::va_list va_list;
typedef win32::LPSTR LPSTR;
typedef win32::HRESULT HRESULT;
typedef win32::DWORD DWORD;
typedef win32::LPVOID LPVOID;
typedef win32::HANDLE HANDLE;
typedef win32::HWND HWND;
*/

// Compile time "Assert".  The compiler can't allocate an array of 0 elements.
// If the expression inside [ ] is false, the array size will be zero!

#ifdef UNIX
#define COMPILE_TIME_ASSERT(x,y)
#else
#ifdef _WIN64
// If we don't do this we get a zero length array error
#define COMPILE_TIME_ASSERT(x,y)
#else
#define COMPILE_TIME_ASSERT(x,y)  typedef int _farf_##x[sizeof(x) == (y)]
#endif
#endif

#include "../../debug/include/msxmldbg.h"
#include "memutil.h"
#include "tls.hxx"
#include "pointer.h"
#include "../base/_reference.hxx"
#include "../com/_unknown.hxx"
#include "../lang/object.hxx"
#include "base.hxx"
#include "mt.hxx"
#include "slot.hxx"
#include "_array.hxx"
#include "../lang/package.hxx"
#include "../com/_com.hxx"
#include "../com/_dispatch.hxx"
#include "../com/_gitpointer.hxx"
#include "../base/hresult.hxx"
#include "../base/guid.hxx"

#include "../io/package.hxx"
#include "../util/package.hxx"

#ifdef PROFILE
#include "../util/pure.h"
#endif

// Include for ActiveScript Engine
#include <activscp.h>
#include <mshtml.h>
#include <objsafe.h>

//#define DEBUG_OLEAUT

#ifdef DEBUG_OLEAUT

#define SysAllocString DebugSysAllocString
#define SysFreeString DebugSysFreeString
#define SysAllocStringLen DebugSysAllocStringLen
#define VariantClear DebugVariantClear
#define SysStringLen DebugSysStringLen

BSTR DebugSysAllocString(const OLECHAR *);
BSTR DebugSysAllocStringLen(const OLECHAR *, UINT);
void DebugSysFreeString(BSTR);
UINT DebugSysStringLen(BSTR);
HRESULT DebugVariantClear(VARIANTARG * pvarg);

#endif
