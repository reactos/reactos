/* $Id: shm.h,v 1.5 2002/10/29 04:45:21 rex Exp $
 */
/*
 * sys/shm.h
 *
 * shared memory facility. Conforming to the Single UNIX(r) Specification
 * Version 2, System Interface & Headers Issue 5
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
#ifndef __SYS_SHM_H_INCLUDED__
#define __SYS_SHM_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
typedef unsigned short shmatt_t;

struct shmid_ds
{
 struct ipc_perm shm_perm;  /* operation permission structure */
 size_t          shm_segsz;  /* size of segment in bytes */
 pid_t           shm_lpid;   /* process ID of last shared memory operation */
 pid_t           shm_cpid;   /* process ID of creator */
 shmatt_t        shm_nattch; /* number of current attaches */
 time_t          shm_atime;  /* time of last shmat() */
 time_t          shm_dtime;  /* time of last shmdt() */
 time_t          shm_ctime;  /* time of last change by shmctl() */
};

/* CONSTANTS */
#define SHM_RDONLY (0x00000200) /* Attach read-only (else read-write). */
#define SHM_RND    (0x00000400) /* Round attach address to SHMLBA. */

#define SHMLBA     (4096) /* Segment low boundary address multiple. */

/* PROTOTYPES */
void *shmat(int, const void *, int);
int   shmctl(int, int, struct shmid_ds *);
int   shmdt(const void *);
int   shmget(key_t, size_t, int);

/* MACROS */

#endif /* __SYS_SHM_H_INCLUDED__ */

/* EOF */

