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

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <pseh/pseh.h>
#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>
#include <pseh/framebased.h>

#include <excpt.h>

/* Assembly helpers, see i386/framebased.asm */
extern void __cdecl _SEHCleanHandlerEnvironment(void);
extern struct __SEHRegistration * __cdecl _SEHRegisterFrame(_SEHRegistration_t *);
extern void __cdecl _SEHUnregisterFrame(void);
extern void __cdecl _SEHGlobalUnwind(_SEHPortableFrame_t *);
extern _SEHRegistration_t * __cdecl _SEHCurrentRegistration(void);

/* Borland C++ uses a different decoration (i.e. none) for stdcall functions */
extern void __stdcall RtlUnwind(void *, void *, void *, void *);
void const * _SEHRtlUnwind = RtlUnwind;

void __stdcall _SEHLocalUnwind
(
 _SEHPortableFrame_t * frame,
 _SEHPortableTryLevel_t * dsttrylevel
)
{
 _SEHPortableTryLevel_t * trylevel;

 for
 (
  trylevel = frame->SPF_TopTryLevel;
  trylevel != dsttrylevel;
  trylevel = trylevel->SPT_Next
 )
 {
  _SEHFinally_t pfnFinally;

  /* ASSERT(trylevel); */

  pfnFinally = trylevel->SPT_Handlers->SH_Finally;

  if(pfnFinally)
   pfnFinally(frame);
 }
}

void __cdecl _SEHCallHandler
(
 _SEHPortableFrame_t * frame,
 _SEHPortableTryLevel_t * trylevel
)
{
 DbgPrint("_SEHCallHandler: REG %p\n", _SEHCurrentRegistration());
 _SEHGlobalUnwind(frame);
 DbgPrint("_SEHCallHandler: REG %p\n", _SEHCurrentRegistration());
 _SEHLocalUnwind(frame, trylevel);
 DbgPrint("_SEHCallHandler: REG %p\n", _SEHCurrentRegistration());
 frame->SPF_Handler(trylevel);
}

int __cdecl _SEHFrameHandler
(
 struct _EXCEPTION_RECORD * ExceptionRecord,
 void * EstablisherFrame,
 struct _CONTEXT * ContextRecord,
 void * DispatcherContext
)
{
 _SEHPortableFrame_t * frame;

 _SEHCleanHandlerEnvironment();

 frame = EstablisherFrame;

 /* Unwinding */
 if(ExceptionRecord->ExceptionFlags & (4 | 2))
  _SEHLocalUnwind(frame, NULL);
 /* Handling */
 else
 {
  int ret;
  _SEHPortableTryLevel_t * trylevel;

  if(ExceptionRecord->ExceptionCode)
   frame->SPF_Code = ExceptionRecord->ExceptionCode;
  else
   frame->SPF_Code = 0xC0000001;

  for
  (
   trylevel = frame->SPF_TopTryLevel;
   trylevel != NULL;
   trylevel = trylevel->SPT_Next
  )
  {
   _SEHFilter_t pfnFilter = trylevel->SPT_Handlers->SH_Filter;

   switch((UINT_PTR)pfnFilter)
   {
    case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER):
    case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH):
    case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_CONTINUE_EXECUTION):
    {
     ret = (int)((UINT_PTR)pfnFilter) - 2;
     break;
    }

    default:
    {
     if(trylevel->SPT_Handlers->SH_Filter)
     {
      EXCEPTION_POINTERS ep;

      ep.ExceptionRecord = ExceptionRecord;
      ep.ContextRecord = ContextRecord;

      ret = pfnFilter(&ep, frame);
     }
     else
      ret = _SEH_CONTINUE_SEARCH;

     break;
    }
   }

   /* _SEH_CONTINUE_EXECUTION */
   if(ret < 0)
    return ExceptionContinueExecution;
   /* _SEH_EXECUTE_HANDLER */
   else if(ret > 0)
    _SEHCallHandler(frame, trylevel);
   /* _SEH_CONTINUE_SEARCH */
   else
    continue;
  }

  /* FALLTHROUGH */
 }

 return ExceptionContinueSearch;
}

void __stdcall _SEHEnterFrame_s
(
 _SEHPortableFrame_t * frame,
 _SEHPortableTryLevel_t * trylevel
)
{
 _SEHEnterFrame_f(frame, trylevel);
}

void __stdcall _SEHEnterTry_s(_SEHPortableTryLevel_t * trylevel)
{
 _SEHEnterTry_f(trylevel);
}

void __stdcall _SEHLeave_s(void)
{
 _SEHLeave_f();
}

void _SEH_FASTCALL _SEHEnterFrame_f
(
 _SEHPortableFrame_t * frame,
 _SEHPortableTryLevel_t * trylevel
)
{
 /* ASSERT(frame); */
 /* ASSERT(trylevel); */
 frame->SPF_Registration.SER_Handler = _SEHFrameHandler;
 frame->SPF_Code = 0;
 frame->SPF_TopTryLevel = trylevel;
 trylevel->SPT_Next = NULL;
 _SEHRegisterFrame(&frame->SPF_Registration);
}

void _SEH_FASTCALL _SEHEnterTry_f(_SEHPortableTryLevel_t * trylevel)
{
 _SEHPortableFrame_t * frame;

 frame = _SEH_CONTAINING_RECORD
 (
  _SEHCurrentRegistration(),
  _SEHPortableFrame_t,
  SPF_Registration
 );

 trylevel->SPT_Next = frame->SPF_TopTryLevel;
 frame->SPF_TopTryLevel = trylevel;
}

void _SEH_FASTCALL _SEHLeave_f(void)
{
 _SEHPortableFrame_t * frame;
 _SEHPortableTryLevel_t * trylevel;

 frame = _SEH_CONTAINING_RECORD
 (
  _SEHCurrentRegistration(),
  _SEHPortableFrame_t,
  SPF_Registration
 );

 /* ASSERT(frame); */

 trylevel = frame->SPF_TopTryLevel;

 /* ASSERT(trylevel); */

 if(trylevel->SPT_Next)
  frame->SPF_TopTryLevel = trylevel->SPT_Next;
 else
  _SEHUnregisterFrame();
}

/* EOF */
