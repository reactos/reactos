//
// errno.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// System error numbers for use with errno and errno_t.
//
#pragma once
#ifndef _INC_ERRNO // include guard for 3rd party interop
#define _INC_ERRNO

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#if _CRT_FUNCTIONS_REQUIRED
    _ACRTIMP int* __cdecl _errno(void);
    #define errno (*_errno())

    _ACRTIMP errno_t __cdecl _set_errno(_In_ int _Value);
    _ACRTIMP errno_t __cdecl _get_errno(_Out_ int* _Value);

    _ACRTIMP unsigned long* __cdecl __doserrno(void);
    #define _doserrno (*__doserrno())

    _ACRTIMP errno_t __cdecl _set_doserrno(_In_ unsigned long _Value);
    _ACRTIMP errno_t __cdecl _get_doserrno(_Out_ unsigned long * _Value);
#endif // _CRT_FUNCTIONS_REQUIRED



// Error codes
#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define EDEADLK         36
#define ENAMETOOLONG    38
#define ENOLCK          39
#define ENOSYS          40
#define ENOTEMPTY       41

// Error codes used in the Secure CRT functions
#ifndef RC_INVOKED
    #define _SECURECRT_ERRCODE_VALUES_DEFINED
    #define EINVAL          22
    #define ERANGE          34
    #define EILSEQ          42
    #define STRUNCATE       80
#endif

// Support EDEADLOCK for compatibility with older Microsoft C versions
#define EDEADLOCK       EDEADLK

// POSIX Supplement
#ifndef _CRT_NO_POSIX_ERROR_CODES
    #define EADDRINUSE      100
    #define EADDRNOTAVAIL   101
    #define EAFNOSUPPORT    102
    #define EALREADY        103
    #define EBADMSG         104
    #define ECANCELED       105
    #define ECONNABORTED    106
    #define ECONNREFUSED    107
    #define ECONNRESET      108
    #define EDESTADDRREQ    109
    #define EHOSTUNREACH    110
    #define EIDRM           111
    #define EINPROGRESS     112
    #define EISCONN         113
    #define ELOOP           114
    #define EMSGSIZE        115
    #define ENETDOWN        116
    #define ENETRESET       117
    #define ENETUNREACH     118
    #define ENOBUFS         119
    #define ENODATA         120
    #define ENOLINK         121
    #define ENOMSG          122
    #define ENOPROTOOPT     123
    #define ENOSR           124
    #define ENOSTR          125
    #define ENOTCONN        126
    #define ENOTRECOVERABLE 127
    #define ENOTSOCK        128
    #define ENOTSUP         129
    #define EOPNOTSUPP      130
    #define EOTHER          131
    #define EOVERFLOW       132
    #define EOWNERDEAD      133
    #define EPROTO          134
    #define EPROTONOSUPPORT 135
    #define EPROTOTYPE      136
    #define ETIME           137
    #define ETIMEDOUT       138
    #define ETXTBSY         139
    #define EWOULDBLOCK     140
#endif // _CRT_NO_POSIX_ERROR_CODES



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_ERRNO
