/* $Id: utime.h,v 1.4 2002/10/29 04:45:26 rex Exp $
 */
/*
 * utime.h
 *
 * access and modification times structure. Conforming to the Single
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
#ifndef __UTIME_H_INCLUDED__
#define __UTIME_H_INCLUDED__

/* INCLUDES */
#include <sys/types.h>

/* OBJECTS */

/* TYPES */
struct utimbuf
{
 time_t actime;  /* access time */
 time_t modtime; /* modification time */
};

/* CONSTANTS */

/* PROTOTYPES */
int utime(const char *, const struct utimbuf *);

/* MACROS */

#endif /* __UTIME_H_INCLUDED__ */

/* EOF */

