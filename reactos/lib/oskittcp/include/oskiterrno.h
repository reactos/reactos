/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)errno.h	8.5 (Berkeley) 1/21/94
 */

#ifndef _OSK_SYS_ERRNO_H_
#define _OSK_SYS_ERRNO_H_

#ifndef __REACTOS__
#ifndef KERNEL
extern int errno;			/* global error number */
#endif
#endif

#define	OSK_EPERM		1		/* Operation not permitted */
#define	OSK_ENOENT		2		/* No such file or directory */
#define	OSK_ESRCH		3		/* No such process */
#define	OSK_EINTR		4		/* Interrupted system call */
#define	OSK_EIO		5		/* Input/output error */
#define	OSK_ENXIO		6		/* Device not configured */
#define	OSK_E2BIG		7		/* Argument list too long */
#define	OSK_ENOEXEC		8		/* Exec format error */
#define	OSK_EBADF		9		/* Bad file descriptor */
#define	OSK_ECHILD		10		/* No child processes */
#define	OSK_EDEADLK		11		/* Resource deadlock avoided */
/* 11 was OSK_EAGAIN */
#define	OSK_ENOMEM		12		/* Cannot allocate memory */
#define	OSK_EACCES		13		/* Permission denied */
#define	OSK_EFAULT		14		/* Bad address */
#ifndef _POSIX_SOURCOSK_E
#define	OSK_ENOTBLK		15		/* Block device required */
#endif
#define	OSK_EBUSY		16		/* Device busy */
#define	OSK_EEXIST		17		/* File exists */
#define	OSK_EXDEV		18		/* Cross-device link */
#define	OSK_ENODEV		19		/* Operation not supported by device */
#define	OSK_ENOTDIR		20		/* Not a directory */
#define	OSK_EISDIR		21		/* Is a directory */
#define	OSK_EINVAL		22		/* Invalid argument */
#define	OSK_ENFILE		23		/* Too many open files in system */
#define	OSK_EMFILE		24		/* Too many open files */
#define	OSK_ENOTTY		25		/* Inappropriate ioctl for device */
#ifndef _POSIX_SOURCE
#define	OSK_ETXTBSY		26		/* Text file busy */
#endif
#define	OSK_EFBIG		27		/* File too large */
#define	OSK_ENOSPC		28		/* No space left on device */
#define	OSK_ESPIPE		29		/* Illegal seek */
#define	OSK_EROFS		30		/* Read-only file system */
#define	OSK_EMLINK		31		/* Too many links */
#define	OSK_EPIPE		32		/* Broken pipe */

/* math software */
#define	OSK_EDOM		33		/* Numerical argument out of domain */
#define	OSK_ERANGE		34		/* Result too large */

/* non-blocking and interrupt i/o */
#define	OSK_EAGAIN		35		/* Resource temporarily unavailable */
#ifndef _POSIX_SOURCE
#define	OSK_EWOULDBLOCK	EAGAIN		/* Operation would block */
#define	OSK_EINPROGRESS	36		/* Operation now in progress */
#define	OSK_EALREADY	37		/* Operation already in progress */

/* ipc/network software -- argument errors */
#define	OSK_ENOTSOCK	38		/* Socket operation on non-socket */
#define	OSK_EDESTADDRREQ	39		/* Destination address required */
#define	OSK_EMSGSIZE	40		/* Message too long */
#define	OSK_EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	OSK_ENOPROTOOPT	42		/* Protocol not available */
#define	OSK_EPROTONOSUPPORT	43		/* Protocol not supported */
#define	OSK_ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	OSK_EOPNOTSUPP	45		/* Operation not supported */
#define	OSK_EPFNOSUPPORT	46		/* Protocol family not supported */
#define	OSK_EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	OSK_EADDRINUSE	48		/* Address already in use */
#define	OSK_EADDRNOTAVAIL	49		/* Can't assign requested address */

/* ipc/network software -- operational errors */
#define	OSK_ENETDOWN	50		/* Network is down */
#define	OSK_ENETUNREACH	51		/* Network is unreachable */
#define	OSK_ENETRESET	52		/* Network dropped connection on reset */
#define	OSK_ECONNABORTED	53		/* Software caused connection abort */
#define	OSK_ECONNRESET	54		/* Connection reset by peer */
#define	OSK_ENOBUFS		55		/* No buffer space available */
#define	OSK_EISCONN		56		/* Socket is already connected */
#define	OSK_ENOTCONN	57		/* Socket is not connected */
#define	OSK_ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	OSK_ETOOMANYREFS	59		/* Too many references: can't splice */
#define	OSK_ETIMEDOUT	60		/* Operation timed out */
#define	OSK_ECONNREFUSED	61		/* Connection refused */

#define	OSK_ELOOP		62		/* Too many levels of symbolic links */
#endif /* _POSIX_SOURCE */
#define	OSK_ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#ifndef _POSIX_SOURCE
#define	OSK_EHOSTDOWN	64		/* Host is down */
#define	OSK_EHOSTUNREACH	65		/* No route to host */
#endif /* _POSIX_SOURCE */
#define	OSK_ENOTEMPTY	66		/* Directory not empty */

/* quotas & mush */
#ifndef _POSIX_SOURCE
#define	OSK_EPROCLIM	67		/* Too many processes */
#define	OSK_EUSERS		68		/* Too many users */
#define	OSK_EDQUOT		69		/* Disc quota exceeded */

/* Network File System */
#define	OSK_ESTALE		70		/* Stale NFS file handle */
#define	OSK_EREMOTE		71		/* Too many levels of remote in path */
#define	OSK_EBADRPC		72		/* RPC struct is bad */
#define	OSK_ERPCMISMATCH	73		/* RPC version wrong */
#define	OSK_EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	OSK_EPROGMISMATCH	75		/* Program version wrong */
#define	OSK_EPROCUNAVAIL	76		/* Bad procedure for program */
#endif /* _POSIX_SOURCE */

#define	OSK_ENOLCK		77		/* No locks available */
#define	OSK_ENOSYS		78		/* Function not implemented */

#ifndef _POSIX_SOURCE
#define	OSK_EFTYPE		79		/* Inappropriate file type or format */
#define	OSK_EAUTH		80		/* Authentication error */
#define	OSK_ENEEDAUTH	81		/* Need authenticator */
#define	OSK_ELAST		81		/* Must be equal largest errno */
#endif /* _POSIX_SOURCE */

#ifdef KERNEL
/* pseudo-errors returned inside kernel to modify return to process */
#define	OSK_ERESTART	-1		/* restart syscall */
#define	OSK_EJUSTRETURN	-2		/* don't modify regs, just return */
#endif

#endif/*OSK_SYS_ERRNO*/
