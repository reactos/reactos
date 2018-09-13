//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       unicwrap.cpp
//
//  Contents:   Wrappers for all Unicode functions used in MSHTML.
//              Any Unicode parameters/structure fields/buffers are converted
//              to ANSI, and then the corresponding ANSI version of the function
//              is called.
//
//----------------------------------------------------------------------------

// This file expects to be compiled UNICODE

//----------------------------------------------------------------------------
//
//  HOW TO WRITE A WRAPPER
//
//----------------------------------------------------------------------------
//
//  Suppose you want to write a wrapper for CreateMutex.
//
//  -   Look up CreateMutex and see that it is a function in KERNEL32.DLL.
//      If your function is in some other DLL, make the appropriate changes
//      to the instructions below.
//
//  -   Write a wrapper for your function, wrapped inside the appropriate
//      #ifdef for the DLL being wrapped.
//
//          #ifdef NEED_KERNEL32_WRAPPER
//          STDAPI_(HANDLE) WINAPI
//          CreateMutexWrapW(...)
//          {
//              VALIDATE_PROTOTYPE(CreateMutex);
//              blahblahblah
//          }
//          #endif // NEED_KERNEL32_WRAPPER
//
//  -   If your wrapped function is in a DLL other than KERNEL32,
//      USER32, GDI32, or ADVAPI32, then you need to write a delay-load
//      stub in dllload.c.
//
//      Furthermore, if the delay-load stub uses DELAY_MAP instead of
//      DELAY_LOAD (SHELL32, WINMM, COMDLG32), then the line
//
//          VALIDATE_PROTOTYPE(WrappedFunction)
//
//      needs to be changed to
//
//          VALIDATE_PROTOTYPE_DELAYLOAD(WrappedFunction, _WrappedFunction)
//
//  -   Add a line for your function to thunk.h.  In our case, we would
//      look for the #ifndef NEED_KERNEL32_WRAPPER section and add the
//      line
//
//          #define CreateMutexWrapW CreateMutexW
//
//      (This prevents shlwapi from trying to call a nonexistent wrapper.)
//
//  -   Add a line for your function to shlwapi.src.
//
//          KERNEL32_WRAP(CreateMutexW)     @nnn NONAME PRIVATE
//
//  -   If your wrapper is high-frequency, you may want to use
//      FORWARD_AW or THUNK_AW to make it faster on NT.
//
//----------------------------------------------------------------------------

// WARNING: if you add a new call to FORWARD_AW or THUNK_AW (or modify the
// macros), make sure you build this file on both x86 and non-x86 (or
// alternately, build w/ PERF_ASM=0 on x86) to make sure there are no
// syntax errors.  the macros are *very* different between x86 and non-x86,
// so successful compilation on one platform in no way guarantees success
// on the other.  i speak from experience, not theory...

#include "priv.h"
#define _SHELL32_
#define _SHDOCVW_
#include <shellapi.h>
#include <commctrl.h>       // for treeview
#include "unicwrap.h"
#include <wininet.h>            // INTERNET_MAX_URL_LENGTH
#include <winnetp.h>
#include "mlui.h"

#include <platform.h>
#include "apithk.h"

#define DM_PERF     0    // perf stats

#ifndef ANSI_SHELL32_ON_UNIX
#define UseUnicodeShell32() ( g_bRunningOnNT )
#else
#define UseUnicodeShell32() ( FALSE )
#endif

//
//  Do this in every wrapper function to make sure the wrapper
//  prototype matches the function it is intending to replace.
//
#define VALIDATE_PROTOTYPE(f) if (f##W == f##WrapW) 0
#define VALIDATE_PROTOTYPEA(f) if (f##A == f##WrapA) 0
#define VALIDATE_PROTOTYPEX(f) if (f == f##Wrap) 0
#define VALIDATE_PROTOTYPE_DELAYLOAD(fWrap, fDelay) if (fDelay##W == fWrap##WrapW) 0
#define VALIDATE_PROTOTYPE_DELAYLOADX(fWrap, fDelay) if (fDelay == fWrap##Wrap) 0
#define VALIDATE_PROTOTYPE_NO_W(f) if (f## == f##Wrap) 0


//
//  Do this in every wrapper function that has an output parameter.
//  It raises assertion failures on the main code path so that
//  the same assertions are raised on NT and 95.  The CStrOut class
//  doesn't like it when you say that an output buffer is NULL yet
//  has nonzero length.  Without this macro, the bug would go undetected
//  on NT and appear only on Win95.
//
// BUGBUG raymondc - Turn this on after fixing all the RIPs it catches
// (otherwise we assert too much)
#if 0
#define VALIDATE_OUTBUF(s, cch) ASSERT((s) != NULL || (cch) == 0)
#else
#define VALIDATE_OUTBUF(s, cch)
#endif

// compiler should do this opt for us (call-thunk => jmp), but it doesn't
// so we do it ourselves (i raided it and they're adding it to vc6.x so
// hopefully we'll get it someday)
#if _X86_
#define PERF_ASM        1       // turn on inline-asm opts
#endif

// todo??? #ifdef SUNDOWN    #undef PERF_ASM

#if PERF_ASM // {

// BUGBUG workaround compiler bug
// compiler should know this, but doesn't, so we make it explicit
#define IMPORT_PTR  dword ptr       // BUGBUG sundown


//***   FORWARD_AW, THUNK_AW -- simple forwarders and thunks
// ENTRY/EXIT
//  - declare function w/ FORWARD_API
//  - if you're using THUNK_AW, create the 'A' thunk helper
//  - make the body FORWARD_AW or THUNK_AW.
//  - make sure there's *no* other code in the func.  o.w. you'll get bogus
//  code.
// EXAMPLE
//  int FORWARD_API WINAPI FooWrapW(int i, void *p)
//  {
//      VALIDATE_PROTOTYPE(Foo);
//
//      FORWARD_AW(Foo, (i, p));
//  }
//
//  int WINAPI BarAThunk(HWND hwnd, WPARAM wParam)
//  {
//      ... ansi thunk helper ...
//  }
//
//  int FORWARD_API WINAPI BarWrapW(HWND hwnd, WPARAM wParam)
//  {
//      VALIDATE_PROTOTYPE(Bar);
//
//      THUNK_AW(Bar, (hwnd, wParam));
//  }
// NOTES
//  - WARNING: can only be used for 'simple' thunks (NAKED => no non-global
//  vars, etc.).
//  - WARNING: calling func must be declared FORWARD_API.  if not you'll
//  get bogus code.  happily if you forget you get the (obscure) error
//  message "error C4035: 'FooW': no return value"
//  - note that the macro ends up w/ an extra ";" from the caller, oh well...
//  - TODO: perf: better still would be to have a g_pfnCallWndProc, init
//  it 1x, and then jmp indirect w/o the test.  it would cost us a ptr but
//  we only do it for the top-2 funcs (CallWindowProc and SendMessage)
#define FORWARD_API     _declspec(naked)

#define FORWARD_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        _asm { jmp     IMPORT_PTR _fn##W } \
    } \
    _asm { jmp     IMPORT_PTR _fn##A }

#define THUNK_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        _asm { jmp     IMPORT_PTR _fn##W } \
    } \
    _asm { jmp     _fn##AThunk }    // n.b. no IMPORT_PTR

#else // }{

#define FORWARD_API     /*NOTHING*/

#define FORWARD_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        return _fn##W _args; \
    } \
    return _fn##A _args;

#define THUNK_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        return _fn##W _args; \
    } \
    return _fn##AThunk _args;

#endif // }

//
//  Windows 95 and NT5 do not have the hbmpItem field in their MENUITEMINFO
//  structure.
//
#if (_WIN32_WINNT >= 0x0500)
#define MENUITEMINFOSIZE_WIN95  FIELD_OFFSET(MENUITEMINFOW, hbmpItem)
#else
#define MENUITEMINFOSIZE_WIN95  sizeof(MENUITEMINFOW)
#endif


//
// BUGBUG - Undefine functions that are defined in thunk.h
//          These are duplicates of functions defined here
//          and should be deleted from thunk.c.

#undef RegSetValueExW
#undef CompareStringW
#undef GetFileAttributesW
#undef GetFullPathNameW
#undef SearchPathW
#undef GetWindowsDirectoryW
#undef GetSystemDirectoryW
#undef GetEnvironmentVariableW

// Undefine mappings
#undef CharLowerW


//
// BUGBUG - There are numerous registry functions that are duplicated
//          here and in reg.c
//

//
//  Some W functions are implemented on Win95, so complain if anybody
//  writes thunks for them.
//
//  Though Win95's implementation of TextOutW is incomplete for FE languages.
//  Remove this section when we implement FE-aware TextOutW for Win95.
//
#if defined(TextOutWrap)
#error Do not write thunks for TextOutW; Win95 supports it.
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStr::Free()
{
    if (_pstr != _ach && HIWORD64(_pstr) != 0 && !IsAtom())
    {
        delete [] _pstr;
    }

    _pstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStrW::Free()
{
    if (_pwstr != _awch && HIWORD64(_pwstr) != 0)
    {
        delete [] _pwstr;
    }

    _pwstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::Init
//
//  Synopsis:   Converts a LPSTR function argument to a LPWSTR.
//
//  Arguments:  [pstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrInW::Init(LPCSTR pstr, int cch)
{
    int cchBufReq;

    _cwchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pstr) == 0)
    {
        _pwstr = (LPWSTR) pstr;
        return;
    }

    ASSERT(cch == -1 || cch > 0);

    //
    // Convert string to preallocated buffer, and return if successful.
    //
    // Since the passed in buffer may not be null terminated, we have
    // a problem if cch==ARRAYSIZE(_awch), because MultiByteToWideChar
    // will succeed, and we won't be able to null terminate the string!
    // Decrease our buffer by one for this case.
    //
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _awch, ARRAYSIZE(_awch)-1);

    if (_cwchLen > 0)
    {
        // Some callers don't NULL terminate.
        //
        // We could check "if (-1 != cch)" before doing this,
        // but always doing the null is less code.
        //
        _awch[_cwchLen] = 0;

        if (0 == _awch[_cwchLen-1]) // account for terminator
            _cwchLen--;

        _pwstr = _awch;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //

    cchBufReq = MultiByteToWideChar( CP_ACP, 0, pstr, cch, NULL, 0 );

    // Again, leave room for null termination
    cchBufReq++;

    ASSERT(cchBufReq > 0);
    _pwstr = new WCHAR[cchBufReq];
    if (!_pwstr)
    {
        // On failure, the argument will point to the empty string.
        _awch[0] = 0;
        _pwstr = _awch;
        return;
    }

    ASSERT(HIWORD64(_pwstr));
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _pwstr, cchBufReq );

#if DBG == 1 /* { */
    if (0 == _cwchLen)
    {
        int errcode = GetLastError();
        ASSERT(0 && "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    // Again, make sure we're always null terminated
    ASSERT(_cwchLen < cchBufReq);
    _pwstr[_cwchLen] = 0;

    if (0 == _pwstr[_cwchLen-1]) // account for terminator
        _cwchLen--;

}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class.
//
//  NOTE:       Don't inline this function or you'll increase code size
//              by pushing -1 on the stack for each call.
//
//----------------------------------------------------------------------------

CStrIn::CStrIn(LPCWSTR pwstr) : CConvertStr(CP_ACP)
{
    Init(pwstr, -1);
}


CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr) : CConvertStr(uCP)
{
    Init(pwstr, -1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::Init
//
//  Synopsis:   Converts a LPWSTR function argument to a LPSTR.
//
//  Arguments:  [pwstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD(pwstr) == 0).
//
//              [cwch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//  Note:       We ignore AreFileApisANSI() and always use CP_ACP.
//              The reason is that nobody uses SetFileApisToOEM() except
//              console apps, and once you set file APIs to OEM, you
//              cannot call shell/user/gdi APIs, since they assume ANSI
//              regardless of the FileApis setting.  So you end up in
//              this horrible messy state where the filename APIs interpret
//              the strings as OEM but SHELL32 interprets the strings
//              as ANSI and you end up with a big mess.
//
//----------------------------------------------------------------------------

void
CStrIn::Init(LPCWSTR pwstr, int cwch)
{
    int cchBufReq;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    _cchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pwstr) == 0 || IsAtom())
    {
        _pstr = (LPSTR) pwstr;
        return;
    }

    if ( cwch == 0 )
    {
        *_ach = '\0';
        _pstr = _ach;
        return;
    }

    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _ach, ARRAYSIZE(_ach)-1, NULL, NULL);

    if (_cchLen > 0)
    {
        // This is DBCS safe since byte before _cchLen is last character
        _ach[_cchLen] = 0;
        // BUGBUG DBCS REVIEW: this may not be safe if the last character
        // was a multibyte character...
        if (_ach[_cchLen-1]==0)
            _cchLen--;          // account for terminator
        _pstr = _ach;
        return;
    }


    cchBufReq = WideCharToMultiByte(
            CP_ACP, 0, pwstr, cwch, NULL, 0, NULL, NULL);

    cchBufReq++;

    ASSERT(cchBufReq > 0);
    _pstr = new char[cchBufReq];
    if (!_pstr)
    {
        // On failure, the argument will point to the empty string.
        _ach[0] = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _pstr, cchBufReq, NULL, NULL);
#if DBG == 1 /* { */
    if (_cchLen < 0)
    {
        errcode = GetLastError();
        ASSERT(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

    // Again, make sure we're always null terminated
    ASSERT(_cchLen < cchBufReq);
    _pstr[_cchLen] = 0;
    if (0 == _pstr[_cchLen-1]) // account for terminator
        _cchLen--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrInMulti::CStrInMulti
//
//  Synopsis:   Converts mulitple LPWSTRs to a multiple LPSTRs.
//
//  Arguments:  [pwstr] -- The strings to convert.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

CStrInMulti::CStrInMulti(LPCWSTR pwstr)
{
    LPCWSTR pwstrT;

    // We don't handle atoms because we don't need to.
    ASSERT(HIWORD64(pwstr));

    //
    // Count number of characters to convert.
    //

    pwstrT = pwstr;
    if (pwstr)
    {
        do {
            while (*pwstrT++)
                ;

        } while (*pwstrT++);
    }

    Init(pwstr, (int)(pwstrT - pwstr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CPPFIn::CPPFIn
//
//  Synopsis:   Inits the class.  Truncates the filename to MAX_PATH
//              so Win9x DBCS won't fault.  Win9x SBCS silently truncates
//              to MAX_PATH, so we're bug-for-bug compatible.
//
//----------------------------------------------------------------------------

CPPFIn::CPPFIn(LPCWSTR pwstr)
{
    SHUnicodeToAnsi(pwstr, _ach, ARRAYSIZE(_ach));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::CStrOut
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pwstr]   -- The Unicode buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cwchBuf] -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOut::CStrOut(LPWSTR pwstr, int cwchBuf) : CConvertStr(CP_ACP)
{
    Init(pwstr, cwchBuf);
}

CStrOut::CStrOut(UINT uCP, LPWSTR pwstr, int cwchBuf) : CConvertStr(uCP)
{
    Init(pwstr, cwchBuf);
}

void
CStrOut::Init(LPWSTR pwstr, int cwchBuf) 
{
    ASSERT(cwchBuf >= 0);

    _pwstr = pwstr;
    _cwchBuf = cwchBuf;

    if (!pwstr)
    {
        // Force cwchBuf = 0 because many callers (in particular, registry
        // munging functions) pass garbage as the length because they know
        // it will be ignored.
        _cwchBuf = 0;
        _pstr = NULL;
        return;
    }

    ASSERT(HIWORD64(pwstr));

    // Initialize buffer in case Windows API returns an error.
    _ach[0] = 0;

    // Use preallocated buffer if big enough.
    if (cwchBuf * 2 <= ARRAYSIZE(_ach))
    {
        _pstr = _ach;
        return;
    }

    // Allocate buffer.
    _pstr = new char[cwchBuf * 2];
    if (!_pstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        ASSERT(cwchBuf > 0);
        _pwstr[0] = 0;
        _cwchBuf = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _pstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::CStrOutW
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pstr]    -- The MBCS buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cchBuf]  -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOutW::CStrOutW(LPSTR pstr, int cchBuf)
{
    ASSERT(cchBuf >= 0);

    _pstr = pstr;
    _cchBuf = cchBuf;

    if (!pstr)
    {
        // Force cchBuf = 0 because many callers (in particular, registry
        // munging functions) pass garbage as the length because they know
        // it will be ignored.
        _cchBuf = 0;
        _pwstr = NULL;
        return;
    }

    ASSERT(HIWORD64(pstr));

    // Initialize buffer in case Windows API returns an error.
    _awch[0] = 0;

    // Use preallocated buffer if big enough.
    if (cchBuf <= ARRAYSIZE(_awch))
    {
        _pwstr = _awch;
        return;
    }

    // Allocate buffer.
    _pwstr = new WCHAR[cchBuf];
    if (!_pwstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        ASSERT(cchBuf > 0);
        _pstr[0] = 0;
        _cchBuf = 0;
        _pwstr = _awch;
        return;
    }

    ASSERT(HIWORD64(_pwstr));
    _pwstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertIncludingNul()
{
    int cch;

    if (!_pstr)
        return 0;

    cch = SHAnsiToUnicodeCP(_uCP, _pstr, _pwstr, _cwchBuf);

#if DBG == 1 /* { */
    if (cch == 0 && _cwchBuf > 0)
    {
        int errcode = GetLastError();
        ASSERT(0 && "SHAnsiToUnicode failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from Unicode to MBCS
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOutW::ConvertIncludingNul()
{
    int cch;

    if (!_pwstr)
        return 0;

    cch = SHUnicodeToAnsi(_pwstr, _pstr, _cchBuf);

#if DBG == 1 /* { */
    if (cch == 0 && _cchBuf > 0)
    {
        int errcode = GetLastError();
        ASSERT(0 && "SHUnicodeToAnsi failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertExcludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count EXCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertExcludingNul()
{
    int ret = ConvertIncludingNul();
    if (ret > 0)
    {
        ret -= 1;
    }
    return ret;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::~CStrOut
//
//  Synopsis:   Converts the buffer from MBCS to Unicode.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOut::~CStrOut()
{
    ConvertIncludingNul();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::~CStrOutW
//
//  Synopsis:   Converts the buffer from Unicode to MBCS.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOutW::~CStrOutW()
{
    ConvertIncludingNul();
}

#ifdef NEED_KERNEL32_WRAPPER
//+---------------------------------------------------------------------------
//
//  Class:      CWin32FindDataInOut
//
//  Purpose:    Converts WIN32_FIND_DATA structures from UNICODE to ANSI
//              on the way in, then ANSI to UNICODE on the way out.
//
//----------------------------------------------------------------------------

class CWin32FindDataInOut
{
public:
    operator LPWIN32_FIND_DATAA();
    CWin32FindDataInOut(LPWIN32_FIND_DATAW pfdW);
    ~CWin32FindDataInOut();

protected:
    LPWIN32_FIND_DATAW _pfdW;
    WIN32_FIND_DATAA _fdA;
};

//+---------------------------------------------------------------------------
//
//  Member:     CWin32FindDataInOut::CWin32FindDataInOut
//
//  Synopsis:   Convert the non-string fields to ANSI.  You'd think this
//              isn't necessary, but it is, because Win95 puts secret
//              goo into the dwReserved fields that must be preserved.
//
//----------------------------------------------------------------------------

inline
CWin32FindDataInOut::CWin32FindDataInOut(LPWIN32_FIND_DATAW pfdW) :
    _pfdW(pfdW)
{
    memcpy(&_fdA, _pfdW, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
}

//+---------------------------------------------------------------------------
//
//  Member:     CWin32FindDataInOut::~CWin32FindDataInOut
//
//  Synopsis:   Convert all the fields from ANSI back to UNICODE.
//
//----------------------------------------------------------------------------

inline
CWin32FindDataInOut::~CWin32FindDataInOut()
{
    memcpy(_pfdW, &_fdA, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));

    SHAnsiToUnicode(_fdA.cFileName, _pfdW->cFileName, ARRAYSIZE(_pfdW->cFileName));
    SHAnsiToUnicode(_fdA.cAlternateFileName, _pfdW->cAlternateFileName, ARRAYSIZE(_pfdW->cAlternateFileName));
}

//+---------------------------------------------------------------------------
//
//  Member:     CWin32FindDataInOut::operator LPWIN32_FIND_DATAA
//
//  Synopsis:   Returns the WIN32_FIND_DATAA.
//
//----------------------------------------------------------------------------

inline
CWin32FindDataInOut::operator LPWIN32_FIND_DATAA()
{
    return &_fdA;
}
#endif // NEED_KERNEL32_WRAPPER

//+------------------------------------------------------------------------
//
//  Implementation of the wrapped functions
//
//-------------------------------------------------------------------------

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
AppendMenuWrapW(
        HMENU   hMenu,
        UINT    uFlags,
        UINT_PTR uIDnewItem,
        LPCWSTR lpnewItem)
{
    VALIDATE_PROTOTYPE(AppendMenu);

    // Make the InsertMenu wrapper do all the work
    return InsertMenuWrapW(hMenu, (UINT)-1,
                           uFlags | MF_BYPOSITION, uIDnewItem, lpnewItem);
}

UINT GetLocaleAnsiCodePage(LCID Locale) 
{
    TCHAR szCodePage[7];
    GetLocaleInfoWrapW(Locale, LOCALE_IDEFAULTANSICODEPAGE, szCodePage, ARRAYSIZE(szCodePage));
    return StrToInt(szCodePage);
}

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI
CallWindowProcWrapW(
    WNDPROC lpPrevWndFunc,
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(CallWindowProc);

    // perf: better still would be to have a g_pfnCallWndProc, init it 1x,
    // and then jmp indirect w/o the test.  it would cost us a ptr but we
    // only do it for the top-2 funcs (CallWindowProc and SendMessage)
    FORWARD_AW(CallWindowProc, (lpPrevWndFunc, hWnd, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(BOOL FORWARD_API) CallMsgFilterWrapW(LPMSG lpMsg, int nCode)
{
    VALIDATE_PROTOTYPE(CallMsgFilter);

    FORWARD_AW(CallMsgFilter, (lpMsg, nCode));
}

#endif // NEED_USER32_WRAPPER



//----------------------------------------------------------------------
//
// function:    CharLowerWrapW( LPWSTR pch )
//
// purpose:     Converts character to lowercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Lowercased character or string.  In the string case,
//              the lowercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

LPWSTR WINAPI
CharLowerWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharLower);

    if (g_bRunningOnNT)
    {
        return CharLowerW( pch );
    }

    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharLowerBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharLowerBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}

#endif // NEED_USER32_WRAPPER


//----------------------------------------------------------------------
//
// function:    CharLowerBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to lowercase.  String must be cch
//              characters in length.
//
// returns:     Character count (cch).  The lowercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI
CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    VALIDATE_PROTOTYPE(CharLowerBuff);

    if (g_bRunningOnNT)
    {
        return CharLowerBuffW( pch, cchLength );
    }

    DWORD cch;

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharUpperWrapW(ch))
        {
            if (ch < 0x0100)
            {
                *pch += 32;             // Get Latin-1 out of the way first
            }
            else if (ch < 0x0531)
            {
                if (ch < 0x0391)
                {
                    if (ch < 0x01cd)
                    {
                        if (ch <= 0x178)
                        {
                            if (ch < 0x0178)
                            {
                                *pch += (ch == 0x0130) ? 0 : 1;
                            }
                            else
                            {
                                *pch -= 121;
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                            {  // 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
            /* 0x0179-0x17f */           1,   0,   1,   0,   1,   0,   0,
            /* 0x0180-0x187 */      0, 210,   1,   0,   1,   0, 206,   1,
            /* 0x0188-0x18f */      0, 205, 205,   1,   0,   0,  79, 202,
            /* 0x0190-0x197 */    203,   1,   0, 205, 207,   0, 211, 209,
            /* 0x0198-0x19f */      1,   0,   0,   0, 211, 213,   0, 214,
            /* 0x01a0-0x1a7 */      1,   0,   1,   0,   1,   0,   0,   1,
            /* 0x01a8-0x1af */      0, 218,   0,   0,   1,   0, 218,   1,
            /* 0x01b0-0x1b7 */      0, 217, 217,   1,   0,   1,   0, 219,
            /* 0x01b8-0x1bf */      1,   0,   0,   0,   1,   0,   0,   0,
            /* 0x01c0-0x1c7 */      0,   0,   0,   0,   2,   0,   0,   2,
            /* 0x01c8-0x1cb */      0,   0,   2,   0
                            };

                            *pch += abLookup[ch-0x0179];
                        }
                    }
                    else if (ch < 0x0386)
                    {
                        switch (ch)
                        {
                            case 0x01f1: *pch += 2; break;
                            case 0x01f2: break;
                            default: *pch += 1;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                            { 38, 0, 37, 37, 37, 0, 64, 0, 63, 63 };

                        *pch += abLookup[ch-0x0386];
                    }
                }
                else
                {
                    if (ch < 0x0410)
                    {
                        if (ch < 0x0401)
                        {
                            if (ch < 0x03e2)
                            {
                                if (!InRange(ch, 0x03d2, 0x03d4) &&
                                    !(InRange(ch, 0x3da, 0x03e0) & !(ch & 1)))
                                {
                                    *pch += 32;
                                }
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else
                        {
                            *pch += 80;
                        }
                    }
                    else
                    {
                        if (ch < 0x0460)
                        {
                            *pch += 32;
                        }
                        else
                        {
                            *pch += 1;
                        }
                    }
                }
            }
            else
            {
                if (ch < 0x2160)
                {
                    if (ch < 0x1fba)
                    {
                        if (ch < 0x1f08)
                        {
                            if (ch < 0x1e00)
                            {
                                *pch += 48;
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else if (!(InRange(ch, 0x1f88, 0x1faf) && (ch & 15)>7))
                        {
                            *pch -= 8;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                        {  // 8    9    a    b    c    d    e    f
                              0,   0,  74,  74,   0,   0,   0,   0,
                             86,  86,  86,  86,   0,   0,   0,   0,
                              8,   8, 100, 100,   0,   0,   0,   0,
                              8,   8, 112, 112,   7,   0,   0,   0,
                            128, 128, 126, 126,   0,   0,   0,   0
                        };
                        int i = (ch-0x1fb0);

                        *pch -= (int)abLookup[((i>>1) & ~7) | (i & 7)];
                    }
                }
                else
                {
                    if (ch < 0xff21)
                    {
                        if (ch < 0x24b6)
                        {
                            *pch += 16;
                        }
                        else
                        {
                            *pch += 26;
                        }
                    }
                    else
                    {
                        *pch += 32;
                    }
                }
            }
        }
        else
        {
            // These are Unicode Number Forms.  They have lowercase counter-
            // parts, but are not considered uppercase.  Why, I don't know.

            if (InRange(ch, 0x2160, 0x216f))
            {
                *pch += 16;
            }
        }
    }

    return cchLength;
}

#endif // NEED_USER32_WRAPPER

//
// BUGBUG - Do CharNextWrap and CharPrevWrap need to call the
//          CharNextW, CharPrevW on WinNT?  Couldn't these be MACROS?
//

LPWSTR WINAPI
CharNextWrapW(LPCWSTR lpszCurrent)
{
    VALIDATE_PROTOTYPE(CharNext);

    if (*lpszCurrent)
    {
        return (LPWSTR) lpszCurrent + 1;
    }
    else
    {
        return (LPWSTR) lpszCurrent;
    }
}

LPWSTR WINAPI
CharPrevWrapW(LPCWSTR lpszStart, LPCWSTR lpszCurrent)
{
    VALIDATE_PROTOTYPE(CharPrev);

    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}


#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
CharToOemWrapW(LPCWSTR lpszSrc, LPSTR lpszDst)
{
    VALIDATE_PROTOTYPE(CharToOem);

    if (g_bRunningOnNT)
    {
        CharToOemW(lpszSrc, lpszDst);
    }

    CStrIn  str(lpszSrc);

    return CharToOemA(str, lpszDst);
}

#endif // NEED_USER32_WRAPPER

//----------------------------------------------------------------------
//
// function:    CharUpperWrapW( LPWSTR pch )
//
// purpose:     Converts character to uppercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Uppercased character or string.  In the string case,
//              the uppercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

LPWSTR WINAPI
CharUpperWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharUpper);

    if (g_bRunningOnNT)
    {
        return CharUpperW( pch );
    }

    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharUpperBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharUpperBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}

#endif // NEED_USER32_WRAPPER


//----------------------------------------------------------------------
//
// function:    CharUpperBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to uppercase.  String must be cch
//              characters in length.  Note that this function is
//              is messier that CharLowerBuffWrap, and the reason for
//              this is many Unicode characters are considered uppercase,
//              even when they don't have an uppercase counterpart.
//
// returns:     Character count (cch).  The uppercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI
CharUpperBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    VALIDATE_PROTOTYPE(CharUpperBuff);

    if (g_bRunningOnNT)
    {
        return CharUpperBuffW( pch, cchLength );
    }

    DWORD cch;

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharLowerWrapW(ch))
        {
            if (ch < 0x00ff)
            {
                *pch -= ((ch != 0xdf) << 5);
            }
            else if (ch < 0x03b1)
            {
                if (ch < 0x01f5)
                {
                    if (ch < 0x01ce)
                    {
                        if (ch < 0x017f)
                        {
                            if (ch < 0x0101)
                            {
                                *pch += 121;
                            }
                            else
                            {
                                *pch -= (ch != 0x0131 &&
                                         ch != 0x0138 &&
                                         ch != 0x0149);
                            }
                        }
                        else if (ch < 0x01c9)
                        {
                            static const BYTE abMask[] =
                            {                       // 6543210f edcba987
                                0xfc, 0xbf,         // 11111100 10111111
                                0xbf, 0x67,         // 10111111 01100111
                                0xff, 0xef,         // 11111111 11101111
                                0xff, 0xf7,         // 11111111 11110111
                                0xbf, 0xfd          // 10111111 11111101
                            };

                            int i = ch - 0x017f;

                            *pch -= ((abMask[i>>3] >> (i&7)) & 1) +
                                    (ch == 0x01c6);
                        }
                        else
                        {
                            *pch -= ((ch != 0x01cb)<<1);
                        }
                    }
                    else
                    {
                        if (ch < 0x01df)
                        {
                            if (ch < 0x01dd)
                            {
                                *pch -= 1;
                            }
                            else
                            {
                                *pch -= 79;
                            }
                        }
                        else
                        {
                            *pch -= 1 + (ch == 0x01f3) -
                                    InRange(ch,0x01f0,0x01f2);
                        }
                    }
                }
                else if (ch < 0x0253)
                {
                    *pch -= (ch < 0x0250);
                }
                else if (ch < 0x03ac)
                {
                    static const BYTE abLookup[] =
                    {// 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
    /* 0x0253-0x0257 */                210, 206,   0, 205, 205,
    /* 0x0258-0x025f */   0, 202,   0, 203,   0,   0,   0,   0,
    /* 0x0260-0x0267 */ 205,   0,   0, 207,   0,   0,   0,   0,
    /* 0x0268-0x026f */ 209, 211,   0,   0,   0,   0,   0, 211,
    /* 0x0270-0x0277 */   0,   0, 213,   0,   0, 214,   0,   0,
    /* 0x0278-0x027f */   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0x0280-0x0287 */   0,   0,   0, 218,   0,   0,   0,   0,
    /* 0x0288-0x028f */ 218,   0, 217, 217,   0,   0,   0,   0,
    /* 0x0290-0x0297 */   0,   0, 219
                    };

                    if (ch <= 0x0292)
                    {
                        *pch -= abLookup[ch - 0x0253];
                    }
                }
                else
                {
                    *pch -= (ch == 0x03b0) ? 0 : (37 + (ch == 0x03ac));
                }
            }
            else
            {
                if (ch < 0x0561)
                {
                    if (ch < 0x0451)
                    {
                        if (ch < 0x03e3)
                        {
                            if (ch < 0x03cc)
                            {
                                *pch -= 32 - (ch == 0x03c2);
                            }
                            else
                            {
                                int i = (ch < 0x03d0);
                                *pch -= (i<<6) - i + (ch == 0x03cc);
                            }
                        }
                        else if (ch < 0x0430)
                        {
                            *pch -= (ch < 0x03f0);
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                    else if (ch < 0x0461)
                    {
                        *pch -= 80;
                    }
                    else
                    {
                        *pch -= 1;
                    }
                }
                else
                {
                    if (ch < 0x1fb0)
                    {
                        if (ch < 0x1f70)
                        {
                            if (ch < 0x1e01)
                            {
                                int i = ch != 0x0587 && ch < 0x10d0;
                                *pch -= ((i<<5)+(i<<4)); /* 48 */
                            }
                            else if (ch < 0x1f00)
                            {
                                *pch -= !InRange(ch, 0x1e96, 0x1e9a);
                            }
                            else
                            {
                                int i = !InRange(ch, 0x1f50, 0x1f56)||(ch & 1);
                                *pch += (i<<3);
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                                { 74, 86, 86, 100, 128, 112, 126 };

                            if ( ch <= 0x1f7d )
                            {
                                *pch += abLookup[(ch-0x1f70)>>1];
                            }
                        }
                    }
                    else
                    {
                        if (ch < 0x24d0)
                        {
                            if (ch < 0x1fe5)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 8 : 0;
                            }
                            else if (ch < 0x2170)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 7 : 0;
                            }
                            else
                            {
                                *pch -= ((ch > 0x24b5)<<4);
                            }
                        }
                        else if (ch < 0xff41)
                        {
                            int i = !InRange(ch, 0xfb00, 0xfb17);
                            *pch -= (i<<4)+(i<<3)+(i<<1); /* 26 */
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                }
            }
        }
        else
        {
            int i = InRange(ch, 0x2170, 0x217f);
            *pch -= (i<<4);
        }
    }

    return cchLength;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int FORWARD_API WINAPI
CopyAcceleratorTableWrapW(
        HACCEL  hAccelSrc,
        LPACCEL lpAccelDst,
        int     cAccelEntries)
{
    VALIDATE_PROTOTYPE(CopyAcceleratorTable);

    FORWARD_AW(CopyAcceleratorTable, (hAccelSrc, lpAccelDst, cAccelEntries));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HACCEL FORWARD_API WINAPI
CreateAcceleratorTableWrapW(LPACCEL lpAccel, int cEntries)
{
    VALIDATE_PROTOTYPE(CreateAcceleratorTable);

    FORWARD_AW(CreateAcceleratorTable, (lpAccel, cEntries));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER
typedef HDC (*FnCreateHDCA)(LPCSTR, LPCSTR, LPCSTR, CONST DEVMODEA *);

HDC WINAPI
CreateHDCWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData,
        FnCreateHDCA        pfn)
{
    DEVMODEA *  pdevmode = NULL;
    CStrIn      strDriver(lpszDriver);
    CStrIn      strDevice(lpszDevice);
    CStrIn      strOutput(lpszOutput);
    HDC         hdcReturn = 0;

    if (lpInitData)
    {
        pdevmode = (DEVMODEA *) LocalAlloc( LPTR, lpInitData->dmSize + lpInitData->dmDriverExtra );

        if (pdevmode)
        {
            // LPBYTE->LPSTR casts below
            SHUnicodeToAnsi(lpInitData->dmDeviceName, (LPSTR)pdevmode->dmDeviceName, ARRAYSIZE(pdevmode->dmDeviceName));
            memcpy(&pdevmode->dmSpecVersion,
                    &lpInitData->dmSpecVersion,
                    FIELD_OFFSET(DEVMODEW,dmFormName) - FIELD_OFFSET(DEVMODEW,dmSpecVersion));
            SHUnicodeToAnsi(lpInitData->dmFormName, (LPSTR)pdevmode->dmFormName, ARRAYSIZE(pdevmode->dmFormName));
            memcpy(&pdevmode->dmLogPixels,
                    &lpInitData->dmLogPixels,
                    lpInitData->dmDriverExtra + lpInitData->dmSize - FIELD_OFFSET(DEVMODEW, dmLogPixels));

            pdevmode->dmSize -= (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);
        }
    }

    hdcReturn = (*pfn)(strDriver, strDevice, strOutput, pdevmode);

    if (pdevmode)
    {
        LocalFree(pdevmode);
    }

    return hdcReturn;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI
CreateDCWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
    VALIDATE_PROTOTYPE(CreateDC);

    if (g_bRunningOnNT)
    {
        return CreateDCW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }
    return CreateHDCWrapW(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateDCA);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI
CreateICWrapW(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
    VALIDATE_PROTOTYPE(CreateIC);

    if (g_bRunningOnNT)
    {
        return CreateICW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }

    return CreateHDCWrapW(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateICA);
}

#endif // NEED_GDI32_WRAPPER


#undef CreateDialogIndirectParamW

HWND WINAPI
CreateDialogIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam)
{
    VALIDATE_PROTOTYPE(CreateDialogIndirectParam);

    if (fDoMungeUI(hInstance))
    {
        return MLCreateDialogIndirectParamI(
                    hInstance,
                    hDialogTemplate,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam);
    }
    
    if (g_bRunningOnNT)
    {
        return CreateDialogIndirectParamW(
                    hInstance,
                    hDialogTemplate,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam);
    }

    return CreateDialogIndirectParamA(
                hInstance,
                hDialogTemplate,
                hWndParent,
                lpDialogFunc,
                dwInitParam);
}


#undef CreateDialogParamW

HWND WINAPI
CreateDialogParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpTemplateName,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam)
{
    VALIDATE_PROTOTYPE(CreateDialogParam);
    ASSERT(HIWORD64(lpTemplateName) == 0);

    if (fDoMungeUI(hInstance))
        return MLCreateDialogParamI(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
        
    if (g_bRunningOnNT)
    {
        return CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }

    return CreateDialogParamA(hInstance, (LPSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}


#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
CreateDirectoryWrapW(
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes)
{
    VALIDATE_PROTOTYPE(CreateDirectory);

    if (g_bRunningOnNT)
    {
        return CreateDirectoryW(lpPathName, lpSecurityAttributes);
    }

    CStrIn  str(lpPathName);

    ASSERT(!lpSecurityAttributes);
    return CreateDirectoryA(str, lpSecurityAttributes);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
CreateEventWrapW(
        LPSECURITY_ATTRIBUTES   lpEventAttributes,
        BOOL                    bManualReset,
        BOOL                    bInitialState,
        LPCWSTR                 lpName)
{
    VALIDATE_PROTOTYPE(CreateEvent);

    //Totally bogus assert.
    //ASSERT(!lpName);

    // cast means we can't use FORWARD_AW
    if (g_bRunningOnNT)
    {
        return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
    }

    return CreateEventA(lpEventAttributes, bManualReset, bInitialState, (LPCSTR)lpName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
CreateFileWrapW(
        LPCWSTR                 lpFileName,
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDisposition,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile)
{
    VALIDATE_PROTOTYPE(CreateFile);

    if (g_bRunningOnNT)
    {
        return CreateFileW(
                lpFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);
    }

    CStrIn  str(lpFileName);

    return CreateFileA(
            str,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HFONT WINAPI
CreateFontIndirectWrapW(CONST LOGFONTW * plfw)
{
    VALIDATE_PROTOTYPE(CreateFontIndirect);

    if (g_bRunningOnNT)
    {
        return CreateFontIndirectW(plfw);
    }

    LOGFONTA  lfa;
    HFONT     hFont;

    memcpy(&lfa, plfw, FIELD_OFFSET(LOGFONTA, lfFaceName));
    SHUnicodeToAnsi(plfw->lfFaceName, lfa.lfFaceName, ARRAYSIZE(lfa.lfFaceName));
    hFont = CreateFontIndirectA(&lfa);

    return hFont;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
CreateWindowExWrapW(
        DWORD       dwExStyle,
        LPCWSTR     lpClassName,
        LPCWSTR     lpWindowName,
        DWORD       dwStyle,
        int         X,
        int         Y,
        int         nWidth,
        int         nHeight,
        HWND        hWndParent,
        HMENU       hMenu,
        HINSTANCE   hInstance,
        LPVOID      lpParam)
{
    VALIDATE_PROTOTYPE(CreateWindowEx);

    if (g_bRunningOnNT)
    {
        return CreateWindowExW(
                dwExStyle,
                lpClassName,
                lpWindowName,
                dwStyle,
                X,
                Y,
                nWidth,
                nHeight,
                hWndParent,
                hMenu,
                hInstance,
                lpParam);
    }

    CStrIn  strClass(lpClassName);
    CStrIn  strWindow(lpWindowName);

    return CreateWindowExA(
            dwExStyle,
            strClass,
            strWindow,
            dwStyle,
            X,
            Y,
            nWidth,
            nHeight,
            hWndParent,
            hMenu,
            hInstance,
            lpParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI DefWindowProcWrapW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    VALIDATE_PROTOTYPE(DefWindowProc);

    FORWARD_AW(DefWindowProc, (hWnd, msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI DeleteFileWrapW(LPCWSTR pwsz)
{
    VALIDATE_PROTOTYPE(DeleteFile);

    if (g_bRunningOnNT)
    {
        return DeleteFileW(pwsz);
    }

    CStrIn  str(pwsz);

    return DeleteFileA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#undef DialogBoxIndirectParamW

INT_PTR WINAPI
DialogBoxIndirectParamWrapW(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam)
{
    VALIDATE_PROTOTYPE(DialogBoxIndirectParam);

    if (fDoMungeUI(hInstance))
        return MLDialogBoxIndirectParamI(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (g_bRunningOnNT)
    {
        return DialogBoxIndirectParamW(
                    hInstance,
                    hDialogTemplate,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam);
    }

    return DialogBoxIndirectParamA(
                hInstance,
                hDialogTemplate,
                hWndParent,
                lpDialogFunc,
                dwInitParam);
}


#undef DialogBoxParamW

INT_PTR WINAPI
DialogBoxParamWrapW(
        HINSTANCE   hInstance,
        LPCWSTR     lpszTemplate,
        HWND        hWndParent,
        DLGPROC     lpDialogFunc,
        LPARAM      dwInitParam)
{
    VALIDATE_PROTOTYPE(DialogBoxParam);
    ASSERT(HIWORD64(lpszTemplate) == 0);

    if (fDoMungeUI(hInstance))
        return MLDialogBoxParamI(hInstance, lpszTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (g_bRunningOnNT)
    {
        return DialogBoxParamW(
                    hInstance,
                    lpszTemplate,
                    hWndParent,
                    lpDialogFunc,
                    dwInitParam);
    }

    return DialogBoxParamA(hInstance, (LPCSTR) lpszTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI
DispatchMessageWrapW(CONST MSG * lpMsg)
{
    VALIDATE_PROTOTYPE(DispatchMessage);

    FORWARD_AW(DispatchMessage, (lpMsg));
}

#endif // NEED_USER32_WRAPPER

BOOL _MayNeedFontLinking(LPCWSTR lpStr, UINT cch)
{
#ifndef UNIX
    //
    // Scan the string to see if we might need to use font linking to draw it.
    // If you've got a better way of doing this, I'd love to hear it.
    //
    BOOL fRet = FALSE;

    int cChars = ((cch == -1) ? lstrlenW(lpStr) : cch);

    for (int i=0; i < cChars; i++)
    {
        if (lpStr[i] > 127)
        {
            fRet = TRUE;
            break;
        }
    }
    return fRet;
#else
    return FALSE;
#endif
}

#ifdef NEED_USER32_WRAPPER

int WINAPI
DrawTextWrapW(
        HDC     hDC,
        LPCWSTR lpString,
        int     nCount,
        LPRECT  lpRect,
        UINT    uFormat)
{
    VALIDATE_PROTOTYPE(DrawText);

    if (_MayNeedFontLinking(lpString, nCount))
    {
        return DrawTextFLW(hDC, lpString, nCount, lpRect, uFormat);
    }
    else if (g_bRunningOnNT)
    {
        return DrawTextW(hDC, lpString, nCount, lpRect, uFormat);
    }
    CStrIn  str(lpString, nCount);
    return DrawTextA(hDC, str, str.strlen(), lpRect, uFormat);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

struct EFFSTAT
{
    LPARAM          lParam;
    FONTENUMPROC    lpEnumFontProc;
    BOOL            fFamilySpecified;
};

int CALLBACK
EnumFontFamiliesCallbackWrap(
        ENUMLOGFONTA *  lpelf,
        NEWTEXTMETRIC * lpntm,
        DWORD           FontType,
        LPARAM          lParam)
{
    ENUMLOGFONTW    elf;

    //  Convert strings from ANSI to Unicode
    if (((EFFSTAT *)lParam)->fFamilySpecified && (FontType & TRUETYPE_FONTTYPE) )
    {
        // LPBYTE->LPCSTR cast below
        SHAnsiToUnicode((LPCSTR)lpelf->elfFullName, elf.elfFullName, ARRAYSIZE(elf.elfFullName));
        SHAnsiToUnicode((LPCSTR)lpelf->elfStyle, elf.elfStyle, ARRAYSIZE(elf.elfStyle));
    }
    else
    {
        elf.elfStyle[0] = L'\0';
        elf.elfFullName[0] = L'\0';
    }

    SHAnsiToUnicode(lpelf->elfLogFont.lfFaceName, elf.elfLogFont.lfFaceName, ARRAYSIZE(elf.elfLogFont.lfFaceName));

    //  Copy the non-string data
    memcpy(
            &elf.elfLogFont,
            &lpelf->elfLogFont,
            FIELD_OFFSET(LOGFONTA, lfFaceName));

    //  Chain to the original callback function
    return (*((EFFSTAT *) lParam)->lpEnumFontProc)(
            (const LOGFONTW *)&elf,
            (const TEXTMETRICW *) lpntm,
            FontType,
            ((EFFSTAT *) lParam)->lParam);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
EnumFontFamiliesWrapW(
        HDC          hdc,
        LPCWSTR      lpszFamily,
        FONTENUMPROC lpEnumFontProc,
        LPARAM       lParam)
{
    VALIDATE_PROTOTYPE(EnumFontFamilies);

    if (g_bRunningOnNT)
    {
        return EnumFontFamiliesW(
                hdc,
                lpszFamily,
                lpEnumFontProc,
                lParam);
    }

    CStrIn  str(lpszFamily);
    EFFSTAT effstat;

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lpszFamily != NULL;

    return EnumFontFamiliesA(
            hdc,
            str,
            (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
            (LPARAM) &effstat);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
EnumFontFamiliesExWrapW(
        HDC          hdc,
        LPLOGFONTW   lplfw,
        FONTENUMPROC lpEnumFontProc,
        LPARAM       lParam,
        DWORD        dwFlags )
{
    VALIDATE_PROTOTYPE(EnumFontFamiliesEx);

    if (g_bRunningOnNT)
    {
        return EnumFontFamiliesExW(
                hdc,
                lplfw,
                lpEnumFontProc,
                lParam,
                dwFlags);
    }

    LOGFONTA lfa;
    CStrIn   str(lplfw->lfFaceName);
    EFFSTAT  effstat;

    ASSERT( FIELD_OFFSET(LOGFONTW, lfFaceName) == FIELD_OFFSET(LOGFONTA, lfFaceName) );

    memcpy( &lfa, lplfw, sizeof(LOGFONTA) - FIELD_OFFSET(LOGFONTA, lfFaceName) );
    memcpy( lfa.lfFaceName, str, LF_FACESIZE );

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lplfw->lfFaceName != NULL;

    return EnumFontFamiliesExA(
            hdc,
            &lfa,
            (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
            (LPARAM) &effstat,
            dwFlags );
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
EnumResourceNamesWrapW(
        HINSTANCE        hModule,
        LPCWSTR          lpType,
        ENUMRESNAMEPROCW lpEnumFunc,
        LPARAM           lParam)
{
    VALIDATE_PROTOTYPE(EnumResourceNames);
    ASSERT(HIWORD64(lpType) == 0);

    if (g_bRunningOnNT)
    {
        return EnumResourceNamesW(hModule, lpType, lpEnumFunc, lParam);
    }

    return EnumResourceNamesA(hModule, (LPCSTR) lpType, (ENUMRESNAMEPROCA)lpEnumFunc, lParam);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
FindFirstFileWrapW(
        LPCWSTR             lpFileName,
        LPWIN32_FIND_DATAW  pwszFd)
{
    VALIDATE_PROTOTYPE(FindFirstFile);

    if (g_bRunningOnNT)
    {
        return FindFirstFileW(lpFileName, pwszFd);
    }

    CStrIn              str(lpFileName);
    CWin32FindDataInOut fd(pwszFd);

    return FindFirstFileA(str, fd);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HRSRC WINAPI
FindResourceWrapW(HINSTANCE hModule, LPCWSTR lpName, LPCWSTR lpType)
{
    VALIDATE_PROTOTYPE(FindResource);

    if (g_bRunningOnNT)
    {
        return FindResourceW(hModule, lpName, lpType);
    }

    CStrIn  strName(lpName);
    CStrIn strType(lpType);

    return FindResourceA(hModule, strName, strType);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
FindWindowWrapW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
    VALIDATE_PROTOTYPE(FindWindow);

    if (g_bRunningOnNT)
    {
        return FindWindowW(lpClassName, lpWindowName);
    }

    // Let FindWindowExWrapW do the thunking
    return FindWindowExWrapW(NULL, NULL, lpClassName, lpWindowName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
FindWindowExWrapW(HWND hwndParent, HWND hwndChildAfter, LPCWSTR pwzClassName, LPCWSTR pwzWindowName)
{
    VALIDATE_PROTOTYPE(FindWindowEx);

    if (g_bRunningOnNT)
        return FindWindowExW(hwndParent, hwndChildAfter, pwzClassName, pwzWindowName);


    CStrIn  strClass(pwzClassName);
    CStrIn  strWindow(pwzWindowName);

    return FindWindowExA(hwndParent, hwndChildAfter, strClass, strWindow);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

//
// FindNextArgInfo - Finds the argument format and argument number from a FormatMessage like template
//
// Returns - True if arg is wide string, False if other
//
BOOL FindNextArgInfo(LPCWSTR *pszTemplate, UINT *puiNum, LPWSTR pszFormat, UINT cchFormat)
{
    BOOL fRet = FALSE;
    
    if(*pszTemplate == NULL)
        return(FALSE);

    LPCWSTR psz = *pszTemplate;
    LPCWSTR pszT1 = NULL;
    LPCWSTR pszT2 = NULL;
    LPWSTR pszFmt = pszFormat;
    BOOL fHaveStart = FALSE;
    UINT cchFmt;

    *puiNum = 0;

    if(cchFormat > 0)
    {
        *pszFmt = TEXT('%');
        pszFmt++;
        cchFormat--;
    }

    if(*psz >= TEXT('1') && *psz <= TEXT('9'))
    {
        for(; *psz; psz++)
        {
            if(*psz == TEXT('!'))
            {
                if(fHaveStart)  // Done
                {
                    pszT2 = ++psz;  // Mark end of type
                    break;
                }
                else
                {
                    fHaveStart = TRUE;
                    psz++;
                    if(pszT1 == NULL)
                        pszT1 = psz;  // Mark start of arg format string
                }
            }

            if(!fHaveStart) // Get arg number
            {
                if(*psz >= TEXT('0') && *psz <= TEXT('9'))
                    *puiNum = (*puiNum * 10) + (*psz - TEXT('0'));
                else
                    break;
            }
        }

        if(*puiNum == 0)
            return(FALSE);
    }
    else    // Special format specifier
    {
        pszT1 = psz;
        pszT2 = psz+1;
    }

    if(pszT1 != NULL)    // We have an arg format string
    {
        cchFmt = cchFormat;
        if(cchFmt > (UINT)(pszT2 - pszT1))
            cchFmt = (pszT2 - pszT1); 
        StrCpyN(pszFmt, pszT1, cchFmt);

        // Is argument type a string
        if(StrChrI(pszFormat, TEXT('s')))
        {
            fRet = TRUE;
        }
        else
        {
            cchFmt = cchFormat;
            if(cchFmt > (UINT)(pszT2 - *pszTemplate)+1)
                cchFmt = (pszT2 - *pszTemplate)+1; 
            StrCpyN(pszFmt, *pszTemplate, cchFmt);
        }
    }
    else    // No arg format string, FormatMessage default is !s!
    {
        pszT2 = psz;
        fRet = TRUE;
        cchFmt = cchFormat;
        if(cchFmt > 3)
            cchFmt = 3;
        StrCpyN(pszFmt, TEXT("ws"), cchFmt);
    }

    *pszTemplate = pszT2;   // Move template string beyond this format specifier

    return(fRet);
}

//
// The following is used to work around Win9x FormatMessage problems related to converting 
// DBCS strings to Unicode.
//                     
#define FML_BUFFER_SIZE 1024
#define FML_BUFFER_INC   256

DWORD
FormatMessageLiteW(
    DWORD       dwFlags,
    LPCWSTR     lpSource,
    PVOID *     pDest,
    DWORD       nSize,
    va_list *   Arguments)
{
    BOOL fIsStr;
    UINT uiNum;
    UINT uiDataCnt = 0;
    va_list pArgList = *Arguments;
    va_list pArgList2;
    WCHAR *pszBuf2;
    WCHAR szFmt[256];
    VOID *pData[10];
    LPCWSTR psz = lpSource;
    LPCWSTR psz1 = NULL;
    LPWSTR lpBuffer;
    INT cchBufUsed = 0;
    INT cchBuf2;
    INT cch;

    if(lpSource == NULL || pDest == NULL || Arguments == NULL)
        return(0);

    if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        nSize = max(nSize, FML_BUFFER_SIZE);
        if((lpBuffer = (WCHAR *)LocalAlloc(LPTR, nSize * sizeof(WCHAR))) == NULL)
            return(0);
    }
    else
    {
        if(nSize == 0)
            return(0);
        lpBuffer = (LPWSTR)pDest;
    }

    cchBuf2 = nSize + lstrlen(lpSource);
    if((pszBuf2 = (WCHAR *)LocalAlloc(LPTR, cchBuf2 * sizeof(WCHAR))) != NULL)
    {
        *lpBuffer = TEXT('\0');
        while(*psz)
        {
            if(*psz == TEXT('%'))
            {
                psz++;
                if(psz1 != NULL)    // Copy any previous text to the buffer
                {
                    if((cch = psz - psz1) > (INT)(nSize - cchBufUsed))
                    {
                        if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
                        {
                            LPWSTR lpRealloc;
                            nSize = max(FML_BUFFER_INC, cch);
                            if((lpRealloc = (WCHAR *)LocalReAlloc(lpBuffer, nSize * sizeof(WCHAR), LMEM_ZEROINIT | LMEM_MOVEABLE)) == NULL)
                            {
                                LocalFree(lpBuffer);
                                LocalFree(pszBuf2);
                                return(0);
                            }
                            lpBuffer = lpRealloc;
                        }
                        else
                        {
                            RIPMSG(FALSE, "Output buffer not large enough.  Truncating.");
                            cch = nSize - cchBufUsed;
                        }
                    }
                    StrCpyNW(lpBuffer+cchBufUsed, psz1, cch);
                    cchBufUsed += cch-1;
                    psz1 = NULL;
                }

                fIsStr = FindNextArgInfo(&psz, &uiNum, szFmt, ARRAYSIZE(szFmt));

                if(fIsStr)
                {
                    if(uiNum > uiDataCnt)
                    {
                        for(UINT i = uiDataCnt; i < uiNum; i++)    // Find the iTH argument
                        {
                            pData[i] = va_arg(pArgList, VOID *);
                            uiDataCnt++;
                        }
                    }
                        
                    cch = wnsprintfW(pszBuf2, cchBuf2, szFmt, pData[uiNum-1]);
                    if(cch == cchBuf2)
                    {
                        RIPMSG(FALSE, "Param buffer may be too small. Output string may be truncated.");
                    }
                }
                else
                {
                    // Call FormatMessage on non-string arguments
                    pArgList2 = *Arguments;
                    CStrIn strSource(CP_ACP, (LPCWSTR)szFmt, -1);
                    CStrOut str(pszBuf2, cchBuf2);
                    FormatMessageA(FORMAT_MESSAGE_FROM_STRING, strSource, 0, 0, str, str.BufSize(), &pArgList2);
                    cch = str.ConvertExcludingNul();
                }

                if(cch > 0)
                {
                    if(cch > (INT)(nSize - cchBufUsed))
                    {
                        if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
                        {
                            LPWSTR lpRealloc;
                            nSize += max(FML_BUFFER_INC, cch);
                            if((lpRealloc = (WCHAR *)LocalReAlloc(lpBuffer, nSize * sizeof(WCHAR), LMEM_ZEROINIT | LMEM_MOVEABLE)) == NULL)
                            {
                                LocalFree(lpBuffer);
                                LocalFree(pszBuf2);
                                return(0);
                            }
                            lpBuffer = lpRealloc;
                        }
                        else
                        {
                            RIPMSG(FALSE, "Output buffer not large enough.  Truncating.");
                            cch = nSize - cchBufUsed;
                        }
                    }
                    StrCpyW(lpBuffer+cchBufUsed, pszBuf2);
                    cchBufUsed += cch;
                }
                else
                {
                    RIPMSG(FALSE, "Argument string conversion failed.  Argument not copied to output buffer");
                }
                continue;
            }

            if(psz1 == NULL)
                psz1 = psz;    // Start of text block

            psz++;
        }

        if(psz1)    // Copy any remaining text to the output buffer
        {
            if((cch = (psz - psz1)+1) > (INT)(nSize - cchBufUsed))
            {
                if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
                {
                    LPWSTR lpRealloc;
                    nSize = max(FML_BUFFER_INC, cch);
                    if((lpRealloc = (WCHAR *)LocalReAlloc(lpBuffer, nSize * sizeof(WCHAR), LMEM_ZEROINIT | LMEM_MOVEABLE)) == NULL)
                    {
                        LocalFree(lpBuffer);
                        LocalFree(pszBuf2);
                        return(0);
                    }
                    lpBuffer = lpRealloc;
                }
                else
                {
                    RIPMSG(FALSE, "Output buffer not large enough.  Truncating.");
                    cch = nSize - cchBufUsed;
                }
            }
            StrCpyNW(lpBuffer+cchBufUsed, psz1, cch);
            cchBufUsed += cch - 1;  // substract the null from the count
        }

        LocalFree(pszBuf2);
        pszBuf2 = NULL;
    }

    if(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        *pDest = (PVOID *)lpBuffer;
    }
    
    return((DWORD)cchBufUsed);
}

DWORD WINAPI
FormatMessageWrapW(
    DWORD       dwFlags,
    LPCVOID     lpSource,
    DWORD       dwMessageId,
    DWORD       dwLanguageId,
    LPWSTR      lpBuffer,
    DWORD       nSize,
    va_list *   Arguments)
{
    VALIDATE_PROTOTYPE(FormatMessage);
    if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER))
    {
        VALIDATE_OUTBUF(lpBuffer, nSize);
    }

    DWORD dwResult;

#ifdef DEBUG
    // If a source string is passed, make sure that all string insertions
    // have explicit character set markers.  Otherwise, you get random
    // behavior depending on whether we need to thunk to ANSI or not.
    // (We are not clever enough to thunk the inserts; that's the caller's
    // responsibility.)
    //
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        LPCWSTR pwsz;
        for (pwsz = (LPCWSTR)lpSource; *pwsz; pwsz++)
        {
            if (*pwsz == L'%')
            {
                pwsz++;
                // Found an insertion.  Get the digit or two.
                if (*pwsz == L'0')
                    continue;       // "%0" is special
                if (*pwsz < L'0' || *pwsz > L'9')
                    continue;        // skip % followed by nondigit
                pwsz++;            // Skip the digit
                if (*pwsz >= L'0' && *pwsz <= L'9')
                    pwsz++;        // Skip the optional second digit
                // The next character MUST be an exclamation point!
                ASSERT(*pwsz == L'!' &&
                       "FormatMessageWrapW: All string insertions must have explicit character sets.");
                // I'm not going to validate that the insertion contains
                // an explicit character set override because if you went
                // so far as to do a %n!...!, you'll get the last bit right too.
            }
        }
    }
#endif

    if (g_bRunningOnNT)
    {
        return FormatMessageW(
                dwFlags,
                lpSource,
                dwMessageId,
                dwLanguageId,
                lpBuffer,
                nSize,
                Arguments);
    }

    //
    //  FORMAT_MESSAGE_FROM_STRING means that the source is a string.
    //  Otherwise, it's an opaque LPVOID (aka, an atom).
    //
    if(((dwFlags == FORMAT_MESSAGE_FROM_STRING) || 
        (dwFlags == (FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER))) &&
       (dwMessageId == 0) && (dwLanguageId == 0))
    {
        TraceMsg(TF_WARNING, "This is a lite version of FormatMessage.  It may not act as you expect.");
        dwResult = FormatMessageLiteW(dwFlags, (LPCWSTR)lpSource, (PVOID *)lpBuffer, nSize, Arguments);
    }
    else
    {
        CStrIn strSource((dwFlags & FORMAT_MESSAGE_FROM_STRING) ? CP_ACP : CP_ATOM,
                         (LPCWSTR)lpSource, -1);

        if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER))
        {
            CStrOut str(lpBuffer, nSize);
            FormatMessageA(
                    dwFlags,
                    strSource,
                    dwMessageId,
                    dwLanguageId,
                    str,
                    str.BufSize(),
                    Arguments);         // We don't handle Arguments != NULL

            dwResult = str.ConvertExcludingNul();
        }
        else
        {
            LPSTR pszBuffer = NULL;
            LPWSTR * ppwzOut = (LPWSTR *)lpBuffer;

            *ppwzOut = NULL;
            FormatMessageA(
                    dwFlags,
                    strSource,
                    dwMessageId,
                    dwLanguageId,
                    (LPSTR)&pszBuffer,
                    0,
                    Arguments);

            if (pszBuffer)
            {
                DWORD cchSize = (lstrlenA(pszBuffer) + 1);
                LPWSTR pszThunkedBuffer;

                if (cchSize < nSize)
                    cchSize = nSize;

                pszThunkedBuffer = (LPWSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
                if (pszThunkedBuffer)
                {
                    *ppwzOut = pszThunkedBuffer;
                    SHAnsiToUnicode(pszBuffer, pszThunkedBuffer, cchSize);
                }

                LocalFree(pszBuffer);
            }
            dwResult = (*ppwzOut ? lstrlenW(*ppwzOut) : 0);
        }
    }

    return dwResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
GetClassInfoWrapW(HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW)
{
    VALIDATE_PROTOTYPE(GetClassInfo);

    if (g_bRunningOnNT)
    {
        return GetClassInfoW(hModule, lpClassName, lpWndClassW);
    }

    BOOL    ret;

    CStrIn  strClassName(lpClassName);

    ASSERT(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    ret = GetClassInfoA(hModule, strClassName, (LPWNDCLASSA) lpWndClassW);

    lpWndClassW->lpszMenuName = NULL;
    lpWndClassW->lpszClassName = NULL;
    return ret;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

DWORD FORWARD_API WINAPI
GetClassLongWrapW(HWND hWnd, int nIndex)
{
    VALIDATE_PROTOTYPE(GetClassLong);

    FORWARD_AW(GetClassLong, (hWnd, nIndex));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetClassNameWrapW(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
    VALIDATE_PROTOTYPE(GetClassName);
    VALIDATE_OUTBUF(lpClassName, nMaxCount);

    if (g_bRunningOnNT)
    {
        return GetClassNameW(hWnd, lpClassName, nMaxCount);
    }

    CStrOut strClassName(lpClassName, nMaxCount);

    GetClassNameA(hWnd, strClassName, strClassName.BufSize());
    return strClassName.ConvertIncludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetClipboardFormatNameWrapW(UINT format, LPWSTR lpFormatName, int cchFormatName)
{
    VALIDATE_PROTOTYPE(GetClipboardFormatName);
    VALIDATE_OUTBUF(lpFormatName, cchFormatName);

    if (g_bRunningOnNT)
    {
        return GetClipboardFormatNameW(format, lpFormatName, cchFormatName);
    }

    CStrOut strFormatName(lpFormatName, cchFormatName);

    GetClipboardFormatNameA(format, strFormatName, strFormatName.BufSize());
    return strFormatName.ConvertIncludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetCurrentDirectoryWrapW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    VALIDATE_PROTOTYPE(GetCurrentDirectory);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetCurrentDirectoryW(nBufferLength, lpBuffer);
    }

    CStrOut str(lpBuffer, nBufferLength);

    GetCurrentDirectoryA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
GetDlgItemTextWrapW(
        HWND    hWndDlg,
        int     idControl,
        LPWSTR  lpsz,
        int     cchMax)
{
    VALIDATE_PROTOTYPE(GetDlgItemText);
    VALIDATE_OUTBUF(lpsz, cchMax);

    HWND hWnd = GetDlgItem(hWndDlg, idControl);

    if (hWnd)
        return GetWindowTextWrapW(hWnd, lpsz, cchMax);

    /*
     * If we couldn't find the window, just null terminate lpch so that the
     * app doesn't gp fault if it tries to run through the text.
      */
    if (cchMax)
        *lpsz = (WCHAR)0;

    return 0;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetFileAttributesWrapW(LPCWSTR lpFileName)
{
    VALIDATE_PROTOTYPE(GetFileAttributes);

    if (g_bRunningOnNT)
    {
        return GetFileAttributesW(lpFileName);
    }

    CStrIn  str(lpFileName);

    return GetFileAttributesA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI
GetLocaleInfoWrapW(LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData)
{
    VALIDATE_PROTOTYPE(GetLocaleInfo);
    VALIDATE_OUTBUF(lpsz, cchData);

    if (g_bRunningOnNT)
    {
        return GetLocaleInfoW(Locale, LCType, lpsz, cchData);
    }

    CStrOut str(lpsz, cchData);

    GetLocaleInfoA(Locale, LCType, str, str.BufSize());
    return str.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
GetMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax)
{
    VALIDATE_PROTOTYPE(GetMessage);

    FORWARD_AW(GetMessage, (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetModuleFileNameWrapW(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize)
{
    VALIDATE_PROTOTYPE(GetModuleFileName);
    VALIDATE_OUTBUF(pwszFilename, nSize);

    if (g_bRunningOnNT)
    {
        return GetModuleFileNameW(hModule, pwszFilename, nSize);
    }

    CStrOut str(pwszFilename, nSize);

    GetModuleFileNameA(hModule, str, str.BufSize());
    return str.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetSystemDirectoryWrapW(LPWSTR lpBuffer, UINT uSize)
{
    VALIDATE_PROTOTYPE(GetSystemDirectory);
    VALIDATE_OUTBUF(lpBuffer, uSize);

    if (g_bRunningOnNT)
    {
        return GetSystemDirectoryW(lpBuffer, uSize);
    }

    CStrOut str(lpBuffer, uSize);

    GetSystemDirectoryA(str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
SearchPathWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpFileName,
        LPCWSTR lpExtension,
        DWORD   cchReturnBuffer,
        LPWSTR  lpReturnBuffer,
        LPWSTR *  plpfilePart)
{
    VALIDATE_PROTOTYPE(SearchPath);
    VALIDATE_OUTBUF(lpReturnBuffer, cchReturnBuffer);

    if (g_bRunningOnNT)
    {
        return SearchPathW(
            lpPathName,
            lpFileName,
            lpExtension,
            cchReturnBuffer,
            lpReturnBuffer,
            plpfilePart);
    }

    CStrIn  strPath(lpPathName);
    CStrIn  strFile(lpFileName);
    CStrIn  strExtension(lpExtension);
    CStrOut strReturnBuffer(lpReturnBuffer, cchReturnBuffer);

    DWORD dwLen = SearchPathA(
            strPath,
            strFile,
            strExtension,
            strReturnBuffer.BufSize(),
            strReturnBuffer,
            (LPSTR *)plpfilePart);

    //
    // Getting the correct value for plpfilePart requires
    // a strrchr on the converted string.  If this value
    // is needed, just add the code to do it here.
    //

    *plpfilePart = NULL;

    if (cchReturnBuffer == 0)
        dwLen = 2*dwLen;
    else
        dwLen = strReturnBuffer.ConvertExcludingNul();

    return dwLen;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HMODULE WINAPI
GetModuleHandleWrapW(LPCWSTR lpModuleName)
{
    VALIDATE_PROTOTYPE(GetModuleHandle);

    if (g_bRunningOnNT)
    {
        return GetModuleHandleW(lpModuleName);
    }

    CStrIn  str(lpModuleName);
    return GetModuleHandleA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
GetObjectWrapW(HGDIOBJ hgdiObj, int cbBuffer, LPVOID lpvObj)
{
    VALIDATE_PROTOTYPE(GetObject);

    if (g_bRunningOnNT)
    {
        return GetObjectW(hgdiObj, cbBuffer, lpvObj);
    }

    int nRet;

    if(cbBuffer != sizeof(LOGFONTW))
    {
        nRet = GetObjectA(hgdiObj, cbBuffer, lpvObj);
    }
    else
    {
        LOGFONTA lfa;

        nRet = GetObjectA(hgdiObj, sizeof(lfa), &lfa);
        if (nRet > 0)
        {
            memcpy(lpvObj, &lfa, FIELD_OFFSET(LOGFONTW, lfFaceName));
            SHAnsiToUnicode(lfa.lfFaceName, ((LOGFONTW*)lpvObj)->lfFaceName, ARRAYSIZE(((LOGFONTW*)lpvObj)->lfFaceName));
            nRet = sizeof(LOGFONTW);
        }
    }

    return nRet;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

LPITEMIDLIST WINAPI SHBrowseForFolderWrapW(LPBROWSEINFOW pbiW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHBrowseForFolder, _SHBrowseForFolder);


    if (UseUnicodeShell32())
        return _SHBrowseForFolderW(pbiW);

    LPITEMIDLIST pidl = NULL;
    if (EVAL(pbiW))
    {
        CStrIn strTitle(pbiW->lpszTitle);
        CStrOut strDisplayName(pbiW->pszDisplayName, MAX_PATH);
        BROWSEINFOA biA;

        biA = * (LPBROWSEINFOA) pbiW;
        biA.lpszTitle = strTitle;
        biA.pszDisplayName = strDisplayName;

        pidl = _SHBrowseForFolderA(&biA);
        if (pidl)
            strDisplayName.ConvertIncludingNul();
    }

    return pidl;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

BOOL WINAPI SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl, LPWSTR pwzPath)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHGetPathFromIDList, _SHGetPathFromIDList);

    if (UseUnicodeShell32())
        return _SHGetPathFromIDListW(pidl, pwzPath);

    CStrOut strPathOut(pwzPath, MAX_PATH);
    BOOL fResult = _SHGetPathFromIDListA(pidl, strPathOut);
    if (fResult)
        strPathOut.ConvertIncludingNul();

    return fResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER
BOOL WINAPI ShellExecuteExWrapW(LPSHELLEXECUTEINFOW pExecInfoW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(ShellExecuteEx, _ShellExecuteEx);

    if (g_bRunningOnNT)
        return _ShellExecuteExW(pExecInfoW);

    BOOL fResult = FALSE;
    if (EVAL(pExecInfoW))
    {
        SHELLEXECUTEINFOA ExecInfoA;

        CStrIn strVerb(pExecInfoW->lpVerb);
        CStrIn strParameters(pExecInfoW->lpParameters);
        CStrIn strDirectory(pExecInfoW->lpDirectory);
        CStrIn strClass(pExecInfoW->lpClass);
        CHAR szFile[MAX_PATH + INTERNET_MAX_URL_LENGTH + 2];


        ExecInfoA = *(LPSHELLEXECUTEINFOA) pExecInfoW;
        ExecInfoA.lpVerb = strVerb;
        ExecInfoA.lpParameters = strParameters;
        ExecInfoA.lpDirectory = strDirectory;
        ExecInfoA.lpClass = strClass;

        if (pExecInfoW->lpFile)
        {
            ExecInfoA.lpFile = szFile;
            SHUnicodeToAnsi(pExecInfoW->lpFile, szFile, ARRAYSIZE(szFile));

            // SEE_MASK_FILEANDURL passes "file\0url".  What a hack!
            if (pExecInfoW->fMask & SEE_MASK_FILEANDURL)
            {
                // We are so lucky that Win9x implements lstrlenW
                int cch = lstrlenW(pExecInfoW->lpFile) + 1;
                cch += lstrlenW(pExecInfoW->lpFile + cch) + 1;
                if (!WideCharToMultiByte(CP_ACP, 0, pExecInfoW->lpFile, cch, szFile, ARRAYSIZE(szFile), NULL, NULL))
                {
                    // Return a completely random error code
                    pExecInfoW->hInstApp = (HINSTANCE)SE_ERR_OOM;
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
            }
        }

        fResult = _ShellExecuteExA(&ExecInfoA);

        // Out parameters
        pExecInfoW->hInstApp = ExecInfoA.hInstApp;
        pExecInfoW->hProcess = ExecInfoA.hProcess;
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return fResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

int WINAPI SHFileOperationWrapW(LPSHFILEOPSTRUCTW pFileOpW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHFileOperation, _SHFileOperation);
    // We don't thunk multiple files.
    ASSERT(!(pFileOpW->fFlags & FOF_MULTIDESTFILES));

    if (UseUnicodeShell32())
        return _SHFileOperationW(pFileOpW);

    int nResult = 1;    // non-Zero is failure.
    if (EVAL(pFileOpW))
    {
        SHFILEOPSTRUCTA FileOpA;
        CStrIn strTo(pFileOpW->pTo);
        CStrIn strFrom(pFileOpW->pFrom);
        CStrIn strProgressTitle(pFileOpW->lpszProgressTitle);

        FileOpA = *(LPSHFILEOPSTRUCTA) pFileOpW;
        FileOpA.pFrom = strFrom;
        FileOpA.pTo = strTo;
        FileOpA.lpszProgressTitle = strProgressTitle;


        nResult = _SHFileOperationA(&FileOpA);
    }

    return nResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

UINT WINAPI ExtractIconExWrapW(LPCWSTR pwzFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(ExtractIconEx, _ExtractIconEx);

    if ( UseUnicodeShell32() )
        return _ExtractIconExW(pwzFile, nIconIndex, phiconLarge, phiconSmall, nIcons);

    CStrIn  str(pwzFile);
    return _ExtractIconExA(str, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

#endif // NEED_SHELL32_WRAPPER


#ifdef NEED_SHELL32_WRAPPER
//
// Shell_GetCachedImageIndexWrapA/W are exported from shlwapi.
// Only the W version should be in NEED_SHELL32_WRAPPER
int WINAPI Shell_GetCachedImageIndexWrapW(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    if (UseUnicodeShell32())
        return _Shell_GetCachedImageIndexW((LPVOID)pszIconPath, iIconIndex, uIconFlags);

    CStrIn  strIconPath(pszIconPath);
    return _Shell_GetCachedImageIndexW((LPVOID)strIconPath, iIconIndex, uIconFlags);
}
#endif // NEED_SHELL32_WRAPPER

int WINAPI Shell_GetCachedImageIndexWrapA(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    if (UseUnicodeShell32())
    {
        WCHAR szIconPath[MAX_PATH];
        SHAnsiToUnicode(pszIconPath, szIconPath, ARRAYSIZE(szIconPath));
        return _Shell_GetCachedImageIndexW((LPVOID)szIconPath, iIconIndex, uIconFlags);
    }
    
    return _Shell_GetCachedImageIndexW((LPVOID)pszIconPath, iIconIndex, uIconFlags);
}


#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI SetFileAttributesWrapW(LPCWSTR pwzFile, DWORD dwFileAttributes)
{
    VALIDATE_PROTOTYPE(SetFileAttributes);

    if (g_bRunningOnNT)
        return SetFileAttributesW(pwzFile, dwFileAttributes);

    CStrIn  str(pwzFile);
    return SetFileAttributesA(str, dwFileAttributes);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetNumberFormatWrapW(LCID Locale, DWORD dwFlags, LPCWSTR pwzValue, CONST NUMBERFMTW * pFormatW, LPWSTR pwzNumberStr, int cchNumber)
{
    VALIDATE_PROTOTYPE(GetNumberFormat);

    if (g_bRunningOnNT)
        return GetNumberFormatW(Locale, dwFlags, pwzValue, pFormatW, pwzNumberStr, cchNumber);

    int nResult;
    NUMBERFMTA FormatA;
    CStrIn  strValue(pwzValue);
    CStrIn  strDecimalSep(pFormatW ? pFormatW->lpDecimalSep : NULL);
    CStrIn  strThousandSep(pFormatW ? pFormatW->lpThousandSep : NULL);
    CStrOut strNumberStr(pwzNumberStr, cchNumber);

    if (pFormatW)
    {
        FormatA = *(NUMBERFMTA *) pFormatW;
        FormatA.lpDecimalSep = strDecimalSep;
        FormatA.lpThousandSep = strThousandSep;
    }

    nResult = GetNumberFormatA(Locale, dwFlags, strValue, (pFormatW ? &FormatA : NULL), strNumberStr, strNumberStr.BufSize());
    if (ERROR_SUCCESS == nResult)
        strNumberStr.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI MessageBoxWrapW(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType)
{
    VALIDATE_PROTOTYPE(MessageBox);

    if (g_bRunningOnNT)
        return MessageBoxW(hwnd, pwzText, pwzCaption, uType);

    CStrIn  strCaption(pwzCaption);
    CStrIn  strText(pwzText);
    return MessageBoxA(hwnd, strText, strCaption, uType);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI FindNextFileWrapW(HANDLE hSearchHandle, LPWIN32_FIND_DATAW pFindFileDataW)
{
    VALIDATE_PROTOTYPE(FindNextFile);

    if (g_bRunningOnNT)
        return FindNextFileW(hSearchHandle, pFindFileDataW);

    CWin32FindDataInOut fd(pFindFileDataW);
    return FindNextFileA(hSearchHandle, fd);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

//--------------------------------------------------------------
//      GetFullPathNameWrap
//--------------------------------------------------------------

DWORD
WINAPI
GetFullPathNameWrapW( LPCWSTR lpFileName,
                     DWORD  nBufferLength,
                     LPWSTR lpBuffer,
                     LPWSTR *lpFilePart)
{
    VALIDATE_PROTOTYPE(GetFullPathName);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
    }

    CStrIn  strIn(lpFileName);
    CStrOut  strOut(lpBuffer,nBufferLength);
    LPSTR   pFile;
    DWORD   dwRet;

    dwRet = GetFullPathNameA(strIn, nBufferLength, strOut, &pFile);
    strOut.ConvertIncludingNul();
    // BUGBUG raymondc - This is wrong if we had to do DBCS or related goo
    *lpFilePart = lpBuffer + (pFile - strOut);
    return dwRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetShortPathNameWrapW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD   cchBuffer)
{
    VALIDATE_PROTOTYPE(GetShortPathName);

    if (g_bRunningOnNT)
    {
        return GetShortPathNameW(lpszLongPath, lpszShortPath, cchBuffer);

    }

    CStrIn strLongPath(lpszLongPath);
    CStrOut strShortPath(lpszShortPath, cchBuffer);

    return GetShortPathNameA(strLongPath, strShortPath, strShortPath.BufSize());
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
GetStringTypeExWrapW(LCID lcid, DWORD dwInfoType, LPCTSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
    VALIDATE_PROTOTYPE(GetStringTypeEx);

    if (g_bRunningOnNT)
    {
        return GetStringTypeExW(lcid, dwInfoType, lpSrcStr, cchSrc, lpCharType);
    }

    CStrIn  str(lpSrcStr, cchSrc);
    return GetStringTypeExA(lcid, dwInfoType, str, str.strlen(), lpCharType);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetPrivateProfileIntWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        INT     nDefault,
        LPCWSTR lpFileName)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileInt);

    if (g_bRunningOnNT)
    {
        return GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);
    }

    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CPPFIn  strFile(lpFileName); // PrivateProfile filename needs special class

    return GetPrivateProfileIntA(strApp, strKey, nDefault, strFile);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetProfileStringWrapW(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        LPCWSTR lpDefault,
        LPWSTR  lpBuffer,
        DWORD   dwBuffersize)
{
    VALIDATE_PROTOTYPE(GetProfileString);
    VALIDATE_OUTBUF(lpBuffer, dwBuffersize);

    if (g_bRunningOnNT)
    {
        return GetProfileStringW(lpAppName, lpKeyName, lpDefault, lpBuffer, dwBuffersize);
    }

    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CStrIn  strDefault(lpDefault);
    CStrOut strBuffer(lpBuffer, dwBuffersize);

    GetProfileStringA(strApp, strKey, strDefault, strBuffer, dwBuffersize);
    return strBuffer.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HANDLE WINAPI
GetPropWrapW(HWND hWnd, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(GetProp);

    if (g_bRunningOnNT)
    {
        return GetPropW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return GetPropA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetTempFileNameWrapW(
        LPCWSTR lpPathName,
        LPCWSTR lpPrefixString,
        UINT    uUnique,
        LPWSTR  lpTempFileName)
{
    VALIDATE_PROTOTYPE(GetTempFileName);
    VALIDATE_OUTBUF(lpTempFileName, MAX_PATH);

    if (g_bRunningOnNT)
    {
        return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);
    }


    CStrIn  strPath(lpPathName);
    CStrIn  strPrefix(lpPrefixString);
    CStrOut strFileName(lpTempFileName, MAX_PATH);

    return GetTempFileNameA(strPath, strPrefix, uUnique, strFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetTempPathWrapW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    VALIDATE_PROTOTYPE(GetTempPath);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetTempPathW(nBufferLength, lpBuffer);
    }


    CStrOut str(lpBuffer, nBufferLength);

    GetTempPathA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

BOOL APIENTRY
GetTextExtentPoint32WrapW(
        HDC     hdc,
        LPCWSTR pwsz,
        int     cb,
        LPSIZE  pSize)
{
    VALIDATE_PROTOTYPE(GetTextExtentPoint32);

    if (g_bRunningOnNT)
    {
        return GetTextExtentPoint32W(hdc, pwsz, cb, pSize);
    }


    CStrIn str(pwsz,cb);

    return GetTextExtentPoint32A(hdc, str, str.strlen(), pSize);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
GetTextFaceWrapW(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName)
{
    VALIDATE_PROTOTYPE(GetTextFace);
    VALIDATE_OUTBUF(lpFaceName, cch);

    if (g_bRunningOnNT)
    {
        return GetTextFaceW(hdc, cch, lpFaceName);
    }


    CStrOut str(lpFaceName, cch);

    GetTextFaceA(hdc, str.BufSize(), str);
    return str.ConvertIncludingNul();
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

BOOL WINAPI
GetTextMetricsWrapW(HDC hdc, LPTEXTMETRICW lptm)
{
    VALIDATE_PROTOTYPE(GetTextMetrics);

    if (g_bRunningOnNT)
    {
        return GetTextMetricsW(hdc, lptm);
    }


   BOOL         ret;
   TEXTMETRICA  tm;

    ret = GetTextMetricsA(hdc, &tm);

    if (ret)
    {
        lptm->tmHeight              = tm.tmHeight;
        lptm->tmAscent              = tm.tmAscent;
        lptm->tmDescent             = tm.tmDescent;
        lptm->tmInternalLeading     = tm.tmInternalLeading;
        lptm->tmExternalLeading     = tm.tmExternalLeading;
        lptm->tmAveCharWidth        = tm.tmAveCharWidth;
        lptm->tmMaxCharWidth        = tm.tmMaxCharWidth;
        lptm->tmWeight              = tm.tmWeight;
        lptm->tmOverhang            = tm.tmOverhang;
        lptm->tmDigitizedAspectX    = tm.tmDigitizedAspectX;
        lptm->tmDigitizedAspectY    = tm.tmDigitizedAspectY;
        lptm->tmItalic              = tm.tmItalic;
        lptm->tmUnderlined          = tm.tmUnderlined;
        lptm->tmStruckOut           = tm.tmStruckOut;
        lptm->tmPitchAndFamily      = tm.tmPitchAndFamily;
        lptm->tmCharSet             = tm.tmCharSet;

        // LPBYTE -> LPCSTR casts below
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmFirstChar, 1, &lptm->tmFirstChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmLastChar, 1, &lptm->tmLastChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmDefaultChar, 1, &lptm->tmDefaultChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmBreakChar, 1, &lptm->tmBreakChar, 1);
    }

    return ret;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

BOOL WINAPI GetUserNameWrapW(LPWSTR pszBuffer, LPDWORD pcch)
{
    VALIDATE_PROTOTYPE(GetUserName);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = GetUserNameW(pszBuffer, pcch);
    }
    else
    {
        CStrOut stroBuffer(pszBuffer, *pcch);

        fRet = GetUserNameA(stroBuffer, pcch);

        if (fRet)
            *pcch = stroBuffer.ConvertIncludingNul();
    }

    return fRet;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LONG FORWARD_API WINAPI
GetWindowLongWrapW(HWND hWnd, int nIndex)
{
    VALIDATE_PROTOTYPE(GetWindowLong);

    FORWARD_AW(GetWindowLong, (hWnd, nIndex));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetWindowTextWrapW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
    VALIDATE_PROTOTYPE(GetWindowText);
    VALIDATE_OUTBUF(lpString, nMaxCount);

    if (MLIsEnabled(hWnd))
        return MLGetControlTextI(hWnd, lpString, nMaxCount);

    if (g_bRunningOnNT)
    {
        return GetWindowTextW(hWnd, lpString, nMaxCount);
    }


    CStrOut str(lpString, nMaxCount);

    GetWindowTextA(hWnd, str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetWindowTextLengthWrapW(HWND hWnd)
{
    VALIDATE_PROTOTYPE(GetWindowTextLength);

    if (g_bRunningOnNT)
    {
        return GetWindowTextLengthW(hWnd);
    }

    return GetWindowTextLengthA(hWnd);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetWindowsDirectoryWrapW(LPWSTR lpWinPath, UINT cch)
{
    VALIDATE_PROTOTYPE(GetWindowsDirectory);
    VALIDATE_OUTBUF(lpWinPath, cch);

    if (g_bRunningOnNT)
    {
        return GetWindowsDirectoryW(lpWinPath, cch);
    }

    CStrOut str(lpWinPath, cch);

    GetWindowsDirectoryA(str, str.BufSize());

    return str.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

STDAPI_(DWORD) WINAPI
GetEnvironmentVariableWrapW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
    VALIDATE_PROTOTYPE(GetEnvironmentVariable);
    VALIDATE_OUTBUF(lpBuffer, nSize);

    if (g_bRunningOnNT)
    {
        return GetEnvironmentVariableW(lpName,lpBuffer, nSize);
    }

    CStrOut str(lpBuffer, nSize);
    CStrIn  strName(lpName);

    GetEnvironmentVariableA(strName, str, str.BufSize());
    return str.ConvertExcludingNul();

}


#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
IsDialogMessageWrapW(HWND hWndDlg, LPMSG lpMsg)
{
    VALIDATE_PROTOTYPE(IsDialogMessage);

    FORWARD_AW(IsDialogMessage, (hWndDlg, lpMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HACCEL WINAPI
LoadAcceleratorsWrapW(HINSTANCE hInstance, LPCWSTR lpTableName)
{
    VALIDATE_PROTOTYPE(LoadAccelerators);
    ASSERT(HIWORD64(lpTableName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadAcceleratorsW(hInstance, lpTableName);
    }

    return LoadAcceleratorsA(hInstance, (LPCSTR) lpTableName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HBITMAP WINAPI
LoadBitmapWrapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
    VALIDATE_PROTOTYPE(LoadBitmap);
    ASSERT(HIWORD64(lpBitmapName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadBitmapW(hInstance, lpBitmapName);
    }

    return LoadBitmapA(hInstance, (LPCSTR) lpBitmapName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HCURSOR WINAPI
LoadCursorWrapW(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
    VALIDATE_PROTOTYPE(LoadCursor);
    ASSERT(HIWORD64(lpCursorName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadCursorW(hInstance, lpCursorName);
    }

    return LoadCursorA(hInstance, (LPCSTR) lpCursorName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HICON WINAPI
LoadIconWrapW(HINSTANCE hInstance, LPCWSTR lpIconName)
{
    VALIDATE_PROTOTYPE(LoadIcon);
    ASSERT(HIWORD64(lpIconName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadIconW(hInstance, lpIconName);
    }

    return LoadIconA(hInstance, (LPCSTR) lpIconName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

//+-------------------------------------------------------------------------
// Returns an icon from a dll/exe loaded as a datafile.  Note that this is 
// very similar to the standard LoadImage API.  However, LoadImage does
// not work properly (returns bogus icons) on win95/98 when the dll is
// loaded as a datafile.  (Stevepro 18-Sept-98)
//--------------------------------------------------------------------------
HICON _LoadIconFromInstanceA
(
    HINSTANCE hInstance,    // instance containing the icon
    LPCSTR pszName,         // name or id of icon
    int cxDesired,
    int cyDesired,
    UINT fuLoad             // load flags, LR_DEFAULTCOLOR | LR_MONOCHROME
) 
{ 
    HICON hIcon;

    UINT fUnsupportedFlags = ~(LR_DEFAULTCOLOR | LR_MONOCHROME);
    if (fuLoad & fUnsupportedFlags)
    {
        // We saw a flag that we don't support, so default to the standard API
        hIcon = (HICON)LoadImageA(hInstance, pszName, IMAGE_ICON, cxDesired, cyDesired, fuLoad);
    }
    else
    {
        // Find the group icon 
        HRSRC hRsrc; 
        HGLOBAL hGlobal; 
        PBYTE pIconDir;

        if ((hRsrc = FindResourceA(hInstance, pszName, (LPCSTR)RT_GROUP_ICON)) == NULL ||
            (hGlobal = LoadResource(hInstance, hRsrc)) == NULL || 
            (pIconDir = (PBYTE)LockResource(hGlobal)) == NULL)
        {
            // Note that FreeResource and UnlockResource are obsolete APIs, so we just exit.
            return NULL; 
        }
 
        // Find the icon that best matches the desired size 
        int nID = LookupIconIdFromDirectoryEx(pIconDir, TRUE, cxDesired, cyDesired, fuLoad); 

        PBYTE pbRes; 
        if (0 == nID ||
            (hRsrc = FindResourceA(hInstance, MAKEINTRESOURCEA(nID), (LPCSTR)RT_ICON)) == NULL ||
            (hGlobal = LoadResource(hInstance, hRsrc)) == NULL || 
            (pbRes = (PBYTE)LockResource(hGlobal)) == NULL)
        {
            return NULL; 
        }

        // Let the OS make us an icon 
        LPBITMAPINFOHEADER pbmh = (LPBITMAPINFOHEADER)pbRes;
        hIcon = CreateIconFromResourceEx(pbRes, SizeofResource(hInstance, hRsrc), TRUE, 0x00030000,  
                pbmh->biWidth, pbmh->biHeight/2, fuLoad); 
     
        // It failed, odds are good we're on NT so try the non-Ex way 
        if (hIcon == NULL) 
        { 
            // We would break on NT if we try with a 16bpp image 
            if(pbmh->biBitCount != 16) 
            { 
                hIcon = CreateIconFromResource(pbRes, SizeofResource(hInstance, hRsrc), TRUE, 0x00030000); 
            } 
        } 
    }

    return hIcon; 
}

HANDLE WINAPI
LoadImageWrapA(
        HINSTANCE hInstance,
        LPCSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad)
{
    VALIDATE_PROTOTYPE(LoadImage);

    //
    // If the hInstance is loaded as a datafile, LoadImage returns the 
    // wrong icons on win95/98.  (Kernel can't find the datafile and uses
    // null instead)
    //
    // bugbug: Should also fix loading other resource types from datafiles
    //
    if (!g_bRunningOnNT && uType == IMAGE_ICON)
    {
        return _LoadIconFromInstanceA(
                hInstance,
                lpName,
                cxDesired,
                cyDesired,
                fuLoad);
    }

    return LoadImageA(
        hInstance,
        lpName,
        uType,
        cxDesired,
        cyDesired,
        fuLoad);
}


HANDLE WINAPI
LoadImageWrapW(
        HINSTANCE hInstance,
        LPCWSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad)
{
    VALIDATE_PROTOTYPE(LoadImage);

    if (g_bRunningOnNT)
    {
        return LoadImageW(
            hInstance,
            lpName,
            uType,
            cxDesired,
            cyDesired,
            fuLoad);
    }

    CStrIn  str(lpName);

    return LoadImageWrapA(
            hInstance,
            str,
            uType,
            cxDesired,
            cyDesired,
            fuLoad);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HINSTANCE WINAPI
LoadLibraryExWrapW(
        LPCWSTR lpLibFileName,
        HANDLE  hFile,
        DWORD   dwFlags)
{
    VALIDATE_PROTOTYPE(LoadLibraryEx);

    if (g_bRunningOnNT)
        return LoadLibraryExW(lpLibFileName, hFile, dwFlags);

    CStrIn  str(lpLibFileName);

    // Win9X will crash if the pathname is longer than MAX_PATH bytes.

    if (str.strlen() >= MAX_PATH)
    {
        SetLastError( ERROR_BAD_PATHNAME );
        return NULL;
    }
    else
    {
        return LoadLibraryExA(str, hFile, dwFlags);
    }
}

#endif // NEED_KERNEL32_WRAPPER

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL fDoMungeLangId(LANGID lidUI);

BOOL NeedMenuOwnerDraw(void)
{
    // BUGBUG: need to be improved
    //         It checks whether current UI language is cross codepage or not
    if (g_bRunningOnNT5OrHigher)
        return FALSE;

    return fDoMungeLangId(MLGetUILanguage());
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

#define PUI_OWNERDRAW_SIG   0xFFFF0000

// NOTE: This structure is only visible from subclassed OwnerDrawSubclassProc
//       with WM_DRAWITEM and WM_MEASUREITEM. All other places won't see this
//       structure. We restore original format MENUITEMINFO structure without
//       MFT_OWNERDRAW flag unless it's a real owner draw item.
//       For this, we do/undo munge in Get/SetMenuItemInfo and InsertMenuItem.

typedef struct tagPUIMENUITEM
{
    DWORD  dwSig;       // signature for this structure
    HMENU  hmenu;       // menu handle
    UINT   fMask;       // original MENUITEMINFO fMask value
    UINT   fType;       // original MENUITEMINFO fType value
    DWORD_PTR dwItemData; // original MENUITEMINFO dwItemData value
    LPWSTR lpwz;        // unicode menu string
    UINT   cch;         // number of character for menu string
} PUIMENUITEM, *LPPUIMENUITEM;

void MungeMenuItem(HMENU hMenu, LPCMENUITEMINFOW lpmiiW, LPMENUITEMINFOW lpmiiNewW)
{
    if (lpmiiW && lpmiiNewW)
    {
        *lpmiiNewW = *lpmiiW;

        if ((MIIM_TYPE & lpmiiW->fMask) && !(MFT_NONSTRING & lpmiiW->fType))
        {
            LPWSTR lpmenuText = (LPWSTR)lpmiiW->dwTypeData;

            if (lpmenuText)
            {
                LPPUIMENUITEM lpItem = (LPPUIMENUITEM)LocalAlloc(LPTR, sizeof(PUIMENUITEM));

                if (lpItem)
                {
                    lpItem->dwSig = PUI_OWNERDRAW_SIG;
                    lpItem->hmenu = hMenu;
                    lpItem->fMask = lpmiiW->fMask;
                    lpItem->fType = lpmiiW->fType;
                    lpItem->dwItemData = lpmiiW->dwItemData;
                    lpItem->cch = lstrlenW(lpmenuText);
                    lpItem->lpwz = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (lpItem->cch + 1));
                    if (lpItem->lpwz)
                        StrCpyNW(lpItem->lpwz, lpmenuText, lpItem->cch + 1);
                    else
                        lpItem->cch = 0;

                    lpmiiNewW->fType |= MFT_OWNERDRAW;
                    lpmiiNewW->fMask |= MIIM_DATA;
                    lpmiiNewW->dwItemData = (ULONG_PTR)lpItem;
                    lpmiiNewW->dwTypeData = 0;
                    lpmiiNewW->cch = 0;
                }
            }
        }
    }
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

void DeleteOwnerDrawMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition);

void DeleteOwnerDrawMenu(HMENU hMenu)
{
    int i, nItem = GetMenuItemCount(hMenu);

    for (i = 0; i < nItem; i++)
        DeleteOwnerDrawMenuItem(hMenu, i, TRUE);
}

void DeleteOwnerDrawMenuItem(HMENU hMenu, UINT uItem, BOOL fByPosition)
{
    MENUITEMINFOA miiA;

    miiA.cbSize = sizeof(miiA);
    miiA.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
    miiA.cch = 0;
    if (GetMenuItemInfoA(hMenu, uItem, fByPosition, &miiA))
    {
        if (miiA.hSubMenu)
            DeleteOwnerDrawMenu(miiA.hSubMenu);
        else
        {
            if ((MIIM_TYPE & miiA.fMask) && (MFT_OWNERDRAW & miiA.fType))
            {
                LPPUIMENUITEM lpItem = (LPPUIMENUITEM)miiA.dwItemData;

                if (HIWORD64(lpItem) && PUI_OWNERDRAW_SIG == lpItem->dwSig && !(MFT_OWNERDRAW & lpItem->fType))
                {
                    if (lpItem->lpwz && lpItem->cch)
                        LocalFree(lpItem->lpwz);
                    LocalFree(lpItem);
                }
            }
        }
    }
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
DeleteMenuWrap(HMENU hMenu, UINT uPosition, UINT uFlags)
{
    VALIDATE_PROTOTYPEX(DeleteMenu);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();

    if (bOwnerDraw)
        DeleteOwnerDrawMenuItem(hMenu, uPosition, (MF_BYPOSITION & uFlags)? TRUE: FALSE);

    return DeleteMenu(hMenu, uPosition, uFlags);
}


// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
DestroyMenuWrap(HMENU hMenu)
{
    VALIDATE_PROTOTYPEX(DestroyMenu);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();

    if (bOwnerDraw)
        DeleteOwnerDrawMenu(hMenu);

    return DestroyMenu(hMenu);
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

LPBYTE MenuLoadMENUTemplates(LPBYTE lpMenuTemplate, HMENU *phMenu)
{
    HMENU hMenu;
    UINT menuFlags = 0;
    ULONG_PTR menuId = 0;
    LPWSTR lpmenuText;
    MENUITEMINFO mii;

    if (!(hMenu = CreateMenu()))
        goto memoryerror;

    do
    {
        // Get the menu flags.
        menuFlags = (UINT)(*(WORD *)lpMenuTemplate);
        lpMenuTemplate += 2;
        if (menuFlags & ~MF_VALID) {
            goto memoryerror;
        }

        if (!(menuFlags & MF_POPUP))
        {
            menuId = *(WORD *)lpMenuTemplate;
            lpMenuTemplate += 2;
        }

        lpmenuText = (LPWSTR)lpMenuTemplate;

        if (*lpmenuText)    // If a string exists, then skip to the end of it.
            lpMenuTemplate += lstrlenW(lpmenuText) * sizeof(WCHAR);
        else
            lpmenuText = NULL;

        // Skip over terminating NULL of the string (or the single NULL if empty string).
        lpMenuTemplate += sizeof(WCHAR);
        lpMenuTemplate = (BYTE *)(((ULONG_PTR)lpMenuTemplate + 1) & ~1);    // word align

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

        if (menuFlags & MF_POPUP)
        {
            mii.fMask |= MIIM_SUBMENU;
            lpMenuTemplate = MenuLoadMENUTemplates(lpMenuTemplate, (HMENU *)&menuId);
            if (!lpMenuTemplate)
                goto memoryerror;
            mii.hSubMenu = (HMENU)menuId;
        }

        // Don't allow bitmaps from the resource file.
        if (menuFlags & MF_BITMAP)
            menuFlags = (UINT)((menuFlags | MFT_RIGHTJUSTIFY) & ~MF_BITMAP);

        // We have to take out MFS_HILITE since that bit marks the end of a menu in
        // a resource file.  Since we shouldn't have any pre hilited items in the
        // menu anyway, this is no big deal.
        mii.fState = (menuFlags & MFS_OLDAPI_MASK) & ~MFS_HILITE;
        mii.fType = (menuFlags & MFT_OLDAPI_MASK);
        if (menuFlags & MFT_OWNERDRAW)
        {
            mii.fMask |= MIIM_DATA;
            mii.dwItemData = (ULONG_PTR) lpmenuText;
            lpmenuText = 0;
        }

        mii.dwTypeData = (LPTSTR)lpmenuText;
        mii.cch = (UINT)-1;
        mii.wID = (UINT)menuId;

        if (!InsertMenuItemWrapW(hMenu, 0xFFFFFFFF, TRUE, &mii))
        {
            if (menuFlags & MF_POPUP)
                DestroyMenuWrap(mii.hSubMenu);
            goto memoryerror;
        }
    } while (!(menuFlags & MF_END));

    *phMenu = hMenu;
    return lpMenuTemplate;

memoryerror:
    if (hMenu != NULL)
        DestroyMenuWrap(hMenu);
    *phMenu = NULL;
    return NULL;
}

PMENUITEMTEMPLATE2 MenuLoadMENUEXTemplates(PMENUITEMTEMPLATE2 lpMenuTemplate, HMENU *phMenu, WORD wResInfo)
{
    HMENU hMenu;
    HMENU hSubMenu;
    long menuId = 0;
    LPWSTR lpmenuText;
    MENUITEMINFO mii;
    UINT cch = 0;
    DWORD dwHelpID;

    if (!(hMenu = CreateMenu()))
        goto memoryerror;

    do
    {
        if (!(wResInfo & MFR_POPUP))
        {
            // If the PREVIOUS wResInfo field was not a POPUP, the
            // dwHelpID field is not there.  Back up so things fit.
            lpMenuTemplate = (PMENUITEMTEMPLATE2)(((LPBYTE)lpMenuTemplate) - sizeof(lpMenuTemplate->dwHelpID));
            dwHelpID = 0;
        }
        else
            dwHelpID = lpMenuTemplate->dwHelpID;

        menuId = lpMenuTemplate->menuId;

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

        mii.fType = lpMenuTemplate->fType;
        if (mii.fType & ~MFT_MASK)
            goto memoryerror;

        mii.fState  = lpMenuTemplate->fState;
        if (mii.fState & ~MFS_MASK)
            goto memoryerror;

        wResInfo = lpMenuTemplate->wResInfo;
        if (wResInfo & ~(MF_END | MFR_POPUP))
            goto memoryerror;

        if (dwHelpID)
            SetMenuContextHelpId(hMenu,dwHelpID);

        if (lpMenuTemplate->mtString[0])
            lpmenuText = lpMenuTemplate->mtString;
        else
            lpmenuText = NULL;

        cch = lstrlenW(lpmenuText);
        mii.dwTypeData = (LPTSTR) lpmenuText;

        // skip to next menu item template (DWORD boundary)
        lpMenuTemplate = (PMENUITEMTEMPLATE2)
                (((LPBYTE)lpMenuTemplate) +
                sizeof(MENUITEMTEMPLATE2) +
                ((cch * sizeof(WCHAR) + 3) & ~3));

        if (mii.fType & MFT_OWNERDRAW)
        {
            mii.fMask |= MIIM_DATA;
            mii.dwItemData = (ULONG_PTR) mii.dwTypeData;
            mii.dwTypeData = 0;
        }

        if (wResInfo & MFR_POPUP)
        {
            mii.fMask |= MIIM_SUBMENU;
            lpMenuTemplate = MenuLoadMENUEXTemplates(lpMenuTemplate, &hSubMenu, MFR_POPUP);
            if (lpMenuTemplate == NULL)
                goto memoryerror;
            mii.hSubMenu = hSubMenu;
        }

        // Don't allow bitmaps from the resource file.
        if (mii.fType & MFT_BITMAP)
            mii.fType = (mii.fType | MFT_RIGHTJUSTIFY) & ~MFT_BITMAP;

        mii.cch = (UINT)-1;
        mii.wID = menuId;
        if (!InsertMenuItemWrapW(hMenu, 0xFFFFFFFF, TRUE, &mii))
        {
            if (wResInfo & MFR_POPUP)
                DestroyMenuWrap(mii.hSubMenu);
            goto memoryerror;
        }
        wResInfo &= ~MFR_POPUP;
    } while (!(wResInfo & MFR_END));

    *phMenu = hMenu;
    return lpMenuTemplate;

memoryerror:
    if (hMenu != NULL)
        DestroyMenuWrap(hMenu);
    *phMenu = NULL;
    return NULL;
}

HMENU CreateMenuFromResource(LPBYTE lpMenuTemplate)
{
    HMENU hMenu = NULL;
    UINT menuTemplateVersion;
    UINT menuTemplateHeaderSize;

    // menu resource: First, strip version number word out of the menu
    // template.  This value should be 0 for MENU, 1 for MENUEX.
    menuTemplateVersion = *(WORD *)lpMenuTemplate;
    lpMenuTemplate += 2;
    if (menuTemplateVersion > 1)
        return NULL;

    menuTemplateHeaderSize = *(WORD *)lpMenuTemplate;
    lpMenuTemplate += 2;
    lpMenuTemplate += menuTemplateHeaderSize;
    switch (menuTemplateVersion)
    {
        case 0:
            MenuLoadMENUTemplates(lpMenuTemplate, &hMenu);
            break;

        case 1:
            MenuLoadMENUEXTemplates((PMENUITEMTEMPLATE2)lpMenuTemplate, &hMenu, 0);
            break;
    }
    return hMenu;
}

HMENU WINAPI
LoadMenuWrapW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
    VALIDATE_PROTOTYPE(LoadMenu);
    ASSERT(HIWORD64(lpMenuName) == 0);

    if (NeedMenuOwnerDraw() && MLIsMLHInstance(hInstance))
    {
        HRSRC hRes;

        if (hRes = FindResourceWrapW(hInstance, lpMenuName, RT_MENU))
        {
            LPBYTE lpMenuTemplate = (LPBYTE)LoadResource(hInstance, hRes);

            if (lpMenuTemplate)
            {
                HMENU hmenu = CreateMenuFromResource(lpMenuTemplate);

                if (hmenu)
                    return hmenu;
            }
        }
    }

    if (g_bRunningOnNT)
    {
        return LoadMenuW(hInstance, lpMenuName);
    }

    return LoadMenuA(hInstance, (LPCSTR) lpMenuName);
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

// BUGBUG: need to be improved
#define CHK_RADIO   L"o"
#define CHK_NORMAL  L"v"

void DrawMenuItem(LPDRAWITEMSTRUCT lpdis, LPRECT lprc)
{
    LPPUIMENUITEM lpItem = (LPPUIMENUITEM)lpdis->itemData;

    if (lpItem && lpItem->lpwz)
    {
        HDC hdc = lpdis->hDC;
        UINT uFormat = DT_VCENTER | DT_SINGLELINE | DT_LEFT | DT_EXPANDTABS;
        LPCWSTR lpwChkSym = CHK_NORMAL; 
        COLORREF clrTxt;
        int iBkMode;
        RECT rc;

        if (ODS_CHECKED & lpdis->itemState)
        {
            MENUITEMINFOA miiA;

            miiA.cbSize = sizeof(miiA);
            miiA.fMask = MIIM_TYPE;
            miiA.cch = 0;
            if (GetMenuItemInfoA(lpItem->hmenu, lpdis->itemID, FALSE, &miiA))
            {
                if (MFT_RADIOCHECK & miiA.fType)
                    lpwChkSym = CHK_RADIO; 
            }
        }

        iBkMode = SetBkMode(hdc, TRANSPARENT);

        if (ODS_GRAYED & lpdis->itemState)
        {
            clrTxt = SetTextColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));

            if (ODS_CHECKED & lpdis->itemState)
            {
                CopyRect(&rc, &(lpdis->rcItem));
                OffsetRect(&rc, 1, 1);
                DrawTextFLW(hdc, lpwChkSym, 1, &rc, uFormat);
            }
            CopyRect(&rc, lprc);
            OffsetRect(&rc, 1, 1);
            DrawTextFLW(hdc, lpItem->lpwz, lstrlenW(lpItem->lpwz), &rc, uFormat);

            SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
        }

        if (ODS_CHECKED & lpdis->itemState)
            DrawTextFLW(hdc, lpwChkSym, 1, &(lpdis->rcItem), uFormat);
        DrawTextFLW(hdc, lpItem->lpwz, lstrlenW(lpItem->lpwz), lprc, uFormat);

        if (ODS_GRAYED & lpdis->itemState)
            SetTextColor(hdc, clrTxt);
        SetBkMode(hdc, iBkMode);
    }
}

LRESULT OwnerDrawSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_DRAWITEM:
            if (0 == wParam)    // This is a menu
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                LPPUIMENUITEM lpItem = (LPPUIMENUITEM)lpdis->itemData;

                if (lpdis->CtlType == ODT_MENU
                    && HIWORD64(lpItem) && PUI_OWNERDRAW_SIG == lpItem->dwSig
                    && !(MFT_OWNERDRAW & lpItem->fType))
                {
                    int cyTemp;
                    RECT rc;
                    HDC hdc = lpdis->hDC;
                    HFONT hfont, hfontSav;
                    TEXTMETRICA tm;
                    NONCLIENTMETRICSA ncm;

                    ncm.cbSize = sizeof(ncm);
                    if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
                        break;

                    hfont = CreateFontIndirectA(&ncm.lfMenuFont);
                    if (NULL == hfont)
                        break;

                    hfontSav = (HFONT)SelectObject(hdc, hfont);

                    GetTextMetricsA(hdc, &tm);
                    CopyRect(&rc, &lpdis->rcItem);
                    rc.left += GetSystemMetrics(SM_CXMENUCHECK);
                    rc.top += tm.tmExternalLeading;

                    cyTemp = (lpdis->rcItem.bottom - lpdis->rcItem.top) - (tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYBORDER));
                    if (cyTemp > 0)
                        rc.top += (cyTemp / 2);

                    switch (lpdis->itemAction)
                    {
                        case ODA_DRAWENTIRE:
                            DrawMenuItem(lpdis, &rc);
                            break;

                        case ODA_FOCUS:
                            break;

                        case ODA_SELECT:
                        {
                            COLORREF clrTxt, clrBk, clrTxtSav, clrBkSav;

                            if (ODS_SELECTED & lpdis->itemState)
                            {
                                clrTxt = GetSysColor(COLOR_HIGHLIGHTTEXT);
                                clrBk = GetSysColor(COLOR_HIGHLIGHT);
                            }
                            else
                            {
                                clrTxt = GetSysColor(COLOR_MENUTEXT);
                                clrBk = GetSysColor(COLOR_MENU);
                            }
                            clrTxtSav = SetTextColor(hdc, clrTxt);
                            clrBkSav = SetBkColor(hdc, clrBk);
                            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lpdis->rcItem, TEXT(""), 0, NULL);
                            DrawMenuItem(lpdis, &rc);
                            SetTextColor(hdc, clrTxtSav);
                            SetBkColor(hdc, clrBkSav);
                            break;
                        }
                    }
                    SelectObject(hdc, hfontSav);
                    DeleteObject(hfont);
                    return TRUE;
                }
            }
            break;

        case WM_MEASUREITEM:
            if (0 == wParam)    // This is a menu
            {
                LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
                LPPUIMENUITEM lpItem = (LPPUIMENUITEM)lpmis->itemData;

                if (lpmis->CtlType == ODT_MENU
                    && HIWORD64(lpItem) && PUI_OWNERDRAW_SIG == lpItem->dwSig
                    && !(MFT_OWNERDRAW & lpItem->fType) && lpItem->lpwz)
                {
                    RECT rc;
                    HDC hdc;
                    HFONT hfont, hfontSav;
                    NONCLIENTMETRICSA ncm;

                    ncm.cbSize = sizeof(ncm);
                    if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
                        break;

                    hfont = CreateFontIndirectA(&ncm.lfMenuFont);
                    if (NULL == hfont)
                        break;

                    hdc = GetDC(hWnd);
                    hfontSav = (HFONT)SelectObject(hdc, hfont);
                    DrawTextFLW(hdc, lpItem->lpwz, lstrlenW(lpItem->lpwz), &rc, DT_SINGLELINE | DT_CALCRECT | DT_EXPANDTABS);
                    SelectObject(hdc, hfontSav);
                    DeleteObject(hfont);
                    ReleaseDC(hWnd, hdc);

                    lpmis->itemWidth = rc.right - rc.left + GetSystemMetrics(SM_CXMENUCHECK) + GetSystemMetrics(SM_CXEDGE) * 2;
                    lpmis->itemHeight = rc.bottom - rc.top + GetSystemMetrics(SM_CYEDGE) * 2;
                    return TRUE;
                }
            }
            break;

        case WM_MENUCHAR:
            // BUGBUG: need to be implemented
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

int WINAPI
GetMenuStringWrapW(
        HMENU   hMenu,
        UINT    uIDItem,
        LPWSTR  lpString,
        int     nMaxCount,
        UINT    uFlag)
{
    VALIDATE_PROTOTYPE(GetMenuString);
    VALIDATE_OUTBUF(lpString, nMaxCount);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();

    if (bOwnerDraw)
    {
        MENUITEMINFOA miiA;

        miiA.cbSize = sizeof(miiA);
        miiA.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;
        miiA.cch = 0;
        if (GetMenuItemInfoA(hMenu, uIDItem, (uFlag & MF_BYPOSITION)? TRUE: FALSE, &miiA))
        {
            if ((MIIM_TYPE & miiA.fMask) && (MFT_OWNERDRAW & miiA.fType))
            {
                LPPUIMENUITEM lpItem = (LPPUIMENUITEM)miiA.dwItemData;

                if (HIWORD64(lpItem) && PUI_OWNERDRAW_SIG == lpItem->dwSig && !(MFT_OWNERDRAW & lpItem->fType))
                {
                    if (lpItem->lpwz && lpItem->cch)
                    {
                        StrCpyNW(lpString, lpItem->lpwz, nMaxCount);
                        return min((UINT)(nMaxCount - 1), lpItem->cch);
                    }
                }
            }
        }
    }

    if (g_bRunningOnNT)
    {
        return GetMenuStringW(hMenu, uIDItem, lpString, nMaxCount, uFlag);
    }

    CStrOut str(lpString, nMaxCount);

    GetMenuStringA(hMenu, uIDItem, str, str.BufSize(), uFlag);
    return str.ConvertExcludingNul();
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
InsertMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR uIDNewItem,
        LPCWSTR lpNewItem)
{
    VALIDATE_PROTOTYPE(InsertMenu);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();

    if (bOwnerDraw)
    {
        if (!((MF_BITMAP | MF_OWNERDRAW) & uFlags)) // if MF_STRING is set. MF_STRING is 0
        {
            MENUITEMINFOW miiW;

            miiW.cbSize = sizeof(miiW);
            miiW.fMask = MIIM_TYPE | MIIM_STATE;
            miiW.fType = MFT_STRING;
            miiW.fState = uFlags & (MF_DEFAULT | MF_CHECKED | MF_UNCHECKED | MF_HILITE | MF_UNHILITE| MF_ENABLED | MF_DISABLED | MF_GRAYED);
            if (uFlags & (MF_GRAYED | MF_DISABLED))
                miiW.fState |= MFS_GRAYED;
            if (MF_POPUP & uFlags)
            {
                miiW.fMask |= MIIM_SUBMENU;
                miiW.hSubMenu = (HMENU)uIDNewItem;
            }
            else
            {
                miiW.fMask |= MIIM_ID;
                miiW.wID = (UINT)uIDNewItem;
            }
            miiW.dwTypeData = (LPWSTR)lpNewItem;
            miiW.cch = lstrlenW(lpNewItem);
            return InsertMenuItemWrapW(hMenu, uPosition, (MF_BYPOSITION & uFlags)? TRUE: FALSE, &miiW);
        }
    }

    if (g_bRunningOnNT)
    {
        return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
    }

    //
    //  You can't test for MFT_STRING because MFT_STRING is zero!
    //  So instead you have to check for everything *other* than
    //  a string.
    //
    //  The presence of any non-string menu type turns lpnewItem into
    //  an atom.
    //
    CStrIn str((uFlags & MFT_NONSTRING) ? CP_ATOM : CP_ACP, lpNewItem);

    return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL IsWindowOnCurrentThread(HWND hWnd)
{
    DWORD foo;

    if (!IsWindow(hWnd))
        // bail if the window is dead so we dont bogusly rip
        return TRUE;

    if (GetCurrentThreadId() != GetWindowThreadProcessId(hWnd, &foo))
        return FALSE;
    else
        return TRUE;
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
TrackPopupMenuWrap(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT *prcRect)
{
    VALIDATE_PROTOTYPEX(TrackPopupMenu);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();
    BOOL bIsWindowOnCurrentThread, bRet;

    if (bOwnerDraw && (bIsWindowOnCurrentThread = IsWindowOnCurrentThread(hWnd)))
        SetWindowSubclass(hWnd, OwnerDrawSubclassProc, 0, NULL);

    bRet = TrackPopupMenu(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);

    if (bOwnerDraw && bIsWindowOnCurrentThread)
        RemoveWindowSubclass(hWnd, OwnerDrawSubclassProc,  0);

    return bRet;
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
TrackPopupMenuExWrap(HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
    VALIDATE_PROTOTYPEX(TrackPopupMenuEx);

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();
    BOOL bIsWindowOnCurrentThread, bRet;

    if (bOwnerDraw && (bIsWindowOnCurrentThread = IsWindowOnCurrentThread(hWnd)))
        SetWindowSubclass(hWnd, OwnerDrawSubclassProc, 0, NULL);

    bRet = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
    
    if (bOwnerDraw && bIsWindowOnCurrentThread)
        RemoveWindowSubclass(hWnd, OwnerDrawSubclassProc,  0);

    return bRet;
}

#ifdef NEED_USER32_WRAPPER

int WINAPI
LoadStringWrapW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    VALIDATE_PROTOTYPE(LoadString);

    if (g_bRunningOnNT)
    {
        return LoadStringW(hInstance, uID, lpBuffer, nBufferMax);
    }

    //
    //  Do it manually.  The old code used to call LoadStringA and then
    //  convert it up to unicode, which is stupid since resources are
    //  physically already Unicode!  Just suck it out directly.
    //
    //  The old code was also buggy in the case where the loaded string
    //  contains embedded NULLs.
    //

    if (nBufferMax <= 0) return 0;                  // sanity check

    PWCHAR pwch;

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    HRSRC hrsrc;
    int cwch = 0;

    hrsrc = FindResourceA(hInstance, (LPSTR)(LONG_PTR)(1 + uID / 16), (LPSTR)RT_STRING);
    if (hrsrc) {
        pwch = (PWCHAR)LoadResource(hInstance, hrsrc);
        if (pwch) {
            /*
             *  Now skip over the strings in the resource until we
             *  hit the one we want.  Each entry is a counted string,
             *  just like Pascal.
             */
            for (uID %= 16; uID; uID--) {
                pwch += *pwch + 1;
            }
            cwch = min(*pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
        }
    }
    lpBuffer[cwch] = L'\0';                 /* Terminate the string */
    return cwch;
}

#endif // NEED_USER32_WRAPPER

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp1( WCHAR **ppch )
//
// purpose:     Stand-in for TransformCharWidth.  Used by the function
//              CompareStringString.
//
// returns:     Character at *ppch.  The value *ppch is incremented.
//
//----------------------------------------------------------------------

static WCHAR
TransformCharNoOp1( LPCWSTR *ppch, int )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharWidth( WCHAR **ppch, cchRemaining )
//
// purpose:     Converts halfwidth characters to fullwidth characters.
//              Also combines voiced (dakuon) and semi-voiced (handakuon)
//              characters.  *pch is advanced by one, unless there is a
//              (semi)voiced character, in which case it is advanced by
//              two characters.
//
//              Note that unlike the full widechar version, we do not
//              combine other characters, notably the combining Hiragana
//              characters (U+3099 and U+309A.)  This is to keep the
//              tables from getting unnecessarily large.
//
//              cchRemaining is passed so as to not include the voiced
//              marks if it's passed the end of the specified buffer.
//
// returns:     Full width character. *pch is incremented.
//
//----------------------------------------------------------------------

static WCHAR
TransformCharWidth( LPCWSTR *ppch, int cchRemaining )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    if (ch == 0x0020)
    {
        ch = 0x3000;
    }
    else if (ch == 0x005c)
    {
        // REVERSE SOLIDUS (aka BACKSLASH) maps to itself
    }
    else if (InRange(ch, 0x0021, 0x07e))
    {
        ch += 65248;
    }
    else if (InRange(ch, 0x00a2, 0x00af))
    {
        static const WCHAR achFull[] =
        {
            0xffe0, 0xffe1, 0x00a4, 0xffe5, 0xffe4, 0x00a7, 0x00a8, // 0xa2-0xa8
            0x00a9, 0x00aa, 0x00ab, 0xffe2, 0x00ad, 0x00ae, 0xffe3  // 0xa9-0xaf
        };

        ch = achFull[ch - 0x00a2];
    }
    else if (ch == 0x20a9) // WON SIGN
    {
        ch = 0xffe6;
    }
    else if (InRange(ch, 0xff61, 0xffdc))
    {
        WCHAR chNext = (cchRemaining > 1) ? **ppch : 0;

        if (chNext == 0xff9e && InRange(ch, 0xff73, 0xff8e))
        {
            if (cchRemaining != 1)
            {
                static const WCHAR achFull[] =
                {
/* 0xff73-0xff79 */  0xb0f4, 0x30a8, 0x30aa, 0xb0ac, 0xb0ae, 0xb0b0, 0xb0b2,
/* 0xff7a-0xff80 */  0xb0b4, 0xb0b6, 0xb0b8, 0xb0ba, 0xb0bc, 0xb0be, 0xb0c0,
/* 0xff81-0xff87 */  0xb0c2, 0xb0c5, 0xb0c7, 0xb0c9, 0x30ca, 0x30cb, 0x30cc,
/* 0xff88-0xff8e */  0x30cd, 0x30ce, 0xb0d0, 0xb0d3, 0xb0d6, 0xb0d9, 0xb0dc
                };

                // HALFWIDTH KATAKANA VOICED SOUND MARK

                WCHAR chTemp = achFull[ch - 0xff73];

                // Some in the range absorb the sound mark.
                // These are indicated by the set high-bit.

                ch = chTemp & 0x7fff;

                if (chTemp & 0x8000)
                {
                    (*ppch)++;
                }
            }
        }
        else if (chNext == 0xff9f && InRange(ch, 0xff8a, 0xff8e))
        {
            // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK

            ch = 0x30d1 + (ch - 0xff8a) * 3;
            (*ppch)++;
        }
        else
        {
            static const WCHAR achMapFullFFxx[] =
            {
                0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,  // 0xff61-0xff67
                0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7,  // 0xff68-0xff6e
                0x30c3, 0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa,  // 0xff6f-0xff75
                0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7,  // 0xff76-0xff7c
                0x30b9, 0x30bb, 0x30bd, 0x30bf, 0x30c1, 0x30c4, 0x30c6,  // 0xff7d-0xff83
                0x30c8, 0x30ca, 0x30cb, 0x30cc, 0x30cd, 0x30ce, 0x30cf,  // 0xff84-0xff8a
                0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, 0x30df, 0x30e0,  // 0xff8b-0xff91
                0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, 0x30ea,  // 0xff92-0xff98
                0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,  // 0xff99-0xff9f
                0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136,  // 0xffa0-0xffa6
                0x3137, 0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d,  // 0xffa7-0xffad
                0x313e, 0x313f, 0x3140, 0x3141, 0x3142, 0x3143, 0x3144,  // 0xffae-0xffb4
                0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b,  // 0xffb5-0xffbb
                0x314c, 0x314d, 0x314e, 0xffbf, 0xffc0, 0xffc1, 0x314f,  // 0xffbc-0xffc2
                0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0xffc8, 0xffc9,  // 0xffc3-0xffc9
                0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a, 0xffd0,  // 0xffca-0xffd0
                0xffd1, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,  // 0xffd1-0xffd7
                0xffd8, 0xffd9, 0x3161, 0x3162, 0x3163                   // 0xffd8-0xffac
            };

            ch = achMapFullFFxx[ch - 0xff61];
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharNoOp2( WCHAR ch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Original character
//
//----------------------------------------------------------------------

static WCHAR
TransformCharNoOp2( WCHAR ch )
{
    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharKana( WCHAR ch )
//
// purpose:     Converts Hiragana characters to Katakana characters
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

static WCHAR
TransformCharKana( WCHAR ch )
{
    if (((ch & 0xff00) == 0x3000) &&
        (InRange(ch, 0x3041, 0x3094) || InRange(ch, 0x309d, 0x309e)))
    {
        ch += 0x060;
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp3( LPWSTR pch, DWORD cch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Character count (cch).
//
//----------------------------------------------------------------------

static DWORD
TransformCharNoOp3( LPWSTR, DWORD cch )
{
    return cch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharFinal( WCHAR ch )
//
// purpose:     Converts "final" forms to regular forms
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

// BUGBUG (cthrash) We do not fold Presentation Forms (Alphabetic or Arabic)

static WCHAR
TransformCharFinal( WCHAR ch )
{
    WCHAR chRet = ch;

    if (ch >= 0x3c2)                    // short-circuit ASCII +
    {
        switch (ch)
        {
            case 0x03c2:                // GREEK SMALL LETTER FINAL SIGMA
            case 0x05da:                // HEBREW LETTER FINAL KAF
            case 0x05dd:                // HEBREW LETTER FINAL MEM
            case 0x05df:                // HEBREW LETTER FINAL NUN
            case 0x05e3:                // HEBREW LETTER FINAL PE
            case 0x05e5:                // HEBREW LETTER FINAL TSADI
            case 0xfb26:                // HEBREW LETTER WIDE FINAL MEM
            case 0xfb3a:                // HEBREW LETTER FINAL KAF WITH DAGESH
            case 0xfb43:                // HEBREW LETTER FINAL PE WITH DAGESH
                chRet++;
                break;
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    CompareStringString( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the string comparsion for CompareStringWrap.
//              We can convert each character to (1) fullwidth,
//              (2) Katakana, and (3) lowercase, as necessary.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

static int
CompareStringString(
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    int nRet = 0;
    WCHAR wchIgnoreNulA = cchA == -1 ? 0 : -1;
    WCHAR wchIgnoreNulB = cchB == -1 ? 0 : -1;
    WCHAR (*pfnTransformWidth)(LPCWSTR *, int);
    WCHAR (*pfnTransformKana)(WCHAR);
    DWORD (*pfnTransformLower)(LPWSTR, DWORD);
    WCHAR (*pfnTransformFinal)(WCHAR);


    pfnTransformWidth = (dwFlags & NORM_IGNOREWIDTH)
                        ? TransformCharWidth : TransformCharNoOp1;
    pfnTransformKana  = (dwFlags & NORM_IGNOREKANATYPE)
                        ? TransformCharKana : TransformCharNoOp2;
    pfnTransformLower = (dwFlags & NORM_IGNORECASE)
                        ? CharLowerBuffWrap : TransformCharNoOp3;
    pfnTransformFinal = (dwFlags & NORM_IGNORECASE)
                        ? TransformCharFinal : TransformCharNoOp2;

    while (   !nRet
           && cchA
           && cchB
           && (*lpA | wchIgnoreNulA)
           && (*lpB | wchIgnoreNulB) )
    {
        WCHAR chA, chB;
        LPCWSTR lpAOld = lpA;
        LPCWSTR lpBOld = lpB;

        chA = (*pfnTransformWidth)(&lpA, cchA);
        chA = (*pfnTransformKana)(chA);
        (*pfnTransformLower)(&chA, 1);
        chA = (*pfnTransformFinal)(chA);

        chB = (*pfnTransformWidth)(&lpB, cchB);
        chB = (*pfnTransformKana)(chB);
        (*pfnTransformLower)(&chB, 1);
        chB = (*pfnTransformFinal)(chB);

        nRet = (int)chA - (int)chB;
        cchA -= (int) (lpA - lpAOld);
        cchB -= (int) (lpB - lpBOld);
    }

    if (!nRet)
    {
        nRet = cchA - cchB;
    }

    if (nRet)
    {
        nRet = nRet > 0 ? 1 : -1;
    }

    return nRet + 2;
}

//----------------------------------------------------------------------
//
// function:    CompareStringWord( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the word comparsion for CompareStringWrap.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

static int
CompareStringWord(
    LCID    lcid,
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    // BUGBUG (cthrash) We won't properly support word compare for the
    // time being.  Do the same old CP_ACP trick, which should cover
    // enough cases.

    // fail if either string is NULL, as it causes assert on debug windows
    if (!lpA || !lpB)
        return 0;

    CStrIn strA(lpA, cchA);
    CStrIn strB(lpB, cchB);

    cchA = strA.strlen();
    cchB = strB.strlen();

    return CompareStringA(lcid, dwFlags, strA, cchA, strB, cchB);
}

//----------------------------------------------------------------------
//
// function:    CompareStringWrapW( ... )
//
// purpose:     Unicode wrapper of CompareString for Win95.
//
//              Note not all bits in dwFlags are honored; specifically,
//              since we don't do a true widechar word compare, we
//              won't properly handle NORM_IGNORENONSPACE or
//              NORM_IGNORESYMBOLS for arbitrary widechar strings.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

LWSTDAPI_(int)
CompareStringAltW(
    LCID    lcid,
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    if (g_bRunningOnNT)
    {
        return CompareStringW(lcid, dwFlags, lpA, cchA, lpB, cchB);
    }

    int nRet;

    if (dwFlags & SORT_STRINGSORT)
    {
        nRet = CompareStringString(dwFlags, lpA, cchA, lpB, cchB);
    }
    else
    {
        nRet = CompareStringWord(lcid, dwFlags, lpA, cchA, lpB, cchB);
    }

    return nRet;
}

#ifdef NEED_KERNEL32_WRAPPER

int StrLenN(LPCWSTR psz, int cchMin)
{
    LPCWSTR pszEnd = psz;

    while (((pszEnd - psz) < cchMin) && *pszEnd)
    {
        pszEnd++;
    }
    return pszEnd - psz;
}

int WINAPI CompareStringWrapW(LCID Locale, DWORD dwFlags,
                              LPCWSTR psz1, int cch1,
                              LPCWSTR psz2, int cch2)
{
    VALIDATE_PROTOTYPE(CompareString);

    if (g_bRunningOnNT)
    {
        return CompareStringW(Locale, dwFlags, psz1, cch1, psz2, cch2);
    }
    else if (psz1 && psz2)
    {
        if (dwFlags & NORM_STOP_ON_NULL)
        {
            cch1 = StrLenN(psz1, cch1);
            cch2 = StrLenN(psz2, cch2);
            dwFlags &= ~NORM_STOP_ON_NULL;
        }

        CStrIn strString1(psz1, cch1);
        CStrIn strString2(psz2, cch2);

        cch1 = strString1.strlen();
        cch2 = strString2.strlen();

        return CompareStringA(Locale, dwFlags, strString1, cch1, strString2, cch2);
    }
    else
        return 0;   // fail if either string is NULL, as it causes assert on debug windows
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

#ifndef UNIX
BOOL WINAPI
MessageBoxIndirectWrapW(CONST MSGBOXPARAMS *pmbp)
#else
int WINAPI
MessageBoxIndirectWrapW(LPMSGBOXPARAMS pmbp)
#endif /* UNIX */
{
    VALIDATE_PROTOTYPE(MessageBoxIndirect);
    ASSERT(HIWORD64(pmbp->lpszIcon) == 0);

    if (g_bRunningOnNT)
    {
        return MessageBoxIndirectW(pmbp);
    }

    CStrIn        strText(pmbp->lpszText);
    CStrIn        strCaption(pmbp->lpszCaption);
    MSGBOXPARAMSA mbp;

    memcpy(&mbp, pmbp, sizeof(mbp));
    mbp.lpszText = strText;
    mbp.lpszCaption = strCaption;

    return MessageBoxIndirectA(&mbp);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

DWORD GetCharacterPlacementWrapW(
    HDC hdc,            // handle to device context
    LPCTSTR lpString,   // pointer to string
    int nCount,         // number of characters in string
    int nMaxExtent,     // maximum extent for displayed string
    LPGCP_RESULTS lpResults, // pointer to buffer for placement result
    DWORD dwFlags       // placement flags
   )
{
    VALIDATE_PROTOTYPE(GetCharacterPlacement);
    // Leave for someone else.
    ASSERT (lpResults->lpOutString == NULL);
    ASSERT (lpResults->lpClass == NULL);

    if (g_bRunningOnNT)
    {
        return GetCharacterPlacementW (hdc,
                                       lpString,
                                       nCount,
                                       nMaxExtent,
                                       lpResults,
                                       dwFlags);
    }

    CStrIn strText(lpString);
    DWORD dwRet;

    dwRet = GetCharacterPlacementA (hdc, strText, nCount, nMaxExtent,
                                    (LPGCP_RESULTSA)lpResults,
                                    dwFlags);
    return dwRet;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

//
// Note that we're calling get GetCharWidthA instead of GetCharWidth32A
// because the 32 version doesn't exist under Win95.
BOOL WINAPI GetCharWidth32WrapW(
     HDC hdc,
     UINT iFirstChar,
     UINT iLastChar,
     LPINT lpBuffer)
{
    VALIDATE_PROTOTYPE(GetCharWidth32);

    if (g_bRunningOnNT)
    {
         return GetCharWidth32W (hdc, iFirstChar, iLastChar, lpBuffer);
    }

    // Note that we expect to do only one character at a time for anything but
    // ISO Latin 1.
    if (iFirstChar > 255)
    {
        UINT mbChar=0;
        WCHAR ch;

        // Convert string
        ch = (WCHAR)iFirstChar;
        WideCharToMultiByte(CP_ACP, 0, &ch, 1,
                            (char *)&mbChar, 2, NULL, NULL);
    }

    return (GetCharWidthA (hdc, iFirstChar, iLastChar, lpBuffer));
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

//
//  Note:  Win95 does support ExtTextOutW.  This thunk is not for
//  ANSI/UNICODE wrapping.  It's to work around an ISV app bug.
//
//  Y'see, there's an app that patches Win95 GDI and their ExtTextOutW handler
//  is broken.  It always dereferences the lpStr parameter, even if
//  cch is zero.  Consequently, any time we are about to pass NULL as
//  the lpStr, we have to change our mind and pass a null UNICODE string
//  instead.
//
//  The name of this app:  Lotus SmartSuite ScreenCam 97.
//
LWSTDAPI_(BOOL)
ExtTextOutWrapW(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpStr, UINT cch, CONST INT *lpDx)
{
    VALIDATE_PROTOTYPE(ExtTextOut);
    if (lpStr == NULL)              // Stupid workaround
        lpStr = L"";                // for ScreenCam 97

    if (_MayNeedFontLinking(lpStr, cch))
    {
        return ExtTextOutFLW(hdc, x, y, fuOptions, lprc, lpStr, cch, lpDx);
    }

    return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpStr, cch, lpDx);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
ModifyMenuWrapW(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT_PTR uIDNewItem,
        LPCWSTR lpNewItem)
{
    VALIDATE_PROTOTYPE(ModifyMenu);
    ASSERT(!(uFlags & MF_BITMAP) && !(uFlags & MF_OWNERDRAW));

    if (g_bRunningOnNT)
    {
        return ModifyMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
    }

    CStrIn  str(lpNewItem);

    return ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
CopyFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
    VALIDATE_PROTOTYPE(CopyFile);

    if (g_bRunningOnNT)
    {
        return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
    }

    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return CopyFileA(strOld, strNew, bFailIfExists);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
MoveFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    VALIDATE_PROTOTYPE(MoveFile);

    if (g_bRunningOnNT)
    {
        return MoveFileW(lpExistingFileName, lpNewFileName);
    }

    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return MoveFileA(strOld, strNew);
}


#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
OemToCharWrapW(LPCSTR lpszSrc, LPWSTR lpszDst)
{
    VALIDATE_PROTOTYPE(OemToChar);
    VALIDATE_OUTBUF(lpszDst, lstrlenA(lpszSrc));

    if (g_bRunningOnNT)
    {
        return OemToCharW(lpszSrc, lpszDst);
    }

    CStrOut strDst(lpszDst, lstrlenA(lpszSrc));

    return OemToCharA(lpszSrc, strDst);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
OpenEventWrapW(
        DWORD                   fdwAccess,
        BOOL                    fInherit,
        LPCWSTR                 lpszEventName)
{
    VALIDATE_PROTOTYPE(OpenEvent);

    if (g_bRunningOnNT)
    {
        return OpenEventW(fdwAccess, fInherit, lpszEventName);
    }

    CStrIn strEventName(lpszEventName);

    return OpenEventA(fdwAccess, fInherit, strEventName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

VOID WINAPI
OutputDebugStringWrapW(LPCWSTR lpOutputString)
{
    VALIDATE_PROTOTYPE(OutputDebugString);

    if (g_bRunningOnNT)
    {
        OutputDebugStringW(lpOutputString);
        return;
    }

    CStrIn  str(lpOutputString);

    OutputDebugStringA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PeekMessageWrapW(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax,
        UINT    wRemoveMsg)
{
    VALIDATE_PROTOTYPE(PeekMessage);

    FORWARD_AW(PeekMessage, (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_WINMM_WRAPPER

LWSTDAPI_(BOOL)
PlaySoundWrapW(
        LPCWSTR pszSound,
        HMODULE hmod,
        DWORD fdwSound)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(PlaySound, _PlaySound);

    if (g_bRunningOnNT)
    {
        return _PlaySoundW(pszSound, hmod, fdwSound);
    }

    CStrIn strSound(pszSound);

    return _PlaySoundA(strSound, hmod, fdwSound);
}

#endif // NEED_WINMM_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PostMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(PostMessage);

    FORWARD_AW(PostMessage, (hWnd, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PostThreadMessageWrapW(
        DWORD idThread,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam)
{
    VALIDATE_PROTOTYPE(PostThreadMessage);

    FORWARD_AW(PostThreadMessage, (idThread, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegCreateKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
    VALIDATE_PROTOTYPE(RegCreateKey);

    if (g_bRunningOnNT)
    {
        return RegCreateKeyW(hKey, lpSubKey, phkResult);
    }

    CStrIn  str(lpSubKey);

    return RegCreateKeyA(hKey, str, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegCreateKeyExWrapW(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    VALIDATE_PROTOTYPE(RegCreateKeyEx);

    if (g_bRunningOnNT)
    {
        return RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes,  phkResult, lpdwDisposition);
    }

    CStrIn strSubKey(lpSubKey);
    CStrIn strClass(lpClass);

    return RegCreateKeyExA(hKey, strSubKey, Reserved, strClass, dwOptions, samDesired, lpSecurityAttributes,  phkResult, lpdwDisposition);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

//
//  Subtle difference:  RegDeleteKey on Win9x will recursively delete subkeys.
//  On NT, it fails if the key being deleted has subkeys.  If you need to
//  force NT-style behavior, use SHDeleteEmptyKey.  To force 95-style
//  behavior, use SHDeleteKey.
//
LONG APIENTRY
RegDeleteKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey)
{
    VALIDATE_PROTOTYPE(RegDeleteKey);

    if (g_bRunningOnNT)
    {
        return RegDeleteKeyW(hKey, pwszSubKey);
    }

    CStrIn  str(pwszSubKey);

    return RegDeleteKeyA(hKey, str);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegDeleteValueWrapW(HKEY hKey, LPCWSTR pwszSubKey)
{
    VALIDATE_PROTOTYPE(RegDeleteValue);

    if (g_bRunningOnNT)
    {
        return RegDeleteValueW(hKey, pwszSubKey);
    }

    CStrIn  str(pwszSubKey);

    return RegDeleteValueA(hKey, str);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegEnumKeyWrapW(
        HKEY    hKey,
        DWORD   dwIndex,
        LPWSTR  lpName,
        DWORD   cbName)
{
    VALIDATE_PROTOTYPE(RegEnumKey);
    VALIDATE_OUTBUF(lpName, cbName);

    if (g_bRunningOnNT)
    {
        return RegEnumKeyW(hKey, dwIndex, lpName, cbName);
    }

    CStrOut str(lpName, cbName);

    return RegEnumKeyA(hKey, dwIndex, str, str.BufSize());
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegEnumKeyExWrapW(
        HKEY        hKey,
        DWORD       dwIndex,
        LPWSTR      lpName,
        LPDWORD     lpcbName,
        LPDWORD     lpReserved,
        LPWSTR      lpClass,
        LPDWORD     lpcbClass,
        PFILETIME   lpftLastWriteTime)
{
    VALIDATE_PROTOTYPE(RegEnumKeyEx);
    if (lpcbName) {VALIDATE_OUTBUF(lpName, *lpcbName);}
    if (lpcbClass) {VALIDATE_OUTBUF(lpClass, *lpcbClass);}

    if (g_bRunningOnNT)
    {
        return RegEnumKeyExW(
            hKey,
            dwIndex,
            lpName,
            lpcbName,
            lpReserved,
            lpClass,
            lpcbClass,
            lpftLastWriteTime);
    }

    long    ret;
    DWORD   dwClass = 0;

    if (!lpcbClass)
    {
        lpcbClass = &dwClass;
    }

    CStrOut strName(lpName, *lpcbName);
    CStrOut strClass(lpClass, *lpcbClass);

    ret = RegEnumKeyExA(
            hKey,
            dwIndex,
            strName,
            lpcbName,
            lpReserved,
            strClass,
            lpcbClass,
            lpftLastWriteTime);

    *lpcbName = strName.ConvertExcludingNul();
    *lpcbClass = strClass.ConvertExcludingNul();

    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegOpenKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    VALIDATE_PROTOTYPE(RegOpenKey);

    if (g_bRunningOnNT)
    {
        return RegOpenKeyW(hKey, pwszSubKey, phkResult);
    }

    CStrIn  str(pwszSubKey);

    return RegOpenKeyA(hKey, str, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegOpenKeyExWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   ulOptions,
        REGSAM  samDesired,
        PHKEY   phkResult)
{
    VALIDATE_PROTOTYPE(RegOpenKeyEx);

    if (g_bRunningOnNT)
    {
        return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    CStrIn  str(lpSubKey);

    return RegOpenKeyExA(hKey, str, ulOptions, samDesired, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryInfoKeyWrapW(
        HKEY hKey,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        LPDWORD lpReserved,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcbMaxClassLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime)
{
    VALIDATE_PROTOTYPE(RegQueryInfoKey);

    if (g_bRunningOnNT)
    {
            return RegQueryInfoKeyW(
                hKey,
                lpClass,
                lpcbClass,
                lpReserved,
                lpcSubKeys,
                lpcbMaxSubKeyLen,
                lpcbMaxClassLen,
                lpcValues,
                lpcbMaxValueNameLen,
                lpcbMaxValueLen,
                lpcbSecurityDescriptor,
                lpftLastWriteTime);

    }

    CStrIn  str(lpClass);

    return RegQueryInfoKeyA(
                hKey,
                str,
                lpcbClass,
                lpReserved,
                lpcSubKeys,
                lpcbMaxSubKeyLen,
                lpcbMaxClassLen,
                lpcValues,
                lpcbMaxValueNameLen,
                lpcbMaxValueLen,
                lpcbSecurityDescriptor,
                lpftLastWriteTime);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryValueWrapW(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        LPWSTR  pwszValue,
        PLONG   lpcbValue)
{
    VALIDATE_PROTOTYPE(RegQueryValue);
    if (lpcbValue) {VALIDATE_OUTBUF(pwszValue, *lpcbValue);}

    if (g_bRunningOnNT)
    {
        return RegQueryValueW(hKey, pwszSubKey, pwszValue, lpcbValue);
    }

    long    ret;
    long    cb;
    CStrIn  strKey(pwszSubKey);
    CStrOut strValue(pwszValue, (lpcbValue ? ((*lpcbValue) / sizeof(WCHAR)) : 0));

    cb = strValue.BufSize();
    ret = RegQueryValueA(hKey, strKey, strValue, (lpcbValue ? &cb : NULL));
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    if (strValue)
    {
        cb = strValue.ConvertIncludingNul();
    }

    if (lpcbValue)
        *lpcbValue = cb * sizeof(WCHAR);

Cleanup:
    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryValueExWrapW(
        HKEY    hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData)
{
    VALIDATE_PROTOTYPE(RegQueryValueEx);
    if (lpcbData) {VALIDATE_OUTBUF(lpData, *lpcbData);}

    LONG    ret;
    DWORD   dwTempType;

    if (g_bRunningOnNT)
    {
#ifdef DEBUG
        if (lpType == NULL)
            lpType = &dwTempType;
#endif
        ret = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

        if (ret == ERROR_SUCCESS)
        {
            // The Win9x wrapper does not support REG_MULTI_SZ, so it had
            // better not be one
            ASSERT(*lpType != REG_MULTI_SZ);
        }

        return ret;
    }

    CStrIn  strValueName(lpValueName);
    DWORD   cb;

    //
    // Determine the type of buffer needed
    //

    ret = RegQueryValueExA(hKey, strValueName, lpReserved, &dwTempType, NULL, (lpcbData ? &cb : NULL));
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    ASSERT(dwTempType != REG_MULTI_SZ);

    switch (dwTempType)
    {
    case REG_EXPAND_SZ:
    case REG_SZ:
        {
            CStrOut strData((LPWSTR) lpData, (lpcbData ? ((*lpcbData) / sizeof(WCHAR)) : 0));

            cb = strData.BufSize();
            ret = RegQueryValueExA(hKey, strValueName, lpReserved, lpType, (LPBYTE)(LPSTR)strData, (lpcbData ? &cb : NULL));
            if (ret != ERROR_SUCCESS)
                break;

            if (strData)
            {
                cb = strData.ConvertIncludingNul();
            }

            if (lpcbData)
                *lpcbData = cb * sizeof(WCHAR);
            break;
        }

    default:
        {
            ret = RegQueryValueExA(
                    hKey,
                    strValueName,
                    lpReserved,
                    lpType,
                    lpData,
                    lpcbData);

            break;
        }
    }

Cleanup:
    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegSetValueWrapW(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   dwType,
        LPCWSTR lpData,
        DWORD   cbData)
{
    VALIDATE_PROTOTYPE(RegSetValue);

    if (g_bRunningOnNT)
    {
        return RegSetValueW(hKey, lpSubKey, dwType, lpData, cbData);
    }

    CStrIn  strKey(lpSubKey);
    CStrIn  strValue(lpData);

    return RegSetValueA(hKey, strKey, dwType, strValue, cbData);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegSetValueExWrapW(
        HKEY        hKey,
        LPCWSTR     lpValueName,
        DWORD       Reserved,
        DWORD       dwType,
        CONST BYTE* lpData,
        DWORD       cbData)
{
    VALIDATE_PROTOTYPE(RegSetValueEx);
    ASSERT(dwType != REG_MULTI_SZ);

    if (g_bRunningOnNT)
    {
        return RegSetValueExW(
            hKey,
            lpValueName,
            Reserved,
            dwType,
            lpData,
            cbData);
    }


    CStrIn      strKey(lpValueName);
    CStrIn      strSZ((dwType == REG_SZ || dwType == REG_EXPAND_SZ) ? (LPCWSTR) lpData : NULL);

    if (strSZ)
    {
        lpData = (LPBYTE) (LPSTR) strSZ;
        cbData = strSZ.strlen() + 1;
    }

    return RegSetValueExA(
            hKey,
            strKey,
            Reserved,
            dwType,
            lpData,
            cbData);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

ATOM WINAPI
RegisterClassWrapW(CONST WNDCLASSW * lpWndClass)
{
    VALIDATE_PROTOTYPE(RegisterClass);

    if (g_bRunningOnNT)
    {
        return RegisterClassW(lpWndClass);
    }

    WNDCLASSA   wc;
    CStrIn      strMenuName(lpWndClass->lpszMenuName);
    CStrIn      strClassName(lpWndClass->lpszClassName);

    ASSERT(sizeof(wc) == sizeof(*lpWndClass));
    memcpy(&wc, lpWndClass, sizeof(wc));

    wc.lpszMenuName = strMenuName;
    wc.lpszClassName = strClassName;

    return RegisterClassA(&wc);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
RegisterClipboardFormatWrapW(LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RegisterClipboardFormat);

    if (g_bRunningOnNT)
    {
        return RegisterClipboardFormatW(lpString);
    }

    CStrIn  str(lpString);

    return RegisterClipboardFormatA(str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
RegisterWindowMessageWrapW(LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RegisterWindowMessage);

    if (g_bRunningOnNT)
    {
        return RegisterWindowMessageW(lpString);
    }

    CStrIn  str(lpString);

    return RegisterWindowMessageA(str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
RemoveDirectoryWrapW(LPCWSTR lpszDir)
{
    VALIDATE_PROTOTYPE(RemoveDirectory);

    if (g_bRunningOnNT)
    {
        return RemoveDirectoryW(lpszDir);
    }

    CStrIn  strDir(lpszDir);

    return RemoveDirectoryA(strDir);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HANDLE WINAPI
RemovePropWrapW(
        HWND    hWnd,
        LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RemoveProp);

    if (g_bRunningOnNT)
    {
        return RemovePropW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return RemovePropA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT WINAPI SendMessageWrapW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//  NOTE (SumitC) Instead of calling SendDlgItemMessageA below, I'm forwarding to
//       SendMessageWrap so as not to have to re-do the special-case processing.
LRESULT WINAPI
SendDlgItemMessageWrapW(
        HWND    hDlg,
        int     nIDDlgItem,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(SendDlgItemMessage);

    if (g_bRunningOnNT)
    {
        return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
    }

    HWND hWnd;

    hWnd = GetDlgItem(hDlg, nIDDlgItem);

    return SendMessageWrapW(hWnd, Msg, wParam, lParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int CharIndexToByteIndex(LPCSTR psz, int cch)
{
    if (cch <= 0)
        return cch;

    LPCSTR pszTemp = psz;
    while (*pszTemp && cch-- > 0)
    {
        pszTemp = CharNextA(pszTemp);
    }

    return (int)(pszTemp - psz);
}

int ByteIndexToCharIndex(LPCSTR psz, int cb)
{
    if (cb <=0)
        return cb;

    LPCSTR pszTemp = psz;
    LPCSTR pszEnd = &psz[cb];
    int cch = 0;

    while (*pszTemp && pszTemp < pszEnd)
    {
        pszTemp = CharNextA(pszTemp);
        ++cch;
    }

    return cch;
}

//
// Translate between byte positions and "character" positions
//
void TranslateCharPos(HWND hwnd, BOOL fByteIndexToCharIndex, DWORD* pdwPos, int cPos)
{
    int cch = GetWindowTextLengthA(hwnd);
    if (cch > 0)
    {
        char szBuf[MAX_PATH];
        LPSTR pszBuf = szBuf;

        if (cch >= ARRAYSIZE(szBuf))
        {
            pszBuf = new char[cch + 1];
        }

        if (pszBuf)
        {
            GetWindowTextA(hwnd, (LPSTR)pszBuf, cch+1);

            // Translate Each value passed in
            while (cPos--)
            {
                if (fByteIndexToCharIndex)
                {
                    *pdwPos = (DWORD)ByteIndexToCharIndex(pszBuf, (int)*pdwPos);
                }
                else
                {
                    *pdwPos = (DWORD)CharIndexToByteIndex(pszBuf, (int)*pdwPos);
                }
                ++pdwPos;
            }

            if (pszBuf != szBuf)
            {
                delete [] pszBuf;
            }
        }
    }
}

//
//  There is no WM_GETOBJECT/OBJID_QUERYCLASSNAMEIDX code for comboex,
//  so we will just send CCM_GETUNICODEFORMAT and hope for the best.
//  We are relying on the fact that Win95's combo box returns 0 in response
//  to messages it doesn't understand (like CCM_GETUNICODEFORMAT).
//
#define IsUnicodeComboEx(hWnd) SendMessageA(hWnd, CCM_GETUNICODEFORMAT, 0, 0)

#ifdef DEBUG
int g_cSMTot, g_cSMHit;
int g_cSMMod = 100;
#endif

LRESULT WINAPI
SendMessageAThunk(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
#ifdef DEBUG
    if ((g_cSMTot % g_cSMMod) == 0)
        TraceMsg(DM_PERF, "sm: tot=%d hit=%d", g_cSMTot, g_cSMHit);
#endif
    DBEXEC(TRUE, g_cSMTot++);
    // todo: perf? seems to be pretty common case, at least for now...
    DBEXEC(Msg > WM_USER, g_cSMHit++);
#if 0
    if (Msg > WM_USER)
        goto Ldef;
#endif

    switch (Msg)
    {
        case EM_GETLINE:
        {
            LRESULT nLen;

            CStrOut str((LPWSTR) lParam, (* (SHORT *) lParam) + 1);
            * (SHORT *) (LPSTR) str = * (SHORT *) lParam;
            nLen = SendMessageA(hWnd, Msg, (WPARAM) wParam, (LPARAM) (LPSTR) str);
        
            if(nLen > 0)
                ((LPSTR) str)[nLen] = '\0';

            return nLen;
        }

        case EM_SETPASSWORDCHAR:
        {
            WPARAM  wp;

            ASSERT(HIWORD64(wParam) == 0);
            SHUnicodeToAnsi((LPWSTR) &wParam, (LPSTR) &wp, sizeof(wp));
            ASSERT(HIWORD64(wp) == 0);

            return SendMessageA(hWnd, Msg, wp, lParam);
        }

        case EM_SETWORDBREAKPROC:
        {
            // There is a bug with how USER handles WH_CALLWNDPROC global hooks in Win95/98 that
            // causes us to blow up if one is installed and a wordbreakproc is set.  Thus,
            // if an app is running that has one of these hooks installed (intellipoint 1.1 etc.) then
            // if we install our wordbreakproc the app will fault when the proc is called.  There
            // does not appear to be any way for us to work around it since USER's thunking code
            // trashes the stack so this API is disabled for Win9x.
            TraceMsg(TF_WARNING, "EM_SETWORDBREAKPROC can fault on Win9x; see unicwrap.cpp for details");
            return FALSE;
        }

        case EM_GETSEL:
        {
            //
            // Convert multibyte indicies into unicode indicies 
            //
            DWORD dw[2];
            SendMessageA(hWnd, Msg, (WPARAM)&dw[0], (LPARAM)&dw[1]);
            TranslateCharPos(hWnd, TRUE, dw, ARRAYSIZE(dw));

            // Return the results
            DWORD* pdwStart = (DWORD*)wParam;
            DWORD* pdwEnd   = (DWORD*)lParam;
            if (pdwStart)
            {
                *pdwStart = dw[0];
            }
            if (pdwEnd) 
            {
                *pdwEnd = dw[1];
            }

            LRESULT lr = (LRESULT)-1;

            if (dw[0] <= 0xffff && dw[1] <= 0xffff)
            {
                lr = MAKELONG(dw[0], dw[1]);
            }
            return lr;
        }

        case EM_SETSEL:
        {
            //
            // Convert unicode char indicies into multibyte indicies
            //
            DWORD dw[2];
            dw[0] = (DWORD)wParam;  // start
            dw[1] = (DWORD)lParam;  // end

            TranslateCharPos(hWnd, FALSE, dw, ARRAYSIZE(dw));

            return SendMessageA(hWnd, Msg, (WPARAM)dw[0], (LPARAM)dw[1]);
        }

        case EM_CHARFROMPOS:
        {
            DWORD dwPos =  SendMessageA(hWnd, Msg, wParam, lParam);
            if (HIWORD(dwPos) == 0)
            {
                TranslateCharPos(hWnd, TRUE, &dwPos, 1);
            }
            return dwPos;
        }

        case WM_SETTEXT:
        case LB_ADDSTRING:
        case CB_ADDSTRING:
        case EM_REPLACESEL:
            RIPMSG(wParam == 0, "wParam should be 0 for these messages");
            // fall through
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_SELECTSTRING:
        case CB_FINDSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_INSERTSTRING:
        case CB_SELECTSTRING:
        {
            if (MLIsEnabled(hWnd))
            {
                UINT uiMsgTx;

                switch (Msg)
                {
                    case WM_SETTEXT:
                        uiMsgTx = g_ML_SETTEXT;
                        break;
                    case LB_ADDSTRING:
                        uiMsgTx = g_ML_LB_ADDSTRING;
                        break;
                    case CB_ADDSTRING:
                        uiMsgTx = g_ML_CB_ADDSTRING;
                        break;
                    case LB_FINDSTRING:
                        uiMsgTx = g_ML_LB_FINDSTRING;
                        break;
                    case LB_FINDSTRINGEXACT:
                        uiMsgTx = g_ML_LB_FINDSTRINGEXACT;
                        break;
                    case LB_INSERTSTRING:
                        uiMsgTx = g_ML_LB_INSERTSTRING;
                        break;
                    case LB_SELECTSTRING:
                        uiMsgTx = g_ML_LB_SELECTSTRING;
                        break;
                    case CB_FINDSTRING:
                        uiMsgTx = g_ML_CB_FINDSTRING;
                        break;
                    case CB_FINDSTRINGEXACT:
                        uiMsgTx = g_ML_CB_FINDSTRINGEXACT;
                        break;
                    case CB_INSERTSTRING:
                        uiMsgTx = g_ML_CB_INSERTSTRING;
                        break;
                    case CB_SELECTSTRING:
                        uiMsgTx = g_ML_CB_SELECTSTRING;
                        break;

                    default:
                        ASSERT(0);
                }

                return SendMessageA(hWnd, uiMsgTx, wParam, lParam);
            }
            else if (Msg == CB_FINDSTRINGEXACT && IsUnicodeComboEx(hWnd))
            {
                // ComboEx is in UNICODE mode so we can send it the
                // unicode string
                goto Ldef;
            }
            else
            {
                // BUGBUG: in the ADDSTRING and INSERTSTRING cases for OWNERDRAW !HASSTRING
                // then this is a pointer to a structure and not a string!!!  (Seems that only
                // code that does this is comctl32's comboex, which currently doesn't come here.)
                //
                CStrIn  str((LPWSTR) lParam);

                return SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
            }
        }

        case WM_GETTEXT:
        case LB_GETTEXT:
        case CB_GETLBTEXT:
        {
            if (MLIsEnabled(hWnd))
            {
                UINT uiMsgTx;

                switch (Msg)
                {
                    case WM_GETTEXT:
                        uiMsgTx = g_ML_GETTEXT;
                        break;
                    case LB_GETTEXT:
                        uiMsgTx = g_ML_LB_GETTEXT;
                        break;
                    case CB_GETLBTEXT:
                        uiMsgTx = g_ML_CB_GETLBTEXT;
                        break;

                    default:
                        ASSERT(0);
                }

                return SendMessageA(hWnd, uiMsgTx, wParam, lParam);
            }
            else if (Msg == CB_GETLBTEXT && IsUnicodeComboEx(hWnd))
            {
                // ComboEx is in UNICODE mode so we can send it the
                // unicode string
                goto Ldef;
            }
            else
            {
                int iStrLen;

                switch (Msg)
                {
                    case WM_GETTEXT:
                        iStrLen = (int)wParam;
                        break;
                    case LB_GETTEXT:
                        iStrLen = SendMessageA(hWnd, LB_GETTEXTLEN, wParam, lParam);
                        if (iStrLen == LB_ERR)
                            iStrLen = 255;
                        break;
                    case CB_GETLBTEXT:
                        iStrLen = SendMessageA(hWnd, CB_GETLBTEXTLEN, wParam, lParam);
                        if (iStrLen == CB_ERR)
                            iStrLen = 255;
                        break;

                    default:
                        ASSERT(0);
                }

                CStrOut str((LPWSTR)lParam, (iStrLen + 1));
                SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);

                return str.ConvertExcludingNul();
            }
        }

        case WM_GETTEXTLENGTH:
        case LB_GETTEXTLEN:
        case CB_GETLBTEXTLEN:
        {
            UINT uiMsgTx;

            if (MLIsEnabled(hWnd))
            {
                switch (Msg)
                {
                    case WM_GETTEXTLENGTH:
                        uiMsgTx = g_ML_GETTEXTLENGTH;
                        break;
                    case LB_GETTEXTLEN:
                        uiMsgTx = g_ML_LB_GETTEXTLEN;
                        break;
                    case CB_GETLBTEXTLEN:
                        uiMsgTx = g_ML_CB_GETLBTEXTLEN;
                        break;

                    default:
                        ASSERT(0);
                }


                return SendMessageA(hWnd, uiMsgTx, wParam, lParam);
            }
            else
            {
                // Bug: #70280
                // we can not just return the size of ANSI character back, it breaks some localized
                // version of Win98 with localized IE/OE scenario: some apps rely on the returned size
                // in characters to show the string and may cause the garbage displayed beyond the end of
                // the actual string.
                //
                LPWSTR lpwszTemp = NULL;
                int iWCharLen = 0;
                int iAnsiCharLen = SendMessageA(hWnd, Msg, wParam, lParam);
                if ((iAnsiCharLen <= 0) || (LB_ERR == iAnsiCharLen) || (CB_ERR == iAnsiCharLen))
                {
                    iWCharLen = iAnsiCharLen;   // return error if we can not get the ANSI string length
                    goto L_Rtn;
                }

                // we always allocate the wide string buffer in the size of ANSI string length plus 1,
                // it should be big enough to hold the returned wide string.
                lpwszTemp = (LPWSTR)LocalAlloc(LPTR, ((iAnsiCharLen + 1) * sizeof(WCHAR)));
                if (!lpwszTemp)
                    goto L_Rtn;

                switch (Msg)
                {
                    case WM_GETTEXTLENGTH:
                        uiMsgTx = WM_GETTEXT;
                        break;
                    case LB_GETTEXTLEN:
                        uiMsgTx = LB_GETTEXT;
                        break;
                    case CB_GETLBTEXTLEN:
                        uiMsgTx = CB_GETLBTEXT;
                        break;

                    default:
                        ASSERT(0);
                }

                iWCharLen = SendMessageAThunk(hWnd, uiMsgTx,
                                              ((uiMsgTx == WM_GETTEXT) ? (WPARAM)(iAnsiCharLen + 1) : wParam),
                                              (LPARAM)lpwszTemp);

L_Rtn:
                if (lpwszTemp)
                    LocalFree(lpwszTemp);

                // if error occured, we'll return the error (WM_, LB_, CB_),
                // if fail to allocate memory, we'll return 0, the initial value of iWCharlen.
                return (LRESULT)iWCharLen;
            }
        }

        case WM_SETTINGCHANGE:
        {
            if (lParam)
            {
                CStrIn str((LPWSTR)lParam);
                LRESULT lRes = 0;
                SendMessageTimeoutA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str,
                                    SMTO_NORMAL, 3000, (PULONG_PTR)&lRes);

                return lRes;
            }
            goto Ldef;
        }

        // The new unicode comctl32.dll handles these correctly so we don't need to thunk:
        // TTM_DELTOOL, TTM_ADDTOOL, TVM_INSERTITEM, TVM_GETITEM, TCM_INSERTITEM, TCM_SETITEM

        default:
Ldef:
            return SendMessageA(hWnd, Msg, wParam, lParam);
    }
}


LRESULT WINAPI
SendMessageTimeoutAThunk(
        HWND    hWnd,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam,
        UINT    fuFlags,
        UINT    uTimeout,
        PULONG_PTR lpdwResult)
{
    switch (uMsg)
    {
    case WM_SETTINGCHANGE:
        {
            if (lParam)
            {
                CStrIn str((LPWSTR)lParam);
                return SendMessageTimeoutA(hWnd, uMsg, wParam, (LPARAM)(LPSTR)str,
                                    fuFlags, uTimeout, lpdwResult);
            }
        }
        break;
    }
    return SendMessageTimeoutA(hWnd, uMsg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
}


LRESULT FORWARD_API WINAPI
SendMessageTimeoutWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam,
        UINT    fuFlags,
        UINT    uTimeout,
        PULONG_PTR lpdwResult)
{
    VALIDATE_PROTOTYPE(SendMessage);

    // perf: we should do _asm here (see CallWindowProcWrapW), but
    // to do that we need to 'outline' the below switch (o.w. we
    // can't be 'naked').  that in turn slows down the non-NT case...

#ifndef UNIX
    THUNK_AW(SendMessageTimeout, (hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult));
#else
    return SendMessageTimeoutW(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
#endif
}

LRESULT FORWARD_API WINAPI
SendMessageWrapW(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(SendMessage);

    // perf: we should do _asm here (see CallWindowProcWrapW), but
    // to do that we need to 'outline' the below switch (o.w. we
    // can't be 'naked').  that in turn slows down the non-NT case...

#ifndef UNIX
    // n.b. THUNK not FORWARD
    THUNK_AW(SendMessage, (hWnd, Msg, wParam, lParam));
#else
    return SendMessageW(hWnd, Msg, wParam, lParam);
#endif
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
SetCurrentDirectoryWrapW(LPCWSTR lpszCurDir)
{
    VALIDATE_PROTOTYPE(SetCurrentDirectory);

    if (g_bRunningOnNT)
    {
        return SetCurrentDirectoryW(lpszCurDir);
    }

    CStrIn  str(lpszCurDir);

    return SetCurrentDirectoryA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetDlgItemTextWrapW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(SetDlgItemText);

    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);

    if (hWnd)
        return SetWindowTextWrapW(hWnd, lpString);
    else
        return FALSE;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetPropWrapW(
    HWND    hWnd,
    LPCWSTR lpString,
    HANDLE  hData)
{
    VALIDATE_PROTOTYPE(SetProp);

    if (g_bRunningOnNT)
    {
        return SetPropW(hWnd, lpString, hData);
    }

    CStrIn  str(lpString);

    return SetPropA(hWnd, str, hData);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LONG FORWARD_API WINAPI
SetWindowLongWrapW(HWND hWnd, int nIndex, LONG dwNewLong)
{
    VALIDATE_PROTOTYPE(SetWindowLong);

    FORWARD_AW(SetWindowLong, (hWnd, nIndex, dwNewLong));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HHOOK
FORWARD_API WINAPI
SetWindowsHookExWrapW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    VALIDATE_PROTOTYPE(SetWindowsHookEx);

    FORWARD_AW(SetWindowsHookEx, (idHook, lpfn, hmod, dwThreadId));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetWindowTextWrapW(HWND hWnd, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(SetWindowText);

    if (MLIsEnabled(hWnd))
        return MLSetControlTextI(hWnd, lpString);

    if (g_bRunningOnNT)
    {
        return SetWindowTextW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return SetWindowTextA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SystemParametersInfoWrapW(
        UINT    uiAction,
        UINT    uiParam,
        PVOID   pvParam,
        UINT    fWinIni)
{
    VALIDATE_PROTOTYPE(SystemParametersInfo);

    if (g_bRunningOnNT)
    {
        return SystemParametersInfoW(
                        uiAction,
                        uiParam,
                        pvParam,
                        fWinIni);
    }

    BOOL        ret;
    char        ach[LF_FACESIZE];

    if (uiAction == SPI_SETDESKWALLPAPER)
    {
        CStrIn str((LPCWSTR) pvParam);

        ret = SystemParametersInfoA(
                        uiAction,
                        uiParam,
                        str,
                        fWinIni);
    }
    else
        ret = SystemParametersInfoA(
                        uiAction,
                        uiParam,
                        pvParam,
                        fWinIni);

    if ((uiAction == SPI_GETICONTITLELOGFONT) && ret)
    {
        strcpy(ach, ((LPLOGFONTA)pvParam)->lfFaceName);
        SHAnsiToUnicode(ach, ((LPLOGFONTW)pvParam)->lfFaceName, ARRAYSIZE(((LPLOGFONTW)pvParam)->lfFaceName));
    }

    return ret;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int FORWARD_API WINAPI
TranslateAcceleratorWrapW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
    VALIDATE_PROTOTYPE(TranslateAccelerator);

    FORWARD_AW(TranslateAccelerator, (hWnd, hAccTable, lpMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
UnregisterClassWrapW(LPCWSTR lpClassName, HINSTANCE hInstance)
{
    VALIDATE_PROTOTYPE(UnregisterClass);

    if (g_bRunningOnNT)
    {
        return UnregisterClassW(lpClassName, hInstance);
    }

    CStrIn  str(lpClassName);

    return UnregisterClassA(str, hInstance);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

SHORT
WINAPI
VkKeyScanWrapW(WCHAR ch)
{
    VALIDATE_PROTOTYPE(VkKeyScan);

    if (g_bRunningOnNT)
    {
        return VkKeyScanW(ch);
    }

    CStrIn str(&ch, 1);

    return VkKeyScanA(*(char *)str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
WinHelpWrapW(HWND hwnd, LPCWSTR szFile, UINT uCmd, ULONG_PTR dwData)
{
    VALIDATE_PROTOTYPE(WinHelp);
  
    if (g_bRunningOnNT)
    {
        return WinHelpW(hwnd, szFile, uCmd, dwData);
    }

    CStrIn  str(szFile);

    return WinHelpA(hwnd, str, uCmd, dwData);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   wvsprintfW
//
//  Synopsis:   Nightmare string function
//
//  Arguments:  [pwszOut]    --
//              [pwszFormat] --
//              [...]        --
//
//  Returns:
//
//  History:    1-06-94   ErikGav   Created
//
//  Notes:      If you're reading this, you're probably having a problem with
//              this function.  Make sure that your "%s" in the format string
//              says "%ws" if you are passing wide strings.
//
//              %s on NT means "wide string"
//              %s on Chicago means "ANSI string"
//
//  BUGBUG:     This function should not be used.  Use Format instead.
//
//----------------------------------------------------------------------------

int WINAPI
wvsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, va_list arglist)
{
    VALIDATE_PROTOTYPE(wvsprintf);

    if (g_bRunningOnNT)
    {
        return wvsprintfW(pwszOut, pwszFormat, arglist);
    }

    // Old code created a 1K ansi output buffer for wvsprintfA and then
    // thunked the result.  If we're going to do that, might as
    // well create a 1K unicode output buffer and call our wvnsprintfW
    // implementation directly.  Two benefits: 1) no buffer overrun
    // and 2) native W implementation so we can handle unicode strings!
    WCHAR wszOut[1024];

    wvnsprintfW(wszOut, ARRAYSIZE(wszOut), pwszFormat, arglist);

    // Roll our own "ConvertExcludingNul" since this is W->W
    int ret = SHUnicodeToUnicode(wszOut, pwszOut, ARRAYSIZE(wszOut));
    if (ret > 0)
    {
        ret -= 1;
    }
    return ret;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_MPR_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   WNetRestoreConnectionWrapW
//
//----------------------------------------------------------------------------

DWORD WINAPI WNetRestoreConnectionWrapW(IN HWND hwndParent, IN LPCWSTR pwzDevice)
{
    VALIDATE_PROTOTYPE(WNetRestoreConnection);

    if (g_bRunningOnNT)
    {
        return WNetRestoreConnectionW(hwndParent, pwzDevice);
    }

    CStrIn  strIn(pwzDevice);
    return WNetRestoreConnectionA(hwndParent, strIn);
}

#endif // NEED_MPR_WRAPPER

#ifdef NEED_MPR_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   WNetGetLastErrorWrapW
//
//
//----------------------------------------------------------------------------

DWORD WINAPI WNetGetLastErrorWrapW(OUT LPDWORD pdwError, OUT LPWSTR pwzErrorBuf, IN DWORD cchErrorBufSize, OUT LPWSTR pwzNameBuf, IN DWORD cchNameBufSize)
{
    VALIDATE_PROTOTYPE(WNetGetLastError);

    if (g_bRunningOnNT)
    {
        return WNetGetLastErrorW(pdwError, pwzErrorBuf, cchErrorBufSize, pwzNameBuf, cchNameBufSize);
    }

    // Consider: Out-string bufsize too large or small?
    CStrOut strErrorOut(pwzErrorBuf, cchErrorBufSize);
    CStrOut strNameOut(pwzNameBuf, cchNameBufSize);

    DWORD dwResult = WNetGetLastErrorA(pdwError, strErrorOut, strErrorOut.BufSize(), strNameOut, strNameOut.BufSize());

    strErrorOut.ConvertExcludingNul();
    strNameOut.ConvertExcludingNul();
    return dwResult;
}

#endif // NEED_MPR_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI DrawTextExWrapW(
    HDC hdc,    // handle of device context
    LPWSTR pwzText, // address of string to draw
    int cchText,    // length of string to draw
    LPRECT lprc,    // address of rectangle coordinates
    UINT dwDTFormat,    // formatting options
    LPDRAWTEXTPARAMS lpDTParams // address of structure for more options
   )
{
    VALIDATE_PROTOTYPE(DrawTextEx);
    if (_MayNeedFontLinking(pwzText, cchText))
    {
        return DrawTextExFLW(hdc, pwzText, cchText, lprc, dwDTFormat, lpDTParams);
    }
    else if (g_bRunningOnNT)
    {
        return DrawTextExW(hdc, pwzText, cchText, lprc, dwDTFormat, lpDTParams);
    }
    CStrIn strText(pwzText, cchText);
    return DrawTextExA(hdc, strText, strText.strlen(), lprc, dwDTFormat, lpDTParams);
}

#endif // NEED_USER32_WRAPPER

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

void SetThunkMenuItemInfoWToA(LPCMENUITEMINFOW pmiiW, LPMENUITEMINFOA pmiiA, LPSTR pszBuffer, DWORD cchSize)
{
    *pmiiA = *(LPMENUITEMINFOA) pmiiW;

    // MFT_STRING is Zero. So MFT_STRING & anything evaluates to False.
    // so instead you have to check for the absence of non-string items
    if ((pmiiW->dwTypeData) && (MFT_NONSTRING & pmiiW->fType) == 0)
    {
        pmiiA->dwTypeData = pszBuffer;
        pmiiA->cch = cchSize;
        SHUnicodeToAnsi(pmiiW->dwTypeData, pmiiA->dwTypeData, cchSize);
    }
}

void GetThunkMenuItemInfoWToA(LPCMENUITEMINFOW pmiiW, LPMENUITEMINFOA pmiiA, LPSTR pszBuffer, DWORD cchSize)
{
    *pmiiA = *(LPMENUITEMINFOA) pmiiW;

    if ((pmiiW->dwTypeData) && (MFT_STRING & pmiiW->fType))
    {
        pszBuffer[0] = 0;
        pmiiA->dwTypeData = pszBuffer;
        pmiiA->cch = cchSize;
    }
}

void GetThunkMenuItemInfoAToW(LPCMENUITEMINFOA pmiiA, LPMENUITEMINFOW pmiiW)
{
    LPWSTR pwzText = pmiiW->dwTypeData;
    UINT cch = pmiiW->cch;

    *pmiiW = *(LPMENUITEMINFOW) pmiiA;
    pmiiW->dwTypeData = pwzText;
    pmiiW->cch = cch;

    if ((pmiiA->cch > 0) && (pmiiA->dwTypeData) && (pwzText) && !((MFT_SEPARATOR | MFT_BITMAP) & pmiiA->fType))
        SHAnsiToUnicode(pmiiA->dwTypeData, pmiiW->dwTypeData, pmiiW->cch);
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI GetMenuItemInfoWrapW(
    HMENU  hMenu,
    UINT  uItem,
    BOOL  fByPosition,
    LPMENUITEMINFOW  pmiiW)
{
    BOOL fResult;
    VALIDATE_PROTOTYPE(GetMenuItemInfo);
    ASSERT(pmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();
    BOOL fItemData;
    DWORD_PTR dwItemData;
    LPWSTR pwz;
    UINT cch;

    if (bOwnerDraw)
    {
        if (MIIM_DATA & pmiiW->fMask)
            fItemData = TRUE;
        else
        {
            fItemData = FALSE;
            pmiiW->fMask |= MIIM_DATA;
        }
        pwz = pmiiW->dwTypeData;        // save original buffer pointer
        cch = pmiiW->cch;               // save original buffer size
        dwItemData = pmiiW->dwItemData; // save original dwItemData size
    }

#ifndef UNIX
    // Widechar API's are messed up in MAINWIN. For now assume not ruuning on
    // NT for this.
    if (g_bRunningOnNT)
        fResult = GetMenuItemInfoW(hMenu, uItem, fByPosition, pmiiW);
    else
#endif
    {
        if (pmiiW->fMask & MIIM_TYPE)
        {
            MENUITEMINFOA miiA = *(LPMENUITEMINFOA)pmiiW;
            LPSTR pszText = NULL;

            if (pmiiW->cch > 0)
                pszText = new char[pmiiW->cch * 2];  // for DBCS, we multifly by 2

            miiA.dwTypeData = pszText;
            miiA.cch = (pszText)? pmiiW->cch * 2: 0; // set correct buffer size
            fResult = GetMenuItemInfoA(hMenu, uItem, fByPosition, &miiA);
            GetThunkMenuItemInfoAToW(&miiA, pmiiW);

            if (pszText)
                delete pszText;
        }
        else
            fResult = GetMenuItemInfoA(hMenu, uItem, fByPosition, (LPMENUITEMINFOA) pmiiW); // It doesn't contain a string so W and A are the same.
    }

    if (bOwnerDraw)
    {
        if ((MIIM_TYPE & pmiiW->fMask) && (MFT_OWNERDRAW & pmiiW->fType))
        {
            LPPUIMENUITEM lpItem = (LPPUIMENUITEM)pmiiW->dwItemData;

            if (HIWORD64(lpItem) && PUI_OWNERDRAW_SIG == lpItem->dwSig && !(MFT_OWNERDRAW & lpItem->fType))
            {
                if ((cch > 0) && pwz && !((MFT_SEPARATOR | MFT_BITMAP) & pmiiW->fType))
                {
                    StrCpyNW(pwz, lpItem->lpwz, cch);
                    pmiiW->dwTypeData = pwz;
                    pmiiW->cch = lpItem->cch;
                    pmiiW->fType &= ~MFT_OWNERDRAW;
                    if (fItemData)
                        pmiiW->dwItemData = lpItem->dwItemData;
                    else
                        pmiiW->dwItemData = dwItemData;
                }
            }
        }
    }

    return fResult;
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI InsertMenuItemWrapW(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFOW pmiiW)
{
    VALIDATE_PROTOTYPE(InsertMenuItem);
    ASSERT(pmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

    BOOL fResult;

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();
    MENUITEMINFOW miiW;

    if (bOwnerDraw)
    {
        MungeMenuItem(hMenu, pmiiW, &miiW);
        pmiiW = &miiW;
    }

    if (g_bRunningOnNT)
        return InsertMenuItemW(hMenu, uItem, fByPosition, pmiiW);

    MENUITEMINFOA miiA;
    CHAR szText[INTERNET_MAX_URL_LENGTH];

    SetThunkMenuItemInfoWToA(pmiiW, &miiA, szText, ARRAYSIZE(szText));
    fResult = InsertMenuItemA(hMenu, uItem, fByPosition, &miiA);

    return fResult;
}

// This is required for ML, so do not put inside #ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetMenuItemInfoWrapW(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFOW lpmiiW)
{
    VALIDATE_PROTOTYPE(SetMenuItemInfo);
    ASSERT(lpmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

    static BOOL bOwnerDraw = NeedMenuOwnerDraw();
    MENUITEMINFOW miiW;

    if (bOwnerDraw)
    {
        if ( (MIIM_TYPE & lpmiiW->fMask) &&
             0 == (lpmiiW->fType & (MFT_BITMAP | MFT_SEPARATOR)))
        {
            DeleteOwnerDrawMenuItem(hMenu, uItem, fByPosition);
            MungeMenuItem(hMenu, lpmiiW, &miiW);
            lpmiiW = &miiW;
        }
    }

    if (g_bRunningOnNT)
    {
        return SetMenuItemInfoW( hMenu, uItem, fByPosition, lpmiiW);
    }

    BOOL fRet;

    ASSERT( sizeof(MENUITEMINFOW) == sizeof(MENUITEMINFOA) &&
            FIELD_OFFSET(MENUITEMINFOW, dwTypeData) ==
            FIELD_OFFSET(MENUITEMINFOA, dwTypeData) );

    if ( (MIIM_TYPE & lpmiiW->fMask) &&
         0 == (lpmiiW->fType & (MFT_BITMAP | MFT_SEPARATOR)))
    {
        MENUITEMINFOA miiA;
        // the cch is ignored on SetMenuItemInfo
        CStrIn str((LPWSTR)lpmiiW->dwTypeData, -1);

        memcpy( &miiA, lpmiiW, sizeof(MENUITEMINFOA) );
        miiA.dwTypeData = (LPSTR)str;
        miiA.cch = str.strlen();

        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition, &miiA );
    }
    else
    {
        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition,
                                 (LPCMENUITEMINFOA)lpmiiW );
    }

    return fRet;
}

#ifdef NEED_GDI32_WRAPPER

HFONT WINAPI
CreateFontWrapW(int  nHeight,   // logical height of font
                int  nWidth,    // logical average character width
                int  nEscapement,   // angle of escapement
                int  nOrientation,  // base-line orientation angle
                int  fnWeight,  // font weight
                DWORD  fdwItalic,   // italic attribute flag
                DWORD  fdwUnderline,    // underline attribute flag
                DWORD  fdwStrikeOut,    // strikeout attribute flag
                DWORD  fdwCharSet,  // character set identifier
                DWORD  fdwOutputPrecision,  // output precision
                DWORD  fdwClipPrecision,    // clipping precision
                DWORD  fdwQuality,  // output quality
                DWORD  fdwPitchAndFamily,   // pitch and family
                LPCWSTR  pwzFace)   // address of typeface name string )
{
    VALIDATE_PROTOTYPE(CreateFont);

    if (g_bRunningOnNT)
    {
        return CreateFontW(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
                        fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
                        fdwClipPrecision, fdwQuality, fdwPitchAndFamily, pwzFace);
    }

    CStrIn  str(pwzFace);
    return CreateFontA(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
                        fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
                        fdwClipPrecision, fdwQuality, fdwPitchAndFamily, str);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI CreateMutexWrapW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR pwzName)
{
    VALIDATE_PROTOTYPE(CreateMutex);

    if (g_bRunningOnNT)
        return CreateMutexW(lpMutexAttributes, bInitialOwner, pwzName);

    CStrIn strText(pwzName);
    return CreateMutexA(lpMutexAttributes, bInitialOwner, strText);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI CreateMetaFileWrapW(LPCWSTR pwzFile)
{
    VALIDATE_PROTOTYPE(CreateMetaFile);

    if (g_bRunningOnNT)
        return CreateMetaFileW(pwzFile);

    CStrIn strText(pwzFile);
    return CreateMetaFileA(strText);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

// ExpandEnvironmentStrings returns the size of the needed string
DWORD WINAPI ExpandEnvironmentStringsWrapW(LPCWSTR pwzSrc, LPWSTR pwzDst, DWORD cchSize)
{
    DWORD dwRet;

    if (pwzDst)
    {
        VALIDATE_OUTBUF(pwzDst, cchSize);
    }

    if (g_bRunningOnNT)
    {
        dwRet = ExpandEnvironmentStringsW(pwzSrc, pwzDst, cchSize);
    }
    else
    {
        CStrIn strTextIn(pwzSrc);
        CStrOut strTextOut(pwzDst, cchSize);
        DWORD dwResult = ExpandEnvironmentStringsA(strTextIn, strTextOut, strTextOut.BufSize());
        DWORD dwResultUnicode = strTextOut.ConvertIncludingNul();

        // NT4 returned the number of bytes in the UNICODE string, not the number
        // of bytes in the ANSI string.  NT5 returns the number of bytes in the UNICODE
        // string divided by 2, so it has even less bearing on the ansi string.  Hopefully
        // the Win9x implementations did the right thing...
        //
        if (dwResult <= cchSize)
        {
            ASSERT(dwResultUnicode <= dwResult);
            dwRet = dwResultUnicode; // we fit in the out buffer, give accurate count
        }
        else
        {
            dwRet = dwResult; // buffer was too small, let caller know (NOTE: this may be bigger than needed)
        }
    }

    return dwRet;
}

#endif // NEED_KERNEL32_WRAPPER

// SHExpandEnvironmentStrings
//
// In all cases, this returns a valid output buffer.  The buffer may
// be empty, or it may be truncated, but you can always use the string.
//
// RETURN VALUE:
//   0  implies failure, either a truncated expansion or no expansion whatsoever
//   >0 implies complete expansion, value is count of characters written (excluding NULL)
//
DWORD WINAPI SHExpandEnvironmentStringsForUserW(HANDLE hToken, LPCWSTR pwzSrc, LPWSTR pwzDst, DWORD cchSize)
{
    DWORD   dwRet;

    // 99/05/28 vtan: Handle specified users here. It's a Windows NT
    // thing only. Check for both conditions then load the function
    // dynamically out of userenv.dll. If the function cannot be
    // located or returns a problem default to the current user as
    // NULL hToken.

    if (g_bRunningOnNT5OrHigher && (hToken != NULL))
    {
        VALIDATE_OUTBUF(pwzDst, cchSize);
        if (NT5_ExpandEnvironmentStringsForUserW(hToken, pwzSrc, pwzDst, cchSize) != FALSE)
        {

            // userenv!ExpandEnvironmentStringsForUser returns
            // a BOOL result. Convert this to a DWORD result
            // that matches what kernel32!ExpandEnvironmentStrings
            // returns.

            dwRet = lstrlenW(pwzDst) + sizeof('\0');
        }
        else
            dwRet = 0;
    }
    else
    {
        dwRet = ExpandEnvironmentStringsWrapW(pwzSrc, pwzDst, cchSize);
    }

    // The implementations of this function don't seem to gurantee gurantee certain
    // things about the output buffer in failure conditions that callers rely on.
    // So clean things up here.
    //
    // And I found code occasionally that handled semi-failure (success w/ dwRet>cchSize)
    // that assumed the string wasn't properly NULL terminated in this case.  Fix that here
    // in the wrapper so our callers don't have to wig-out about errors.
    //
    // NOTE: we map all failures to 0 too.
    //
    if (dwRet > cchSize)
    {
        // Buffer too small, some code assumed there was still a string there and
        // tried to NULL terminate, do it for them.
        SHTruncateString(pwzDst, cchSize);
        dwRet = 0;
    }
    else if (dwRet == 0)
    {
        // Failure, assume no expansions...
        StrCpyNW(pwzDst, pwzSrc, cchSize);
    }

    return dwRet;
}

DWORD WINAPI SHExpandEnvironmentStringsW(LPCWSTR pwzSrc, LPWSTR pwzDst, DWORD cchSize)
{
    return(SHExpandEnvironmentStringsForUserW(NULL, pwzSrc, pwzDst, cchSize));
}

DWORD WINAPI SHExpandEnvironmentStringsForUserA(HANDLE hToken, LPCSTR pszSrc, LPSTR pszDst, DWORD cchSize)
{
    DWORD dwRet;

    // 99/05/28 vtan: The ANSI version of SHExpandEnvironmentStringsForUser
    // exists in case somebody calls the ANSI implementation in WindowsNT.
    // This is meaningless on Win9x. This just thunks parameters and invokes
    // the Unicode implementation. If a problem occurs when thunking just
    // use the current user.

    if (g_bRunningOnNT5OrHigher && (hToken != NULL))
    {
        DWORD       dwResultAnsi;
        CStrInW     strInW(pszSrc);
        CStrOutW    strOutW(pszDst, cchSize);

        dwRet = SHExpandEnvironmentStringsForUserW(hToken, strInW, strOutW, strOutW.BufSize());
        dwResultAnsi = strOutW.ConvertIncludingNul();
        if (dwResultAnsi <= cchSize)
            dwRet = dwResultAnsi;
    }
    else
    {
        // NT4 returns the number of bytes in the UNICODE string.
        //
        // NT5 (as of June 99) returns the number of bytes in the UNICODE string / 2, so it
        // really has no accurate bearing on the number of ansi characters.  Plus we're not
        // guranteed buffer truncation in failure case.
        //
        // Not much we can do about it here, eh?  Hopefully our callers ignore the return result...
        //
        dwRet = ExpandEnvironmentStringsA(pszSrc, pszDst, cchSize);
    }

    // The implementations of this function don't seem to gurantee gurantee certain
    // things about the output buffer in failure conditions that callers rely on.
    // So clean things up here.
    //
    // And I found code occasionally that handled semi-failure (success w/ dwRet>cchSize)
    // that assumed the string wasn't properly NULL terminated in this case.  Fix that here
    // in the wrapper so our callers don't have to wig-out about errors.
    //
    // NOTE: we map all failures to 0 too.
    //
    if (dwRet > cchSize)
    {
        // Buffer too small, make sure it's NULL terminated.
        SHTruncateString(pszDst, cchSize);
        dwRet = 0;
    }
    else if (dwRet == 0)
    {
        // Failure, assume no expansions...
        StrCpyNA(pszDst, pszSrc, cchSize);
    }

    return dwRet;
}

DWORD WINAPI SHExpandEnvironmentStringsA(LPCSTR pszSrc, LPSTR pszDst, DWORD cchSize)
{
    return(SHExpandEnvironmentStringsForUserA(NULL, pszSrc, pszDst, cchSize));
}

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI CreateSemaphoreWrapW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR pwzName)
{
    VALIDATE_PROTOTYPE(CreateSemaphore);
    if (g_bRunningOnNT)
        return CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, pwzName);

    CStrIn strText(pwzName);
    return CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, strText);
}

#endif // NEED_KERNEL32_WRAPPER

// BUGBUG: Todo - GetStartupInfoWrapW

#ifdef NEED_KERNEL32_WRAPPER

#define ISGOOD 0
#define ISBAD 1

BOOL WINAPI IsBadStringPtrWrapW(LPCWSTR pwzString, UINT_PTR ucchMax)
{
    VALIDATE_PROTOTYPE(IsBadStringPtr);
    if (g_bRunningOnNT)
        return IsBadStringPtrW(pwzString, ucchMax);

    if (!ucchMax)
        return ISGOOD;

    if (!pwzString)
        return ISBAD;

    LPCWSTR pwzStartAddress = pwzString;
    // ucchMax maybe -1 but that's ok because the loop down below will just
    // look for the terminator.
    LPCWSTR pwzEndAddress = &pwzStartAddress[ucchMax - 1];
    TCHAR chTest;

    _try
    {
        chTest = *(volatile WCHAR *)pwzStartAddress;
        while (chTest && (pwzStartAddress != pwzEndAddress))
        {
            pwzStartAddress++;
            chTest = *(volatile WCHAR *)pwzStartAddress;
        }
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
        return ISBAD;
    }
    __endexcept

    return ISGOOD;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HINSTANCE WINAPI LoadLibraryWrapW(LPCWSTR pwzLibFileName)
{
    VALIDATE_PROTOTYPE(LoadLibrary);

    if (g_bRunningOnNT)
        return LoadLibraryW(pwzLibFileName);

    CStrIn  strFileName(pwzLibFileName);
    return LoadLibraryA(strFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR pwzFormat, LPWSTR pwzTimeStr, int cchTime)
{
    VALIDATE_PROTOTYPE(GetTimeFormat);
    if (g_bRunningOnNT)
        return GetTimeFormatW(Locale, dwFlags, lpTime, pwzFormat, pwzTimeStr, cchTime);

    CStrIn strTextIn(pwzFormat);
    CStrOut strTextOut(GetLocaleAnsiCodePage(Locale), pwzTimeStr, cchTime);
    int nResult = GetTimeFormatA(Locale, dwFlags, lpTime, strTextIn, strTextOut, strTextOut.BufSize());
    strTextOut.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR pwzFormat, LPWSTR pwzDateStr, int cchDate)
{
    VALIDATE_PROTOTYPE(GetDateFormat);
    if (g_bRunningOnNT)
        return GetDateFormatW(Locale, dwFlags, lpDate, pwzFormat, pwzDateStr, cchDate);

    CStrIn strTextIn(pwzFormat);
    CStrOut strTextOut(GetLocaleAnsiCodePage(Locale), pwzDateStr, cchDate);
    int nResult = GetDateFormatA(Locale, dwFlags, lpDate, strTextIn, strTextOut, strTextOut.BufSize());
    strTextOut.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI WritePrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName)
{
    VALIDATE_PROTOTYPE(WritePrivateProfileString);
    if (g_bRunningOnNT)
        return WritePrivateProfileStringW(pwzAppName, pwzKeyName, pwzString, pwzFileName);

    CStrIn strTextAppName(pwzAppName);
    CStrIn strTextKeyName(pwzKeyName);
    CStrIn strTextString(pwzString);
    CPPFIn strTextFileName(pwzFileName); // PrivateProfile filename needs special class

    return WritePrivateProfileStringA(strTextAppName, strTextKeyName, strTextString, strTextFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI GetPrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzDefault, LPWSTR pwzReturnedString, DWORD cchSize, LPCWSTR pwzFileName)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileString);
    if (g_bRunningOnNT)
        return GetPrivateProfileStringW(pwzAppName, pwzKeyName, pwzDefault, pwzReturnedString, cchSize, pwzFileName);

    CStrIn strTextAppName(pwzAppName);
    CStrIn strTextKeyName(pwzKeyName);
    CStrIn strTextDefault(pwzDefault);
    CPPFIn strTextFileName(pwzFileName); // PrivateProfile filename needs special class

    CStrOut strTextOut(pwzReturnedString, cchSize);
    DWORD dwResult = GetPrivateProfileStringA(strTextAppName, strTextKeyName, strTextDefault, strTextOut, cchSize, strTextFileName);
    strTextOut.ConvertIncludingNul();

    return dwResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

STDAPI_(DWORD_PTR) SHGetFileInfoWrapW(LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW FAR  *psfi, UINT cbFileInfo, UINT uFlags)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHGetFileInfo, _SHGetFileInfo);
    if (g_bRunningOnNT)
        return _SHGetFileInfoW(pwzPath, dwFileAttributes, psfi, cbFileInfo, uFlags);

    SHFILEINFOA shFileInfo;
    DWORD_PTR dwResult;

    shFileInfo.szDisplayName[0] = 0;        // Terminate so we can always thunk afterward.
    shFileInfo.szTypeName[0] = 0;           // Terminate so we can always thunk afterward.

    // Do we need to thunk the Path?
    if (SHGFI_PIDL & uFlags)
    {
        // No, because it's really a pidl pointer.
        dwResult = _SHGetFileInfoA((LPCSTR)pwzPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
    }
    else
    {
        // Yes
        CStrIn strPath(pwzPath);
        dwResult = _SHGetFileInfoA(strPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
    }

    psfi->hIcon = shFileInfo.hIcon;
    psfi->iIcon = shFileInfo.iIcon;
    psfi->dwAttributes = shFileInfo.dwAttributes;
    SHAnsiToUnicode(shFileInfo.szDisplayName, psfi->szDisplayName, ARRAYSIZE(shFileInfo.szDisplayName));
    SHAnsiToUnicode(shFileInfo.szTypeName, psfi->szTypeName, ARRAYSIZE(shFileInfo.szTypeName));

    return dwResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(ATOM) RegisterClassExWrapW(CONST WNDCLASSEXW FAR * pwcx)
{
    VALIDATE_PROTOTYPE(RegisterClassEx);
    if (g_bRunningOnNT)
        return RegisterClassExW(pwcx);

    CStrIn strMenuName(pwcx->lpszMenuName);
    CStrIn strClassName(pwcx->lpszClassName);
    WNDCLASSEXA wcx = *(CONST WNDCLASSEXA FAR *) pwcx;
    wcx.cbSize = sizeof(wcx);
    wcx.lpszMenuName = strMenuName;
    wcx.lpszClassName = strClassName;

    return RegisterClassExA(&wcx);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(BOOL) GetClassInfoExWrapW(HINSTANCE hinst, LPCWSTR pwzClass, LPWNDCLASSEXW lpwcx)
{
    VALIDATE_PROTOTYPE(GetClassInfoEx);
    if (g_bRunningOnNT)
        return GetClassInfoExW(hinst, pwzClass, lpwcx);

    BOOL fResult;
    CStrIn strClassName(pwzClass);
    WNDCLASSEXA wcx;
    wcx.cbSize = sizeof(wcx);

    fResult = GetClassInfoExA(hinst, strClassName, &wcx);
    *(WNDCLASSEXA FAR *) lpwcx = wcx;
    lpwcx->lpszMenuName = NULL;        // GetClassInfoExA makes this point off to private data that they own.
    lpwcx->lpszClassName = pwzClass;

    return fResult;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

//+---------------------------------------------------------------------------
//      StartDoc
//----------------------------------------------------------------------------

int
StartDocWrapW( HDC hDC, const DOCINFO * lpdi )
{
    VALIDATE_PROTOTYPE(StartDoc);

    if (g_bRunningOnNT)
    {
        return StartDocW( hDC, lpdi );
    }

    CStrIn  strDocName( lpdi->lpszDocName );
    CStrIn  strOutput( lpdi->lpszOutput );
    CStrIn  strDatatype( lpdi->lpszDatatype );
    DOCINFOA dia;

    dia.cbSize = sizeof(DOCINFO);
    dia.lpszDocName = strDocName;
    dia.lpszOutput = strOutput;
    dia.lpszDatatype = strDatatype;
    dia.fwType = lpdi->fwType;

    return StartDocA( hDC, &dia );
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

STDAPI_(UINT) DragQueryFileWrapW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(DragQueryFile, _DragQueryFile);
    VALIDATE_OUTBUF(lpszFile, cch);

    //
    //  We are lazy and do not support lpszFile == NULL to query the length
    //  of an individual string.
    //
    ASSERT(iFile == 0xFFFFFFFF || lpszFile);

    if (g_bRunningOnNT)
        return _DragQueryFileW(hDrop, iFile, lpszFile, cch);

    //
    //  If iFile is 0xFFFFFFFF, then lpszFile and cch are ignored.
    //
    if (iFile == 0xFFFFFFFF)
        return _DragQueryFileA(hDrop, iFile, NULL, 0);

    CStrOut str(lpszFile, cch);

    _DragQueryFileA(hDrop, iFile, str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_VERSION_WRAPPER

//
//  the version APIs are not conducive to using
//  wrap versions of the APIs, but we are going to
//  do something reasonable....
//
#define VERSIONINFO_BUFF   (MAX_PATH * SIZEOF(WCHAR))

STDAPI_(DWORD)
GetFileVersionInfoSizeWrapW(LPWSTR pwzFilename,  LPDWORD lpdwHandle)
{
    if (g_bRunningOnNT)
    {
        return GetFileVersionInfoSizeW(pwzFilename, lpdwHandle);
    }
    else
    {
        char szFilename[MAX_PATH];
        DWORD dwRet;

        ASSERT(pwzFilename);
        SHUnicodeToAnsi(pwzFilename, szFilename, ARRAYSIZE(szFilename));
        dwRet = GetFileVersionInfoSizeA(szFilename, lpdwHandle);
        if (dwRet > 0)
        {
            // Add a scratch buffer to front for converting to UNICODE
            dwRet += VERSIONINFO_BUFF;
        }
        return dwRet;
    }
}

#endif // NEED_VERSION_WRAPPER

#ifdef NEED_VERSION_WRAPPER

STDAPI_(BOOL)
GetFileVersionInfoWrapW(LPWSTR pwzFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    if (g_bRunningOnNT)
    {
        return GetFileVersionInfoW(pwzFilename, dwHandle, dwLen, lpData);
    }
    else
    {
        char szFilename[MAX_PATH];
        BYTE* pb;

        if (dwLen <= VERSIONINFO_BUFF)
        {
            return FALSE;
        }

        ASSERT(pwzFilename);
        SHUnicodeToAnsi(pwzFilename, szFilename, ARRAYSIZE(szFilename));
        //Skip over our scratch buffer at the beginning
        pb = (BYTE*)lpData + VERSIONINFO_BUFF;

        return GetFileVersionInfoA(szFilename, dwHandle, dwLen - VERSIONINFO_BUFF, (void*)pb);
    }
}

#endif // NEED_VERSION_WRAPPER

#ifdef NEED_VERSION_WRAPPER

STDAPI_(BOOL)
VerQueryValueWrapW(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuffer, PUINT puLen)
{
    if (g_bRunningOnNT)
    {
        return VerQueryValueW(pBlock, pwzSubBlock, ppBuffer, puLen);
    }
    else
    {
        const WCHAR pwzStringFileInfo[] = L"\\StringFileInfo";

        //
        // WARNING: This function wipes out any string previously returned
        // for this pBlock because a common buffer at the beginning of the
        // block is used for ansi/unicode translation!
        //
        char szSubBlock[MAX_PATH];
        BOOL fRet;
        BYTE* pb;

        ASSERT(pwzSubBlock);
        SHUnicodeToAnsi(pwzSubBlock, szSubBlock, ARRAYSIZE(szSubBlock));

        // The first chunk is our scratch buffer for converting to UNICODE
        pb = (BYTE*)pBlock + VERSIONINFO_BUFF;
        fRet = VerQueryValueA((void*)pb, szSubBlock, ppBuffer, puLen);

        // Convert to unicode if ansi string returned
        if (fRet && StrCmpNIW(pwzSubBlock, pwzStringFileInfo, ARRAYSIZE(pwzStringFileInfo) - 1) == 0)
        {
            // Convert returned string to UNICODE.  We use the scratch buffer
            // at the beginning of pBlock
            LPWSTR pwzBuff = (LPWSTR)pBlock;
            if (*puLen == 0)
            {
                pwzBuff[0] = L'\0';
            }
            else
            {
                SHAnsiToUnicode((LPCSTR)*ppBuffer, pwzBuff, VERSIONINFO_BUFF/sizeof(WCHAR));
            }
            *ppBuffer = pwzBuff;
        }
        return fRet;
    }
}

#endif // NEED_VERSION_WRAPPER


#ifdef NEED_SHELL32_WRAPPER

HRESULT WINAPI SHDefExtractIconWrapW(LPCWSTR pszFile, int nIconIndex,
                                     UINT uFlags, HICON *phiconLarge,
                                     HICON *phiconSmall, UINT nIconSize)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHDefExtractIcon, _SHDefExtractIcon);

    HRESULT hr;

    if (UseUnicodeShell32())
    {
        hr = _SHDefExtractIconW(pszFile, nIconIndex, uFlags, phiconLarge,
                                phiconSmall, nIconSize);
    }
    else
    {
        CStrIn striFile(pszFile);

        hr = _SHDefExtractIconA(striFile, nIconIndex, uFlags, phiconLarge,
                                phiconSmall, nIconSize);
    }

    return hr;
}

#endif // NEED_SHELL32_WRAPPER


BOOL WINAPI SHGetNewLinkInfoWrapW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHGetNewLinkInfo, _SHGetNewLinkInfo);

    BOOL fRet;

    if (g_bRunningOnNT5OrHigher)
    {
        fRet =  _SHGetNewLinkInfoW(pszpdlLinkTo, pszDir, pszName, pfMustCopy,
                                   uFlags);
    }
    else
    {
        CStrIn  striDir(pszDir);
        CStrOut stroName(pszName, MAX_PATH);

        if (SHGNLI_PIDL & uFlags)
        {
            fRet = _SHGetNewLinkInfoA((LPCSTR)pszpdlLinkTo, striDir,
                                      stroName, pfMustCopy, uFlags);
        }
        else
        {
            CStrIn striLinkTo(pszpdlLinkTo);

            fRet = _SHGetNewLinkInfoA(striLinkTo, striDir, stroName,
                                      pfMustCopy, uFlags);
        }

        if (fRet)
            stroName.ConvertIncludingNul();
    }
    
    return fRet;
}


#ifdef NEED_ADVAPI32_WRAPPER

LONG WINAPI RegEnumValueWrapW(HKEY hkey, DWORD dwIndex, LPWSTR lpValueName,
                              LPDWORD lpcbValueName, LPDWORD lpReserved,
                              LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    VALIDATE_PROTOTYPE(RegEnumValue);

    LONG lRet;

    if (UseUnicodeShell32())
    {
        lRet = RegEnumValueW(hkey, dwIndex, lpValueName, lpcbValueName,
                             lpReserved, lpType, lpData, lpcbData);
    }
    else
    {
        CStrOut stroValueName(lpValueName, *lpcbValueName);
        DWORD   dwTypeTemp;

        if (lpData)
        {
            ASSERT(lpcbData);

            CStrOut stroData((LPWSTR)lpData, (*lpcbData) / sizeof(WCHAR));

            lRet = RegEnumValueA(hkey, dwIndex, stroValueName, lpcbValueName,
                                 lpReserved, &dwTypeTemp,
                                 (LPBYTE)(LPSTR)stroData, lpcbData);

            if (ERROR_SUCCESS == lRet && REG_SZ == dwTypeTemp)
            {
                *lpcbData = sizeof(WCHAR) * stroData.ConvertIncludingNul();
            }
        }
        else
        {
            lRet = RegEnumValueA(hkey, dwIndex, stroValueName, lpcbValueName,
                                 lpReserved, &dwTypeTemp, lpData, lpcbData);
        }

        if (ERROR_SUCCESS == lRet)
            *lpcbValueName = stroValueName.ConvertExcludingNul();

        if (lpType)
            *lpType = dwTypeTemp;
    }

    return lRet;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI WritePrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey,
                                           LPVOID lpStruct, UINT uSizeStruct,
                                           LPCWSTR szFile)
{
    VALIDATE_PROTOTYPE(WritePrivateProfileStruct);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = WritePrivateProfileStructW(lpszSection, lpszKey, lpStruct,
                                          uSizeStruct, szFile);
    }
    else
    {
        CStrIn striSection(lpszSection);
        CStrIn striKey(lpszKey);
        CPPFIn striFile(szFile); // PrivateProfile filename needs special class

        fRet = WritePrivateProfileStructA(striSection, striKey, lpStruct,
                                          uSizeStruct, striFile);
    }

    return fRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI GetPrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey,
                                         LPVOID lpStruct, UINT uSizeStruct,
                                         LPCWSTR szFile)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileStruct);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = GetPrivateProfileStructW(lpszSection, lpszKey, lpStruct,
                                        uSizeStruct, szFile);
    }
    else
    {
        CStrIn striSection(lpszSection);
        CStrIn striKey(lpszKey);
        CPPFIn striFile(szFile); // PrivateProfile filename needs special class

        fRet = GetPrivateProfileStructA(striSection, striKey, lpStruct,
                                        uSizeStruct, striFile);
    }

    return fRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI CreateProcessWrapW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes,
                               BOOL bInheritHandles,
                               DWORD dwCreationFlags,
                               LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory,
                               LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation)
{
    BOOL fRet;
    VALIDATE_PROTOTYPE(CreateProcess);

    if (UseUnicodeShell32())
    {
        fRet = CreateProcessW(lpApplicationName, lpCommandLine,
                              lpProcessAttributes, lpThreadAttributes,
                              bInheritHandles, dwCreationFlags, lpEnvironment,
                              lpCurrentDirectory, lpStartupInfo,
                              lpProcessInformation);
    }
    else
    {
        CStrIn striApplicationName(lpApplicationName);
        CStrIn striCommandLine(lpCommandLine);
        CStrIn striCurrentDirectory(lpCurrentDirectory);

        if (NULL == lpStartupInfo)
        {
            fRet = CreateProcessA(striApplicationName, striCommandLine,
                                  lpProcessAttributes, lpThreadAttributes,
                                  bInheritHandles, dwCreationFlags,
                                  lpEnvironment, striCurrentDirectory,
                                  NULL, lpProcessInformation);
        }
        else
        {
            STARTUPINFOA si = *(STARTUPINFOA*)lpStartupInfo;

            CStrIn striReserved(lpStartupInfo->lpReserved);
            CStrIn striDesktop(lpStartupInfo->lpDesktop);
            CStrIn striTitle(lpStartupInfo->lpTitle);

            si.lpReserved = striReserved;
            si.lpDesktop  = striDesktop;
            si.lpTitle   = striTitle;

            fRet = CreateProcessA(striApplicationName, striCommandLine,
                                  lpProcessAttributes, lpThreadAttributes,
                                  bInheritHandles, dwCreationFlags,
                                  lpEnvironment, striCurrentDirectory,
                                  &si, lpProcessInformation);
        }

    }

    return fRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

HICON WINAPI ExtractIconWrapW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
    HICON hicon;

    if (UseUnicodeShell32())
    {
        hicon = _ExtractIconW(hInst, lpszExeFileName, nIconIndex);
    }
    else
    {
        CStrIn striExeFileName(lpszExeFileName);

        hicon = _ExtractIconA(hInst, striExeFileName, nIconIndex);
    }

    return hicon;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI DdeInitializeWrapW(LPDWORD pidInst, PFNCALLBACK pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    UINT uRet;

    if (UseUnicodeShell32())
    {
        uRet = DdeInitializeW(pidInst, pfnCallback, afCmd, ulRes);
    }
    else
    {
        //
        // This assumes the callback function will used the wrapped dde
        // string functions (DdeCreateStringHandle and DdeQueryString)
        // to access strings.
        //

        uRet = DdeInitializeA(pidInst, pfnCallback, afCmd, ulRes);
    }

    return uRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HSZ WINAPI DdeCreateStringHandleWrapW(DWORD idInst, LPCWSTR psz, int iCodePage)
{
    HSZ hszRet;

    if (UseUnicodeShell32())
    {
        hszRet = DdeCreateStringHandleW(idInst, psz, iCodePage);
    }
    else
    {
        CStrIn stripsz(psz);

        hszRet = DdeCreateStringHandleA(idInst, stripsz, CP_WINANSI);
    }

    return hszRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI DdeQueryStringWrapW(DWORD idInst, HSZ hsz, LPWSTR psz,
                                 DWORD cchMax, int iCodePage)
{
    DWORD dwRet;

    if (UseUnicodeShell32())
    {
        dwRet = DdeQueryStringW(idInst, hsz, psz, cchMax, iCodePage);
    }
    else
    {
        CStrOut stropsz(psz, cchMax);

        dwRet = DdeQueryStringA(idInst, hsz, stropsz, stropsz.BufSize(),
                                CP_WINANSI);

        if (dwRet && psz)
            dwRet = stropsz.ConvertExcludingNul();        
    }

    return dwRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

BOOL WINAPI GetSaveFileNameWrapW(LPOPENFILENAMEW lpofn)
{
    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = _GetSaveFileNameW(lpofn);
    }
    else
    {
        ASSERT(lpofn);
        ASSERT(sizeof(OPENFILENAMEA) == sizeof(OPENFILENAMEW));

        OPENFILENAMEA ofnA = *(LPOPENFILENAMEA)lpofn;

        // In parameters
        CStrInMulti strimFilter(lpofn->lpstrFilter);
        CStrIn      striInitialDir(lpofn->lpstrInitialDir);
        CStrIn      striTitle(lpofn->lpstrTitle);
        CStrIn      striDefExt(lpofn->lpstrDefExt);
        CStrIn      striTemplateName(lpofn->lpTemplateName);

        ASSERT(NULL == lpofn->lpstrCustomFilter); // add support if you need it.

        // Out parameters
        CStrOut     stroFile(lpofn->lpstrFile, lpofn->nMaxFile);
        CStrOut     stroFileTitle(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);

        //In Out parameters
        SHUnicodeToAnsi(lpofn->lpstrFile, stroFile, stroFile.BufSize());

        // Set up the parameters
        ofnA.lpstrFilter        = strimFilter;
        ofnA.lpstrInitialDir    = striInitialDir;
        ofnA.lpstrTitle         = striTitle;
        ofnA.lpstrDefExt        = striDefExt;
        ofnA.lpTemplateName     = striTemplateName;
        ofnA.lpstrFile          = stroFile;
        ofnA.lpstrFileTitle     = stroFileTitle;

        fRet = _GetSaveFileNameA(&ofnA);

        if (fRet)
        {
            // Copy the out parameters
            lpofn->nFilterIndex = ofnA.nFilterIndex;
            lpofn->Flags        = ofnA.Flags;

            // Get the offset to the filename
            stroFile.ConvertIncludingNul();
            LPWSTR psz = PathFindFileNameW(lpofn->lpstrFile);

            if (psz)
            {
                lpofn->nFileOffset = (int) (psz-lpofn->lpstrFile);

                // Get the offset of the extension
                psz = PathFindExtensionW(psz);

                lpofn->nFileExtension = psz ? (int)(psz-lpofn->lpstrFile) : 0; 
            }
            else
            {
                lpofn->nFileOffset    = 0;
                lpofn->nFileExtension = 0;
            }

        }
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

BOOL WINAPI GetOpenFileNameWrapW(LPOPENFILENAMEW lpofn)
{
    BOOL fRet;

    VALIDATE_PROTOTYPE_DELAYLOAD(GetOpenFileName, _GetOpenFileName);

    if (UseUnicodeShell32())
    {
        fRet = _GetOpenFileNameW(lpofn);
    }
    else
    {
        ASSERT(lpofn);
        ASSERT(sizeof(OPENFILENAMEA) == sizeof(OPENFILENAMEW));

        OPENFILENAMEA ofnA = *(LPOPENFILENAMEA)lpofn;

        // In parameters
        CStrInMulti strimFilter(lpofn->lpstrFilter);
        CStrIn      striInitialDir(lpofn->lpstrInitialDir);
        CStrIn      striTitle(lpofn->lpstrTitle);
        CStrIn      striDefExt(lpofn->lpstrDefExt);
        CStrIn      striTemplateName(lpofn->lpTemplateName);

        ASSERT(NULL == lpofn->lpstrCustomFilter); // add support if you need it.

        // Out parameters
        CStrOut     stroFile(lpofn->lpstrFile, lpofn->nMaxFile);
        CStrOut     stroFileTitle(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);

        //In Out parameters
        SHUnicodeToAnsi(lpofn->lpstrFile, stroFile, stroFile.BufSize());

        // Set up the parameters
        ofnA.lpstrFilter        = strimFilter;
        ofnA.lpstrInitialDir    = striInitialDir;
        ofnA.lpstrTitle         = striTitle;
        ofnA.lpstrDefExt        = striDefExt;
        ofnA.lpTemplateName     = striTemplateName;
        ofnA.lpstrFile          = stroFile;
        ofnA.lpstrFileTitle     = stroFileTitle;

        fRet = _GetOpenFileNameA(&ofnA);

        if (fRet)
        {
            // Copy the out parameters
            lpofn->nFilterIndex = ofnA.nFilterIndex;
            lpofn->Flags        = ofnA.Flags;

            // Get the offset to the filename
            stroFile.ConvertIncludingNul();
            LPWSTR psz = PathFindFileNameW(lpofn->lpstrFile);

            if (psz)
            {
                lpofn->nFileOffset = (int) (psz-lpofn->lpstrFile);

                // Get the offset of the extension
                psz = PathFindExtensionW(psz);

                lpofn->nFileExtension = psz ? (int)(psz-lpofn->lpstrFile) : 0; 
            }
            else
            {
                lpofn->nFileOffset    = 0;
                lpofn->nFileExtension = 0;
            }

        }
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

#define SHCNF_HAS_WSTR_PARAMS(f)   ((f & SHCNF_TYPE) == SHCNF_PATHW     ||    \
                                    (f & SHCNF_TYPE) == SHCNF_PRINTERW  ||    \
                                    (f & SHCNF_TYPE) == SHCNF_PRINTJOBW    )

void SHChangeNotifyWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1,
                        LPCVOID dwItem2)
{
    // Can't do this because this is not a "W" function
    // VALIDATE_PROTOTYPE(SHChangeNotify);

    if (UseUnicodeShell32() || !SHCNF_HAS_WSTR_PARAMS(uFlags))
    {
        _SHChangeNotify(wEventId, uFlags, dwItem1, dwItem2);
    }
    else
    {
        CStrIn striItem1((LPWSTR)dwItem1);
        CStrIn striItem2((LPWSTR)dwItem2);
        
        if ((uFlags & SHCNF_TYPE) == SHCNF_PATHW)
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PATHA;
        }
        else if ((uFlags & SHCNF_TYPE) == SHCNF_PRINTERW)
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PRINTERA;
        }
        else
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PRINTJOBA;
        }

        _SHChangeNotify(wEventId, uFlags, (void*)(LPSTR)striItem1,
                        (void*)(LPSTR)striItem2);
    }

    return;
}

LWSTDAPI_(void) SHFlushSFCacheWrap(void)
{
    HMODULE hShell32 = GetModuleHandleWrap(TEXT("SHELL32"));
    if (hShell32) // if shell32 is not in process, then there's nothing to flush
    {
        // NOTE: GetProcAddress always takes ANSI strings!
        DLLGETVERSIONPROC pfnGetVersion =
            (DLLGETVERSIONPROC)GetProcAddress(hShell32, "DllGetVersion");

        if (pfnGetVersion)
        {
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion(&dllinfo) == NOERROR)
            {
                // first, we need a version of shell32 that supports the function
                if (dllinfo.dwMajorVersion >= 4)
                {
                    // then, we need to work around the "missing critical section" bug
                    // that's been causing so many faults recently...
                    //
                    BOOL fTakeCriticalSection = (dllinfo.dwMajorVersion == 4);
                    CRITICAL_SECTION * pcsShell = NULL;

                    if (fTakeCriticalSection)
                    {
#if 1
                        // Since we have no valid data, just fail at this point.
                        // Turn the real code on when we get data.
                        return;
#else
                        IShellFolder *psf;

                        if (FAILED(SHGetDesktopFolder(&psf)))
                            return;

                        // We have a million flavors of IE4 shell32: IE4.0, IE4.01, IE4.01 CS,
                        // IE 4.01 SP1 / Win98, each localized rebuilt Win98, IE4.01 SP1 CS, IE4.01 SP2,
                        // any QFEs that required a shell32 rebuild, including Win95/NT builds.
                        //
                        // Here's a table of what we know, we'll fail on the rest:
                        //
                        typedef struct tagOSTable {
                            DWORD dwMajorVersion;                   // Major version
                            DWORD dwMinorVersion;                   // Minor version
                            DWORD dwBuildNumber;                    // Build number
                            DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
                            int   nOffset;                          // offset to g_cs
                        } OSTABLE;
                        static const OSTABLE osTable[] =
                        {
                            //{4, 71, 1712, VER_PLATFORM_WIN32_WINDOWS, 0}, // IE4.0
                            //{4, 71, 1712, VER_PLATFORM_WIN32_NT, 0},
                            //{4, 72, 2106, VER_PLATFORM_WIN32_WINDOWS, 0xf15d8}, // IE4.01 (is this debug or retail?!)
                            //{4, 72, 2106, VER_PLATFORM_WIN32_NT, 0},
                            //{4, 72, 2201, VER_PLATFORM_WIN32_WINDOWS, 0}, // IE4.01 CS
                            //{4, 72, 2201, VER_PLATFORM_WIN32_NT, 0},
                            //{4, 72, 3110, VER_PLATFORM_WIN32_WINDOWS, 0}, // IE4.01 SP1 / Win98
                            //{4, 72, 3110, VER_PLATFORM_WIN32_NT, 0},
                            //{4, 72, 3110, VER_PLATFORM_WIN32_WINDOWS, 0}, // IE4.01 SP1 CS
                            //{4, 72, 3110, VER_PLATFORM_WIN32_NT, 0},
                        };

                        for (int i = 0 ; i < ARRAYSIZE(osTable) ; i++)
                        {
                            if (0 == memcmp(&(osTable[i]), &dllinfo.dwMajorVersion, 4*sizeof(DWORD)))
                            {
                                break;
                            }
                        }

                        if (i == ARRAYSIZE(osTable))
                        {
                            RIPMSG(0, "SHFlushSFCacheWrap called on a platform requiring hack but we have no hack data.");
                            return;
                        }

                        // BUGBUG ROBUSTNESS: We should analize this critical section and verify
                        // that it's valid.  steal code from kernel or something like that...
                        pcsShell = (CRITICAL_SECTION*)(((LPBYTE)psf) + osTable[i].nOffset);

                        EnterCriticalSection(pcsShell);
#endif
                    }

                    _SHFlushSFCache();

                    if (fTakeCriticalSection)
                    {
                        LeaveCriticalSection(pcsShell);
                    }
                }
            }
        }
    }
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//+---------------------------------------------------------------------------
// PrintDlgWrap, PageSetupDlgWrap - wrappers
// DevNamesAFromDevNamesW, DevNamesWFromDevNamesA - helper functions
//
//        Copied from mshtml\src\core\wrappers\unicwrap.cpp with some
//        cosmetic changes (peterlee)
//        
//+---------------------------------------------------------------------------

HGLOBAL
DevNamesAFromDevNamesW( HGLOBAL hdnw )
{
    HGLOBAL         hdna = NULL;

    if (hdnw)
    {
        LPDEVNAMES lpdnw = (LPDEVNAMES) GlobalLock( hdnw );
        if (lpdnw)
        {
            CStrIn      strDriver( (LPCWSTR) lpdnw + lpdnw->wDriverOffset );
            CStrIn      strDevice( (LPCWSTR) lpdnw + lpdnw->wDeviceOffset );
            CStrIn      strOutput( (LPCWSTR) lpdnw + lpdnw->wOutputOffset );
            int         cchDriver = strDriver.strlen() + 1;
            int         cchDevice = strDevice.strlen() + 1;
            int         cchOutput = strOutput.strlen() + 1;

            hdna = GlobalAlloc( GHND, sizeof(DEVNAMES) +
                                cchDriver + cchDevice + cchOutput );
            if (hdna)
            {
                LPDEVNAMES lpdna = (LPDEVNAMES) GlobalLock( hdna );
                if (!lpdna)
                {
                    GlobalFree( hdna );
                    hdna = NULL;
                }
                else
                {
                    lpdna->wDriverOffset = sizeof(DEVNAMES);
                    lpdna->wDeviceOffset = lpdna->wDriverOffset + cchDriver;
                    lpdna->wOutputOffset = lpdna->wDeviceOffset + cchDevice;
                    lpdna->wDefault = lpdnw->wDefault;

                    lstrcpyA( (LPSTR) lpdna + lpdna->wDriverOffset, strDriver );
                    lstrcpyA( (LPSTR) lpdna + lpdna->wDeviceOffset, strDevice );
                    lstrcpyA( (LPSTR) lpdna + lpdna->wOutputOffset, strOutput );

                    GlobalUnlock( hdna );
                }
            }

            GlobalUnlock( hdnw );
            GlobalFree( hdnw );
        }
    }

   return hdna;
}

HGLOBAL
DevNamesWFromDevNamesA( HGLOBAL hdna )
{
    HGLOBAL         hdnw = NULL;

    if (hdna)
    {
        LPDEVNAMES lpdna = (LPDEVNAMES) GlobalLock( hdna );
        if (lpdna)
        {
            LPCSTR      lpszDriver = (LPCSTR) lpdna + lpdna->wDriverOffset;
            LPCSTR      lpszDevice = (LPCSTR) lpdna + lpdna->wDeviceOffset;
            LPCSTR      lpszOutput = (LPCSTR) lpdna + lpdna->wOutputOffset;
            int         cchDriver = lstrlenA( lpszDriver ) + 1;
            int         cchDevice = lstrlenA( lpszDevice ) + 1;
            int         cchOutput = lstrlenA( lpszOutput ) + 1;

            // assume the wide charcount won't exceed the multibyte charcount

            hdnw = GlobalAlloc( GHND, sizeof(DEVNAMES) +
                                sizeof(WCHAR) * (cchDriver + cchDevice + cchOutput)  );
            if (hdnw)
            {
                LPDEVNAMES lpdnw = (LPDEVNAMES) GlobalLock( hdnw );
                if (!lpdnw)
                {
                    GlobalFree( hdnw );
                    hdnw = NULL;
                }
                else
                {
                    lpdnw->wDriverOffset = sizeof(DEVNAMES) / sizeof(WCHAR);
                    lpdnw->wDeviceOffset = lpdnw->wDriverOffset + cchDriver;
                    lpdnw->wOutputOffset = lpdnw->wDeviceOffset + cchDevice;
                    lpdnw->wDefault = lpdna->wDefault;

                    SHAnsiToUnicode( (LPSTR) lpszDriver, (LPWSTR) lpdnw + lpdnw->wDriverOffset,
                                     cchDriver );
                    SHAnsiToUnicode( lpszDevice, (LPWSTR) lpdnw + lpdnw->wDeviceOffset,
                                     cchDevice);
                    SHAnsiToUnicode( lpszOutput, (LPWSTR) lpdnw + lpdnw->wOutputOffset,
                                     cchOutput);

                    GlobalUnlock( hdnw );
                }
            }

            GlobalUnlock( hdna );
            GlobalFree( hdna );
        }
    }

   return hdnw;
}

#ifdef UNIX
HGLOBAL
DevModeAFromDevModeW( HGLOBAL hdmw )
{
    HGLOBAL         hdma = NULL;

    if (hdmw)
    {
        LPDEVMODEW lpdmw = (LPDEVMODEW)GlobalLock( hdmw );
    if (lpdmw)
    {
        hdma = GlobalAlloc( GHND, sizeof(DEVMODEA) );
        if (hdma)
        {
            LPDEVMODEA lpdma = (LPDEVMODEA) GlobalLock( hdma );
        if (lpdma)
        {
            CStrIn          strDeviceName( lpdmw->dmDeviceName );
            CStrIn          strFormName( lpdmw->dmFormName );

            // assume memory layout is identical

            memcpy( lpdma->dmDeviceName, strDeviceName, CCHDEVICENAME );

            memcpy( &lpdma->dmSpecVersion,
                &lpdmw->dmSpecVersion,
                offsetof(DEVMODEW, dmFormName) -
                offsetof(DEVMODEW, dmSpecVersion) );

            memcpy( lpdma->dmFormName, strFormName, CCHFORMNAME );

            memcpy( &lpdma->dmLogPixels,
                &lpdmw->dmLogPixels,
                sizeof(DEVMODEW) -
                offsetof(DEVMODEW, dmLogPixels) );

            GlobalUnlock( hdma );
        }
        else
        {
            GlobalFree( hdma );
            hdma = NULL;
        }
        }

        GlobalUnlock( hdmw );
        GlobalFree( hdmw );
    }
    }

    return hdma;
}

//--------------------------------------------------------------
//      DEVMODEW from DEVMODEA
//--------------------------------------------------------------
HGLOBAL
DevModeWFromDevModeA( HGLOBAL hdma )
{
    HGLOBAL         hdmw = NULL;

    if (hdma)
    {
        LPDEVMODEA lpdma = (LPDEVMODEA)GlobalLock( hdma );
    if (lpdma)
    {
        hdmw = GlobalAlloc( GHND, sizeof(DEVMODEW) );
        if (hdmw)
        {
            LPDEVMODEW lpdmw = (LPDEVMODEW) GlobalLock( hdmw );
        if (lpdmw)
        {
            CStrOut     strDeviceName( lpdmw->dmDeviceName, CCHDEVICENAME );
            CStrOut     strFormName( lpdmw->dmFormName, CCHFORMNAME );

            // assume memory layout is identical

            lstrcpyA( strDeviceName, (LPCSTR)lpdma->dmDeviceName );
            strDeviceName.ConvertIncludingNul();

            memcpy( &lpdmw->dmSpecVersion,
                &lpdma->dmSpecVersion,
                offsetof(DEVMODEA, dmFormName) -
                offsetof(DEVMODEA, dmSpecVersion) );

            lstrcpyA( strFormName, (LPCSTR)lpdmw->dmFormName );
            strFormName.ConvertIncludingNul();

            memcpy( &lpdmw->dmLogPixels,
                &lpdma->dmLogPixels,
                sizeof(DEVMODEA) -
                offsetof(DEVMODEA, dmLogPixels) );

            GlobalUnlock( hdmw );
        }
        else
        {
            GlobalFree( hdmw );
            hdmw = NULL;
        }
        }

        GlobalUnlock( hdma );
        GlobalFree( hdma );
    }
    }

    return hdmw;
}
#endif //UNIX
#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//--------------------------------------------------------------
//      PrintDlgW wrapper
//--------------------------------------------------------------

BOOL WINAPI
PrintDlgWrapW(LPPRINTDLGW lppd)
{
    BOOL        fRet;

    VALIDATE_PROTOTYPE_DELAYLOAD(PrintDlg, _PrintDlg);   
    
    if (UseUnicodeShell32())
    {
         fRet = _PrintDlgW(lppd);
    }
    else
    {    
        PRINTDLGA   pda;
        LPCWSTR     lpPrintTemplateName = lppd->lpPrintTemplateName;
        LPCWSTR     lpSetupTemplateName = lppd->lpSetupTemplateName;
        CStrIn      strPrintTemplateName( lpPrintTemplateName );
        CStrIn      strSetupTemplateName( lpSetupTemplateName );

        ASSERT( sizeof(pda) == sizeof( *lppd ));

        memcpy( &pda, lppd, sizeof(pda) );

        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win95 anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)
    
#ifndef UNIX
        // So instead of: pda.hDevMode = DevModeAFromDevModeW( lppd->hDevMode );
        // we just forward the DEVMODE handle:
        pda.hDevMode = lppd->hDevMode;
#else
    pda.hDevMode = DevModeAFromDevModeW( lppd->hDevMode );
#endif
        pda.hDevNames = DevNamesAFromDevNamesW( lppd->hDevNames );
        pda.lpPrintTemplateName = strPrintTemplateName;
        pda.lpSetupTemplateName = strSetupTemplateName;
    
        fRet = _PrintDlgA( &pda );
    
        // copy back wholesale, then restore strings.
    
        memcpy( lppd, &pda, sizeof(pda) );
    
        lppd->lpSetupTemplateName = lpSetupTemplateName;
        lppd->lpPrintTemplateName = lpPrintTemplateName;
        lppd->hDevNames = DevNamesWFromDevNamesA( pda.hDevNames );
    
#ifndef UNIX
        // And instead of: lppd->hDevMode = DevModeWFromDevModeA( pda.hDevMode );
        // we just forward the DEVMODE handle:
        lppd->hDevMode = pda.hDevMode;
#else
    lppd->hDevMode = DevModeWFromDevModeA( pda.hDevMode );
#endif 
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//--------------------------------------------------------------
//      PageSetupDlgW wrapper
//--------------------------------------------------------------

BOOL WINAPI
PageSetupDlgWrapW(LPPAGESETUPDLGW lppsd)
{
    BOOL fRet;

    VALIDATE_PROTOTYPE_DELAYLOAD(PageSetupDlg, _PageSetupDlg);     

    if (UseUnicodeShell32())
    {
        fRet = _PageSetupDlgW(lppsd);
    }
    else
    {   
        PAGESETUPDLGA   psda;
        LPCWSTR         lpPageSetupTemplateName = lppsd->lpPageSetupTemplateName;
        CStrIn          strPageSetupTemplateName( lpPageSetupTemplateName );
    
        ASSERT( sizeof(psda) == sizeof( *lppsd ) );
    
        memcpy( &psda, lppsd, sizeof(psda));
    
        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win95 anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)
    
#ifndef UNIX
        // So instead of: psda.hDevMode = DevModeAFromDevModeW( lppsd->hDevMode );
        // we just forward the DEVMODE handle:
        psda.hDevMode = lppsd->hDevMode;
#else
    psda.hDevMode = DevModeAFromDevModeW( lppsd->hDevMode );
#endif
        psda.hDevNames = DevNamesAFromDevNamesW( lppsd->hDevNames );
        psda.lpPageSetupTemplateName = strPageSetupTemplateName;
    
        fRet = _PageSetupDlgA( (LPPAGESETUPDLGA) &psda );
    
        // copy back wholesale, then restore string.
    
        memcpy( lppsd, &psda, sizeof(psda) );
    
        lppsd->lpPageSetupTemplateName = lpPageSetupTemplateName;
        lppsd->hDevNames = DevNamesWFromDevNamesA( psda.hDevNames );
    
#ifndef UNIX
        // And instead of: lppsd->hDevMode = DevModeWFromDevModeA( psda.hDevMode );
        // we just forward the DEVMODE handle:
        lppsd->hDevMode = psda.hDevMode;            
#else
    lppsd->hDevMode = DevModeWFromDevModeA( psda.hDevMode );
#endif
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_OLE32_WRAPPER

//  CLSIDFromXXX puke when input is >= 248
#define SAFE_OLE_BUF_LEN    247

HRESULT WINAPI CLSIDFromStringWrap(LPOLESTR lpsz, LPCLSID pclsid)
{
#if defined(_X86_)
    VALIDATE_PROTOTYPEX(CLSIDFromString);
#endif

    return GUIDFromStringW(lpsz, pclsid) ? S_OK : E_INVALIDARG;
}

HRESULT WINAPI CLSIDFromProgIDWrap(LPCOLESTR lpszProgID, LPCLSID lpclsid)
{
    HRESULT hr;
    
    VALIDATE_PROTOTYPE_DELAYLOADX(CLSIDFromProgID, _CLSIDFromProgID);
   
    if (lstrlenW(lpszProgID) < SAFE_OLE_BUF_LEN)
    {
        hr = _CLSIDFromProgID(lpszProgID, lpclsid);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

#endif // NEED_OLE32_WRAPPER


//************************************************************************
// function GETLONGPATHNAME :
//
// These wrappers are needed on all platforms (x86 and Alpha) because they
// implement the functionality for the Kernel32 function GetLongPathName that 
// exists only on Win2K (Unicode/Ansi) and Win98 (Ansi only). Hence these
// wrappers provide this functionality for lower level platforms (NT4 & Win95)
//

#define achGETLONGPATHNAMEA     "GetLongPathNameA"
typedef DWORD (*PROC_GETLONGPATHNAMEA) (LPCSTR, LPSTR, DWORD);

LWSTDAPI_(DWORD)
GetLongPathNameWrapA(
        LPCSTR lpszShortPath,
        LPSTR lpszLongPath, 
        DWORD cchBuffer)
{
    VALIDATE_PROTOTYPE(GetLongPathName);

    // If NT5 or Win98, use the system API for ANSI
    if (g_bRunningOnNT5OrHigher || g_bRunningOnMemphis)
    {
        static PROC_GETLONGPATHNAMEA s_fpGLPNA = NULL;

        if (!s_fpGLPNA)
        {
            // This codepath is used for Memphis also, hence need to use the
            // wrapper for GetModuleHandle.
            s_fpGLPNA = (PROC_GETLONGPATHNAMEA)GetProcAddress(GetModuleHandleWrap(L"kernel32"),achGETLONGPATHNAMEA);
        }

        ASSERT(s_fpGLPNA);
        
        if (s_fpGLPNA)
        {
            return s_fpGLPNA(lpszShortPath, lpszLongPath, cchBuffer);
        }
    }

    // Otherwise use our own logic to do the conversion....
    BOOL    bCountMode = FALSE;
    CHAR    cTmp;
    HANDLE  hFind;
    LPSTR   pTmp = NULL;
    DWORD   dwReturnLen = 0;    
    WIN32_FIND_DATAA Find_Data;  
    UINT    PrevErrorMode;

    // Since we are going to be touchin the media, turn off file error
    // pop-ups. Remember the current mode and set it back in the end.
    PrevErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    if (lpszLongPath == NULL)
    {    // No OUT buffer provided ==> counting mode.
        bCountMode = TRUE;
    }
    else
    {    // Initialize OUT buffer.
        *lpszLongPath = '\0';
    }

    // Validate the input parameters...
    if ((lpszShortPath == NULL) 
        || (0xFFFFFFFF == GetFileAttributesA(lpszShortPath)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        dwReturnLen = 0;
        goto Done;
    }         
                                                                    
    if (PathIsUNCA(lpszShortPath))
    {
        // This is a UNC Path, don't know how to handle
        SetLastError(ERROR_INVALID_PARAMETER);
        dwReturnLen = 0;
        goto Done;
    }

    // Input must be a full path.
    // Since Drive letter cannot be multi-byte, just check 2nd and 3rd chars.
    if ( lpszShortPath[1] != ':' || 
         lpszShortPath[2] != '\\')
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        dwReturnLen = 0;
        goto Done;
    }

    // Create the drive root for the LFN.
    if ( !bCountMode && cchBuffer >= 3)
    {   // Copy the drive letter.
        StrCpyNA(lpszLongPath, lpszShortPath, 3);
    }
    else
    {   // Supplied buffer is so small, can't even copy the drive letter!!
        bCountMode = TRUE;
    }
    dwReturnLen += 2;


    // Create a local copy of the input Short name to party on.
    // Also, as per the documentation, OUT buffer can be the same as the IN.
    // Hence need to save the IN buffer before we start filling the OUT.
    CHAR lpLocalCopy[MAX_PATH];
    StrCpyNA(lpLocalCopy, lpszShortPath, MAX_PATH);

    // Now starts the main processing....
    // Skip past the root backslash in lpszShortPath.
    pTmp = lpLocalCopy+3;
 
    while (*pTmp)
    {
        // Get the next Backslash
        pTmp = StrChrA(pTmp, L'\\');
        if ( pTmp == NULL)
        {
            // Fell off the end of the str. So point pTmp to the terminating \0
            pTmp = lpLocalCopy + lstrlenA(lpLocalCopy);
        }

        cTmp = *pTmp;
        *pTmp = '\0';

        hFind = FindFirstFileA(lpLocalCopy, &Find_Data);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            dwReturnLen += lstrlenA(Find_Data.cFileName);
            dwReturnLen += 1;  // Plus for the '\' introduced by PathCombine.
            if (!bCountMode && dwReturnLen < cchBuffer)
            {   // Add the LFN to the path
                PathCombineA(lpszLongPath, lpszLongPath, Find_Data.cFileName);
            }
            else
            {   // We are out of buffer space. Continue loop to find space reqd.
                bCountMode = TRUE;
                *lpszLongPath = '\0';
            }
        }
        else
        {
            //Error: Input path does not exist. The earlier check should catch
            //this
            SetLastError(ERROR_INVALID_PARAMETER);
            *lpszLongPath = '\0';
            *pTmp = cTmp;
            dwReturnLen = 0;
            goto Done;
        }

        *pTmp = cTmp;
      
        if (*pTmp)
            pTmp = CharNextA(pTmp);
    }

Done:
    // restore error mode.
    SetErrorMode(PrevErrorMode);

    if ( dwReturnLen && bCountMode)
    {   // If everything OK (dwReturnLen!=0) and Counting Mode, add one.
        return (dwReturnLen+1);  
    }
    else
    {   // Return error (dwReturnLen==0) Or 
        // No. of chars (dwReturnLen != 0 && bCountMode == FALSE)
        return (dwReturnLen);   
    }
}

#define achGETLONGPATHNAMEW     "GetLongPathNameW"
typedef DWORD (*PROC_GETLONGPATHNAMEW) (LPCWSTR, LPWSTR, DWORD);

LWSTDAPI_(DWORD)
GetLongPathNameWrapW(
        LPCWSTR lpszShortPath,
        LPWSTR lpszLongPath,
        DWORD cchBuffer)
{
    VALIDATE_PROTOTYPE(GetLongPathName);

    // If we are running on NT5, use the system API for Unicode
    if (g_bRunningOnNT5OrHigher)
    {
        static PROC_GETLONGPATHNAMEW s_fpGLPNW = NULL;

        if (!s_fpGLPNW)
        {
            s_fpGLPNW = (PROC_GETLONGPATHNAMEW)GetProcAddress(GetModuleHandle(TEXT("kernel32")),achGETLONGPATHNAMEW);
        }

        ASSERT(s_fpGLPNW);
        
        if (s_fpGLPNW)
        {
            return s_fpGLPNW(lpszShortPath, lpszLongPath, cchBuffer);
        }
    }

    // All other platforms, convert UNICODE inputs to ANSI and use the
    // ANSI wrapper.
    CStrIn  strShortPath(lpszShortPath);
    CStrOut strLongPath(lpszLongPath, cchBuffer);

    // DWORD dwRet = GetLongPathNameWrapA(strShortPath, strLongPath, strLongPath.BufSize());
    DWORD dwRet = GetLongPathNameWrapA(strShortPath, strLongPath, cchBuffer);
    if (dwRet != 0)
    {   // If the call succeeded, thunk back the size.
        if (dwRet < (DWORD)cchBuffer)
        {   // Succeeded in getting LFN value in the OUT buffer. Thunk it back
            // to Unicode.
            dwRet = strLongPath.ConvertIncludingNul() - 1;
        }
        // else ==> buffer small, need dwRet character space.
    }

    return dwRet;
}

// End of GETLONGPATHNAME wrapper implementation.
// NOTE: This wrapper is to be built on all platforms.
//************************************************************************
