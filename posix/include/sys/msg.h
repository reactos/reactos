/* $Id: msg.h,v 1.3 2002/05/17 01:37:15 hyperion Exp $
 */
/*
 * sys/msg.h
 *
 * message queue structures. Conforming to the Single UNIX(r) Specification
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
#ifndef __SYS_SOCKET_H_INCLUDED__
#define __SYS_SOCKET_H_INCLUDED__

/* INCLUDES */
#include <sys/ipc.h>

/* OBJECTS */

/* TYPES */
typedef unsigned int msgqnum_t; /* Used for the number of messages in the message queue */
typedef unsigned int msglen_t;  /* Used for the number of bytes allowed in a message queue */

struct msqid_ds
{
 struct ipc_perm msg_perm;   /* operation permission structure */
 msgqnum_t       msg_qnum;   /* number of messages currently on queue */
 msglen_t        msg_qbytes; /* maximum number of bytes allowed on queue */
 pid_t           msg_lspid;  /* process ID of last msgsnd() */
 pid_t           msg_lrpid;  /* process ID of last msgrcv() */
 time_t          msg_stime;  /* time of last msgsnd() */
 time_t          msg_rtime;  /* time of last msgrcv() */
 time_t          msg_ctime;  /* time of last change */
};

/* CONSTANTS */
/* Message operation flag */
#define MSG_NOERROR (0x00001000) /* No error if big message */

/* PROTOTYPES */
int       msgctl(int, int, struct msqid_ds *);
int       msgget(key_t, int);
ssize_t   msgrcv(int, void *, size_t, long int, int);
int       msgsnd(int, const void *, size_t, int);

/* MACROS */

#endif /* __SYS_SOCKET_H_INCLUDED__ */

/* EOF */

