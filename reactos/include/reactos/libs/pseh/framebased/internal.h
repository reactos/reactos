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

#ifndef KJK_PSEH_FRAMEBASED_INTERNAL_H_
#define KJK_PSEH_FRAMEBASED_INTERNAL_H_

struct _EXCEPTION_RECORD;
struct _EXCEPTION_POINTERS;
struct _CONTEXT;
 
typedef int (__cdecl * _SEHFrameHandler_t)
(
 struct _EXCEPTION_RECORD *,
 void *,
 struct _CONTEXT *,
 void *
);

typedef struct __SEHRegistration
{
 struct __SEHRegistration * SER_Prev;
 _SEHFrameHandler_t SER_Handler;
}
_SEHRegistration_t;

struct __SEHPortableFrame;
struct __SEHPortableTryLevel;

typedef long (__stdcall * _SEHFilter_t)
(
 struct _EXCEPTION_POINTERS *,
 struct __SEHPortableFrame *
);

typedef void (__stdcall * _SEHHandler_t)
(
 struct __SEHPortableTryLevel *
);

typedef void (__stdcall * _SEHFinally_t)
(
 struct __SEHPortableFrame *
);

typedef struct __SEHHandlers
{
 _SEHFilter_t SH_Filter;
 _SEHFinally_t SH_Finally;
}
_SEHHandlers_t;

typedef struct __SEHPortableTryLevel
{
 struct __SEHPortableTryLevel * SPT_Next;
 const _SEHHandlers_t * SPT_Handlers;
}
_SEHPortableTryLevel_t;

typedef struct __SEHPortableFrame
{
 _SEHRegistration_t SPF_Registration;
 unsigned long SPF_Code;
 _SEHHandler_t SPF_Handler;
 _SEHPortableTryLevel_t * SPF_TopTryLevel;
}
_SEHPortableFrame_t;

#ifdef __cplusplus
extern "C"
{
#endif

extern void __stdcall _SEHEnterFrame_s
(
 _SEHPortableFrame_t *,
 _SEHPortableTryLevel_t *
);

extern void __stdcall _SEHEnterTry_s(_SEHPortableTryLevel_t *);
extern void __stdcall _SEHLeave_s(void);

#if !defined(_SEH_NO_FASTCALL)
# ifdef _M_IX86
#  define _SEH_FASTCALL __fastcall
# else
#  define _SEH_FASTCALL __stdcall
# endif

extern void _SEH_FASTCALL _SEHEnterFrame_f
(
 _SEHPortableFrame_t *,
 _SEHPortableTryLevel_t *
);

extern void _SEH_FASTCALL _SEHEnterTry_f(_SEHPortableTryLevel_t *);
extern void _SEH_FASTCALL _SEHLeave_f(void);

# define _SEHEnterFrame _SEHEnterFrame_f
# define _SEHEnterTry   _SEHEnterTry_f
# define _SEHLeave      _SEHLeave_f
#else
# define _SEHEnterFrame _SEHEnterFrame_s
# define _SEHEnterTry   _SEHEnterTry_s
# define _SEHLeave      _SEHLeave_s
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
