/* $Id: errno.h,v 1.5 2002/10/29 04:45:08 rex Exp $
 */
/*
 * errno.h
 *
 * system error numbers. Conforming to the Single UNIX(r) Specification
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
#ifndef __ERRNO_H_INCLUDED__
#define __ERRNO_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */

/* CONSTANTS */
/* errors from 0 to 42 are the same as in Microsoft POSIX */
#define EZERO           (  0) /* No error. */
#define EPERM           (  1) /* Operation not permitted. */
#define ENOENT          (  2) /* No such file or directory. */
#define ESRCH           (  3) /* No such process. */
#define EINTR           (  4) /* Interrupted function. */
#define EIO             (  5) /* I/O error. */
#define ENXIO           (  6) /* No such device or address. */
#define E2BIG           (  7) /* Argument list too long. */
#define ENOEXEC         (  8) /* Executable file format error. */
#define EBADF           (  9) /* Bad file descriptor. */
#define ECHILD          ( 10) /* No child processes. */
#define EAGAIN          ( 11) /* Resource unavailable, try again */
#define ENOMEM          ( 12) /* Not enough space. */
#define EACCES          ( 13) /* Permission denied. */
#define EFAULT          ( 14) /* Bad address. */
#define ENOTBLK         ( 15) /* Reserved.	*/
#define EBUSY           ( 16) /* Device or resource busy. */
#define EEXIST          ( 17) /* File exists. */
#define EXDEV           ( 18) /* Cross-device link. */
#define ENODEV          ( 19) /* No such device. */
#define ENOTDIR         ( 20) /* Not a directory. */
#define EISDIR          ( 21) /* Is a directory. */
#define EINVAL          ( 22) /* Invalid argument. */
#define ENFILE          ( 23) /* Too many files open in system. */
#define EMFILE          ( 24) /* Too many open files. */
#define ENOTTY          ( 25) /* Inappropriate I/O control operation. */
#define ETXTBSY         ( 26) /* Text file busy. */
#define EFBIG           ( 27) /* File too large. */
#define ENOSPC          ( 28) /* No space left on device. */
#define ESPIPE          ( 29) /* Invalid seek. */
#define EROFS           ( 30) /* Read-only file system. */
#define EMLINK          ( 31) /* Too many links. */
#define EPIPE           ( 32) /* Broken pipe. */
#define EDOM            ( 33) /* Mathematics argument out of domain of function. */
#define ERANGE          ( 34) /* Result too large. */
#define EUCLEAN	        ( 35) /* Reserved. */
#define EDEADLK         ( 36) /* Resource deadlock would occur. */
#define UNKNOWN         ( 37) /* Reserved. */
#define ENAMETOOLONG    ( 38) /* Filename too long. */
#define ENOLCK          ( 39) /* No locks available. */
#define ENOSYS          ( 40) /* Function not supported. */
#define ENOTEMPTY       ( 41) /* Directory not empty. */
#define EILSEQ          ( 42) /* Illegal byte sequence. */
/* from this point, constants are in no particular order */
#define ENODATA         ( 44) /* No message is available on the STREAM head read queue. */
#define ENOSR           ( 45) /* No STREAM resources. */
#define ENOSTR          ( 46) /* Not a STREAM. */
#define ECANCELED       ( 47) /* Operation canceled. */
#define ENOBUFS         ( 48) /* No buffer space available. */
#define EOVERFLOW       ( 49) /* Value too large to be stored in data type. */
#define ENOTSUP         ( 50) /* Not supported. */
#define EADDRINUSE      ( 51) /* Address in use. */
#define EADDRNOTAVAIL   ( 52) /* Address not available. */
#define EAFNOSUPPORT    ( 53) /* Address family not supported. */
#define ECONNABORTED    ( 54) /* Connection aborted. */
#define ECONNREFUSED    ( 55) /* Connection refused. */
#define ECONNRESET      ( 56) /* Connection reset. */
#define EALREADY        ( 57) /* Connection already in progress. */
#define EDESTADDRREQ    ( 58) /* Destination address required. */
#define EHOSTUNREACH    ( 59) /* Host is unreachable. */
#define EISCONN         ( 60) /* Socket is connected. */
#define ENETDOWN        ( 61) /* Network is down. */
#define ENETUNREACH     ( 62) /* Network unreachable. */
#define ENOPROTOOPT     ( 63) /* Protocol not available. */
#define ENOTCONN        ( 64) /* The socket is not connected. */
#define ENOTSOCK        ( 65) /* Not a socket. */
#define EPROTO          ( 66) /* Protocol error. */
#define EPROTONOSUPPORT ( 67) /* Protocol not supported. */
#define EPROTOTYPE      ( 68) /* Socket type not supported. */
#define EOPNOTSUPP      ( 69) /* Operation not supported on socket. */
#define ETIMEDOUT       ( 70) /* Connection timed out. */
#define EINPROGRESS     ( 71) /* Operation in progress. */
#define EBADMSG         ( 72) /* Bad message. */
#define EMSGSIZE        ( 73) /* Message too large. */
#define ENOMSG          ( 74) /* No message of the desired type. */
#define EDQUOT          ( 75) /* Reserved. */
#define EIDRM           ( 76) /* Identifier removed. */
#define ELOOP           ( 77) /* Too many levels of symbolic links. */
#define EMULTIHOP       ( 78) /* Reserved. */
#define ENOLINK         ( 79) /* Reserved. */
#define ESTALE          ( 80) /* Reserved. */
#define ETIME           ( 81) /* Streamioctl() timeout. */
#define EWOULDBLOCK     ( 82) /* Operation would block */

#define EDEADLOCK	EDEADLK /* Resource deadlock avoided		*/

/* PROTOTYPES */
int * __PdxGetThreadErrNum(void); /* returns a pointer to the current thread's errno */

/* MACROS */
#define errno (*__PdxGetThreadErrNum())

#endif /* __ERRNO_H_INCLUDED__ */

/* EOF */

