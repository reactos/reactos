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

#define _NTSYSTEM_
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <pseh/pseh.h>
#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>
#include <pseh/framebased.h>

#include <excpt.h>

/* Tracing */
#ifdef _SEH_ENABLE_TRACE
extern unsigned long __cdecl DbgPrint(const char * format, ...);

#define _SEH_TRACE_HEADER_(FRAME_) \
	DbgPrint("[PSEH:%p]%s:%d:", FRAME_, __FILE__, __LINE__);

#define _SEH_TRACE_TRAILER_ \
	DbgPrint("\n");

#define _SEH_FILTER_RET_STRING_(RET_) \
	(((int)(RET_) < 0) ? "_SEH_CONTINUE_EXECUTION" : (((int)(RET_) > 0) ? "_SEH_EXECUTE_HANDLER" : "_SEH_CONTINUE_SEARCH"))

#define _SEH_TRACE_LINE_(FRAME_, ARGS_) \
{ \
	_SEH_TRACE_HEADER_(FRAME_); \
	DbgPrint ARGS_; \
	_SEH_TRACE_TRAILER_; \
}

#define _SEH_TRACE_ENTER(FRAME_, FUNCNAME_, ARGS_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_ENTER_LEAVE) \
	{ \
		_SEH_TRACE_HEADER_(FRAME_); \
		DbgPrint(">>> %s(", (FUNCNAME_)); \
		DbgPrint ARGS_; \
		DbgPrint(")"); \
		_SEH_TRACE_TRAILER_; \
	} \
}

#define _SEH_TRACE_LEAVE(FRAME_, FUNCNAME_, ARGS_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_ENTER_LEAVE) \
	{ \
		_SEH_TRACE_HEADER_(FRAME_); \
		DbgPrint("<<< %s => ", (FUNCNAME_)); \
		DbgPrint ARGS_; \
		_SEH_TRACE_TRAILER_; \
	} \
}

#define _SEH_TRACE_EXCEPTION_RECORD(FRAME_, ER_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_EXCEPTION_RECORD) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"ExceptionRecord %p = { ExceptionCode : %08X, ExceptionFlags : %08X, ExceptionRecord : %p, ExceptionAddress : %p }", \
				(ER_), \
				(ER_)->ExceptionCode, \
				(ER_)->ExceptionFlags, \
				(ER_)->ExceptionRecord, \
				(ER_)->ExceptionAddress \
			) \
		); \
	} \
}

#ifdef _X86_
#define _SEH_TRACE_CONTEXT(FRAME_, CONTEXT_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CONTEXT) \
	{ \
		if(((CONTEXT_)->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) \
		{ \
			_SEH_TRACE_LINE_ \
			( \
				(FRAME_), \
				( \
					"eax=%08X ebx=%08X ecx=%08X edx=%08X esi=%08X edi=%08X", \
					(CONTEXT_)->Eax, \
					(CONTEXT_)->Ebx, \
					(CONTEXT_)->Ecx, \
					(CONTEXT_)->Edx, \
					(CONTEXT_)->Esi, \
					(CONTEXT_)->Edi \
				) \
			); \
		} \
	\
		if(((CONTEXT_)->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) \
		{ \
			_SEH_TRACE_LINE_ \
			( \
				(FRAME_), \
				( \
					"eip=%08X esp=%08X ebp=%08X efl=%08X cs=%08X ss=%08X", \
					(CONTEXT_)->Eip, \
					(CONTEXT_)->Esp, \
					(CONTEXT_)->Ebp, \
					(CONTEXT_)->EFlags, \
					(CONTEXT_)->SegCs, \
					(CONTEXT_)->SegSs \
				) \
			); \
		} \
	\
		if(((CONTEXT_)->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS) \
		{ \
			_SEH_TRACE_LINE_ \
			( \
				(FRAME_), \
				( \
					"ds=%08X es=%08X fs=%08X gs=%08X", \
					(CONTEXT_)->SegDs, \
					(CONTEXT_)->SegEs, \
					(CONTEXT_)->SegFs, \
					(CONTEXT_)->SegGs \
				) \
			); \
		} \
	} \
}
#else
#define _SEH_TRACE_CONTEXT(FRAME_, CONTEXT_)
#endif

#define _SEH_TRACE_UNWIND(FRAME_, ARGS_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_UNWIND) \
	{ \
		_SEH_TRACE_LINE_((FRAME_), ARGS_); \
	} \
}

#define _SEH_TRACE_TRYLEVEL(FRAME_, TRYLEVEL_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_TRYLEVEL) \
	{ \
		_SEH_TRACE_LINE_((FRAME_), ("trylevel %p, filter %p", (TRYLEVEL_), (TRYLEVEL_)->SPT_Handlers.SH_Filter)); \
	} \
}

#define _SEH_TRACE_ENTER_CALL_FILTER(FRAME_, TRYLEVEL_, ER_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CALL_FILTER) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"trylevel %p, calling filter %p, ExceptionCode %08X", \
				(TRYLEVEL_), \
				(TRYLEVEL_)->SPT_Handlers.SH_Filter, \
				(ER_)->ExceptionCode \
			) \
		); \
	} \
}

#define _SEH_TRACE_LEAVE_CALL_FILTER(FRAME_, TRYLEVEL_, RET_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CALL_FILTER) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"trylevel %p, filter %p => %s", \
				(TRYLEVEL_), \
				(TRYLEVEL_)->SPT_Handlers.SH_Filter, \
				_SEH_FILTER_RET_STRING_(RET_) \
			) \
		); \
	} \
}

#define _SEH_TRACE_FILTER(FRAME_, TRYLEVEL_, RET_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_FILTER) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"trylevel %p => %s", \
				(TRYLEVEL_), \
				_SEH_FILTER_RET_STRING_(RET_) \
			) \
		); \
	} \
}

#define _SEH_TRACE_ENTER_CALL_HANDLER(FRAME_, TRYLEVEL_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CALL_HANDLER) \
	{ \
		_SEH_TRACE_LINE_((FRAME_), ("trylevel %p, handling", (TRYLEVEL_))); \
	} \
}

#define _SEH_TRACE_ENTER_CALL_FINALLY(FRAME_, TRYLEVEL_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CALL_FINALLY) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"trylevel %p, calling exit routine %p", \
				(TRYLEVEL_), \
				(TRYLEVEL_)->SPT_Handlers.SH_Finally \
			) \
		); \
	} \
}

#define _SEH_TRACE_LEAVE_CALL_FINALLY(FRAME_, TRYLEVEL_) \
{ \
	if((FRAME_)->SPF_Tracing & _SEH_DO_TRACE_CALL_FINALLY) \
	{ \
		_SEH_TRACE_LINE_ \
		( \
			(FRAME_), \
			( \
				"trylevel %p, exit routine %p returned", \
				(TRYLEVEL_), \
				(TRYLEVEL_)->SPT_Handlers.SH_Finally \
			) \
		); \
	} \
}

#else
#define _SEH_TRACE_ENTER(FRAME_, FUNCNAME_, ARGS_)
#define _SEH_TRACE_LEAVE(FRAME_, FUNCNAME_, ARGS_)
#define _SEH_TRACE_EXCEPTION_RECORD(FRAME_, ER_)
#define _SEH_TRACE_CONTEXT(FRAME_, CONTEXT_)
#define _SEH_TRACE_UNWIND(FRAME_, ARGS_)
#define _SEH_TRACE_TRYLEVEL(FRAME_, TRYLEVEL_)
#define _SEH_TRACE_ENTER_CALL_FILTER(FRAME_, TRYLEVEL_, ER_)
#define _SEH_TRACE_LEAVE_CALL_FILTER(FRAME_, TRYLEVEL_, RET_)
#define _SEH_TRACE_FILTER(FRAME_, TRYLEVEL_, RET_)
#define _SEH_TRACE_ENTER_CALL_HANDLER(FRAME_, TRYLEVEL_)
#define _SEH_TRACE_ENTER_CALL_FINALLY(FRAME_, TRYLEVEL_)
#define _SEH_TRACE_LEAVE_CALL_FINALLY(FRAME_, TRYLEVEL_)
#endif

/* Assembly helpers, see i386/framebased.asm */
extern void __cdecl _SEHCleanHandlerEnvironment(void);
extern struct __SEHRegistration * __cdecl _SEHRegisterFrame(_SEHRegistration_t *);
extern void __cdecl _SEHUnregisterFrame(void);
extern void __cdecl _SEHGlobalUnwind(_SEHPortableFrame_t *);
extern _SEHRegistration_t * __cdecl _SEHCurrentRegistration(void);

/* Borland C++ uses a different decoration (i.e. none) for stdcall functions */
extern void __stdcall RtlUnwind(void *, void *, PEXCEPTION_RECORD, void *);
void const * _SEHRtlUnwind = RtlUnwind;

static void __stdcall _SEHLocalUnwind
(
	_SEHPortableFrame_t * frame,
	_SEHPortableTryLevel_t * dsttrylevel
)
{
	_SEHPortableTryLevel_t * trylevel;

	_SEH_TRACE_UNWIND(frame, ("enter local unwind from %p to %p", frame->SPF_TopTryLevel, dsttrylevel));

	for
	(
		trylevel = frame->SPF_TopTryLevel;
		trylevel != dsttrylevel;
		trylevel = trylevel->SPT_Next
	)
	{
		_SEHFinally_t pfnFinally;

		/* ASSERT(trylevel); */

		pfnFinally = trylevel->SPT_Handlers.SH_Finally;

		if(pfnFinally)
		{
			_SEH_TRACE_ENTER_CALL_FINALLY(frame, trylevel);
			pfnFinally(frame);
			_SEH_TRACE_LEAVE_CALL_FINALLY(frame, trylevel);
		}
	}

	_SEH_TRACE_UNWIND(frame, ("leave local unwind from %p to %p", frame->SPF_TopTryLevel, dsttrylevel));
}

static void __cdecl _SEHCallHandler
(
	_SEHPortableFrame_t * frame,
	_SEHPortableTryLevel_t * trylevel
)
{
	_SEHGlobalUnwind(frame);
	_SEHLocalUnwind(frame, trylevel);
	_SEH_TRACE_ENTER_CALL_HANDLER(frame, trylevel);
	frame->SPF_Handler(trylevel);
	/* ASSERT(0); */
}

static int __cdecl _SEHFrameHandler
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

	_SEH_TRACE_ENTER
	(
		frame,
		"_SEHFrameHandler",
		(
			"%p, %p, %p, %p",
			ExceptionRecord,
			EstablisherFrame,
			ContextRecord,
			DispatcherContext
		)
	);

	_SEH_TRACE_EXCEPTION_RECORD(frame, ExceptionRecord);
	_SEH_TRACE_CONTEXT(frame, ContextRecord);

	/* Unwinding */
	if(ExceptionRecord->ExceptionFlags & (4 | 2))
	{
		_SEH_TRACE_UNWIND(frame, ("enter forced unwind"));
		_SEHLocalUnwind(frame, NULL);
		_SEH_TRACE_UNWIND(frame, ("leave forced unwind"));
	}
	/* Handling */
	else
	{
		int ret;
		_SEHPortableTryLevel_t * trylevel;

		if(ExceptionRecord->ExceptionCode)
			frame->SPF_Code = ExceptionRecord->ExceptionCode;
		else
			frame->SPF_Code = 0xC0000001;

		for
		(
			trylevel = frame->SPF_TopTryLevel;
			trylevel != NULL;
			trylevel = trylevel->SPT_Next
		)
		{
			_SEHFilter_t pfnFilter = trylevel->SPT_Handlers.SH_Filter;

			_SEH_TRACE_TRYLEVEL(frame, trylevel);

			switch((UINT_PTR)pfnFilter)
			{
				case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER):
				case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_CONTINUE_SEARCH):
				case (UINT_PTR)_SEH_STATIC_FILTER(_SEH_CONTINUE_EXECUTION):
				{
					ret = (int)((UINT_PTR)pfnFilter) - 2;
					break;
				}

				default:
				{
					if(trylevel->SPT_Handlers.SH_Filter)
					{
						EXCEPTION_POINTERS ep;

						ep.ExceptionRecord = ExceptionRecord;
						ep.ContextRecord = ContextRecord;

						_SEH_TRACE_ENTER_CALL_FILTER(frame, trylevel, ExceptionRecord);
						ret = pfnFilter(&ep, frame);
						_SEH_TRACE_LEAVE_CALL_FILTER(frame, trylevel, ret);
					}
					else
						ret = _SEH_CONTINUE_SEARCH;

					break;
				}
			}

			_SEH_TRACE_FILTER(frame, trylevel, ret);

			/* _SEH_CONTINUE_EXECUTION */
			if(ret < 0)
			{
				_SEH_TRACE_LEAVE(frame, "_SEHFrameHandler", ("ExceptionContinueExecution"));
				return ExceptionContinueExecution;
			}
			/* _SEH_EXECUTE_HANDLER */
			else if(ret > 0)
				_SEHCallHandler(frame, trylevel);
			/* _SEH_CONTINUE_SEARCH */
			else
				continue;
		}

		/* FALLTHROUGH */
	}

	_SEH_TRACE_LEAVE(frame, "_SEHFrameHandler", ("ExceptionContinueSearch"));
	return ExceptionContinueSearch;
}

void __stdcall _SEHEnterFrame_s(_SEHPortableFrame_t * frame)
{
	_SEHEnterFrame_f(frame);
}

void __stdcall _SEHLeaveFrame_s(void)
{
	_SEHLeaveFrame_f();
}

void __stdcall _SEHReturn_s(void)
{
	_SEHReturn_f();
}

void _SEH_FASTCALL _SEHEnterFrame_f(_SEHPortableFrame_t * frame)
{
	/* ASSERT(frame); */
	/* ASSERT(trylevel); */
	frame->SPF_Registration.SER_Handler = _SEHFrameHandler;
	frame->SPF_Code = 0;
	_SEHRegisterFrame(&frame->SPF_Registration);
}

void _SEH_FASTCALL _SEHLeaveFrame_f(void)
{
	_SEHPortableFrame_t * frame;

	frame = _SEH_CONTAINING_RECORD
	(
		_SEHCurrentRegistration(),
		_SEHPortableFrame_t,
		SPF_Registration
	);

	/* ASSERT(frame); */
	/* ASSERT(frame->SPF_TopTryLevel == NULL) */

	_SEHUnregisterFrame();
}

void _SEH_FASTCALL _SEHReturn_f(void)
{
	_SEHPortableFrame_t * frame;

	frame = _SEH_CONTAINING_RECORD
	(
		_SEHCurrentRegistration(),
		_SEHPortableFrame_t,
		SPF_Registration
	);

	_SEHLocalUnwind(frame, NULL);
	_SEHUnregisterFrame();
}

/* EOF */
