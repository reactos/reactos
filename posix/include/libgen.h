/*
 * libgen.h
 *
 * definitions for pattern matching functions. Conforming to the Single
 * UNIX(r) Specification Version 2, System Interface & Headers Issue 5
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
#ifndef __LIBGEN_H_INCLUDED__
#define __LIBGEN_H_INCLUDED__

/* INCLUDES */
#include <stddef.h>

/* OBJECTS */
extern char *__loc1; /* LEGACY */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */
char  *basename(char *);
char  *dirname(char *);
char  *regcmp(const char *, ...); /* LEGACY */
char  *regex(const char *, const char *, ...); /* LEGACY */

wchar_t *_Wbasename(wchar_t *);
wchar_t *_Wdirname(wchar_t *);

/* MACROS */

#endif /* __LIBGEN_H_INCLUDED__ */

/* EOF */

