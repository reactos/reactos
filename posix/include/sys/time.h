/* $Id: time.h,v 1.5 2002/10/29 04:45:22 rex Exp $
 */
/*
 * sys/time.h
 *
 * time types. Conforming to the Single UNIX(r) Specification Version 2,
 * System Interface & Headers Issue 5
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
#ifndef __SYS_TIME_H_INCLUDED__
#define __SYS_TIME_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct timeval
{
 time_t      tv_sec;  /* seconds */
 suseconds_t tv_usec; /* microseconds */
};

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */

#endif /* __SYS_TIME_H_INCLUDED__ */

/* EOF */

