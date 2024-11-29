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

#define __USE_PSEH2__

#if defined(_USE_NATIVE_SEH) || defined(_MSC_VER)

#define _SEH2_TRY __try
#define _SEH2_FINALLY __finally
#define _SEH2_EXCEPT(...) __except(__VA_ARGS__)
#define _SEH2_END
#define _SEH2_GetExceptionInformation() (GetExceptionInformation())
#define _SEH2_GetExceptionCode() (GetExceptionCode())
#define _SEH2_AbnormalTermination() (AbnormalTermination())
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE __leave
#define _SEH2_VOLATILE

#define __endtry

#elif defined(__GNUC__) && !defined(__clang__) && defined(_M_AMD64)

#include "pseh2_64.h"

#elif defined(_USE_DUMMY_PSEH) || defined (__arm__) || defined(_M_AMD64)

#ifdef __cplusplus
extern"C"
{
#endif
extern int _SEH2_Volatile0;
extern int _SEH2_VolatileExceptionCode;
#ifdef __cplusplus
} // extern "C"
#endif

#define _SEH2_TRY                                   \
_Pragma("GCC diagnostic push")                      \
_Pragma("GCC diagnostic ignored \"-Wunused-label\"")\
{                                                   \
    __label__ __seh2_scope_end__;

#define _SEH2_FINALLY                               \
    __seh2_scope_end__:;                            \
    }                                               \
    if (1)                                          \
    {                                               \
        __label__ __seh2_scope_end__;

#define _SEH2_EXCEPT(...)                           \
    __seh2_scope_end__:;                            \
    }                                               \
    if (_SEH2_Volatile0 || (0 && (__VA_ARGS__)))    \
    {                                               \
        __label__ __seh2_scope_end__;

#define _SEH2_END                                   \
    __seh2_scope_end__:;                            \
    }                                               \
_Pragma("GCC diagnostic pop")

#define _SEH2_GetExceptionInformation() ((struct _EXCEPTION_POINTERS*)0)
#define _SEH2_GetExceptionCode() _SEH2_VolatileExceptionCode
#define _SEH2_AbnormalTermination() (0)
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE goto __seh2_scope_end__;
#define _SEH2_VOLATILE volatile

#define __try _SEH2_TRY
#define __except _SEH2_EXCEPT
#define __finally _SEH2_FINALLY
#define __endtry _SEH2_END
#define __leave _SEH2_LEAVE
#define _exception_code() 0
#define _exception_info() ((void*)0)

#elif defined(_USE_PSEH3)

#include "pseh3.h"

/* Compatibility macros */
#define _SEH2_TRY _SEH3_TRY
#define _SEH2_EXCEPT _SEH3_EXCEPT
#define _SEH2_FINALLY _SEH3_FINALLY
#define _SEH2_END _SEH3_END
#define _SEH2_GetExceptionInformation() ((struct _EXCEPTION_POINTERS*)_exception_info())
#define _SEH2_GetExceptionCode _exception_code
#define _SEH2_AbnormalTermination _abnormal_termination
#define _SEH2_LEAVE _SEH3_LEAVE
#define _SEH2_YIELD(x) x
#define _SEH2_VOLATILE volatile

#ifndef __try // Conflict with GCC's STL
#define __try _SEH3_TRY
#define __except _SEH3_EXCEPT
#define __finally _SEH3_FINALLY
#define __endtry _SEH3_END
#define __leave _SEH3_LEAVE
#endif

#elif defined(__GNUC__)

struct _EXCEPTION_RECORD;
struct _EXCEPTION_POINTERS;
struct _CONTEXT;

typedef int (__cdecl * _SEH2FrameHandler_t)
(
	struct _EXCEPTION_RECORD *,
	void *,
	struct _CONTEXT *,
	void *
);

typedef struct __SEH2Registration
{
	struct __SEH2Registration * SER_Prev;
	_SEH2FrameHandler_t SER_Handler;
}
_SEH2Registration_t;

typedef struct __SEH2Frame
{
	_SEH2Registration_t SF_Registration;
	volatile struct __SEH2TryLevel * volatile SF_TopTryLevel;
	volatile unsigned long SF_Code;
}
_SEH2Frame_t;

typedef struct __SEH2TryLevel
{
	volatile struct __SEH2TryLevel * ST_Next;
	void * ST_Filter;
	void * ST_Body;
}
_SEH2TryLevel_t;

typedef struct __SEH2HandleTryLevel
{
	_SEH2TryLevel_t  SHT_Common;
	void * volatile SHT_Esp;
	void * volatile SHT_Ebp;
	void * volatile SHT_Ebx;
	void * volatile SHT_Esi;
	void * volatile SHT_Edi;
}
_SEH2HandleTryLevel_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int __cdecl _SEH2EnterFrameAndTrylevel(_SEH2Frame_t *, volatile _SEH2TryLevel_t *);
extern __attribute__((returns_twice)) int __cdecl _SEH2EnterFrameAndHandleTrylevel(_SEH2Frame_t *, volatile _SEH2HandleTryLevel_t *, void *);
extern __attribute__((returns_twice)) int __cdecl _SEH2EnterHandleTrylevel(_SEH2Frame_t *, volatile _SEH2HandleTryLevel_t *, void *);
extern void __cdecl _SEH2LeaveFrame(void);
extern void __cdecl _SEH2Return(void);

#ifdef __cplusplus
}
#endif

/* Prevent gcc from inlining functions that use SEH. */
#if ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 7))
static inline __attribute__((always_inline)) __attribute__((returns_twice)) void _SEH_DontInline() {}
#define __PREVENT_GCC_FROM_INLINING_SEH_FUNCTIONS() _SEH_DontInline();
#else
#define __PREVENT_GCC_FROM_INLINING_SEH_FUNCTIONS()
#endif

/* A no-op side effect that scares GCC */
#define __SEH_SIDE_EFFECT __asm__ __volatile__("#")

/* A no-op without any real side effects, but silences warnings */
#define __SEH_PRETEND_SIDE_EFFECT (void)0

/* Forces GCC to consider the specified label reachable */
#define __SEH_USE_LABEL(L_) if(__SEH_VOLATILE_FALSE) goto L_;

/* Makes GCC pretend the specified label is reachable, to silence warnings */
#define __SEH_PRETEND_USE_LABEL(L_) (void)(&&L_)

/* Soft memory barrier */
#define __SEH_BARRIER __asm__ __volatile__("#":::"memory")

/* GCC doesn't know that this equals zero */
#define __SEH_VOLATILE_ZERO ({ int zero = 0; __asm__ __volatile__("#" : "+g" (zero)); zero; })

#define __SEH_VOLATILE_FALSE __builtin_expect(__SEH_VOLATILE_ZERO, 0)
#define __SEH_VOLATILE_TRUE  __builtin_expect(!__SEH_VOLATILE_ZERO, 1)

#define ___SEH_STRINGIFY(X_) # X_
#define __SEH_STRINGIFY(X_) ___SEH_STRINGIFY(X_)

#define __SEH_EXCEPT_RET long
#define __SEH_EXCEPT_ARGS __attribute__((unused)) _SEH2Frame_t * _SEH2FrameP, __attribute__((unused)) struct _EXCEPTION_POINTERS * _SEHExceptionInformation
#define __SEH_EXCEPT_ARGS_ , __SEH_EXCEPT_ARGS
#define __SEH_EXCEPT_PFN __SEH_DECLARE_EXCEPT_PFN
#define __SEH_DECLARE_EXCEPT_PFN(NAME_) __SEH_EXCEPT_RET (__cdecl * NAME_)(__SEH_EXCEPT_ARGS)
#define __SEH_DECLARE_EXCEPT(NAME_) __SEH_EXCEPT_RET __cdecl NAME_(__SEH_EXCEPT_ARGS)
#define __SEH_DEFINE_EXCEPT(NAME_) __SEH_EXCEPT_RET __cdecl NAME_(__SEH_EXCEPT_ARGS)

#define __SEH_FINALLY_RET void
#define __SEH_FINALLY_ARGS void
#define __SEH_FINALLY_ARGS_
#define __SEH_FINALLY_PFN __SEH_DECLARE_FINALLY_PFN
#define __SEH_DECLARE_FINALLY_PFN(NAME_) __SEH_FINALLY_RET (__cdecl * NAME_)(__SEH_FINALLY_ARGS)
#define __SEH_DECLARE_FINALLY(NAME_) __SEH_FINALLY_RET __cdecl NAME_(__SEH_FINALLY_ARGS)
#define __SEH_DEFINE_FINALLY(NAME_) __SEH_FINALLY_RET __cdecl NAME_(__SEH_FINALLY_ARGS)

#define __SEH_RETURN_EXCEPT(R_) return (long)(R_)
#define __SEH_RETURN_FINALLY() return

#define __SEH_BEGIN_TRY \
	{ \
		__label__ _SEHEndTry; \
 \
		__SEH_PRETEND_USE_LABEL(_SEHEndTry); \
 \
		{ \
			__SEH_BARRIER;

#define __SEH_END_TRY \
			__SEH_BARRIER; \
		} \
		_SEHEndTry:; \
	}

#define __SEH_SET_TRYLEVEL(TRYLEVEL_) \
	{ \
		__SEH_BARRIER; _SEH2FrameP->SF_TopTryLevel = (TRYLEVEL_); __SEH_BARRIER; \
	}

#define __SEH_ENTER_FRAME_AND_TRYLEVEL(TRYLEVEL_) (_SEH2EnterFrameAndTrylevel(_SEH2FrameP, (TRYLEVEL_)))
#define __SEH_ENTER_TRYLEVEL(TRYLEVEL_) ((__SEH_SET_TRYLEVEL((TRYLEVEL_))), 0)

#define __SEH_ENTER_FRAME_AND_HANDLE_TRYLEVEL(TRYLEVEL_, HANDLE_) _SEH2EnterFrameAndHandleTrylevel(_SEH2FrameP, (TRYLEVEL_), (HANDLE_))
#define __SEH_ENTER_HANDLE_TRYLEVEL(TRYLEVEL_, HANDLE_) _SEH2EnterHandleTrylevel(_SEH2FrameP, (TRYLEVEL_), (HANDLE_))

#define __SEH_ENTER_SCOPE(TRYLEVEL_) (_SEHTopTryLevel ? __SEH_ENTER_FRAME_AND_TRYLEVEL(TRYLEVEL_) : __SEH_ENTER_TRYLEVEL(TRYLEVEL_))
#define __SEH_ENTER_HANDLE_SCOPE(TRYLEVEL_, HANDLE_) (({ __SEH_BARRIER; __asm__ __volatile__("mov %%esp, %0" : "=m" ((TRYLEVEL_)->SHT_Esp)); __SEH_BARRIER; }), (_SEHTopTryLevel ? __SEH_ENTER_FRAME_AND_HANDLE_TRYLEVEL((TRYLEVEL_), (HANDLE_)) : __SEH_ENTER_HANDLE_TRYLEVEL((TRYLEVEL_), (HANDLE_))))

#define __SEH_LEAVE_TRYLEVEL() \
	if(!_SEHTopTryLevel) \
	{ \
		__SEH_SET_TRYLEVEL(_SEHPrevTryLevelP); \
	} \

#define __SEH_LEAVE_FRAME() \
	if(_SEHTopTryLevel) \
	{ \
		_SEH2LeaveFrame(); \
	}

#define __SEH_END_SCOPE_CHAIN \
	static __attribute__((unused)) const int _SEH2ScopeKind = 1; \
	static __attribute__((unused)) _SEH2Frame_t * const _SEH2FrameP = 0; \
	static __attribute__((unused)) _SEH2TryLevel_t * const _SEH2TryLevelP = 0;

#define __SEH_BEGIN_SCOPE \
	for(;;) \
	{ \
		const int _SEHTopTryLevel = (_SEH2ScopeKind != 0); \
		_SEH2Frame_t * const _SEHCurFrameP = _SEH2FrameP; \
		volatile _SEH2TryLevel_t * const _SEHPrevTryLevelP = _SEH2TryLevelP; \
		__attribute__((unused)) int _SEHAbnormalTermination; \
 \
        (void)_SEHTopTryLevel; \
        (void)_SEHCurFrameP; \
        (void)_SEHPrevTryLevelP; \
 \
		{ \
			__label__ _SEHBeforeTry; \
			__label__ _SEHDoTry; \
			__label__ _SEHAfterTry; \
			static const int _SEH2ScopeKind = 0; \
			volatile _SEH2TryLevel_t _SEHTryLevel; \
			volatile _SEH2HandleTryLevel_t _SEHHandleTryLevel; \
			_SEH2Frame_t _SEH2Frame[_SEHTopTryLevel ? 1 : 0]; \
			volatile _SEH2TryLevel_t * _SEH2TryLevelP; \
			_SEH2Frame_t * const _SEH2FrameP = _SEHTopTryLevel ? \
				_SEH2Frame : _SEHCurFrameP; \
 \
			(void)_SEH2ScopeKind; \
			(void)_SEHTryLevel; \
			(void)_SEHHandleTryLevel; \
			(void)_SEH2FrameP; \
			(void)_SEH2TryLevelP; \
 \
			goto _SEHBeforeTry; \
 \
			_SEHDoTry:;

#define __SEH_END_SCOPE \
		} \
 \
		break; \
	}

#define __SEH_SCOPE_LOCALS \
	__label__ _SEHBeginExcept; \
	__label__ _SEHEndExcept; \
 \
	auto __SEH_DECLARE_FINALLY(_SEHFinally);

#define _SEH2_TRY \
	__PREVENT_GCC_FROM_INLINING_SEH_FUNCTIONS() \
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
		_SEHTryLevel.ST_Filter = 0; \
		_SEHTryLevel.ST_Body = &_SEHFinally; \
		_SEHTryLevel.ST_Next = _SEHPrevTryLevelP; \
		__SEH_ENTER_SCOPE(&_SEHTryLevel); \
		_SEH2TryLevelP = &_SEHTryLevel; \
 \
		_SEHAbnormalTermination = 1; \
 \
		goto _SEHDoTry; \
		_SEHAfterTry:; \
 \
		_SEHAbnormalTermination = 0; \
 \
		__SEH_LEAVE_TRYLEVEL(); \
 \
		_SEHFinally(); \
		goto _SEHEndExcept; \
 \
		_SEHBeginExcept:; \
 \
		__attribute__((noinline)) __SEH_DEFINE_FINALLY(_SEHFinally) \
		{ \
			__SEH_END_SCOPE_CHAIN; \
 \
			(void)_SEH2ScopeKind; \
			(void)_SEH2FrameP; \
			(void)_SEH2TryLevelP; \
 \
 			for(;; ({ __SEH_RETURN_FINALLY(); })) \
			{

#define _SEH2_EXCEPT(...) \
		} \
		__SEH_END_TRY; \
 \
		goto _SEHAfterTry; \
 \
		_SEHBeforeTry:; \
 \
		{ \
			__attribute__((unused)) struct _EXCEPTION_POINTERS * volatile _SEHExceptionInformation; \
 \
			if(__builtin_constant_p((__VA_ARGS__)) && (__VA_ARGS__) <= 0) \
			{ \
				if((__VA_ARGS__) < 0) \
				{ \
					_SEHTryLevel.ST_Filter = (void *)-1; \
					_SEHTryLevel.ST_Body = 0; \
				} \
				else \
				{ \
					_SEHTryLevel.ST_Filter = (void *)0; \
					_SEHTryLevel.ST_Body = 0; \
				} \
 \
				_SEHTryLevel.ST_Next = _SEHPrevTryLevelP; \
				__SEH_ENTER_SCOPE(&_SEHTryLevel); \
				_SEH2TryLevelP = &_SEHTryLevel; \
			} \
			else \
			{ \
				if(__builtin_constant_p((__VA_ARGS__)) && (__VA_ARGS__) > 0) \
					_SEHHandleTryLevel.SHT_Common.ST_Filter = (void *)1; \
				else \
				{ \
					__SEH_DEFINE_EXCEPT(_SEHExcept) \
					{ \
						__SEH_RETURN_EXCEPT((__VA_ARGS__)); \
					} \
 \
					_SEHHandleTryLevel.SHT_Common.ST_Filter = &_SEHExcept; \
				} \
 \
				_SEHHandleTryLevel.SHT_Common.ST_Next = _SEHPrevTryLevelP; \
				_SEH2TryLevelP = &_SEHHandleTryLevel.SHT_Common; \
 \
				if(__builtin_expect(__SEH_ENTER_HANDLE_SCOPE(&_SEHHandleTryLevel, &&_SEHBeginExcept), 0)) \
					goto _SEHBeginExcept; \
			} \
		} \
 \
		goto _SEHDoTry; \
 \
		__attribute__((unused)) __SEH_DEFINE_FINALLY(_SEHFinally) { __SEH_RETURN_FINALLY(); } \
 \
		_SEHAfterTry:; \
		__SEH_LEAVE_TRYLEVEL(); \
 \
		goto _SEHEndExcept; \
 \
		_SEHBeginExcept:; \
		{ \
			{ \
				__SEH_BARRIER;

#define _SEH2_END \
				__SEH_BARRIER; \
			} \
		} \
 \
		_SEHEndExcept:; \
 \
		__SEH_LEAVE_FRAME(); \
	} \
	__SEH_END_SCOPE;

#define _SEH2_GetExceptionInformation() (_SEHExceptionInformation)
#define _SEH2_GetExceptionCode() ((_SEH2FrameP)->SF_Code)
#define _SEH2_AbnormalTermination() (_SEHAbnormalTermination)

#define _SEH2_YIELD(STMT_) \
	for(;;) \
	{ \
		if(!_SEH2ScopeKind) \
			_SEH2Return(); \
 \
		STMT_; \
	}

#define _SEH2_LEAVE goto _SEHEndTry

__SEH_END_SCOPE_CHAIN;

#define _SEH2_VOLATILE volatile

#else
#error no PSEH support
#endif

#endif /* !KJK_PSEH2_H_ */

/* EOF */
