/*
	aligncheck: Some insane hacking to ensure stack alignment on library entry.

	copyright 2010 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	intially written by Thomas Orgis

	Include this header once at the appropriate place. It is not supposed for general use...
*/


#define ALIGNCHECK(mh)
#define ALIGNCHECKK
/* On compilers that support data alignment but not the automatic stack realignment.
   We check for properly aligned stack before risking a crash because of badly compiled
   client program. */
#if (defined CCALIGN) && (defined NEED_ALIGNCHECK) && ((defined DEBUG) || (defined CHECK_ALIGN))

/* Common building block. */
#define ALIGNMAINPART \
	/* minimum size of 16 bytes, not all compilers would align a smaller piece of data */ \
	double ALIGNED(16) altest[2]; \
	debug2("testing alignment, with %lu %% 16 = %lu", \
		(unsigned long)altest, (unsigned long)((size_t)altest % 16)); \
	if((size_t)altest % 16 != 0)

#undef ALIGNCHECK
#define ALIGNCHECK(mh) \
	ALIGNMAINPART \
	{ \
		error("Stack variable is not aligned! Your combination of compiler/library is dangerous!"); \
		if(mh != NULL) mh->err = MPG123_BAD_ALIGN; \
\
		return MPG123_ERR; \
	}
#undef ALIGNCHECKK
#define ALIGNCHECKK \
	ALIGNMAINPART \
	{ \
		error("Stack variable is not aligned! Your combination of compiler/library is dangerous!"); \
		return MPG123_BAD_ALIGN; \
	}

#endif
