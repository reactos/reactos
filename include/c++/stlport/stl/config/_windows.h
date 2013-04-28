/*
 * Copyright (c) 1997-1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * Copyright (c) 2003
 * Francois Dumont
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */
#ifndef _STLP_INTERNAL_WINDOWS_H
#define _STLP_INTERNAL_WINDOWS_H

#if !defined (_STLP_PLATFORM)
#  define _STLP_PLATFORM "Windows"
#endif

#if !defined (_STLP_BIG_ENDIAN) && !defined (_STLP_LITTLE_ENDIAN)
#  if defined (_MIPSEB)
#    define _STLP_BIG_ENDIAN 1
#  endif
#  if defined (__i386) || defined (_M_IX86) || defined (_M_ARM) || \
      defined (__amd64__) || defined (_M_AMD64) || defined (__x86_64__) || \
      defined (__alpha__)
#    define _STLP_LITTLE_ENDIAN 1
#  endif
#  if defined (__ia64__)
    /* itanium allows both settings (for instance via gcc -mbig-endian) - hence a seperate check is required */
#    if defined (__BIG_ENDIAN__)
#      define _STLP_BIG_ENDIAN 1
#    else
#      define _STLP_LITTLE_ENDIAN 1
#    endif
#  endif
#endif /* _STLP_BIG_ENDIAN */

#if !defined (_STLP_WINDOWS_H_INCLUDED)
#  define _STLP_WINDOWS_H_INCLUDED
#  if defined (__BUILDING_STLPORT)
#    include <stl/config/_native_headers.h>
/* Here we define _STLP_OUTERMOST_HEADER_ID to avoid indirect inclusion
 * of STLport stuffs from C/C++ Standard headers exposed by STLport
 * as configuration is not yet completed. */
#    if !defined (_STLP_OUTERMOST_HEADER_ID)
#      define _STLP_OUTERMOST_HEADER_ID 0x100
#    endif
#    if !defined (WIN32_LEAN_AND_MEAN)
#      define WIN32_LEAN_AND_MEAN
#    endif
#    if !defined (VC_EXTRALEAN)
#      define VC_EXTRALEAN
#    endif
/* Don't let windows.h define its min and max macros. */
#    if !defined (NOMINMAX)
#      define NOMINMAX
#    endif
#    if !defined (STRICT)
#      define STRICT
#    endif
#    if defined (_STLP_USE_MFC)
#      include <afx.h>
#    else
#      include <windows.h>
#    endif
#    if (_STLP_OUTERMOST_HEADER_ID == 0x100)
#      undef _STLP_OUTERMOST_HEADER_ID
#    endif
#  else
/* This section serves as a replacement for windows.h header. */
#    if defined (__cplusplus)
extern "C" {
#    endif
#    if (defined (_M_AMD64) || defined (_M_IA64) || (!defined (_STLP_WCE) && defined (_M_MRX000)) || defined (_M_ALPHA) || \
        (defined (_M_PPC) && (_STLP_MSVC_LIB >= 1000))) && !defined (RC_INVOKED)
#      define InterlockedIncrement       _InterlockedIncrement
#      define InterlockedDecrement       _InterlockedDecrement
#      define InterlockedExchange        _InterlockedExchange
#      define _STLP_STDCALL
#    else
#      if defined (_MAC)
#        define _STLP_STDCALL _cdecl
#      else
#        define _STLP_STDCALL __stdcall
#      endif
#    endif

#    if defined (_STLP_NEW_PLATFORM_SDK) && !defined(_WDMDDK_)
/* Define WIN32_NO_STATUS to prevent status codes redefinitions */
#      if !defined(WIN32_NO_STATUS)
#        define WIN32_NO_STATUS
#      endif
#      include <windef.h>

#ifndef InterlockedIncrement
_STLP_IMPORT_DECLSPEC LONG _STLP_STDCALL InterlockedIncrement(IN OUT LONG volatile *);
_STLP_IMPORT_DECLSPEC LONG _STLP_STDCALL InterlockedDecrement(IN OUT LONG volatile *);
_STLP_IMPORT_DECLSPEC LONG _STLP_STDCALL InterlockedExchange(IN OUT LONG volatile *, LONG);
_STLP_IMPORT_DECLSPEC void _STLP_STDCALL Sleep(DWORD);
#endif
#      if defined (_WIN64)
_STLP_IMPORT_DECLSPEC void* _STLP_STDCALL _InterlockedExchangePointer(void* volatile *, void*);
#      endif
#    elif !defined (_STLP_WCE)
/* boris : for the latest SDK, you may actually need the other version of the declaration (above)
 * even for earlier VC++ versions. There is no way to tell SDK versions apart, sorry ...
 */
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedIncrement(long volatile *);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedDecrement(long volatile *);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedExchange(long volatile *, long);
#    else
/* start of eMbedded Visual C++ specific section */
#      include <stl/config/_native_headers.h>

/* Don't let windef.h define its min and max macros. */
#      if !defined (NOMINMAX)
#        define NOMINMAX
#      endif
#      include <windef.h> /* needed for basic windows types */

       /** in SDKs generated with PB5, windef.h somehow includes headers which then
       define setjmp. */
#      if (_WIN32_WCE >= 0x500)
#        define _STLP_NATIVE_SETJMP_H_INCLUDED
#      endif

#      ifndef _WINBASE_ /* winbase.h already included? */
long WINAPI InterlockedIncrement(long*);
long WINAPI InterlockedDecrement(long*);
long WINAPI InterlockedExchange(long*, long);
#      endif

#      ifndef __WINDOWS__ /* windows.h already included? */

#        if defined (x86)
#          include <winbase.h> /* needed for inline versions of Interlocked* functions */
#        endif

#        ifndef _MFC_VER

#          define MessageBox MessageBoxW
int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

#          define wvsprintf wvsprintfW
int WINAPI wvsprintfW(LPWSTR, LPCWSTR, va_list ArgList);

void WINAPI ExitThread(DWORD dwExitCode);

#          if !defined (COREDLL)
#            define _STLP_WCE_WINBASEAPI DECLSPEC_IMPORT
#          else
#            define _STLP_WCE_WINBASEAPI
#          endif

_STLP_WCE_WINBASEAPI int WINAPI
MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                    int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

_STLP_WCE_WINBASEAPI UINT WINAPI GetACP();

_STLP_WCE_WINBASEAPI BOOL WINAPI TerminateProcess(HANDLE hProcess, DWORD uExitCode);

#          define OutputDebugString OutputDebugStringW
void WINAPI OutputDebugStringW(LPCWSTR);

_STLP_WCE_WINBASEAPI void WINAPI Sleep(DWORD);

#          undef _STLP_WCE_WINBASEAPI

#        endif /* !_MFC_VER */

#      endif /* !__WINDOWS__ */

/* end of eMbedded Visual C++ specific section */
#    endif

#    if !defined (_STLP_WCE)
_STLP_IMPORT_DECLSPEC void _STLP_STDCALL OutputDebugStringA(const char* lpOutputString);
#    endif

#    if defined (InterlockedIncrement) && defined(_MSC_VER)
#      pragma intrinsic(_InterlockedIncrement)
#      pragma intrinsic(_InterlockedDecrement)
#      pragma intrinsic(_InterlockedExchange)
#      if defined (_WIN64)
#        pragma intrinsic(_InterlockedExchangePointer)
#      endif
#    endif
#    if defined (__cplusplus)
} /* extern "C" */
#    endif

#  endif

/* Here we use a macro different than the InterlockedExchangePointer SDK one
 * to avoid macro definition conflict. */
#  if !defined (_WIN64)
/* Under 32 bits platform we rely on a simple InterlockedExchange call. */
#    if defined (__cplusplus) && defined(__BUILDING_STLPORT)
/* We do not define this function if we are not in a C++ translation unit just
 * because of the 'inline' keyword portability issue it would introduce. We will
 * have to fix it the day we need this function for a C translation unit.
 */
inline
void* _STLP_CALL STLPInterlockedExchangePointer(void* volatile* __a, void* __b) {
#      if defined (_STLP_MSVC)
/* Here MSVC produces warning if 64 bits portability issue is activated.
 * MSVC do not see that _STLP_ATOMIC_EXCHANGE_PTR is a macro which content
 * is based on the platform, Win32 or Win64
 */
#        pragma warning (push)
#        pragma warning (disable : 4311) // pointer truncation from void* to long
#        pragma warning (disable : 4312) // conversion from long to void* of greater size
#      endif
#      if !defined (_STLP_NO_NEW_STYLE_CASTS)
  return reinterpret_cast<void*>(InterlockedExchange(reinterpret_cast<long*>(const_cast<void**>(__a)),
                                                     reinterpret_cast<long>(__b)));
#      else
  return (void*)InterlockedExchange((long*)__a, (long)__b);
#      endif
#      if defined (_STLP_MSVC)
#        pragma warning (pop)
#      endif
}
#    endif
#  else
#    define STLPInterlockedExchangePointer _InterlockedExchangePointer
#  endif

#endif /* _STLP_WINDOWS_H_INCLUDED */

/* _STLP_WIN95_LIKE signal the Windows 95 OS or assimilated Windows OS version that
 * has Interlockeded[Increment, Decrement] Win32 API functions not returning modified
 * value.
 */
#if (defined (WINVER) && (WINVER < 0x0410) && (!defined (_WIN32_WINNT) || (_WIN32_WINNT < 0x400))) || \
    (!defined (WINVER) && (defined (_WIN32_WINDOWS) && (_WIN32_WINDOWS < 0x0410) || \
                          (defined (_WIN32_WINNT) && (_WIN32_WINNT < 0x400))))
#  define _STLP_WIN95_LIKE
#endif

/* Between Windows 95 (0x400) and later Windows OSes an API enhancement forces us
 * to change _Refcount_Base internal struct. As _Refcount_base member methods might
 * be partially inlined we need to check that STLport build/use are coherent. To do
 * so we try to generate a link time error thanks to the following macro.
 * This additional check is limited to old compilers that might still be used with
 * Windows 95. */
#if (defined (_DEBUG) || defined (_STLP_DEBUG)) && \
    (defined (_STLP_MSVC) && (_STLP_MSVC < 1310) || \
     defined (__GNUC__) && (__GNUC__ < 3))
/* We invert symbol names based on macro detection, when building for Windows
 * 95 we expose a
 * building_for_windows95_or_previous_but_library_built_for_windows98_or_later
 * function in order to have a more obvious link error message signaling how
 * the lib has been built and how it is used. */
#  if defined (__BUILDING_STLPORT)
#    if defined (_STLP_WIN95_LIKE)
#      define _STLP_SIGNAL_RUNTIME_COMPATIBILITY building_for_windows95_but_library_built_for_at_least_windows98
#    else
#      define _STLP_SIGNAL_RUNTIME_COMPATIBILITY building_for_at_least_windows98_but_library_built_for_windows95
#    endif
#  else
#    if defined (_STLP_WIN95_LIKE)
#      define _STLP_CHECK_RUNTIME_COMPATIBILITY building_for_windows95_but_library_built_for_at_least_windows98
#    else
#      define _STLP_CHECK_RUNTIME_COMPATIBILITY building_for_at_least_windows98_but_library_built_for_windows95
#    endif
#  endif
#endif

#if defined (__WIN16) || defined (WIN16) || defined (_WIN16)
#  define _STLP_WIN16
#else
#  define _STLP_WIN32
#endif

#if 0
#if defined(_STLP_WIN32)
#  define _STLP_USE_WIN32_IO /* CreateFile/ReadFile/WriteFile */
#endif
#endif

#if defined(__MINGW32__) && !defined(_STLP_USE_STDIO_IO)
#  define _STLP_USE_WIN32_IO /* CreateFile/ReadFile/WriteFile */
#endif /* __MINGW32__ */

#ifdef _STLP_WIN16
#  define _STLP_USE_UNIX_EMULATION_IO /* _open/_read/_write */
#  define _STLP_LDOUBLE_80
#endif

#endif /* _STLP_INTERNAL_WINDOWS_H */
