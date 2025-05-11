/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <winnls.h>

/* <wine/debug.h> */
#if DBG
    #ifndef __RELFILE__
        #define __RELFILE__ __FILE__
    #endif

    #define ERR_LEVEL   0x1
    #define TRACE_LEVEL 0x8

    #define WINE_DEFAULT_DEBUG_CHANNEL(x) \
        static PCSTR DbgDefaultChannel = #x; \
        static const PCSTR * const _DbgDefaultChannel_ = &DbgDefaultChannel;

    BOOL IntIsDebugChannelEnabled(_In_ PCSTR channel);

    ULONG __cdecl DbgPrint(_In_z_ _Printf_format_string_ PCSTR Format, ...);

    #define DBG_PRINT(ch, level, tag, fmt, ...) (void)( \
        (((level) == ERR_LEVEL) || IntIsDebugChannelEnabled(ch)) ? \
        (DbgPrint("(%s:%d) %s" fmt, __RELFILE__, __LINE__, (tag), ##__VA_ARGS__), FALSE) : TRUE \
    )

    #define TRACE_ON(ch) IntIsDebugChannelEnabled(#ch)
    #define ERR(fmt, ...)   DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "err: ",   fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)  DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "warn: ",  fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...) DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "fixme: ", fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...) DBG_PRINT(DbgDefaultChannel, TRACE_LEVEL, "",        fmt, ##__VA_ARGS__)

    #define UNIMPLEMENTED FIXME("%s is unimplemented", __FUNCTION__);

    PCSTR debugstr_an(_In_opt_ PCSTR pszA, _In_ INT cchA);
    PCSTR debugstr_wn(_In_opt_ PCWSTR pszW, _In_ INT cchW);
    PCSTR debugstr_guid(_In_opt_ const GUID *id);
    PCSTR wine_dbgstr_rect(_In_opt_ LPCRECT prc);
    PCSTR wine_dbg_sprintf(_In_ PCSTR format, ...);

    #define debugstr_a(pszA) debugstr_an((pszA), -1)
    #define debugstr_w(pszW) debugstr_wn((pszW), -1)
#else
    #define WINE_DEFAULT_DEBUG_CHANNEL(x)
    #define IntIsDebugChannelEnabled(channel) FALSE
    #define DBG_PRINT(ch, level)
    #define TRACE_ON(ch) FALSE
    #define ERR(fmt, ...)
    #define WARN(fmt, ...)
    #define FIXME(fmt, ...)
    #define TRACE(fmt, ...)
    #define UNIMPLEMENTED
    #define debugstr_a(pszA) ((PCSTR)NULL)
    #define debugstr_w(pszW) ((PCSTR)NULL)
    #define debugstr_an(pszA, cchA) ((PCSTR)NULL)
    #define debugstr_wn(pszW, cchW) ((PCSTR)NULL)
    #define debugstr_guid(id) ((PCSTR)NULL)
    #define wine_dbgstr_rect(prc) ((PCSTR)NULL)
    #define wine_dbg_sprintf(format, ... ) ((PCSTR)NULL)
#endif

/* <wine/unicode.h> */
#define memicmpW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strlenW(s) wcslen((s))
#define strcpyW(d,s) wcscpy((d),(s))
#define strcatW(d,s) wcscat((d),(s))
#define strcspnW(d,s) wcscspn((d),(s))
#define strstrW(d,s) wcsstr((d),(s))
#define strtolW(s,e,b) wcstol((s),(e),(b))
#define strchrW(s,c) wcschr((s),(c))
#define strrchrW(s,c) wcsrchr((s),(c))
#define strncmpW(s1,s2,n) wcsncmp((s1),(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((s1),(s2),(n))
#define strcmpW(s1,s2) wcscmp((s1),(s2))
#define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((s1),(s2),(b))
#define strspnW(str, accept) wcsspn((str),(accept))
#define strpbrkW(str, accept) wcspbrk((str),(accept))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define iscntrlW(n) iswcntrl((n))
#define atoiW(s) _wtoi((s))
#define atolW(s) _wtol((s))
#define strlwrW(s) _wcslwr((s))
#define struprW(s) _wcsupr((s))
#define sprintfW _swprintf
#define vsprintfW _vswprintf
#define snprintfW _snwprintf
#define vsnprintfW _vsnwprintf
#define isprintW iswprint

/* <wine/exception.h> */
#include <ndk/rtltypes.h>
#include <setjmp.h>
#include <intrin.h>
#include <excpt.h>

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10
#define EH_TARGET_UNWIND    0x20
#define EH_COLLIDED_UNWIND  0x40

#define EXCEPTION_WINE_STUB       0x80000100
#define EXCEPTION_WINE_ASSERTION  0x80000101
#define EXCEPTION_VM86_INTx       0x80000110
#define EXCEPTION_VM86_STI        0x80000111
#define EXCEPTION_VM86_PICRETURN  0x80000112

#define __TRY _SEH2_TRY
#define __EXCEPT(func) _SEH2_EXCEPT(func(_SEH2_GetExceptionInformation()))
#define __EXCEPT_CTX(func, ctx) _SEH2_EXCEPT((func)(GetExceptionInformation(), ctx))
#define __EXCEPT_PAGE_FAULT _SEH2_EXCEPT(_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION)
#define __EXCEPT_ALL _SEH2_EXCEPT(1)
#define __ENDTRY _SEH2_END
#define __FINALLY(func) _SEH2_FINALLY { func(!_SEH2_AbnormalTermination()); }
#define __FINALLY_CTX(func, ctx) _SEH2_FINALLY { func(!_SEH2_AbnormalTermination(), ctx); }; _SEH2_END

#ifndef GetExceptionCode
#define GetExceptionCode() _SEH2_GetExceptionCode()
#endif

#ifndef GetExceptionInformation
#define GetExceptionInformation() _SEH2_GetExceptionInformation()
#endif

#ifndef AbnormalTermination
#define AbnormalTermination() _SEH2_AbnormalTermination()
#endif

#if defined(__MINGW32__) || defined(__CYGWIN__)
#define sigjmp_buf jmp_buf
#define sigsetjmp(buf,sigs) setjmp(buf)
#define siglongjmp(buf,val) longjmp(buf,val)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4733)
#endif

static inline EXCEPTION_REGISTRATION_RECORD *__wine_push_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#ifdef __i386__
    frame->Next = (struct _EXCEPTION_REGISTRATION_RECORD *)__readfsdword(0);
    __writefsdword(0, (unsigned long)frame);
    return frame->Next;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->Next = teb->ExceptionList;
    teb->ExceptionList = frame;
    return frame->Next;
#endif
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#ifdef __i386__
    __writefsdword(0, (unsigned long)frame->Next);
    return frame->Next;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->Next = teb->ExceptionList;
    teb->ExceptionList = frame;
    return frame->Next;
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
