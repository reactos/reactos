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
/*
Pretty PSEH

Made to be macro compatible with ms seh syntax and to be pretty, having the
finally block inline etc. Being pretty is not cheap. PPSEH has more
overhead than PSEH, thou mostly during exception/unwinding. Normal execution
only add the overhead of one setjmp, and only in the try/finally case.
PPSEH is probably much less portable than PSEH also.....

ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!ALERT!
-must always use non-native NLG cause a special version of longjmp (which doesn't
restore esp) is needed

-compiler must use ebp as stack frame pointer, cause the finally block rely
on this to access external variables

-all external variables that are used in the except/finally block MUST be volatile
to prevent variables from being optimized into registers.
See: http://alphalinux.org/archives/axp-list/1998/February1998/0330.html



USAGE:

__TRY{
   __LEAVE;
}
__EXCEPT(_SEH_STATIC_FILTER(EXCEPTION_EXECUTE_HANDLER)){
}
__ENDTRY;


__TRY{
}
__FINALLY{
}
__ENDTRY;


_SEH_FILTER(filter){
   return EXCEPTION_EXECUTE_HANDLER;
}
__TRY2{
}
__EXCEPT2(filter){
}
__FINALLY2{
}
__ENDTRY2;



-Gunnar

*/


#ifndef KJK_PPSEH_FRAMEBASED_H_
#define KJK_PPSEH_FRAMEBASED_H_

#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>
#include <malloc.h>

#ifndef offsetof
# include <stddef.h>
#endif

/*
Must always use non-native NLG since we need a special version of longjmp which does
not restore esp
*/
#include <pseh/setjmp.h>


typedef struct __SEHFrame
{
 _SEHPortableFrame_t SEH_Header;
 _SEHJmpBuf_t SEH_JmpBuf;
/* alternative: _SEHJmpBuf_t* SEH_JmpRetPtr; */
}
_SEHFrame_t;

/*
 Note: just define __inline to an empty symbol if your C compiler doesn't
 support it
*/
#ifdef __cplusplus
# ifndef __inline
#  define __inline inline
# endif
#endif


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


static __declspec(noreturn) __inline void __stdcall _SEHCompilerSpecificHandler
(
 _SEHPortableFrame_t * frame
)
{
   _SEHFrame_t * myframe;
   myframe = (_SEHFrame_t *)(((char *)frame) - offsetof(_SEHFrame_t, SEH_Header));
   _SEHLongJmp(myframe->SEH_JmpBuf, 1);
}



void __stdcall _FinallyPretty
(
 struct __SEHPortableFrame * frame
)
{
   _SEHFrame_t * myframe;
   _SEHJmpBuf_t jmpRetBuf;
   
   myframe = (_SEHFrame_t *)(((char *)frame) - offsetof(_SEHFrame_t, SEH_Header));   
    
   if(_SEHSetJmp(jmpRetBuf) == 0)
   {
      _SEHLongJmp_KeepEsp(myframe->SEH_JmpBuf, (int)&jmpRetBuf);
   
      /* alternative:
      myframe->SEH_JmpRetPtr = &jmpRetBuf;
      _SEHLongJmp_KeepEsp(myframe->SEH_JmpBuf, 2);
      */
   }
}


        
#define ___EXCEPT_DUAL(filter)                                                \
         } while(0);                                                          \
                                                                              \
         _SEHLeave(&_SEHFrame->SEH_Header);                                   \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         _SEHHandlers.SH_Filter = (filter);                                   \
         _SEHHandlers.SH_Finally = _FinallyPretty;                            \
                                                                              \
         if(_loop == 2 || (_ret = _SEHSetJmp(_SEHFrame->SEH_JmpBuf)))         \
         {                                                                    \
            if (_ret == 1)                                                    \
            {                                                                 \
               _SEHLeave(&_SEHFrame->SEH_Header);                             \
               do                                                             \
               { 


               
#define ___FINALLY_DUAL                                                       \
               } while (0);                                                   \
            }                                                                 \
                                                                              \
            do                                                                \
            {
               
               
               
#define ___EXCEPT_SINGLE(filter)                                              \
         } while(0);                                                          \
                                                                              \
         _SEHLeave(&_SEHFrame->SEH_Header);                                   \
         break;                                                               \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         _SEHHandlers.SH_Filter = (filter);                                   \
         _SEHHandlers.SH_Finally = (NULL);                                    \
                                                                              \
         if(_SEHSetJmp(_SEHFrame->SEH_JmpBuf))                                \
         {                                                                    \
            _SEHLeave(&_SEHFrame->SEH_Header);                                \
            do                                                                \
            { 



#define ___TRY                                                                \
   do                                                                         \
   {                                                                          \
      int _loop =0; int _ret = 0;                                             \
      static _SEHHandlers_t _SEHHandlers =                                    \
      {                                                                       \
      (NULL),                                                                 \
      _SEHCompilerSpecificHandler,                                            \
      (NULL)                                                                  \
      };                                                                      \
                                                                              \
      _SEHFrame_t * _SEHFrame;                                                \
      volatile _SEHPortableFrame_t * _SEHPortableFrame;                       \
                                                                              \
      _SEHFrame = _alloca(sizeof(_SEHFrame_t));                               \
      _SEHFrame->SEH_Header.SPF_Handlers = &_SEHHandlers;                     \
                                                                              \
      _SEHPortableFrame = &_SEHFrame->SEH_Header;                             \
      (void)_SEHPortableFrame;                                                \
                                                                              \
      for(;;_loop++)                                                          \
      if (_loop == 1)                                                         \
      {                                                                       \
         _SEHEnter(&_SEHFrame->SEH_Header);                                   \
                                                                              \
         do                                                                   \
         {


#define ___FINALLY_SINGLE                                                     \
         } while(0);                                                          \
                                                                              \
         _SEHLeave(&_SEHFrame->SEH_Header);                                   \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         _SEHHandlers.SH_Filter = _SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH);   \
         _SEHHandlers.SH_Finally = _FinallyPretty;                            \
                                                                              \
         if(_loop == 2 || (_ret = _SEHSetJmp(_SEHFrame->SEH_JmpBuf)))         \
         {                                                                    \
            do                                                                \
            {



#define ___ENDTRY                                                             \
            } while (0);                                                      \
                                                                              \
            if (_ret > 1)                                                     \
            {                                                                 \
               _SEHLongJmp(*((_SEHJmpBuf_t*)_ret), 1);                        \
               /* alternative: _SEHLongJmp(*_SEHFrame->SEH_JmpRetPtr, 1); */  \
            }                                                                 \
            break;                                                            \
         }                                                                    \
      }                                                                       \
    } while (0);



#ifdef _MSC_VER
   #define __TRY2 __try{__try
   #define __EXCEPT2 __except
   #define __FINALLY2  }__finally
   #define __ENDTRY2
   #define __TRY __try   
   #define __EXCEPT __except
   #define __FINALLY __finally
   #define __ENDTRY
   #define _SEH_STATIC_FILTER(ACTION_) (ACTION_)   
   #define __LEAVE __leave
#else
   #define __TRY2 ___TRY
   #define __EXCEPT2 ___EXCEPT_DUAL
   #define __FINALLY2 ___FINALLY_DUAL
   #define __ENDTRY2 __ENDTRY
   #define __TRY ___TRY   
   #define __EXCEPT ___EXCEPT_SINGLE
   #define __FINALLY ___FINALLY_SINGLE
   #define __ENDTRY ___ENDTRY
   #define __LEAVE break
#endif


#endif
