/*
	abi_align: An attempt to avoid breakage because of mixing
	compilers with different alignment.

	copyright 1995-2015 by the mpg123 project
	free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	There used to be code that checks alignment, but it did not really
	work anyway. The only straw we have is putting that alignment
	attribute to API functions.
*/

#ifndef MPG123_H_ABI_ALIGN
#define MPG123_H_ABI_ALIGN

#include "config.h"

/* ABI conformance for other compilers.
   mpg123 needs 16byte-aligned (or more) stack for SSE and friends.
   gcc provides that, but others don't necessarily. */
#ifdef ABI_ALIGN_FUN

#ifndef attribute_align_arg

#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
/* The gcc that can align the stack does not need the check... nor does it work with gcc 4.3+, anyway. */
#else
#    define attribute_align_arg
#endif

#endif  /* attribute_align_arg */

#else /* ABI_ALIGN_FUN */

#define attribute_align_arg

#endif /* ABI_ALIGN_FUN */

#endif /* MPG123_H_ABI_ALIGN */
