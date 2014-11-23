/* $Id: fmtmsg.h,v 1.4 2002/10/29 04:45:08 rex Exp $
 */
/*
 * fmtmsg.h
 *
 * message display structures. Conforming to the Single UNIX(r)
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
#ifndef __FMTMSG_H_INCLUDED__
#define __FMTMSG_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */
/* Major Classifications */
/* NOTE: these are unique values, not flags. Their bits can overlap, but
   cannot overlap with those of other categories */
#define MM_HARD (0x00000001) /* Source of the condition is hardware. */
#define MM_SOFT (0x00000002) /* Source of the condition is software. */
#define MM_FIRM (0x00000003) /* Source of the condition is firmware. */

/* Message Source Subclassifications */
/* NOTE: these are unique values, not flags. Their bits can overlap, but
   cannot overlap with those of other categories */
#define MM_APPL  (0x00000010) /* Condition detected by application. */
#define MM_UTIL  (0x00000020) /* Condition detected by utility. */
#define MM_OPSYS (0x00000030) /* Condition detected by operating system. */

/* Status Subclassifications */
/* NOTE: these are unique values, not flags. Their bits can overlap, but
   cannot overlap with those of other categories */
#define MM_RECOVER (0x00000100) /* Recoverable error. */
#define MM_NRECOV  (0x00000200) /* Non-recoverable error. */

/* Display Subclassifications */
/* NOTE: these, unlike other classification constants, are flags. Their
   bits must be distinct */
#define MM_PRINT   (0x00001000) /* Display message on standard error. */
#define MM_CONSOLE (0x00002000) /* Display message on system console. */

/* Identifiers for the levels of severity */
#define MM_NOSEV   (0) /* No severity level provided for the message. */
#define MM_INFO    (1) /* Informative message. */
#define MM_WARNING (2) /* Application has detected unusual non-error \
                          condition. */
#define MM_ERROR   (3) /* Application has encountered a non-fatal fault. */
#define MM_HALT    (4) /* Error causing application to halt. */

/* Null values and identifiers */
#define MM_NULLLBL ((char *)0) /* Null label */
#define MM_NULLSEV (0)         /* Null severity */
#define MM_NULLMC  (0L)        /* Null class */
#define MM_NULLTXT ((char *)0) /* Null text */
#define MM_NULLACT ((char *)0) /* Null action */
#define MM_NULLTAG ((char *)0) /* Null tag */

/* Return values */
#define MM_OK    ( 0) /* The function succeeded. */
#define MM_NOTOK (-1) /* The function failed completely. */
#define MM_NOMSG (-2) /* The function was unable to generate a message on \
                         standard error, but otherwise succeeded. */
#define MM_NOCON (-3) /* The function was unable to generate a console \
                         message, but otherwise succeeded. */

/* PROTOTYPES */
int fmtmsg(long, const char*, int, const char*, const char*, const char*);

/* MACROS */

#endif /* __FMTMSG_H_INCLUDED__ */

/* EOF */

