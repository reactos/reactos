#pragma once

/*	msc.h
	portability definitions for msc (microsof c or vc compilers)
	20091216 jcatena
	This shouldn't be necessary if code uses plaf.h macros instead of hardwired compiler specific directives
*/

#if defined(_MSC_VER)
#if 1
#if !defined(__cplusplus) 
#define inline _INLINE
#endif

#define __inline__ _INLINE
#define FORCEINLINE _INLINEF
#define __attribute__(packed)

#endif
#endif	// #if defined(_MSC_VER)
