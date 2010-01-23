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

#ifndef KJK_PSEH_H_
#define KJK_PSEH_H_

/* Some useful macros */
#if defined(__cplusplus)
#	define _SEH_PVOID_CAST(TYPE_, P_) ((TYPE_)(P_))
#else
#	define _SEH_PVOID_CAST(TYPE_, P_) (P_)
#endif

#if defined(FIELD_OFFSET)
#	define _SEH_FIELD_OFFSET FIELD_OFFSET
#else
#	include <stddef.h>
#	define _SEH_FIELD_OFFSET offsetof
#endif

#if defined(CONTAINING_RECORD)
#	define _SEH_CONTAINING_RECORD CONTAINING_RECORD
#else
#	define _SEH_CONTAINING_RECORD(ADDR_, TYPE_, FIELD_) \
	((TYPE_ *)(((char *)(ADDR_)) - _SEH_FIELD_OFFSET(TYPE_, FIELD_)))
#endif

#if defined(__CONCAT)
#	define _SEH_CONCAT __CONCAT
#else
#	define _SEH_CONCAT1(X_, Y_) X_ ## Y_
#	define _SEH_CONCAT(X_, Y_) _SEH_CONCAT1(X_, Y_)
#endif

/*
	Note: just define __inline to an empty symbol if your C compiler doesn't
	support it
*/
#ifdef __cplusplus
#	ifndef __inline
#		define __inline inline
#	endif
#endif

/* Locals sharing support */
#define _SEH_LOCALS_TYPENAME(BASENAME_) \
	struct _SEH_CONCAT(_SEHLocalsTag, BASENAME_)

#define _SEH_DEFINE_LOCALS(BASENAME_) \
	_SEH_LOCALS_TYPENAME(BASENAME_)

#define _SEH_DECLARE_LOCALS(BASENAME_) \
	_SEH_LOCALS_TYPENAME(BASENAME_) _SEHLocals; \
	_SEH_LOCALS_TYPENAME(BASENAME_) * _SEHPLocals; \
	_SEHPLocals = &_SEHLocals;

/* Dummy locals */
static int _SEHLocals;
static void * const _SEHDummyLocals = &_SEHLocals;

#include <pseh/framebased.h>

#endif

/* EOF */
