/* $Id: aio.h,v 1.4 2002/10/29 04:45:06 rex Exp $
 */
/*
 * aio.h
 *
 * asynchronous input and output (REALTIME). Conforming to the Single UNIX(r)
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
#ifndef __AIO_H_INCLUDED__
#define __AIO_H_INCLUDED__

/* INCLUDES */
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>

/* OBJECTS */

/* TYPES */
typedef struct _tag_aiocb
{
 int             aio_fildes;     /* file descriptor */
 off_t           aio_offset;     /* file offset */
 volatile void*  aio_buf;        /* location of buffer */
 size_t          aio_nbytes;     /* length of transfer */
 int             aio_reqprio;    /* request priority offset */
 struct sigevent aio_sigevent;   /* signal number and value */
 int             aio_lio_opcode; /* operation to be performed */
} aiocb;

/* CONSTANTS */
#define AIO_CANCELED    0
#define AIO_NOTCANCELED 1
#define AIO_ALLDONE     2

#define LIO_WAIT        0
#define LIO_NOWAIT      1
#define LIO_READ        2
#define LIO_WRITE       3
#define LIO_NOP         4

/* PROTOTYPES */
int      aio_cancel(int, struct aiocb *);
int      aio_error(const struct aiocb *);
int      aio_fsync(int, struct aiocb *);
int      aio_read(struct aiocb *);
ssize_t  aio_return(struct aiocb *);
int      aio_suspend(const struct aiocb *const[], int, const struct timespec *);
int      aio_write(struct aiocb *);
int      lio_listio(int, struct aiocb *const[], int, struct sigevent *);

/* MACROS */

#endif /* __AIO_H_INCLUDED__ */

/* EOF */

