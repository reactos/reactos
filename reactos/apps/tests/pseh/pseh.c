#include <assert.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <pseh.h>

_SEH_FILTER(main_filter0)
{
 fprintf(stderr, "filter 0: exception code %08lX\n", _SEH_GetExceptionCode());
 return EXCEPTION_EXECUTE_HANDLER;
}

_SEH_FILTER(main_filter1)
{
 static int n;

 fputs("filter 1\n", stderr);

 _SEH_GetExceptionPointers()->ContextRecord->Eax = (DWORD)(UINT_PTR)&n;

 return EXCEPTION_CONTINUE_EXECUTION;
}

_SEH_FILTER(main_filter2)
{
 fputs("filter 2\n", stderr);
 return EXCEPTION_CONTINUE_SEARCH;
}

_SEH_FINALLY(main_finally0)
{
 fputs("finally 0\n", stderr);
}

_SEH_FINALLY(main_finally1)
{
 fputs("finally 1\n", stderr);
 RaiseException(0xC0000006, 0, 0, NULL);
}

_SEH_FINALLY(main_finally2)
{
 fputs("finally 2\n", stderr);
}

int main(void)
{
 _SEH_TRY
 {
  _SEH_TRY
  {
   _SEH_TRY_FILTER(main_filter2)
   {
    _SEH_TRY_FINALLY(main_finally2)
    {         
     _SEH_TRY_FILTER_FINALLY(main_filter0, main_finally0)
     {
      /* This exception is handled below */
      RaiseException(0xC0000005, 0, 0, NULL);
     }
     /* The finally is called no problem */
     _SEH_HANDLE
     /* The filter returns EXCEPTION_EXECUTE_HANDLER */
     {
      /* We handle the exception */
      fprintf(stderr, "caught exception %08lX\n", _SEH_GetExceptionCode());
     }
     _SEH_END;

     _SEH_TRY_FILTER_FINALLY(main_filter1, main_finally1)
     {
      /* This faulting code is corrected by the filter */
#ifdef __GNUC__
      __asm__("xorl %eax, %eax");
      __asm__("movl 0(%eax), %eax");
#else
      __asm xor eax, eax
      __asm mov eax, [eax]
#endif

      /* Execution continues here */
     }
     /* The finally misbehaves and throws an exception */
     _SEH_HANDLE
     {
      /*
       This handler is never called, because the filter returns
       EXCEPTION_CONTINUE_EXECUTION
      */
      assert(0);
     }
     _SEH_END;
    }
    /* This finally is called after the next-to-outermost filter */
    _SEH_END_FINALLY;
   }
   _SEH_HANDLE
   {
    /*
     This handler is never called, because the filter returns
     EXCEPTION_CONTINUE_SEARCH
    */
    assert(0);
   }
   _SEH_END;
  }
  _SEH_HANDLE
  {
   /* This handler handles the misbehavior of a finally */
   fprintf(stderr, "caught exception %08lX\n", _SEH_GetExceptionCode());

   /* This handler misbehaves, too */
   RaiseException(0xC0000007, 0, 0, NULL);
  }
  _SEH_END;
 }
 _SEH_HANDLE
 {
  /* This handler catches the exception thrown by the previous handler */
  fprintf(stderr, "caught exception %08lX\n", _SEH_GetExceptionCode());
 }
 _SEH_END;

 return 0;
}
