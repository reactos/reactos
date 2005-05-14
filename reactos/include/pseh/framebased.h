/*
 Copyright (c) 2004/2005 KJK::Hyperion
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef KJK_PSEH_FRAMEBASED_H_
#define KJK_PSEH_FRAMEBASED_H_

#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>

#ifndef offsetof
# include <stddef.h>
#endif

/*
 Fall back to non-optimal, non-native NLG implementation for environments
 without their own (e.g., currently, kernel-mode ReactOS/Windows). THIS IS NOT
 RECOMMENDED AND IT WILL BE DROPPED IN A FUTURE VERSION BECAUSE IT MAY CAUSE
 SEVERE STACK CORRUPTION. REIMPLEMENT OR PORT YOUR COMPILER'S NATIVE NLG
 IMPLEMENTATION INSTEAD.
*/
#ifdef _SEH_NO_NATIVE_NLG
# include <pseh/setjmp.h>
#else
# include <setjmp.h>
# define _SEHLongJmp longjmp
# define _SEHSetJmp setjmp
# define _SEHJmpBuf_t jmp_buf
#endif
unsigned long DbgPrint(char * Format,...);
typedef struct __SEHFrame
{
 _SEHPortableFrame_t SEH_Header;
 void * SEH_Locals;
}
_SEHFrame_t;

typedef struct __SEHTryLevel
{
 _SEHPortableTryLevel_t ST_Header;
 _SEHJmpBuf_t ST_JmpBuf;
}
_SEHTryLevel_t;

static __declspec(noreturn) __inline void __stdcall _SEHCompilerSpecificHandler
(
 _SEHPortableTryLevel_t * trylevel
)
{
 _SEHTryLevel_t * mytrylevel;
 mytrylevel = _SEH_CONTAINING_RECORD(trylevel, _SEHTryLevel_t, ST_Header);
 _SEHLongJmp(mytrylevel->ST_JmpBuf, 1);
}

static const int _SEHScopeKind = 1;
static _SEHPortableFrame_t * const _SEHPortableFrame = 0;

/* SHARED LOCALS */
/* Access the locals for the current frame */
#define _SEH_ACCESS_LOCALS(LOCALS_) \
 _SEH_LOCALS_TYPENAME(LOCALS_) * _SEHPLocals; \
 _SEHPLocals = \
  _SEH_PVOID_CAST \
  ( \
   _SEH_LOCALS_TYPENAME(LOCALS_) *, \
   _SEH_CONTAINING_RECORD(_SEHPortableFrame, _SEHFrame_t, SEH_Header) \
    ->SEH_Locals \
  );

/* Access local variable VAR_ */
#define _SEH_VAR(VAR_) _SEHPLocals->VAR_

/* FILTER FUNCTIONS */
/* Declares a filter function's prototype */
#define _SEH_FILTER(NAME_) \
 long __stdcall NAME_ \
 ( \
  struct _EXCEPTION_POINTERS * _SEHExceptionPointers, \
  struct __SEHPortableFrame * _SEHPortableFrame \
 )

/* Declares a static filter */
#define _SEH_STATIC_FILTER(ACTION_) ((_SEHFilter_t)((ACTION_) + 2))

/* Declares a PSEH filter wrapping a regular filter function */
#define _SEH_WRAP_FILTER(WRAPPER_, NAME_) \
 static __inline _SEH_FILTER(WRAPPER_) \
 { \
  return (NAME_)(_SEHExceptionPointers); \
 }

/* FINALLY FUNCTIONS */
/* Declares a finally function's prototype */
#define _SEH_FINALLYFUNC(NAME_) \
 void __stdcall NAME_ \
 ( \
  struct __SEHPortableFrame * _SEHPortableFrame \
 )

/* Declares a PSEH finally function wrapping a regular function */
#define _SEH_WRAP_FINALLY(WRAPPER_, NAME_) \
 _SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ())

#define _SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ARGS_) \
 static __inline _SEH_FINALLYFUNC(WRAPPER_) \
 { \
  NAME_ ARGS_; \
 }

#define _SEH_WRAP_FINALLY_LOCALS_ARGS(WRAPPER_, LOCALS_, NAME_, ARGS_) \
 static __inline _SEH_FINALLYFUNC(WRAPPER_) \
 { \
  _SEH_ACCESS_LOCALS(LOCALS_); \
  NAME_ ARGS_; \
 }

/* SAFE BLOCKS */
#define _SEHX_TRY_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  _SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH), \
  (FINALLY_) \
 )

#define _SEHX_END_FINALLY _SEH_HANDLE _SEH_END

#define _SEHX_TRY_FILTER(FILTER_) \
 _SEH_TRY_FILTER_FINALLY((FILTER_), 0)

#define _SEHX_TRY_HANDLE_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  _SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER), \
  (FINALLY_) \
 )

#define _SEHX_TRY \
 _SEH_TRY_HANDLE_FINALLY(0)

#ifdef __cplusplus
# define _SEH_DECLARE_HANDLERS(FILTER_, FINALLY_) \
  static const _SEHHandlers_t _SEHHandlers = { (FILTER_), (FINALLY_) };
#else
# define _SEH_DECLARE_HANDLERS(FILTER_, FINALLY_) \
  _SEHHandlers_t _SEHHandlers = { (0), (0) };                                  \
  _SEHHandlers.SH_Filter = (FILTER_);                                          \
  _SEHHandlers.SH_Finally = (FINALLY_);
#endif

#define _SEHX_TRY_FILTER_FINALLY(FILTER_, FINALLY_) \
 {                                                                             \
  _SEHPortableFrame_t * const _SEHCurPortableFrame = _SEHPortableFrame;        \
                                                                               \
  {                                                                            \
   _SEHFrame_t _SEHFrame;                                                      \
   _SEHTryLevel_t _SEHTryLevel;                                                \
   _SEHPortableFrame_t * const _SEHPortableFrame =                             \
    _SEHScopeKind ? &_SEHFrame.SEH_Header : _SEHCurPortableFrame;              \
                                                                               \
   (void)_SEHPortableFrame;                                                    \
                                                                               \
   _SEH_DECLARE_HANDLERS((FILTER_), (FINALLY_));                               \
                                                                               \
   _SEHTryLevel.ST_Header.SPT_Handlers = &_SEHHandlers;                        \
                                                                               \
   if(_SEHScopeKind)                                                           \
   {                                                                           \
    if(&_SEHLocals != _SEHDummyLocals)                                         \
     _SEHFrame.SEH_Locals = &_SEHLocals;                                       \
                                                                               \
    _SEHFrame.SEH_Header.SPF_Handler = _SEHCompilerSpecificHandler;            \
    _SEHEnterFrame(&_SEHFrame.SEH_Header, &_SEHTryLevel.ST_Header);            \
   }                                                                           \
   else                                                                        \
    _SEHEnterTry(&_SEHTryLevel.ST_Header);                                     \
                                                                               \
   {                                                                           \
    static const int _SEHScopeKind = 0;                                        \
    (void)_SEHScopeKind;                                                       \
                                                                               \
    if(_SEHSetJmp(_SEHTryLevel.ST_JmpBuf) == 0)                                \
    {                                                                          \
     for(;;)                                                                   \
     {

#define _SEHX_HANDLE \
                                                                               \
      break;                                                                   \
     }                                                                         \
                                                                               \
     _SEHLeave();                                                              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
     _SEHLeave();

#define _SEHX_END \
    }                                                                          \
                                                                               \
    if(_SEHHandlers.SH_Finally)                                                \
     _SEHHandlers.SH_Finally(_SEHPortableFrame);                               \
   }                                                                           \
  }                                                                            \
 }

#define _SEHX_LEAVE break

#define _SEHX_GetExceptionCode() (unsigned long)(_SEHPortableFrame->SPF_Code)

#define _SEHX_GetExceptionPointers() \
 ((struct _EXCEPTION_POINTERS *)_SEHExceptionPointers)

#define _SEHX_AbnormalTermination() (_SEHPortableFrame->SPF_Code != 0)

/* New syntax */

/*
 NOTE: do not move, remove or modify any instance of _SEH2_ASSUME and
 _SEH2_ASSUMING without doing extensive tests for correctness. Compilers can
 generate the wrong code in presence of __assume in unpredictable ways. BE SURE
 IT DOESN'T HAPPEN
*/
#if defined(_MSC_VER) && (_MSC_VER > 1200)
# define _SEH2_ASSUME(X_) __assume(X_)
# if !defined(_SEH_NO_NATIVE_NLG)
 /*
  If we use the native setjmp, the compiler stops keeping track of variables, so
  their actual values don't matter anymore. Optimize out some assignments
 */
#  define _SEH2_ASSUMING(X_)
# else
 /* No native setjmp, no magic, no assumptions. Actually set the values */
#  define _SEH2_ASSUMING(X_) X_
# endif
#else
# define _SEH2_ASSUME(X_)
# define _SEH2_ASSUMING(X_) X_
#endif

#ifdef __cplusplus
# define _SEH2_INIT_CONST static const
#else
# define _SEH2_INIT_CONST register const
#endif

#define _SEH_LEAVE break

#define _SEH_TRY \
 {                                                                             \
  _SEH2_INIT_CONST int _SEH2TopTryLevel = (_SEHScopeKind != 0);                \
  _SEHPortableFrame_t * const _SEH2CurPortableFrame = _SEHPortableFrame;       \
                                                                               \
  {                                                                            \
   static const int _SEHScopeKind = 0;                                         \
   register int _SEH2State = 0;                                                \
   register int _SEH2Handle = 0;                                               \
   _SEHFrame_t _SEH2Frame;                                                     \
   _SEHTryLevel_t _SEH2TryLevel;                                               \
   _SEHPortableFrame_t * const _SEHPortableFrame =                             \
    _SEH2TopTryLevel ? &_SEH2Frame.SEH_Header : _SEH2CurPortableFrame;         \
                                                                               \
   (void)_SEHScopeKind;                                                        \
   (void)_SEHPortableFrame;                                                    \
   (void)_SEH2Handle;                                                          \
                                                                               \
   for(;;)                                                                     \
   {                                                                           \
    if(_SEH2State)                                                             \
    {                                                                          \
     for(;;)                                                                   \
     {                                                                         \
      {

#define _SEH_EXCEPT(FILTER_) \
      }                                                                        \
                                                                               \
      break;                                                                   \
     }                                                                         \
                                                                               \
     _SEH2_ASSUME(_SEH2Handle == 0);                                           \
     break;                                                                    \
    }                                                                          \
    else                                                                       \
    {                                                                          \
     _SEH_DECLARE_HANDLERS((FILTER_), 0);                                      \
                                                                               \
     _SEH2TryLevel.ST_Header.SPT_Handlers = &_SEHHandlers;                     \
                                                                               \
     if(_SEH2TopTryLevel)                                                      \
     {                                                                         \
      if(&_SEHLocals != _SEHDummyLocals)                                       \
       _SEH2Frame.SEH_Locals = &_SEHLocals;                                    \
                                                                               \
      _SEH2Frame.SEH_Header.SPF_Handler = _SEHCompilerSpecificHandler;         \
      _SEHEnterFrame(&_SEH2Frame.SEH_Header, &_SEH2TryLevel.ST_Header);        \
     }                                                                         \
     else                                                                      \
      _SEHEnterTry(&_SEH2TryLevel.ST_Header);                                  \
                                                                               \
     if((_SEH2Handle = _SEHSetJmp(_SEH2TryLevel.ST_JmpBuf)) == 0)              \
     {                                                                         \
      _SEH2_ASSUMING(++ _SEH2State);                                           \
      _SEH2_ASSUME(_SEH2State != 0);                                           \
      continue;                                                                \
     }                                                                         \
     else                                                                      \
     {                                                                         \
      break;                                                                   \
     }                                                                         \
    }                                                                          \
                                                                               \
    break;                                                                     \
   }                                                                           \
                                                                               \
   _SEHLeave();                                                                \
                                                                               \
   if(_SEH2Handle) \
   {

#define _SEH_FINALLY(FINALLY_) \
      }                                                                        \
                                                                               \
      break;                                                                   \
     }                                                                         \
                                                                               \
     _SEHLeave();                                                              \
     break;                                                                    \
    }                                                                          \
    else                                                                       \
    {                                                                          \
     _SEH_DECLARE_HANDLERS(0, (FINALLY_));                                     \
                                                                               \
     _SEH2TryLevel.ST_Header.SPT_Handlers = &_SEHHandlers;                     \
                                                                               \
     if(_SEH2TopTryLevel)                                                      \
     {                                                                         \
      if(&_SEHLocals != _SEHDummyLocals)                                       \
       _SEH2Frame.SEH_Locals = &_SEHLocals;                                    \
                                                                               \
      _SEH2Frame.SEH_Header.SPF_Handler = 0;                                   \
      _SEHEnterFrame(&_SEH2Frame.SEH_Header, &_SEH2TryLevel.ST_Header);        \
     }                                                                         \
     else                                                                      \
      _SEHEnterTry(&_SEH2TryLevel.ST_Header);                                  \
                                                                               \
     ++ _SEH2State;                                                            \
     _SEH2_ASSUME(_SEH2State != 0);                                            \
     continue;                                                                 \
    }                                                                          \
                                                                               \
    break;                                                                     \
   }                                                                           \
                                                                               \
   (FINALLY_)(&_SEH2Frame.SEH_Header);                                         \
   if(0)                                                                       \
   {

#define _SEH_END \
   } \
  }                                                                            \
 }

#define _SEH_HANDLE _SEH_EXCEPT(_SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER))

#define _SEH_GetExceptionCode     _SEHX_GetExceptionCode
#define _SEH_GetExceptionPointers _SEHX_GetExceptionPointers
#define _SEH_AbnormalTermination  _SEHX_AbnormalTermination

#endif

/* EOF */
