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

typedef int (__stdcall * _SEHFilter_t)
(
 struct _EXCEPTION_POINTERS *,
 struct __SEHPortableFrame *
);

typedef __declspec(noreturn) void (__stdcall * _SEHHandler_t)
(
 struct __SEHPortableFrame *
);

typedef void (__stdcall * _SEHFinally_t)
(
 struct __SEHPortableFrame *
);

typedef struct __SEHHandlers
{
 _SEHFilter_t SH_Filter;
 _SEHHandler_t SH_Handler;
 _SEHFinally_t SH_Finally;
}
_SEHHandlers_t;

typedef struct __SEHPortableFrame
{
 _SEHRegistration_t SPF_Registration;
 unsigned long SPF_Code;
 int SPF_Handling;
 const _SEHHandlers_t * SPF_Handlers;
}
_SEHPortableFrame_t;

extern void __stdcall _SEHEnter(_SEHPortableFrame_t *);
extern void __stdcall _SEHLeave(_SEHPortableFrame_t *);

#endif

/* EOF */
