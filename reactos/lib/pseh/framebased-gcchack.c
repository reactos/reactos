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

extern _SEH2Registration_t * __cdecl _SEH2CurrentRegistration(void);

extern int __SEH2Except(void *, void *, void *);
extern void __SEH2Finally(void *, void *);

extern
#if defined(__GNUC__)
__attribute__((noreturn))
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
int __SEH2Handle(void *, void *, void *);

static
#if defined(__GNUC__)
__attribute__((always_inline))
#elif defined(_MSC_VER)
__forceinline
#endif
int _SEH2Except(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel, EXCEPTION_POINTERS * ep)
{
	return __SEH2Except(trylevel->ST_Filter, trylevel->ST_FramePointer, ep);
}

static
#if defined(__GNUC__)
__attribute__((noinline))
#elif defined(_MSC_VER)
__declspec(noinline)
#endif
void _SEH2Finally(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	__SEH2Finally(trylevel->ST_Body, trylevel->ST_FramePointer);
}

static
void _SEH2LocalUnwind(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * dsttrylevel)
{
	volatile _SEH2TryLevel_t * trylevel;

	for(trylevel = frame->SF_TopTryLevel; trylevel && trylevel != dsttrylevel; trylevel = trylevel->ST_Next)
	{
		if(!trylevel->ST_Filter)
			_SEH2Finally(frame, trylevel);
	}

	frame->SF_TopTryLevel = dsttrylevel;
}

extern void _SEH2GlobalUnwind(void *);

static
#if defined(__GNUC__)
__attribute__((noreturn))
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
void _SEH2Handle(_SEH2Frame_t * frame, volatile _SEH2TryLevel_t * trylevel)
{
	_SEH2GlobalUnwind(frame);
	_SEH2LocalUnwind(frame, trylevel);
	__SEH2Handle(trylevel->ST_Body, frame->SF_FramePointer, frame->SF_StackPointer);
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
	_SEH2Frame_t * frame;

#if defined(__GNUC__)
	__asm__ __volatile__("cld");
#elif defined(_MSC_VER)
	__asm cld
#endif

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
		volatile _SEH2TryLevel_t * trylevel;

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
void __cdecl __SEH2EnterFrame(_SEH2Frame_t *);

extern
void __cdecl _SEH2EnterFrame(_SEH2Frame_t * frame)
{
	frame->SF_Registration.SER_Handler = _SEH2FrameHandler;
	frame->SF_Code = 0;
	__SEH2EnterFrame(frame);
}

extern
void __cdecl __SEH2LeaveFrame(void);

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
