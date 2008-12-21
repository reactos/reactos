#ifndef __WINE_WINE_EXCEPTION_H
#define __WINE_WINE_EXCEPTION_H

#include <intrin.h>
#include <pseh/pseh2.h>
#include <pseh/excpt.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _EXCEPTION_REGISTRATION_RECORD;
typedef struct _EXCEPTION_REGISTRATION_RECORD EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD * Prev;
    PEXCEPTION_HANDLER Handler;
};

#define __TRY _SEH2_TRY
#define __EXCEPT(func) _SEH2_EXCEPT(func(_SEH2_GetExceptionInformation()))
#define __EXCEPT_PAGE_FAULT _SEH2_EXCEPT(_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION)
#define __EXCEPT_ALL _SEH2_EXCEPT(_SEH_EXECUTE_HANDLER)
#define __ENDTRY _SEH2_END
#define __FINALLY(func) _SEH2_FINALLY { func(!_SEH2_AbnormalTermination()); }

#ifndef GetExceptionCode
#define GetExceptionCode() _SEH2_GetExceptionCode()
#endif

#ifndef GetExceptionInformation
#define GetExceptionInformation() _SEH2_GetExceptionInformation()
#endif

#ifndef AbnormalTermination
#define AbnormalTermination() _SEH2_AbnormalTermination()
#endif

#define EXCEPTION_WINE_STUB       0x80000100
#define EXCEPTION_WINE_ASSERTION  0x80000101

#define EXCEPTION_VM86_INTx       0x80000110
#define EXCEPTION_VM86_STI        0x80000111
#define EXCEPTION_VM86_PICRETURN  0x80000112

static inline EXCEPTION_REGISTRATION_RECORD *__wine_push_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
    frame->Prev = (struct _EXCEPTION_REGISTRATION_RECORD *)__readfsdword(0);
	__writefsdword(0, (unsigned long)frame);
    return frame->Prev;
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
	__writefsdword(0, (unsigned long)frame->Prev);
    return frame->Prev;
}

extern void __wine_enter_vm86( CONTEXT *context );

#ifdef __cplusplus
}
#endif

#endif
