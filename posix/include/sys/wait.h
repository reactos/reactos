/* $Id: wait.h,v 1.3 2002/05/17 01:37:15 hyperion Exp $
 */
/*
 * sys/wait.h
 *
 * declarations for waiting. Conforming to the Single UNIX(r) Specification
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
#ifndef __SYS_WAIT_H_INCLUDED__
#define __SYS_WAIT_H_INCLUDED__

/* INCLUDES */
#include <signal.h>
#include <sys/resource.h>

/* OBJECTS */

/* TYPES */
typedef enum __tagidtype_t
{
 P_ALL,
 P_PID,
 P_PGID
} idtype_t;

/* CONSTANTS */
/* Possible values for the options argument to waitid() */
#define WEXITED    (0x00000001) /* Wait for processes that have exited */
#define WSTOPPED   (0x00000002) /* Status will be returned for any child that has stopped upon receipt of a signal */
#define WNOWAIT    (0x00000004) /* Keep the process whose status is returned in infop in a waitable state */

#define WCONTINUED (0x00000008) /* Status will be returned for any child that was stopped and has been continued */
#define WNOHANG    (0x00000010) /* Return immediately if there are no children to wait for */
#define WUNTRACED  (0x00000020) /* Report status of stopped child process */

/* PROTOTYPES */
pid_t  wait(int *);
pid_t  wait3(int *, int, struct rusage *);
int    waitid(idtype_t, id_t, siginfo_t *, int);
pid_t  waitpid(pid_t, int *, int);

/* MACROS */
/* Macros for analysis of process status values */
#define WEXITSTATUS(__STATUS__)  (1) /* Return exit status */
#define WIFCONTINUED(__STATUS__) (1) /* True if child has been continued */
#define WIFEXITED(__STATUS__)    (1) /* True if child exited normally */
#define WIFSIGNALED(__STATUS__)  (1) /* True if child exited due to uncaught signal */
#define WIFSTOPPED(__STATUS__)   (1) /* True if child is currently stopped */
#define WSTOPSIG(__STATUS__)     (1) /* Return signal number that caused process to stop */
#define WTERMSIG(__STATUS__)     (1) /* Return signal number that caused process to terminate */

#endif /* __SYS_WAIT_H_INCLUDED__ */

/* EOF */

