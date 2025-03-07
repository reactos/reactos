#ifndef __WINE_WINE_EXCEPTION_H
#define __WINE_WINE_EXCEPTION_H

#include <setjmp.h>
#include <intrin.h>
#include <excpt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Win32 seems to use the same flags as ExceptionFlags in an EXCEPTION_RECORD */
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

#ifndef _RTLTYPES_H
struct _EXCEPTION_REGISTRATION_RECORD;

typedef
DWORD
(*PEXCEPTION_HANDLER)(
    struct _EXCEPTION_RECORD*,
    struct _EXCEPTION_REGISTRATION_RECORD *,
    struct _CONTEXT*,
    struct _EXCEPTION_REGISTRATION_RECORD**);

typedef struct _EXCEPTION_REGISTRATION_RECORD EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD * Prev;
    PEXCEPTION_HANDLER Handler;
};
#else
typedef struct _WINE_EXCEPTION_REGISTRATION_RECORD
{
    PVOID Prev;
    PEXCEPTION_ROUTINE Handler;
} WINE_EXCEPTION_REGISTRATION_RECORD, *PWINE_EXCEPTION_REGISTRATION_RECORD;

#define _EXCEPTION_REGISTRATION_RECORD _WINE_EXCEPTION_REGISTRATION_RECORD
#define EXCEPTION_REGISTRATION_RECORD WINE_EXCEPTION_REGISTRATION_RECORD
#define PEXCEPTION_REGISTRATION_RECORD PWINE_EXCEPTION_REGISTRATION_RECORD
#endif

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
    frame->Prev = (struct _EXCEPTION_REGISTRATION_RECORD *)__readfsdword(0);
	__writefsdword(0, (unsigned long)frame);
    return frame->Prev;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->Prev = teb->ExceptionList;
    teb->ExceptionList = (PVOID)frame;
    return frame->Prev;
#endif
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#ifdef __i386__
	__writefsdword(0, (unsigned long)frame->Prev);
    return frame->Prev;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->Prev = teb->ExceptionList;
    teb->ExceptionList = (PVOID)frame;
    return frame->Prev;
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

extern void __wine_enter_vm86( CONTEXT *context );

#ifdef __cplusplus
}
#endif

#endif
