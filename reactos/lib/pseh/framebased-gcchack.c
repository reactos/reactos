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

extern int __SEH2Except(void *, void *, void *);
extern void __SEH2Finally(void *, void *);

static
__attribute__((always_inline))
int _SEH2Except(_SEHFrame_t * frame, volatile _SEHTryLevel_t * trylevel, EXCEPTION_POINTERS * ep)
{
	return __SEH2Except(trylevel->ST_Filter, trylevel->ST_FramePointer, ep);
}

static
__attribute__((noinline))
void _SEH2Finally(_SEHFrame_t * frame, volatile _SEHTryLevel_t * trylevel)
{
	__SEH2Finally(trylevel->ST_Body, trylevel->ST_FramePointer);
}

static
void _SEH2LocalUnwind(_SEHFrame_t * frame, volatile _SEHTryLevel_t * dsttrylevel)
{
	volatile _SEHTryLevel_t * trylevel;

	for(trylevel = frame->SF_TopTryLevel; trylevel && trylevel != dsttrylevel; trylevel = trylevel->ST_Next)
	{
		if(!trylevel->ST_Filter)
			_SEH2Finally(frame, trylevel);
	}

	frame->SF_TopTryLevel = dsttrylevel;
}

extern void _SEH2GlobalUnwind(void *);

static
__attribute__((noreturn))
void _SEH2Handle(_SEHFrame_t * frame, volatile _SEHTryLevel_t * trylevel)
{
	_SEH2GlobalUnwind(frame);
	_SEH2LocalUnwind(frame, trylevel);

	__asm__ __volatile__
	(
		"mov %[frame], %%ebp\n"
		"mov %[stack], %%esp\n"
		"jmp *%[handler]\n" :
		:
		[frame] "m" (frame->SF_FramePointer), [stack] "m" (frame->SF_StackPointer), [handler] "r" (trylevel->ST_Body)
	);

	for(;;);
}

static
int __cdecl _SEH2FrameHandler
(
	struct _EXCEPTION_RECORD * ExceptionRecord,
	void * EstablisherFrame,
	struct _CONTEXT * ContextRecord,
	void * DispatcherContext
)
{
	_SEHFrame_t * frame;

	__asm__ __volatile__("cld");

	frame = EstablisherFrame;

	/* Unwinding */
	if(ExceptionRecord->ExceptionFlags & (4 | 2))
	{
		_SEH2LocalUnwind(frame, NULL);
	}
	/* Handling */
	else
	{
		int ret = 0;
		volatile _SEHTryLevel_t * trylevel;

		frame->SF_Code = ExceptionRecord->ExceptionCode;

		for(trylevel = frame->SF_TopTryLevel; trylevel != NULL; trylevel = trylevel->ST_Next)
		{
			if(trylevel->ST_Filter)
			{
				EXCEPTION_POINTERS ep;

				ep.ExceptionRecord = ExceptionRecord;
				ep.ContextRecord = ContextRecord;

				ret = _SEH2Except(frame, trylevel, &ep);

				if(ret < 0)
					return ExceptionContinueExecution;
				else if(ret > 0)
					_SEH2Handle(frame, trylevel);
			}
		}
	}

	return ExceptionContinueSearch;
}

extern
void __cdecl _SEH2EnterFrame(_SEHFrame_t * frame)
{
	frame->SF_Registration.SER_Handler = _SEH2FrameHandler;
	frame->SF_Code = 0;
	__asm__ __volatile__("movl %%fs:0, %k0\n" : "=q" (frame->SF_Registration.SER_Prev));

	__SEH_BARRIER;
	__asm__ __volatile__("movl %k0, %%fs:0\n" : : "q" (&frame->SF_Registration));
}

extern
void __cdecl _SEH2LeaveFrame()
{
	__asm__ __volatile__
	(
		"movl %%fs:0, %%edx\n"
		"movl 0(%%edx), %%edx\n"
		"movl %%edx, %%fs:0\n" :
		:
		:
		"ecx"
	);
}

/* EOF */
