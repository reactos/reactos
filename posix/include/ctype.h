/*
 * ctype.h
 *
 * character types. Based on the Single UNIX(r) Specification, Version 2
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
#ifndef __CTYPE_H_INCLUDED__
#define __CTYPE_H_INCLUDED__

#ifdef __PSXDLL__

/* headers for internal usage by psxdll.dll and ReactOS */

#else /* ! __PSXDLL__ */

/* standard POSIX headers */

#endif

/* types */

/* constants */

/* prototypes */
int   isalnum(int);
int   isalpha(int);
int   isascii(int);
int   iscntrl(int);
int   isdigit(int);
int   isgraph(int);
int   islower(int);
int   isprint(int);
int   ispunct(int);
int   isspace(int);
int   isupper(int);
int   isxdigit(int);
int   toascii(int);
int   tolower(int);
int   toupper(int);

/* macros */
/* FIXME: the standard isn't clear about these */
#define _toupper(c) (toupper(c))
#define _tolower(c) (tolower(c))

#endif /* __CTYPE_H_INCLUDED__ */

/* EOF */

