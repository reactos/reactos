/*
 * malloc.h
 *
 * Support for programs which want to use malloc.h to get memory management
 * functions. Unless you absolutely need some of these functions and they are
 * not in the ANSI headers you should use the ANSI standard header files
 * instead.
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
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <msvcrt/alloc.h>

#ifdef  __cplusplus
extern "C" {
#endif

int _heapchk(void);    /* Verify heap integrety. */
int _heapmin(void);    /* Return unused heap to the OS. */
int _heapset(unsigned int);
size_t _msize(void*);
void* _expand(void*, size_t);

#ifdef __cplusplus
}
#endif

#endif /* Not _MALLOC_H_ */

#endif /* Not __STRICT_ANSI__ */
