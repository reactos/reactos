/*
 * stddef.h
 *
 * Standard type definitions provided by the C library.
 *
 * NOTE: Actually supplied by the compiler (correct?). As such, GCC
 *       supplies a version of this header file. Unfortunately, GCC's
 *       version is all tied up with the way other headers for the
 *       GNU C library are implemented (or vice-versa), in a similar
 *       way to how the other Mingw32 headers are dependent on
 *       certain internals of this file. It is not clear to me whether
 *       you can safely use the GCC version in place of this version.
 *       TODO: Line up usage in other header files to work with GCC
 *       stddef.h.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1.2.1 $
 * $Author: dwelch $
 * $Date: 1999/03/15 16:19:26 $
 *
 */


#ifndef _STDDEF_H_

/*
 * Any one of these symbols __need_* means that a standard header file
 * wants us just to define one data type.  So don't define
 * the symbols that indicate this file's entire job has been done.
 */
#if (!defined(__need_wchar_t) && !defined(__need_wint_t)	\
     && !defined(__need_size_t) && !defined(__need_ptrdiff_t)	\
     && !defined(__need_NULL))
#define _STDDEF_H_
#endif

/*
 * NOTE: The following typedefs are done using __xxx_TYPE__ defines followed
 * by typedefs using those defines. I have chosen to do it this way because
 * GCC supplies definitions for __xxx_TYPE__ macros and if, for example, your
 * size_t is typedef'ed differently from what GCC expects it will give you
 * warnings when you prototype functions like memcmp and memcpy. The values
 * for __xxx_TYPE__ in this header file are the same as those given by GCC.
 * Those values appear to work with the CRTDLL functions.
 */

/*
 * Signed type of difference of two pointers. 
 */

/* Define this type if we are doing the whole job, or if we want this type
 * in particular.  */
#if defined (_STDDEF_H_) || defined (__need_ptrdiff_t)

#ifndef _PTRDIFF_T_
#define _PTRDIFF_T_
#ifndef __PTRDIFF_TYPE__
#define	__PTRDIFF_TYPE__	int
#endif
typedef __PTRDIFF_TYPE__	ptrdiff_t;
#endif

/* If this symbol has done its job, get rid of it.  */
#undef	__need_ptrdiff_t

#endif /* _STDDEF_H_ or __need_ptrdiff_t.  */

/*
 * Unsigned type of `sizeof' something.
 */


/* Define this type if we are doing the whole job,
 * or if we want this type in particular.  */
#if defined (_STDDEF_H_) || defined (__need_size_t)

#ifndef _SIZE_T_
#define _SIZE_T_
#define SIZE_T_DEFINED
#define _SIZE_T
#ifndef __SIZE_TYPE__
#define	__SIZE_TYPE__		unsigned int
#endif
typedef __SIZE_TYPE__		size_t;
#endif

#undef	__need_size_t

#endif /* _STDDEF_H_ or __need_size_t.  */

/* Wide character type.
   Locale-writers should change this as necessary to
   be big enough to hold unique values not between 0 and 127,
   and not (wchar_t) -1, for each defined multibyte character.  */

/* Define this type if we are doing the whole job,
   or if we want this type in particular.  */
#if defined (_STDDEF_H_) || defined (__need_wchar_t)

#ifndef _WCHAR_T_
#define _WCHAR_T_
#define _WCHAR_T
#ifndef __WCHAR_TYPE__
#define	__WCHAR_TYPE__		short unsigned int
#endif
#ifndef __cplusplus
typedef __WCHAR_TYPE__		wchar_t;
#endif	/* C++ */
#endif	/* wchar_t not already defined */

#undef	__need_wchar_t

#endif	/* _STDDEF_H_ or __need_wchar_t. */

/*
 * wint_t, the equivalent of int in wchar ctype functions.
 */
#if defined (_STDDEF_H_) || defined (__need_wint_t)

#ifndef _WINT_T_
#define _WINT_T_
#define _WINT_T		/* To satisfy libstdc++ */
#ifndef __WINT_TYPE__
#define	__WINT_TYPE__		short int
#endif	/* Not defined __WINT_TYPE__ */

typedef	__WINT_TYPE__		wint_t;
#endif	/* Not defined _WINT_T_ */

#undef	__need_wint_t

#endif	/* _STDDEF_H_ or __need_wint_t. */


/*
 * A null pointer constant.
 */

#if defined (_STDDEF_H_) || defined (__need_NULL)

#undef NULL
#define NULL (0)
#endif /* _STDDEF_H_ or __need_NULL */

#undef	__need_NULL


/*
 * Offsetof, a macro for finding the offset of a member in a structure.
 * Works by returning the 'address' of the MEMBER of a TYPE struct at address
 * zero.
 */

#if defined (_STDDEF_H_)
#define	offsetof(TYPE, MEMBER)	((size_t) &( ((TYPE *) 0)->MEMBER ))
#endif	/* _STDDEF_H_ */

#endif /* not _STDDEF_H_ */
