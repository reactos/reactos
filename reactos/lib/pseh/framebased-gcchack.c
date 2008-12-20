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

#define _NTSYSTEM_
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <pseh/pseh2.h>

#include <excpt.h>

#ifndef EXCEPTION_EXIT_UNWIND
#define EXCEPTION_EXIT_UNWIND 4
#endif

#ifndef EXCEPTION_UNWINDING
#define EXCEPTION_UNWINDING 2
#endif

extern _SEH2Registration_t * __cdecl _SEH2CurrentRegistration(void);
extern void _SEH2GlobalUnwind(void *);

extern int __SEH2Except(void *, void *);
extern void __SEH2Finally(void *, void *);
extern DECLSPEC_NORETURN int __SEH2Handle(void *, void *, void *);

extern void __cdecl __SEH2EnterFrame(_SEH2Registration_t *);
extern void __cdecl __SEH2LeaveFrame(void);

extern int __cdecl __SEH2FrameHandler(struct _EXCEPTION_RECORD *, void *, struct _CONTEXT *, void *);
extern int __cdecl __SEH2NestedHandler(struct _EXCEPTION_RECORD *, void *, struct _CONTEXT *, void *);

FORCEINLINE
int _SEH2Except(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	void * filter = trylevel->ST_Filter;
	void * context = NULL;

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

	return __SEH2Except(filter, context);
}

FORCEINLINE
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

		__SEH2Finally(body, context);
	}
}

extern
int __cdecl _SEH2NestedHandler
(
	struct _EXCEPTION_RECORD * ExceptionRecord,
	void * EstablisherFrame,
	struct _CONTEXT * ContextRecord,
	void * DispatcherContext
)
{
	if(ExceptionRecord->ExceptionFlags & (EXCEPTION_EXIT_UNWIND | EXCEPTION_UNWINDING))
		return ExceptionContinueSearch;

	*((void **)DispatcherContext) = EstablisherFrame;
	return ExceptionCollidedUnwind;
}

static
void _SEH2LocalUnwind(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * dsttrylevel)
{
	volatile _SEH2TryLevel_t * trylevel;
	_SEH2Registration_t nestedframe;

	nestedframe.SER_Handler = &__SEH2NestedHandler;
	__SEH2EnterFrame(&nestedframe);

	for(trylevel = frame->SF_TopTryLevel; trylevel && trylevel != dsttrylevel; trylevel = trylevel->ST_Next)
		_SEH2Finally(frame, trylevel);

	frame->SF_TopTryLevel = dsttrylevel;

	__SEH2LeaveFrame();
}

static DECLSPEC_NORETURN
void _SEH2Handle(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	_SEH2GlobalUnwind(frame);
	_SEH2LocalUnwind(frame, trylevel);
	__SEH2Handle(trylevel->ST_Body, trylevel->ST_Ebp, trylevel->ST_Esp);
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
		frame->SF_ExceptionInformation = &ep;

		for(trylevel = frame->SF_TopTryLevel; trylevel != NULL; trylevel = trylevel->ST_Next)
		{
			ret = _SEH2Except(frame, trylevel);

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
