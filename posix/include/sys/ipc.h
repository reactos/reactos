/* $Id: ipc.h,v 1.3 2002/05/17 01:37:15 hyperion Exp $
 */
/*
 * sys/ipc.h
 *
 * interprocess communication access structure. Conforming to the Single
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
#ifndef __SYS_IPC_H_INCLUDED__
#define __SYS_IPC_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct ipc_perm
{
 uid_t  uid;  /* owner's user ID */
 gid_t  gid;  /* owner's group ID */
 uid_t  cuid; /* creator's user ID */
 gid_t  cgid; /* creator's group ID */
 mode_t mode; /* read/write permission */
};

/* CONSTANTS */
/* Mode bits */
#define IPC_CREAT  (0x00000200) /* Create entry if key does not exist */
#define IPC_EXCL   (0x00000400) /* Fail if key exists */
#define IPC_NOWAIT (0x00000800) /* Error if request must wait */

/* Keys */
#define IPC_PRIVATE (0xFFFFFFFF) /* Private key */

/* Control commands */
#define IPC_RMID (1) /* Remove identifier */
#define IPC_SET  (2) /* Set options */
#define IPC_STAT (3) /* Get options */

/* PROTOTYPES */
key_t  ftok(const char *, int);

/* MACROS */

#endif /* __SYS_IPC_H_INCLUDED__ */

/* EOF */

