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

#ifndef KJK_PSEH_FRAMEBASED_H_
#define KJK_PSEH_FRAMEBASED_H_

#include <pseh/framebased/internal.h>
#include <pseh/excpt.h>

#ifndef offsetof
#	include <stddef.h>
#endif

#if defined(_SEH_NO_NATIVE_NLG)
#	error PSEH setjmp/longjmp fallback is no longer supported
#endif

#if defined(__GNUC__)
#	define _SEHLongJmp __builtin_longjmp
#	define _SEHSetJmp __builtin_setjmp
	typedef void * _SEHJmpBuf_t[5];
#else
#	include <setjmp.h>
#	define _SEHLongJmp longjmp
#	define _SEHSetJmp setjmp
#	define _SEHJmpBuf_t jmp_buf
#endif

#ifdef __cplusplus
#	define _SEH_INIT_CONST static const
#else
#	define _SEH_INIT_CONST register const
#endif

typedef struct __SEHFrame
{
	_SEHPortableFrame_t SEH_Header;
	void * volatile SEH_Locals;
}
_SEHFrame_t;

typedef struct __SEHTryLevel
{
	_SEHPortableTryLevel_t ST_Header;
	_SEHJmpBuf_t ST_JmpBuf;
}
_SEHTryLevel_t;

static __declspec(noreturn) __inline void __stdcall _SEHCompilerSpecificHandler
(
	_SEHPortableTryLevel_t * trylevel
)
{
	_SEHTryLevel_t * mytrylevel;
	mytrylevel = _SEH_CONTAINING_RECORD(trylevel, _SEHTryLevel_t, ST_Header);
	_SEHLongJmp(mytrylevel->ST_JmpBuf, 1);
}

static const int _SEHScopeKind = 1;
static _SEHPortableFrame_t * const _SEHPortableFrame = 0;
static _SEHPortableTryLevel_t * const _SEHPortableTryLevel = 0;

/* SHARED LOCALS */
/* Access the locals for the current frame */
#define _SEH_ACCESS_LOCALS(LOCALS_) \
	_SEH_LOCALS_TYPENAME(LOCALS_) * _SEHPLocals; \
	_SEHPLocals = \
		_SEH_PVOID_CAST \
		( \
			_SEH_LOCALS_TYPENAME(LOCALS_) *, \
			_SEH_CONTAINING_RECORD(_SEHPortableFrame, _SEHFrame_t, SEH_Header) \
				->SEH_Locals \
		);

/* Access local variable VAR_ */
#define _SEH_VAR(VAR_) _SEHPLocals->VAR_

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

/* Declares a PSEH filter wrapping a regular filter function */
#define _SEH_WRAP_FILTER(WRAPPER_, NAME_) \
	static __inline _SEH_FILTER(WRAPPER_) \
	{ \
		return (NAME_)(_SEHExceptionPointers); \
	}

/* FINALLY FUNCTIONS */
/* Declares a finally function's prototype */
#define _SEH_FINALLYFUNC(NAME_) \
	void __stdcall NAME_ \
	( \
		struct __SEHPortableFrame * _SEHPortableFrame \
	)

/* Declares a PSEH finally function wrapping a regular function */
#define _SEH_WRAP_FINALLY(WRAPPER_, NAME_) \
	_SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ())

#define _SEH_WRAP_FINALLY_ARGS(WRAPPER_, NAME_, ARGS_) \
	static __inline _SEH_FINALLYFUNC(WRAPPER_) \
	{ \
		NAME_ ARGS_; \
	}

#define _SEH_WRAP_FINALLY_LOCALS_ARGS(WRAPPER_, LOCALS_, NAME_, ARGS_) \
	static __inline _SEH_FINALLYFUNC(WRAPPER_) \
	{ \
		_SEH_ACCESS_LOCALS(LOCALS_); \
		NAME_ ARGS_; \
	}

/* SAFE BLOCKS */
#ifdef __cplusplus
#	define _SEH_DECLARE_HANDLERS(FILTER_, FINALLY_) \
		static const _SEHHandlers_t _SEHHandlers = { (FILTER_), (FINALLY_) };
#else
#	define _SEH_DECLARE_HANDLERS(FILTER_, FINALLY_) \
		_SEHHandlers_t _SEHHandlers = { (0), (0) }; \
		_SEHHandlers.SH_Filter = (FILTER_); \
		_SEHHandlers.SH_Finally = (FINALLY_);
#endif

#define _SEH_GetExceptionCode() (unsigned long)(_SEHPortableFrame->SPF_Code)

#define _SEH_GetExceptionPointers() \
	((struct _EXCEPTION_POINTERS *)_SEHExceptionPointers)

#define _SEH_AbnormalTermination() (_SEHPortableFrame->SPF_Code != 0)

#define _SEH_LEAVE break

#define _SEH_YIELD(STMT_) \
	for(;;) \
	{ \
		if(!_SEHScopeKind) \
			_SEHReturn(); \
	\
		STMT_; \
	}

#define _SEH_TRY \
	for(;;) \
	{ \
		_SEH_INIT_CONST int _SEHTopTryLevel = (_SEHScopeKind != 0); \
		_SEHPortableFrame_t * const _SEHCurPortableFrame = _SEHPortableFrame; \
		_SEHPortableTryLevel_t * const _SEHPrevPortableTryLevel = _SEHPortableTryLevel; \
	 \
		{ \
			_SEH_INIT_CONST int _SEHScopeKind = 0; \
			register int _SEHState = 0; \
			register int _SEHHandle = 0; \
			_SEHFrame_t _SEHFrame; \
			_SEHTryLevel_t _SEHTryLevel; \
			_SEHPortableFrame_t * const _SEHPortableFrame = \
				_SEHTopTryLevel ? &_SEHFrame.SEH_Header : _SEHCurPortableFrame; \
			_SEHPortableTryLevel_t * const _SEHPortableTryLevel = &_SEHTryLevel.ST_Header; \
	\
			(void)_SEHScopeKind; \
			(void)_SEHPortableFrame; \
			(void)_SEHPortableTryLevel; \
			(void)_SEHHandle; \
	\
			for(;;) \
			{ \
				if(_SEHState) \
				{ \
					for(;;) \
					{ \
						{

#define _SEH_EXCEPT(FILTER_) \
						} \
	\
						break; \
					} \
	\
					break; \
				} \
				else \
				{ \
					if((_SEHHandle = _SEHSetJmp(_SEHTryLevel.ST_JmpBuf)) == 0) \
					{ \
						_SEHTryLevel.ST_Header.SPT_Handlers.SH_Filter = (FILTER_); \
						_SEHTryLevel.ST_Header.SPT_Handlers.SH_Finally = 0; \
	\
						_SEHTryLevel.ST_Header.SPT_Next = _SEHPrevPortableTryLevel; \
						_SEHFrame.SEH_Header.SPF_TopTryLevel = &_SEHTryLevel.ST_Header; \
	\
						if(_SEHTopTryLevel) \
						{ \
							if(&_SEHLocals != _SEHDummyLocals) \
								_SEHFrame.SEH_Locals = &_SEHLocals; \
	\
							_SEH_EnableTracing(_SEH_DO_DEFAULT_TRACING); \
							_SEHFrame.SEH_Header.SPF_Handler = _SEHCompilerSpecificHandler; \
							_SEHEnterFrame(&_SEHFrame.SEH_Header); \
						} \
	\
						++ _SEHState; \
						continue; \
					} \
					else \
					{ \
						break; \
					} \
				} \
	\
				break; \
			} \
	\
			_SEHPortableFrame->SPF_TopTryLevel = _SEHPrevPortableTryLevel; \
	\
			if(_SEHHandle) \
			{

#define _SEH_FINALLY(FINALLY_) \
						} \
	\
						break; \
					} \
	\
					_SEHPortableFrame->SPF_TopTryLevel = _SEHPrevPortableTryLevel; \
					break; \
				} \
				else \
				{ \
					_SEHTryLevel.ST_Header.SPT_Handlers.SH_Filter = 0; \
					_SEHTryLevel.ST_Header.SPT_Handlers.SH_Finally = (FINALLY_); \
	\
					_SEHTryLevel.ST_Header.SPT_Next = _SEHPrevPortableTryLevel; \
					_SEHFrame.SEH_Header.SPF_TopTryLevel = &_SEHTryLevel.ST_Header; \
	\
					if(_SEHTopTryLevel) \
					{ \
						if(&_SEHLocals != _SEHDummyLocals) \
							_SEHFrame.SEH_Locals = &_SEHLocals; \
	\
						_SEH_EnableTracing(_SEH_DO_DEFAULT_TRACING); \
						_SEHFrame.SEH_Header.SPF_Handler = _SEHCompilerSpecificHandler; \
						_SEHEnterFrame(&_SEHFrame.SEH_Header); \
					} \
	\
					++ _SEHState; \
					continue; \
				} \
	\
				break; \
			} \
	\
			(FINALLY_)(&_SEHFrame.SEH_Header); \
	\
			if(0) \
			{

#define _SEH_END \
			} \
		} \
	\
		if(_SEHTopTryLevel) \
			_SEHLeaveFrame(); \
	\
		break; \
	}

#define _SEH_HANDLE _SEH_EXCEPT(_SEH_STATIC_FILTER(_SEH_EXECUTE_HANDLER))

#define _SEH_EnableTracing(LEVEL_) ((void)(_SEHPortableFrame->SPF_Tracing = (LEVEL_)))
#define _SEH_DisableTracing() ((void)(_SEHPortableFrame->SPF_Tracing = _SEH_DO_TRACE_NONE))

#endif

/* EOF */
