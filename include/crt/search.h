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
/*
Documentation for these POSIX definitions and prototypes can be found in 
The Open Group Base Specifications Issue 6
IEEE Std 1003.1, 2004 Edition.
eg:  http://www.opengroup.org/onlinepubs/009695399/functions/twalk.html
*/


typedef struct entry {
	char *key;
	void *data;
} ENTRY;

typedef enum {
	FIND,
	ENTER
} ACTION;

typedef enum {
	preorder,
	postorder,
	endorder,
	leaf
} VISIT;

#ifdef _SEARCH_PRIVATE
typedef struct node {
	char         *key;
	struct node  *llink, *rlink;
} node_t;
#endif

void * __cdecl tdelete (const void * __restrict__, void ** __restrict__,
			int (*)(const void *, const void *))
			__MINGW_ATTRIB_NONNULL (1)  __MINGW_ATTRIB_NONNULL (3);
void * __cdecl tfind (const void *, void * const *,
		      int (*)(const void *, const void *))
		      __MINGW_ATTRIB_NONNULL (1)  __MINGW_ATTRIB_NONNULL (3);
void * __cdecl tsearch (const void *, void **, 
			int (*)(const void *, const void *))
			__MINGW_ATTRIB_NONNULL (1)  __MINGW_ATTRIB_NONNULL (3);
void __cdecl twalk (const void *, void (*)(const void *, VISIT, int));

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
