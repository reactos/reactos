/* $Id: types.h,v 1.2 2002/02/20 09:17:56 hyperion Exp $
 */
/*
 * sys/types.h
 *
 * data types. Conforming to the Single UNIX(r) Specification Version 2,
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
#ifndef __SYS_TYPES_H_INCLUDED__
#define __SYS_TYPES_H_INCLUDED__

/* INCLUDES */
#include <inttypes.h>

/* OBJECTS */

/* TYPES */
/* FIXME: all these types need to be checked */
typedef unsigned long int blkcnt_t; /* Used for file block counts */
typedef unsigned long int blksize_t; /* Used for block sizes */
typedef long long clock_t; /* Used for system times in clock ticks or CLOCKS_PER_SEC */
typedef int clockid_t; /* Used for clock ID type in the clock and timer functions. */
typedef int dev_t; /* Used for device IDs. */
typedef long long fsblkcnt_t; /* Used for file system block counts */
typedef long long fsfilcnt_t; /* Used for file system file counts */
typedef int gid_t; /* Used for group IDs. */
typedef int id_t; /* Used as a general identifier; can be used to contain at least a
                        pid_t, uid_t or a gid_t. */
typedef uint64_t ino_t; /* Used for file serial numbers. */
typedef int key_t; /* Used for interprocess communication. */
typedef int mode_t; /* Used for some file attributes. */
typedef int nlink_t; /* Used for link counts. */
typedef int64_t off_t; /* Used for file sizes. */
typedef unsigned long int pid_t; /* Used for process IDs and process group IDs. */

/* pthread types */
typedef void * pthread_cond_t; /* Used for condition variables. */
typedef void * pthread_condattr_t; /* Used to identify a condition attribute object. */
typedef void * pthread_key_t; /* Used for thread-specific data keys. */
typedef void * pthread_attr_t; /* Used to identify a thread attribute object. */

typedef void * pthread_mutex_t;
typedef void * pthread_mutexattr_t;

typedef void * pthread_once_t; /* Used for dynamic package initialisation. */
typedef void * pthread_rwlock_t; /* Used for read-write locks. */
typedef void * pthread_rwlockattr_t; /* Used for read-write lock attributes. */
typedef unsigned long int pthread_t; /* Used to identify a thread. */

typedef unsigned long int size_t; /* Used for sizes of objects. */
typedef long int ssize_t; /* Used for a count of bytes or an error indication. */
typedef long long suseconds_t; /* Used for time in microseconds */
typedef unsigned long int time_t; /* Used for time in seconds. */
typedef void * timer_t; /* Used for timer ID returned by timer_create(). */
typedef int uid_t; /* Used for user IDs. */
typedef unsigned long long useconds_t; /* Used for time in microseconds. */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */

#endif /* __SYS_TYPES_H_INCLUDED__ */

/* EOF */

