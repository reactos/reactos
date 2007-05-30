/* 
 * search.h
 *
 * Functions for searching and sorting.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Danny Smith  <dannysmith@users.sourceforge.net>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _SEARCH_H_
#define _SEARCH_H_

/* All the headers include this file. */
#include <_mingw.h>

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* bsearch and qsort are also declared in stdlib.h */
_CRTIMP void* __cdecl bsearch (const void*, const void*, size_t, size_t, 
			       int (*)(const void*, const void*));
_CRTIMP void __cdecl qsort (void*, size_t, size_t,
			    int (*)(const void*, const void*));

_CRTIMP void* __cdecl _lfind (const void*, const void*, unsigned int*,
			      unsigned int, int (*)(const void*, const void*));
_CRTIMP void* __cdecl _lsearch (const void*, void*, unsigned int*, unsigned int,
				int (*)(const void*, const void*));

#ifndef	_NO_OLDNAMES
_CRTIMP void* __cdecl lfind (const void*, const void*, unsigned int*,
			     unsigned int, int (*)(const void*, const void*));
_CRTIMP void* __cdecl lsearch (const void*, void*, unsigned int*, unsigned int,
			       int (*)(const void*, const void*));
#endif

#ifdef __cplusplus
}
#endif

#endif /* RC_INVOKED */

#endif /*  _SEARCH_H_ */
