/* $Id: glob.h,v 1.4 2002/10/29 04:45:10 rex Exp $
 */
/*
 * glob.h
 *
 * pathname pattern-matching types. Conforming to the Single UNIX(r)
 * Specification Version 2, System Interface & Headers Issue 5
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
#ifndef __GLOB_H_INCLUDED__
#define __GLOB_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
typedef struct __tagglob_t
{
 size_t   gl_pathc; /* count of paths matched by pattern */
 char   **gl_pathv; /* pointer to a list of matched pathnames */
 size_t   gl_offs;  /* slots to reserve at the beginning of gl_pathv */
} glob_t;

/* CONSTANTS */
/* Values for the flags argument */
#define GLOB_APPEND   (0x00000001) /* Append generated pathnames to \
                                      those previously obtained. */
#define GLOB_DOOFFS   (0x00000002) /* Specify how many null pointers to \
                                      add to the beginning of */
#define GLOB_ERR      (0x00000004) /* Cause glob() to return on error. */
#define GLOB_MARK     (0x00000008) /* Each pathname that is a directory \
                                      that matches pattern has a slash \
                                      appended. */
#define GLOB_NOCHECK  (0x00000010) /* If pattern does not match any pathname, \
                                      then return a list consisting of only \
                                      pattern. */
#define GLOB_NOESCAPE (0x00000020) /* Disable backslash escaping. */
#define GLOB_NOSORT   (0x00000040) /* Do not sort the pathnames returned. */

/* Error return values */
#define GLOB_ABORTED (-1) /* The scan was stopped because GLOB_ERR was set \
                             or errfunc returned non-zero. */
#define GLOB_NOMATCH (-2) /* The pattern does not match any existing pathname, \
                             and GLOB_NOCHECK was not set in flags. */
#define GLOB_NOSPACE (-3) /* An attempt to allocate memory failed. */
#define GLOB_NOSYS   (-4) /* The implementation does not support this function. */

/* PROTOTYPES */
int glob(const char *, int, int (*)(const char *, int), glob_t *);
void globfree (glob_t *);

/* MACROS */

#endif /* __GLOB_H_INCLUDED__ */

/* EOF */

