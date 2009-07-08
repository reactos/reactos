/*
	Copyright (c) 2008 KJK::Hyperion

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

#define _NTSYSTEM_ /* removes dllimport attribute from RtlUnwind */

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <pseh/pseh2.h>
#include <excpt.h>
#include <intrin.h>

#ifndef EXCEPTION_EXIT_UNWIND
#define EXCEPTION_EXIT_UNWIND 4
#endif

#ifndef EXCEPTION_UNWINDING
#define EXCEPTION_UNWINDING 2
#endif

extern DECLSPEC_NORETURN int __SEH2Handle(void *, void *, void *, void *, void *, void *);
extern int __cdecl __SEH2FrameHandler(struct _EXCEPTION_RECORD *, void *, struct _CONTEXT *, void *);
extern int __cdecl __SEH2UnwindHandler(struct _EXCEPTION_RECORD *, void *, struct _CONTEXT *, void *);

FORCEINLINE
_SEH2Registration_t * __cdecl _SEH2CurrentRegistration(void)
{
	return (_SEH2Registration_t *)__readfsdword(0);
}

FORCEINLINE
void __cdecl __SEH2EnterFrame(_SEH2Registration_t * frame)
{
	frame->SER_Prev = _SEH2CurrentRegistration();
	__writefsdword(0, (unsigned long)frame);
}

FORCEINLINE
void __cdecl __SEH2LeaveFrame(void)
{
	__writefsdword(0, (unsigned long)_SEH2CurrentRegistration()->SER_Prev);
}

FORCEINLINE
void _SEH2GlobalUnwind(void * target)
{
	__asm__ __volatile__
	(
		"push %%ebp\n"
		"push $0\n"
		"push $0\n"
		"push $Return%=\n"
		"push %[target]\n"
		"call %c[RtlUnwind]\n"
		"Return%=: pop %%ebp\n" :
		:
		[target] "g" (target), [RtlUnwind] "g" (&RtlUnwind) :
		"eax", "ebx", "ecx", "edx", "esi", "edi", "flags", "memory"
	);
}

static
__SEH_EXCEPT_RET _SEH2Except(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel, struct _EXCEPTION_POINTERS * ep)
{
	void * filter = trylevel->ST_Filter;
	void * context = NULL;
	__SEH_EXCEPT_RET ret;

	if(filter == (void *)0)
		return 0;

	if(filter == (void *)1)
		return 1;

	if(filter == (void *)-1)
		return -1;

	if(_SEHIsTrampoline((_SEHTrampoline_t *)filter))
	{
		context = _SEHClosureFromTrampoline((_SEHTrampoline_t *)filter);
		filter = _SEHFunctionFromTrampoline((_SEHTrampoline_t *)filter);
	}

	__asm__ __volatile__
	(
		"push %[ep]\n"
		"push %[frame]\n"
		"call *%[filter]\n"
		"pop %%edx\n"
		"pop %%edx\n" :
		[ret] "=a" (ret) :
		"c" (context), [filter] "r" (filter), [frame] "g" (frame), [ep] "g" (ep) :
		"edx", "flags", "memory"
	);

	return ret;
}

static
void _SEH2Finally(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	if(trylevel->ST_Filter == NULL && trylevel->ST_Body != NULL)
	{
		void * body = trylevel->ST_Body;
		void * context = NULL;

		if(_SEHIsTrampoline((_SEHTrampoline_t *)body))
		{
			context = _SEHClosureFromTrampoline((_SEHTrampoline_t *)body);
			body = _SEHFunctionFromTrampoline((_SEHTrampoline_t *)body);
		}

		__asm__ __volatile__("call *%1\n" : : "c" (context), "r" (body) : "eax", "edx", "flags", "memory");
	}
}

typedef struct __SEH2UnwindFrame
{
	_SEH2Registration_t SUF_Registration;
	_SEH2Frame_t * SUF_Frame;
	volatile _SEH2TryLevel_t * SUF_TargetTryLevel;
}
_SEH2UnwindFrame_t;

static void _SEH2LocalUnwind(_SEH2Frame_t *, volatile _SEH2TryLevel_t *);

extern
int __cdecl _SEH2UnwindHandler
(
	struct _EXCEPTION_RECORD * ExceptionRecord,
	void * EstablisherFrame,
	struct _CONTEXT * ContextRecord,
	void * DispatcherContext
)
{
	if(ExceptionRecord->ExceptionFlags & (EXCEPTION_EXIT_UNWIND | EXCEPTION_UNWINDING))
	{
		_SEH2UnwindFrame_t * unwindframe = CONTAINING_RECORD(EstablisherFrame, _SEH2UnwindFrame_t, SUF_Registration);
		_SEH2LocalUnwind(unwindframe->SUF_Frame, unwindframe->SUF_TargetTryLevel);
		*((void **)DispatcherContext) = EstablisherFrame;
		return ExceptionCollidedUnwind;
	}

	return ExceptionContinueSearch;
}

static
void _SEH2LocalUnwind(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * dsttrylevel)
{
	volatile _SEH2TryLevel_t * trylevel;
	_SEH2UnwindFrame_t unwindframe;

	unwindframe.SUF_Frame = frame;
	unwindframe.SUF_TargetTryLevel = dsttrylevel;

	unwindframe.SUF_Registration.SER_Handler = &__SEH2UnwindHandler;
	__SEH2EnterFrame(&unwindframe.SUF_Registration);

	for(trylevel = frame->SF_TopTryLevel; trylevel && trylevel != dsttrylevel; trylevel = trylevel->ST_Next)
	{
		frame->SF_TopTryLevel = trylevel->ST_Next;
		_SEH2Finally(frame, trylevel);
	}

	__SEH2LeaveFrame();
}

static DECLSPEC_NORETURN
void _SEH2Handle(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	volatile _SEH2HandleTryLevel_t * fulltrylevel = CONTAINING_RECORD(trylevel, _SEH2HandleTryLevel_t, SHT_Common);

	_SEH2GlobalUnwind(frame);
	_SEH2LocalUnwind(frame, &fulltrylevel->SHT_Common);
	frame->SF_TopTryLevel = fulltrylevel->SHT_Common.ST_Next;

	__SEH2Handle
	(
		fulltrylevel->SHT_Common.ST_Body,
		fulltrylevel->SHT_Esp,
		fulltrylevel->SHT_Ebp,
		fulltrylevel->SHT_Ebx,
		fulltrylevel->SHT_Esi,
		fulltrylevel->SHT_Edi
	);
}

extern
int __cdecl _SEH2FrameHandler
(
	struct _EXCEPTION_RECORD * ExceptionRecord,
	void * EstablisherFrame,
	struct _CONTEXT * ContextRecord,
	void * DispatcherContext
)
{
	_SEH2Frame_t * frame;

	frame = EstablisherFrame;

	/* Unwinding */
	if(ExceptionRecord->ExceptionFlags & (EXCEPTION_EXIT_UNWIND | EXCEPTION_UNWINDING))
	{
		_SEH2LocalUnwind(frame, NULL);
	}
	/* Handling */
	else
	{
		int ret = 0;
		volatile _SEH2TryLevel_t * trylevel;
		EXCEPTION_POINTERS ep;

		ep.ExceptionRecord = ExceptionRecord;
		ep.ContextRecord = ContextRecord;

		frame->SF_Code = ExceptionRecord->ExceptionCode;

		for(trylevel = frame->SF_TopTryLevel; trylevel != NULL; trylevel = trylevel->ST_Next)
		{
			ret = _SEH2Except(frame, trylevel, &ep);

			if(ret < 0)
				return ExceptionContinueExecution;
			else if(ret > 0)
				_SEH2Handle(frame, trylevel);
		}
	}

	return ExceptionContinueSearch;
}

extern
void __cdecl _SEH2EnterFrame(_SEH2Frame_t * frame)
{
	frame->SF_Registration.SER_Handler = __SEH2FrameHandler;
	frame->SF_Code = 0;
	__SEH2EnterFrame(&frame->SF_Registration);
}

extern
int __cdecl _SEH2EnterFrameAndTrylevel(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	frame->SF_TopTryLevel = trylevel;
	_SEH2EnterFrame(frame);
	return 0;
}

extern
void __cdecl _SEH2LeaveFrame(void)
{
	__SEH2LeaveFrame();
}

extern
void __cdecl _SEH2Return(void)
{
	_SEH2LocalUnwind(CONTAINING_RECORD(_SEH2CurrentRegistration(), _SEH2Frame_t, SF_Registration), NULL);
	_SEH2LeaveFrame();
}

/* EOF */
