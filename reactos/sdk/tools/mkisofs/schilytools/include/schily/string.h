/* @(#)string.h	1.12 11/11/24 Copyright 1996-2011 J. Schilling */
/*
 *	Definitions for strings
 *
 *	Copyright (c) 1996-2011 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef _SCHILY_STRING_H
#define	_SCHILY_STRING_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>		/* Try to get size_t */
#endif

/*
 * It may be that IBM's AIX has problems when doing
 * #include <string.h>
 * #include <strings.h>
 * So I moved the #include <strings.h> to the top. As the file strings.h
 * defines strcasecmp() we may need it...
 *
 * Note that the only real problem appears if we use rubbish FSF based code that
 * #defines _NO_PROTO
 */
#ifdef	HAVE_STRINGS_H
#ifndef	_INCL_STRINGS_H
#include <strings.h>
#define	_INCL_STRINGS_H
#endif
#endif	/* HAVE_STRINGS_H */


#ifdef	HAVE_STRING_H
#ifndef	_INCL_STRING_H
#include <string.h>
#define	_INCL_STRING_H
#endif
#else	/* HAVE_STRING_H */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef NULL
#define	NULL	0
#endif

extern void *memcpy	__PR((void *, const void *, int));
extern void *memmove	__PR((void *, const void *, int));
extern char *strcpy	__PR((char *, const char *));
extern char *strncpy	__PR((char *, const char *, int));

extern char *strcat	__PR((char *, const char *));
extern char *strncat	__PR((char *, const char *, int));

extern int memcmp	__PR((const void *, const void *, int));
extern int strcmp	__PR((const char *, const char *));
extern int strcoll	__PR((const char *, const char *));
extern int strncmp	__PR((const char *, const char *, int));
extern int strxfrm	__PR((char *, const char *, int));

extern void *memchr	__PR((const void *, int, int));
extern char *strchr	__PR((const char *, int));

extern int strcspn	__PR((const char *, const char *));
/* #pragma int_to_unsigned strcspn */

extern char *strpbrk	__PR((const char *, const char *));
extern char *strrchr	__PR((const char *, int));

extern int strspn	__PR((const char *, const char *));
/* #pragma int_to_unsigned strspn */

extern char *strstr	__PR((const char *, const char *));
extern char *strtok	__PR((char *, const char *));
extern void *memset	__PR((void *, int, int));
extern char *strerror	__PR((int));

extern int strlen	__PR((const char *));
/* #pragma int_to_unsigned strlen */

extern void *memccpy	__PR((void *, const void *, int, int));

extern int strcasecmp	__PR((const char *, const char *));
extern int strncasecmp	__PR((const char *, const char *, size_t));

#ifdef	__cplusplus
}
#endif

#endif	/* HAVE_STRING_H */

#endif	/* _SCHILY_STRING_H */
