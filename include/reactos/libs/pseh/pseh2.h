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
	void * volatile SF_FramePointer;
	void * volatile SF_StackPointer;
	volatile unsigned long SF_Code;
}
_SEH2Frame_t;

typedef struct __SEH2TryLevel
{
	volatile struct __SEH2TryLevel * ST_Next;
	void * ST_FramePointer;
	void * ST_Filter;
	void * ST_Body;
}
_SEH2TryLevel_t;

#define _SEH2_TRY __try
#define _SEH2_FINALLY __finally
#define _SEH2_EXCEPT(E_) __except((E_))
#define _SEH2_END
#define _SEH2_GetExceptionInformation() ((struct _EXCEPTION_POINTERS *)_exception_info())
#define _SEH2_GetExceptionCode() _exception_code()
#define _SEH2_AbnormalTermination() _abnormal_termination()
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE __leave

#ifdef  __cplusplus
extern "C" {
#endif

unsigned long __cdecl _exception_code(void);
void * __cdecl _exception_info(void);
int __cdecl _abnormal_termination(void);

#ifdef  __cplusplus
}
#endif

#endif

/* EOF */
