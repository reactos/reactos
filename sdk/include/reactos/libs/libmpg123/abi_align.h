/*
	mpg123lib_intern: Common non-public stuff for libmpg123

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	derived from the old mpg123.h
*/

#ifndef MPG123_H_ABI_ALIGN
#define MPG123_H_ABI_ALIGN

#include "config.h"

/* ABI conformance for other compilers.
   mpg123 needs 16byte-aligned stack for SSE and friends.
   gcc provides that, but others don't necessarily. */
#ifdef ABI_ALIGN_FUN
#ifndef attribute_align_arg
#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
/* The gcc that can align the stack does not need the check... nor does it work with gcc 4.3+, anyway. */
#else

#    define attribute_align_arg
/* Other compilers get code to catch misaligned stack.
   Well, except Sun Studio, which accepts the aligned attribute but does not honor it. */
#if !defined(__SUNPRO_C)
#    define NEED_ALIGNCHECK
#endif

#endif
#endif
#else
#define attribute_align_arg
/* We won't try the align check... */
#endif

#endif
