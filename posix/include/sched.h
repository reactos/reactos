/* $Id: sched.h,v 1.2 2002/02/20 09:17:54 hyperion Exp $
 */
/*
 * sched.h
 *
 * execution scheduling (REALTIME). Conforming to the Single UNIX(r)
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
#ifndef __SCHED_H_INCLUDED__
#define __SCHED_H_INCLUDED__

/* INCLUDES */
#include <time.h>

/* OBJECTS */

/* TYPES */
struct sched_param
{
 int sched_priority; /* process execution scheduling priority */
};

/* CONSTANTS */
/* First in-first out (FIFO) scheduling policy */
#define SCHED_FIFO  (1)
/* Round robin scheduling policy */
#define SCHED_RR    (2)
/* Another scheduling policy */
#define SCHED_OTHER (3)

/* PROTOTYPES */
int    sched_get_priority_max(int);
int    sched_get_priority_min(int);
int    sched_getparam(pid_t, struct sched_param *);
int    sched_getscheduler(pid_t);
int    sched_rr_get_interval(pid_t, struct timespec *);
int    sched_setparam(pid_t, const struct sched_param *);
int    sched_setscheduler(pid_t, int, const struct sched_param *);
int    sched_yield(void);

/* MACROS */

#endif /* __SCHED_H_INCLUDED__ */

/* EOF */

