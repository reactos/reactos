/*
 Copyright (c) 2004 KJK::Hyperion
 
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
#include <excpt.h>

#ifdef _SEH_NO_NATIVE_NLG
#include <setjmp.h>
#include <stddef.h>
#else
#include <pseh/setjmp.h>
#define longjmp _SEHLongJmp
#define setjmp _SEHSetJmp
#define jmp_buf _SEHJmpBuf_t
#endif

typedef struct __SEHFrame
{
 _SEHPortableFrame_t SEH_Header;
 jmp_buf SEH_JmpBuf;
 void * SEH_Locals;
}
_SEHFrame_t;

static void __stdcall _SEHCompilerSpecificHandler(_SEHPortableFrame_t * frame)
{
 _SEHFrame_t * myframe;
 myframe = (_SEHFrame_t *)(((char *)frame) - offsetof(_SEHFrame_t, SEH_Header));
 longjmp(myframe->SEH_JmpBuf, 1);
}

#define _SEH_FILTER(NAME_) \
 int __stdcall NAME_ \
 ( \
  struct _EXCEPTION_POINTERS * _SEHExceptionPointers, \
  struct __SEHPortableFrame * _SEHPortableFrame \
 )

#define _SEH_FINALLY(NAME_) \
 void __stdcall NAME_ \
 ( \
  struct __SEHPortableFrame * _SEHPortableFrame \
 )

#define _SEH_TRY_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  (_SEHFilter_t)(EXCEPTION_CONTINUE_SEARCH + 1), \
  (FINALLY_) \
 )

#define _SEH_END_FINALLY _SEH_HANDLE _SEH_END

#define _SEH_TRY_FILTER(FILTER_) \
 _SEH_TRY_FILTER_FINALLY((FILTER_), NULL)

#define _SEH_TRY_HANDLE_FINALLY(FINALLY_) \
 _SEH_TRY_FILTER_FINALLY \
 ( \
  (_SEHFilter_t)(EXCEPTION_EXECUTE_HANDLER + 1), \
  (FINALLY_) \
 )

#define _SEH_TRY \
 _SEH_TRY_HANDLE_FINALLY(NULL)

#define _SEH_TRY_FILTER_FINALLY(FILTER_, FINALLY_) \
 {                                                                             \
  static const _SEHHandlers_t _SEHHandlers =                                   \
  {                                                                            \
   (FILTER_),                                                                  \
   _SEHCompilerSpecificHandler,                                                \
   (FINALLY_)                                                                  \
  };                                                                           \
                                                                               \
  _SEHFrame_t _SEHFrame;                                                       \
  _SEHPortableFrame_t * _SEHPortableFrame;                                     \
                                                                               \
  _SEHFrame.SEH_Header.SPF_Handlers = &_SEHHandlers;                           \
                                                                               \
  _SEHPortableFrame = &_SEHFrame.SEH_Header;                                   \
  (void)_SEHPortableFrame;                                                     \
                                                                               \
  if(setjmp(_SEHFrame.SEH_JmpBuf) == 0)                                        \
  {                                                                            \
   _SEHEnter(&_SEHFrame.SEH_Header);                                           \
                                                                               \
   do                                                                          \
   {

#define _SEH_HANDLE \
                                                                               \
   }                                                                           \
   while(0);                                                                   \
                                                                               \
   _SEHLeave(&_SEHFrame.SEH_Header);                                           \
  }                                                                            \
  else                                                                         \
  {                                                                            \
   _SEHLeave(&_SEHFrame.SEH_Header);                                           \

#define _SEH_END \
  }                                                                            \
                                                                               \
  if(_SEHHandlers.SH_Finally)                                                  \
   _SEHHandlers.SH_Finally(&_SEHFrame.SEH_Header);                             \
 }

#define _SEH_LEAVE break

#define _SEH_GetExceptionCode() (unsigned long)(_SEHPortableFrame->SPF_Code)

#define _SEH_GetExceptionPointers() \
 ((struct _EXCEPTION_POINTERS *)_SEHExceptionPointers)

#define _SEH_AbnormalTermination() (_SEHPortableFrame->SPF_Code != 0)

#endif

/* EOF */
