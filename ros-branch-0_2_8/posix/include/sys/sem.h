/* $Id: sem.h,v 1.5 2002/10/29 04:45:21 rex Exp $
 */
/*
 * sys/sem.h
 *
 * semaphore facility. Conforming to the Single UNIX(r) Specification
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
#ifndef __SYS_SEM_H_INCLUDED__
#define __SYS_SEM_H_INCLUDED__

/* INCLUDES */
#include <sys/ipc.h>

/* OBJECTS */

/* TYPES */
struct semid_ds
{
 struct ipc_perm    sem_perm;  /* operation permission structure */
 unsigned short int sem_nsems; /* number of semaphores in set */
 time_t             sem_otime; /* last semop time */
 time_t             sem_ctime; /* last time changed by semctl() */
};

struct sembuf
{
 unsigned short int sem_num; /* semaphore number */
 short int          sem_op;  /* semaphore operation */
 short int          sem_flg; /* operation flags */
};

/* CONSTANTS */
/* Semaphore operation flags */
#define SEM_UNDO (0x00001000) /* Set up adjust on exit entry */

/* Command definitions for the function semctl() */
#define GETNCNT (1) /* Get semncnt */
#define GETPID  (2) /* Get sempid */
#define GETVAL  (3) /* Get semval */
#define GETALL  (4) /* Get all cases of semval */
#define GETZCNT (5) /* Get semzcnt */
#define SETVAL  (6) /* Set semval */
#define SETALL  (7) /* Set all cases of semval */

/* PROTOTYPES */
int   semctl(int, int, int, ...);
int   semget(key_t, int, int);
int   semop(int, struct sembuf *, size_t);

/* MACROS */

#endif /* __SYS_SEM_H_INCLUDED__ */

/* EOF */

