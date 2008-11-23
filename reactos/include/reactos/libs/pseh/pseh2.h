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

#ifndef KJK_PSEH2_H_
#define KJK_PSEH2_H_

#ifndef __GNUC__
#error TODO
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

#if defined(__i386__)
typedef struct __SEHTrampoline
{
	unsigned char STR_MovEcx;
	unsigned char STR_Closure[4];
	unsigned char STR_Jmp;
	unsigned char STR_Function[4];
}
_SEHTrampoline_t;

static
__inline__
__attribute__((always_inline))
int _SEHIsTrampoline(_SEHTrampoline_t * trampoline_)
{
	return trampoline_->STR_MovEcx == 0xb9 && trampoline_->STR_Jmp == 0xe9;
}

static
__inline__
__attribute__((always_inline))
void * _SEHFunctionFromTrampoline(_SEHTrampoline_t * trampoline_)
{
	return (void *)(*(int *)(&trampoline_->STR_Function[0]) + (int)(trampoline_ + 1));
}

static
__inline__
__attribute__((always_inline))
void * _SEHClosureFromTrampoline(_SEHTrampoline_t * trampoline_)
{
	return (void *)*(int *)(&trampoline_->STR_Closure[0]);
}
#else
#error TODO
#endif

/* A no-op side effect that scares GCC */
#define __SEH_SIDE_EFFECT __asm__ __volatile__("#")

/* A no-op without any real side effects, but silences warnings */
#define __SEH_PRETEND_SIDE_EFFECT (void)0

/* Forces GCC to consider the specified label reachable */
#define __SEH_USE_LABEL(L_) __asm__ __volatile__("# %0\n" : : "i" (&&L_))

/* Makes GCC pretend the specified label is reachable, to silence warnings */
#define __SEH_PRETEND_USE_LABEL(L_) (void)(&&L_)

/* Forces GCC to emit the specified nested function as a function */
#define __SEH_USE_NESTED_FUNCTION(F_) (void)(&F_) /* __attribute__((noinline)) seems to do the trick */

/* Soft memory barrier */
#define __SEH_BARRIER __asm__ __volatile__("#":::"memory")

typedef struct __SEHRegistration
{
	struct __SEHRegistration * SER_Prev;
	_SEHFrameHandler_t SER_Handler;
}
_SEHRegistration_t;

#define __SEH_FORCE_NEST \
	__asm__ __volatile__("#%0" : : "r" (&_SEHFrame))

#define __SEH_NESTED_PROLOG \
	__SEH_FORCE_NEST;

#define __SEH_DECLARE_EXCEPT_PFN(NAME_) int (__cdecl * NAME_)(void *)
#define __SEH_DECLARE_EXCEPT(NAME_) int __cdecl NAME_(void *)
#define __SEH_DEFINE_EXCEPT(NAME_) int __cdecl NAME_(void * _SEHExceptionPointers)

#define __SEH_DECLARE_FINALLY_PFN(NAME_) void (__cdecl * NAME_)(void)
#define __SEH_DECLARE_FINALLY(NAME_) void __cdecl NAME_(void)
#define __SEH_DEFINE_FINALLY(NAME_) void __cdecl NAME_(void)

#define __SEH_RETURN_EXCEPT(R_) return (int)(R_)
#define __SEH_RETURN_FINALLY() return

typedef struct __SEHFrame
{
	_SEHRegistration_t SF_Registration;
	volatile struct __SEHTryLevel * volatile SF_TopTryLevel;
	void * volatile SF_FramePointer;
	void * volatile SF_StackPointer;
	volatile unsigned long SF_Code;
}
_SEHFrame_t;

typedef struct __SEHTryLevel
{
	volatile struct __SEHTryLevel * ST_Next;
	void * ST_FramePointer;
	void * ST_Filter;
	void * ST_Body;
}
_SEHTryLevel_t;

#define __SEH_BEGIN_TRY \
	{ \
		__label__ _SEHBeginTry; \
		__label__ _SEHEndTry; \
	 \
		__SEH_USE_LABEL(_SEHBeginTry); \
		__SEH_USE_LABEL(_SEHEndTry); \
	 \
		_SEHBeginTry: __SEH_SIDE_EFFECT; \
		{ \
			__SEH_BARRIER;

#define __SEH_END_TRY \
			__SEH_BARRIER; \
		} \
		_SEHEndTry: __SEH_SIDE_EFFECT; \
	}

#define __SEH_SET_TRYLEVEL(TRYLEVEL_) \
	{ \
		__SEH_BARRIER; _SEHFrameP->SF_TopTryLevel = (TRYLEVEL_); __SEH_BARRIER; \
	}

#define __SEH_ENTER_TRYLEVEL() __SEH_SET_TRYLEVEL(&_SEHTryLevel)
#define __SEH_LEAVE_TRYLEVEL() __SEH_SET_TRYLEVEL(_SEHPrevTryLevelP)

#define __SEH_END_SCOPE_CHAIN \
	static const int _SEHScopeKind = 1; \
	static _SEHFrame_t * const _SEHFrameP = 0; \
	static _SEHTryLevel_t * const _SEHTryLevelP = 0;

#define __SEH_BEGIN_SCOPE \
	for(;;) \
	{ \
		__label__ _SEHBeginScope; \
		__label__ _SEHEndScope; \
 \
		_SEHBeginScope: __SEH_SIDE_EFFECT; \
 \
		const int _SEHTopTryLevel = (_SEHScopeKind != 0); \
		_SEHFrame_t * const _SEHCurFrameP = _SEHFrameP; \
		volatile _SEHTryLevel_t * const _SEHPrevTryLevelP = _SEHTryLevelP; \
 \
        (void)_SEHTopTryLevel; \
        (void)_SEHCurFrameP; \
        (void)_SEHPrevTryLevelP; \
 \
		__SEH_USE_LABEL(_SEHBeginScope); \
		__SEH_USE_LABEL(_SEHEndScope); \
 \
		{ \
			__label__ _SEHBeforeTry; \
			__label__ _SEHDoTry; \
			__label__ _SEHAfterTry; \
			static const int _SEHScopeKind = 0; \
			_SEHFrame_t _SEHFrame; \
			volatile _SEHTryLevel_t _SEHTryLevel; \
			_SEHFrame_t * const _SEHFrameP = _SEHTopTryLevel ? &_SEHFrame : _SEHCurFrameP; \
			volatile _SEHTryLevel_t * const _SEHTryLevelP = &_SEHTryLevel; \
 \
			(void)_SEHScopeKind; \
			(void)_SEHFrame; \
			(void)_SEHTryLevel; \
			(void)_SEHFrameP; \
			(void)_SEHTryLevelP; \
 \
			_SEHTryLevel.ST_Next = _SEHPrevTryLevelP; \
			goto _SEHBeforeTry; \
 \
			_SEHDoTry:; \
			__SEH_ENTER_TRYLEVEL(); \
 \
			if(_SEHTopTryLevel) \
			{ \
				__SEH_BARRIER; __asm__ __volatile__("mov %%ebp, %0\n#%1" : "=m" (_SEHFrame.SF_FramePointer) : "r" (__builtin_frame_address(0))); __SEH_BARRIER; \
				_SEH2EnterFrame(&_SEHFrame); \
			} \

#define __SEH_END_SCOPE \
		} \
 \
		_SEHEndScope: __SEH_SIDE_EFFECT; \
 \
		break; \
	}

#define __SEH_SCOPE_LOCALS \
	__label__ _SEHBeginExcept; \
	__label__ _SEHEndExcept; \
 \
	auto __SEH_DECLARE_EXCEPT(_SEHExcept); \
	auto __SEH_DECLARE_FINALLY(_SEHFinally);

#define _SEH2_TRY \
	__SEH_BEGIN_SCOPE \
	{ \
		__SEH_SCOPE_LOCALS; \
\
		__SEH_BEGIN_TRY \
		{

#define _SEH2_FINALLY \
		} \
		__SEH_END_TRY; \
 \
		goto _SEHAfterTry; \
		_SEHBeforeTry:; \
 \
		__SEH_PRETEND_USE_LABEL(_SEHBeginExcept); \
		__SEH_PRETEND_USE_LABEL(_SEHEndExcept); \
 \
		__SEH_USE_NESTED_FUNCTION(_SEHFinally); \
 \
		_SEHTryLevel.ST_FramePointer = _SEHClosureFromTrampoline((_SEHTrampoline_t *)&_SEHFinally); \
		_SEHTryLevel.ST_Filter = NULL; \
		_SEHTryLevel.ST_Body = _SEHFunctionFromTrampoline((_SEHTrampoline_t *)&_SEHFinally); \
 \
		goto _SEHDoTry; \
		_SEHAfterTry:; \
 \
		if(_SEHTopTryLevel) \
			_SEH2LeaveFrame(); \
		else \
		{ \
			__SEH_LEAVE_TRYLEVEL(); \
		} \
 \
		_SEHFinally(); \
		goto _SEHEndExcept; \
 \
		_SEHBeginExcept: __SEH_PRETEND_SIDE_EFFECT; \
		__attribute__((unused)) __SEH_DEFINE_EXCEPT(_SEHExcept) { __SEH_RETURN_EXCEPT(0); } \
 \
		__attribute__((noinline)) __attribute__((used)) __SEH_DEFINE_FINALLY(_SEHFinally) \
		{ \
			__SEH_END_SCOPE_CHAIN; \
 \
			(void)_SEHScopeKind; \
			(void)_SEHFrameP; \
			(void)_SEHTryLevelP; \
 \
			__SEH_NESTED_PROLOG; \
 \
 			for(;; ({ __SEH_RETURN_FINALLY(); })) \
			{

#define _SEH2_EXCEPT(E_) \
		} \
		__SEH_END_TRY; \
 \
		goto _SEHAfterTry; \
 \
		_SEHBeforeTry:; \
\
		__SEH_USE_LABEL(_SEHBeginExcept); \
		__SEH_USE_LABEL(_SEHEndExcept); \
\
		__SEH_USE_NESTED_FUNCTION(_SEHExcept); \
 \
		_SEHTryLevel.ST_FramePointer = _SEHClosureFromTrampoline((_SEHTrampoline_t *)&_SEHExcept); \
		_SEHTryLevel.ST_Filter = _SEHFunctionFromTrampoline((_SEHTrampoline_t *)&_SEHExcept); \
		_SEHTryLevel.ST_Body = &&_SEHBeginExcept; \
		__SEH_BARRIER; __asm__ __volatile__("mov %%esp, %0" : "=m" (_SEHFrameP->SF_StackPointer)); __SEH_BARRIER; \
\
		goto _SEHDoTry; \
\
		__attribute__((noinline)) __attribute__((used)) __SEH_DEFINE_EXCEPT(_SEHExcept) \
		{ \
			__SEH_NESTED_PROLOG; \
			__SEH_RETURN_EXCEPT(E_); \
		} \
\
		__attribute__((unused)) __SEH_DEFINE_FINALLY(_SEHFinally) { __SEH_RETURN_FINALLY(); } \
\
		_SEHAfterTry:; \
		if(_SEHTopTryLevel) \
			_SEH2LeaveFrame(); \
		else \
		{ \
			__SEH_LEAVE_TRYLEVEL(); \
		} \
\
		goto _SEHEndExcept; \
\
		_SEHBeginExcept: __SEH_SIDE_EFFECT; \
		{ \
			{ \
				_SEHFrame_t * const _SEHFrameP = _SEHTopTryLevel ? &_SEHFrame : _SEHCurFrameP; \
				(void)_SEHFrameP; \
				__SEH_BARRIER;

#define _SEH2_END \
				__SEH_BARRIER; \
			} \
		} \
		_SEHEndExcept: __SEH_SIDE_EFFECT; \
	} \
	__SEH_END_SCOPE;

#define _SEH2_GetExceptionInformation() ((struct _EXCEPTION_POINTERS *)_SEHExceptionPointers)
#define _SEH2_GetExceptionCode() ((_SEHFrameP)->SF_Code)

#define _SEH2_YIELD(STMT_) \
	for(;;) \
	{ \
		if(!_SEHScopeKind) \
			_SEH2Return(); \
 \
		STMT_; \
	}

#define _SEH2_LEAVE goto _SEHEndTry

__SEH_END_SCOPE_CHAIN;

#ifdef __cplusplus
extern "C"
{
#endif

extern void __cdecl _SEH2EnterFrame(_SEHFrame_t *);
extern void __cdecl _SEH2LeaveFrame(void);
extern void __cdecl _SEH2Return(void);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
