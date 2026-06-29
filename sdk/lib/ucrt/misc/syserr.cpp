/***
*syserr.c - system error list
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the System Error List, containing the full messages for
*       all errno values set by the library routines.
*       Defines sys_errlist, sys_nerr.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <stdlib.h>

#undef _sys_nerr
#undef _sys_errlist

extern "C" char const* const _sys_errlist[] =
{
    /*  0                 */  "No error",
    /*  1 EPERM           */  "Operation not permitted",
    /*  2 ENOENT          */  "No such file or directory",
    /*  3 ESRCH           */  "No such process",
    /*  4 EINTR           */  "Interrupted function call",
    /*  5 EIO             */  "Input/output error",
    /*  6 ENXIO           */  "No such device or address",
    /*  7 E2BIG           */  "Arg list too long",
    /*  8 ENOEXEC         */  "Exec format error",
    /*  9 EBADF           */  "Bad file descriptor",
    /* 10 ECHILD          */  "No child processes",
    /* 11 EAGAIN          */  "Resource temporarily unavailable",
    /* 12 ENOMEM          */  "Not enough space",
    /* 13 EACCES          */  "Permission denied",
    /* 14 EFAULT          */  "Bad address",
    /* 15 ENOTBLK         */  "Unknown error",                     /* not POSIX */
    /* 16 EBUSY           */  "Resource device",
    /* 17 EEXIST          */  "File exists",
    /* 18 EXDEV           */  "Improper link",
    /* 19 ENODEV          */  "No such device",
    /* 20 ENOTDIR         */  "Not a directory",
    /* 21 EISDIR          */  "Is a directory",
    /* 22 EINVAL          */  "Invalid argument",
    /* 23 ENFILE          */  "Too many open files in system",
    /* 24 EMFILE          */  "Too many open files",
    /* 25 ENOTTY          */  "Inappropriate I/O control operation",
    /* 26 ETXTBSY         */  "Unknown error",                     /* not POSIX */
    /* 27 EFBIG           */  "File too large",
    /* 28 ENOSPC          */  "No space left on device",
    /* 29 ESPIPE          */  "Invalid seek",
    /* 30 EROFS           */  "Read-only file system",
    /* 31 EMLINK          */  "Too many links",
    /* 32 EPIPE           */  "Broken pipe",
    /* 33 EDOM            */  "Domain error",
    /* 34 ERANGE          */  "Result too large",
    /* 35 EUCLEAN         */  "Unknown error",                     /* not POSIX */
    /* 36 EDEADLK         */  "Resource deadlock avoided",
    /* 37 UNKNOWN         */  "Unknown error",
    /* 38 ENAMETOOLONG    */  "Filename too long",
    /* 39 ENOLCK          */  "No locks available",
    /* 40 ENOSYS          */  "Function not implemented",
    /* 41 ENOTEMPTY       */  "Directory not empty",
    /* 42 EILSEQ          */  "Illegal byte sequence",
    /* 43                 */  "Unknown error"
};

extern "C" char const* const _sys_posix_errlist[] =
{
    /* 100 EADDRINUSE      */  "address in use",
    /* 101 EADDRNOTAVAIL   */  "address not available",
    /* 102 EAFNOSUPPORT    */  "address family not supported",
    /* 103 EALREADY        */  "connection already in progress",
    /* 104 EBADMSG         */  "bad message",
    /* 105 ECANCELED       */  "operation canceled",
    /* 106 ECONNABORTED    */  "connection aborted",
    /* 107 ECONNREFUSED    */  "connection refused",
    /* 108 ECONNRESET      */  "connection reset",
    /* 109 EDESTADDRREQ    */  "destination address required",
    /* 110 EHOSTUNREACH    */  "host unreachable",
    /* 111 EIDRM           */  "identifier removed",
    /* 112 EINPROGRESS     */  "operation in progress",
    /* 113 EISCONN         */  "already connected",
    /* 114 ELOOP           */  "too many symbolic link levels",
    /* 115 EMSGSIZE        */  "message size",
    /* 116 ENETDOWN        */  "network down",
    /* 117 ENETRESET       */  "network reset",
    /* 118 ENETUNREACH     */  "network unreachable",
    /* 119 ENOBUFS         */  "no buffer space",
    /* 120 ENODATA         */  "no message available",
    /* 121 ENOLINK         */  "no link",
    /* 122 ENOMSG          */  "no message",
    /* 123 ENOPROTOOPT     */  "no protocol option",
    /* 124 ENOSR           */  "no stream resources",
    /* 125 ENOSTR          */  "not a stream",
    /* 126 ENOTCONN        */  "not connected",
    /* 127 ENOTRECOVERABLE */  "state not recoverable",
    /* 128 ENOTSOCK        */  "not a socket",
    /* 129 ENOTSUP         */  "not supported",
    /* 130 EOPNOTSUPP      */  "operation not supported",
    /* 131 EOTHER          */  "Unknown error",
    /* 132 EOVERFLOW       */  "value too large",
    /* 133 EOWNERDEAD      */  "owner dead",
    /* 134 EPROTO          */  "protocol error",
    /* 135 EPROTONOSUPPORT */  "protocol not supported",
    /* 136 EPROTOTYPE      */  "wrong protocol type",
    /* 137 ETIME           */  "stream timeout",
    /* 138 ETIMEDOUT       */  "timed out",
    /* 139 ETXTBSY         */  "text file busy",
    /* 140 EWOULDBLOCK     */  "operation would block",
    /* 141                 */  "Unknown error"
};


extern "C" size_t const _sys_first_posix_error = 100;
extern "C" size_t const _sys_last_posix_error = _sys_first_posix_error + _countof(_sys_posix_errlist) - 1;
extern "C" int const _sys_nerr = _countof(_sys_errlist) - 1;

// The above array contains all the errors including unknown error


/* ***NOTE: Global variable max_system_error_message_count (in file corecrt_internal.h)
   indicates the length of the longest system error message in the above table.
   When you add or modify a message, you must update the value
   max_system_error_message_count, if appropriate. */

/***
*int * __sys_nerr();                                 - return pointer to thread's errno
*const char * const * __cdecl __sys_errlist(void);   - return pointer to thread's _doserrno
*
*Purpose:
*       Returns former global variables
*
*Entry:
*       None.
*
*Exit:
*       See above.
*
*Exceptions:
*
*******************************************************************************/

extern "C" int* __cdecl __sys_nerr()
{
    return const_cast<int*>(&_sys_nerr);
}

extern "C" char** __cdecl __sys_errlist()
{
    return const_cast<char**>(_sys_errlist);
}
