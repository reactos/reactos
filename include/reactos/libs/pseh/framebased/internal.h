/*
	Copyright (c) 2004/2005 KJK::Hyperion

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef KJK_PSEH_FRAMEBASED_INTERNAL_H_
#define KJK_PSEH_FRAMEBASED_INTERNAL_H_

#define _SEH_DO_TRACE_ENTER_LEAVE      (1 <<  0)
#define _SEH_DO_TRACE_EXCEPTION_RECORD (1 <<  1)
#define _SEH_DO_TRACE_CONTEXT          (1 <<  2)
#define _SEH_DO_TRACE_UNWIND           (1 <<  3)
#define _SEH_DO_TRACE_TRYLEVEL         (1 <<  4)
#define _SEH_DO_TRACE_CALL_FILTER      (1 <<  5)
#define _SEH_DO_TRACE_FILTER           (1 <<  6)
#define _SEH_DO_TRACE_CALL_HANDLER     (1 <<  7)
#define _SEH_DO_TRACE_CALL_FINALLY     (1 <<  8)

#define _SEH_DO_TRACE_NONE (0)
#define _SEH_DO_TRACE_ALL (-1)

#ifndef _SEH_DO_DEFAULT_TRACING
#define _SEH_DO_DEFAULT_TRACING _SEH_DO_TRACE_NONE
#endif

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
	struct __SEHPortableTryLevel * volatile SPT_Next;
	volatile _SEHHandlers_t SPT_Handlers;
}
_SEHPortableTryLevel_t;

typedef struct __SEHPortableFrame
{
	_SEHRegistration_t SPF_Registration;
	unsigned long SPF_Code;
	volatile _SEHHandler_t SPF_Handler;
	_SEHPortableTryLevel_t * volatile SPF_TopTryLevel;
	volatile int SPF_Tracing;
}
_SEHPortableFrame_t;

#ifdef __cplusplus
extern "C"
{
#endif

extern void __stdcall _SEHEnterFrame_s(_SEHPortableFrame_t *);
extern void __stdcall _SEHLeaveFrame_s(void);
extern void __stdcall _SEHReturn_s(void);

#if !defined(_SEH_NO_FASTCALL)
# ifdef _M_IX86
#  define _SEH_FASTCALL __fastcall
# else
#  define _SEH_FASTCALL __stdcall
# endif

extern void _SEH_FASTCALL _SEHEnterFrame_f(_SEHPortableFrame_t *);
extern void _SEH_FASTCALL _SEHLeaveFrame_f(void);
extern void _SEH_FASTCALL _SEHReturn_f(void);

# define _SEHEnterFrame _SEHEnterFrame_f
# define _SEHLeaveFrame _SEHLeaveFrame_f
# define _SEHReturn     _SEHReturn_f
#else
# define _SEHEnterFrame _SEHEnterFrame_s
# define _SEHLeaveFrame _SEHLeaveFrame_s
# define _SEHReturn     _SEHReturn_s
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
