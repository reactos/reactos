/*
 * stdarg.h
 *
 * handle variable argument list. Based on the Single UNIX(r)
 * Specification, Version 2
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
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
 */
#ifndef __STDARG_H_INCLUDED__
#define __STDARG_H_INCLUDED__

/* OBJECTS */

/* TYPES */
typedef char* va_list;

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */
/* taken from mingw's stdarg.h */

/* Amount of space required in an argument list (ie. the stack) for an
   argument of type t. */
#define __va_argsiz(t) \
 (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) \
 ((ap) = ((va_list) (&pN) + __va_argsiz(pN)))

#define va_end(ap) ((void)0)

#define va_arg(ap, t) \
 (((ap) = (ap) + __va_argsiz(t)), \
 *((t*) (void*) ((ap) - __va_argsiz(t))))

#endif /* __STDARG_H_INCLUDED__ */

/* EOF */

