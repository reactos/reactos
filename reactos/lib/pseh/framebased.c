#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <pseh/framebased/internal.h>
#include <excpt.h>

/* Assembly helpers, see i386/framebased.asm */
extern void __cdecl _SEHCleanHandlerEnvironment(void);
extern void __cdecl _SEHRegisterFrame(_SEHRegistration_t *);
extern void __cdecl _SEHUnregisterFrame(const _SEHRegistration_t *);
extern void __cdecl _SEHUnwind(_SEHPortableFrame_t *);

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
    case EXCEPTION_EXECUTE_HANDLER + 1:
    case EXCEPTION_CONTINUE_SEARCH + 1:
    case EXCEPTION_CONTINUE_EXECUTION + 1:
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

   /* EXCEPTION_CONTINUE_EXECUTION */
   if(ret < 0)
    return ExceptionContinueExecution;
   /* EXCEPTION_EXECUTE_HANDLER */
   else if(ret > 0)
    _SEHCallHandler(frame);
   /* EXCEPTION_CONTINUE_SEARCH */
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
