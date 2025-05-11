/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Mimic of <wine/exception.h> without Wine dependency
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

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
