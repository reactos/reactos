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

#ifndef KJK_PSEH_NATIVE_H_
#define KJK_PSEH_NATIVE_H_

#include <excpt.h>
#include <pseh/excpt.h>

/*
 Note: just define __inline to an empty symbol if your C compiler doesn't
 support it
*/
#ifdef __cplusplus
# ifndef __inline
#  define __inline inline
# endif
#endif

typedef long (__stdcall * _SEHFilter_t)
(
 long,
 struct _EXCEPTION_POINTERS *,
 void *
);

typedef void (__stdcall * _SEHFinally_t)
(
 int,
 void *
);

static __inline long _SEHCallFilter
(
 _SEHFilter_t _SEHFilter,
 long _SEHExceptionCode,
 struct _EXCEPTION_POINTERS * _SEHExceptionPointers,
 void * _SEHPVLocals
)
{
 if(_SEHFilter == _SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER))
  return _SEH_EXECUTE_HANDLER;
 else if(_SEHFilter == _SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH))
  return _SEH_CONTINUE_SEARCH;
 else if(_SEHFilter == _SEH_STATIC_FILTER(_SEH_CONTINUE_EXECUTION))
  return _SEH_CONTINUE_EXECUTION;
 else if(_SEHFilter)
  return _SEHFilter(_SEHExceptionCode, _SEHExceptionPointers, _SEHPVLocals);
 else
  return _SEH_CONTINUE_SEARCH;
}

static __inline void _SEHCallFinally
(
 _SEHFinally_t _SEHFinally,
 int _SEHAbnormalTermination,
 void * _SEHPVLocals
)
{
 if(_SEHFinally)
  (_SEHFinally)(_SEHAbnormalTermination, _SEHPVLocals);
}

/* SHARED LOCALS */
/* Access the locals for the current frame */
#define _SEH_ACCESS_LOCALS(LOCALS_) \
 _SEH_LOCALS_TYPENAME(LOCALS_) * _SEHPLocals; \
 _SEHPLocals = _SEH_PVOID_CAST(_SEH_LOCALS_TYPENAME(LOCALS_) *, _SEHPVLocals);

/* Access local variable VAR_ */
#define _SEH_VAR(VAR_) _SEHPLocals->VAR_

/* FILTER FUNCTIONS */
/* Declares a filter function's prototype */
#define _SEH_FILTER(NAME_) \
 long __stdcall NAME_ \
 ( \
  long _SEHExceptionCode, \
  struct _EXCEPTION_POINTERS * _SEHExceptionPointers, \
  void * _SEHPVLocals \
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
#define _SEH_FINALLY(NAME_) \
 void __stdcall NAME_ \
 ( \
  int _SEHAbnormalTermination, \
  void * _SEHPVLocals \
 )

/* Declares a PSEH finally function wrapping a regular function */
#define _SEH_WRAP_FINALLY(WRAPPER_, NAME_) \
 _SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ())

#define _SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ARGS_) \
 static __inline _SEH_FINALLY(WRAPPER_) \
 { \
  NAME_ ARGS_; \
 }

#define _SEH_WRAP_FINALLY_LOCALS_ARGS(WRAPPER_, LOCALS_, NAME_, ARGS_) \
 static __inline _SEH_FINALLY(WRAPPER_) \
 { \
  _SEH_ACCESS_LOCALS(LOCALS_); \
  NAME_ ARGS_; \
 }

/* SAFE BLOCKS */
#define _SEH_TRY_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  _SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH), \
  (FINALLY_) \
 )

#define _SEH_END_FINALLY _SEH_HANDLE _SEH_END

#define _SEH_TRY_FILTER(FILTER_) \
 _SEH_TRY_FILTER_FINALLY((FILTER_), NULL)

#define _SEH_TRY_HANDLE_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  _SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER), \
  (FINALLY_) \
 )

#define _SEH_TRY \
 _SEH_TRY_HANDLE_FINALLY(NULL)

#define _SEH_CALL_FILTER(FILTER_) \
 _SEHCallFilter                                                                \
 (                                                                             \
  (FILTER_),                                                                   \
  GetExceptionCode(),                                                          \
  GetExceptionPointers(),                                                      \
  _SEHPVLocals                                                                 \
 )

#define _SEH_CALL_FINALLY(FINALLY_) \
 _SEHCallFinally((FINALLY_), (AbnormalTermination() != 0), _SEHPVLocals)

#define _SEH_TRY_FILTER_FINALLY(FILTER_, FINALLY_) \
 __try                                                                         \
 {                                                                             \
  _SEHFinally_t _SEHFinally = (FINALLY_);                                      \
  _SEHFilter_t _SEHFilter = (FILTER_);                                         \
  void * _SEHPVLocals = &_SEHLocals;                                           \
  (void)_SEHPVLocals;                                                          \
                                                                               \
  __try                                                                        \
  {

#define _SEH_HANDLE \
  }                                                                            \
  __except(_SEH_CALL_FILTER(_SEHFilter))                                       \
  {                                                                            \
   struct _EXCEPTION_POINTERS * _SEHExceptionPointers = GetExceptionPointers();\
   long _SEHExceptionCode = GetExceptionCode();                                \

#define _SEH_END \
  }                                                                            \
 }                                                                             \
 __finally                                                                     \
 {                                                                             \
  _SEH_CALL_FINALLY(_SEHFinally);                                              \
 }

#define _SEH_LEAVE __leave

#define _SEH_GetExceptionCode() (_SEHExceptionCode)
#define _SEH_GetExceptionPointers() (_SEHExceptionPointers)
#define _SEH_AbnormalTermination() (_SEHAbnormalTermination)

/* New syntax */

#define _SEH2_TRY \
 {                                                                             \
  void * _SEHPVLocals = &_SEHLocals;                                           \
  (void)_SEHPVLocals;                                                          \
                                                                               \
  __try                                                                        \
  {

#define _SEH2_EXCEPT(FILTER_) \
  }                                                                            \
  __except(_SEH_CALL_FILTER(FILTER_))                                          \
  {                                                                            \
   struct _EXCEPTION_POINTERS * _SEHExceptionPointers = GetExceptionPointers();\
   long _SEHExceptionCode = GetExceptionCode();                                \

#define _SEH2_FINALLY(FINALLY_) \
  }                                                                            \
  __finally                                                                    \
  {                                                                            \
   _SEH_CALL_FINALLY(FINALLY_)

#define _SEH2_END \
  }                                                                            \
 }

#define _SEH2_HANDLE _SEH2_EXCEPT(_SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER))

#define _SEH2_LEAVE _SEH_LEAVE

#define _SEH2_GetExceptionCode     _SEH_GetExceptionCode
#define _SEH2_GetExceptionPointers _SEH_GetExceptionPointers
#define _SEH2_AbnormalTermination  _SEH_AbnormalTermination

#endif

/* EOF */
