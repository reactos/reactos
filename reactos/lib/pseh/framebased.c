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

#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>
#include <excpt.h>

/* Assembly helpers, see i386/framebased.asm */
extern void __cdecl _SEHCleanHandlerEnvironment(void);
extern void __cdecl _SEHRegisterFrame(_SEHRegistration_t *);
extern void __cdecl _SEHUnregisterFrame(const _SEHRegistration_t *);
extern void __cdecl _SEHUnwind(_SEHPortableFrame_t *);

/* Borland C++ uses a different decoration (i.e. none) for stdcall functions */
extern void __stdcall RtlUnwind(void *, void *, void *, void *);
void const * _SEHRtlUnwind = RtlUnwind;

__declspec(noreturn) void __cdecl _SEHCallHandler(_SEHPortableFrame_t * frame)
{
 frame->SPF_Handling = 1;
 _SEHUnwind(frame);
 frame->SPF_Handlers->SH_Handler(frame);
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
 {
  if(frame->SPF_Handlers->SH_Finally && !frame->SPF_Handling)
   frame->SPF_Handlers->SH_Finally(frame);
 }
 /* Handling */
 else
 {
  if(ExceptionRecord->ExceptionCode)
   frame->SPF_Code = ExceptionRecord->ExceptionCode;
  else
   frame->SPF_Code = 0xC0000001; 

  if(frame->SPF_Handlers->SH_Filter)
  {
   int ret;

   switch((UINT_PTR)frame->SPF_Handlers->SH_Filter)
   {
    case _SEH_EXECUTE_HANDLER + 1:
    case _SEH_CONTINUE_SEARCH + 1:
    case _SEH_CONTINUE_EXECUTION + 1:
    {
     ret = (int)((UINT_PTR)frame->SPF_Handlers->SH_Filter) - 1;
     break;
    }

    default:
    {
     EXCEPTION_POINTERS ep;

     ep.ExceptionRecord = ExceptionRecord;
     ep.ContextRecord = ContextRecord;

     ret = frame->SPF_Handlers->SH_Filter(&ep, frame);
     break;
    }
   }

   /* _SEH_CONTINUE_EXECUTION */
   if(ret < 0)
    return ExceptionContinueExecution;
   /* _SEH_EXECUTE_HANDLER */
   else if(ret > 0)
    _SEHCallHandler(frame);
   /* _SEH_CONTINUE_SEARCH */
   else
    /* fall through */;
  }
 }

 return ExceptionContinueSearch;
}

void __stdcall _SEHEnter(_SEHPortableFrame_t * frame)
{
 frame->SPF_Registration.SER_Handler = _SEHFrameHandler;
 frame->SPF_Code = 0;
 frame->SPF_Handling = 0;
 _SEHRegisterFrame(&frame->SPF_Registration);
}

void __stdcall _SEHLeave(_SEHPortableFrame_t * frame)
{
 _SEHUnregisterFrame(&frame->SPF_Registration);
}

/* EOF */
