/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    s_perror.c

Abstract:

    This module implements the s_perror() operation used by the
    tcp/ip utilities. This is a temporary workaround for beta.
    This will be replaced by NLS support for the final product.

Author:

    John Ballard (jballard)           June 15, 1992

Revision History:

    Ronald Meijer (ronaldm) NLS Enabled     Nov 26, 1992

--*/

#include <stdio.h>
#include <crt\errno.h>
#include <sockets\sock_err.h>

#include <winsock.h>
#include "nlstxt.h"
#define MAX_MSGTABLE 255

extern HMODULE SockModuleHandle;

extern int WSA_perror(
char *yourmsg,
int lerrno);

void
s_perror(
char *yourmsg,
int lerrno)
{
    WCHAR perrW[MAX_MSGTABLE+1];
    CHAR perr[MAX_MSGTABLE+1];
    unsigned msglen;
    unsigned usMsgNum;

    switch (lerrno) {
//        case EZERO:
//            perr = "Error 0";
//            break;
        case EPERM:
        usMsgNum = IDS_EPERM ;
            break;
        case ENOENT:
        usMsgNum = IDS_ENOENT ;
            break;
        case ESRCH:
        usMsgNum = IDS_ESRCH ;
            break;
        case EINTR:
        usMsgNum = IDS_EINTR ;
            break;
        case EIO:
        usMsgNum = IDS_EIO ;
            break;
        case ENXIO:
        usMsgNum = IDS_ENXIO ;
            break;
        case E2BIG:
        usMsgNum = IDS_E2BIG ;
            break;
        case ENOEXEC:
        usMsgNum = IDS_ENOEXEC ;
            break;
        case EBADF:
        usMsgNum = IDS_EBADF ;
            break;
        case ECHILD:
        usMsgNum = IDS_ECHILD ;
            break;
        case EAGAIN:
        usMsgNum = IDS_EAGAIN ;
            break;
        case ENOMEM:
        usMsgNum = IDS_ENOMEM ;
            break;
        case EACCES:
        usMsgNum = IDS_EACCES ;
            break;
        case EFAULT:
        usMsgNum = IDS_EFAULT ;
            break;
//        case ENOTBLK:
//        perr = "Block device required";
//            break;
        case EBUSY:
        usMsgNum = IDS_EBUSY ;
            break;
        case EEXIST:
        usMsgNum = IDS_EEXIST ;
            break;
        case EXDEV:
        usMsgNum = IDS_EXDEV ;
            break;
        case ENODEV:
        usMsgNum = IDS_ENODEV ;
            break;
        case ENOTDIR:
        usMsgNum = IDS_ENOTDIR ;
            break;
        case EISDIR:
        usMsgNum = IDS_EISDIR ;
            break;
        case EINVAL:
        usMsgNum = IDS_EINVAL ;
            break;
        case ENFILE:
        usMsgNum = IDS_ENFILE ;
            break;
        case EMFILE:
        usMsgNum = IDS_EMFILE ;
            break;
        case ENOTTY:
        usMsgNum = IDS_EMFILE ;
            break;
//        case ETXTBSY:
//        perr = "Text file busy";
//            break;
        case EFBIG:
        usMsgNum = IDS_EFBIG ;
            break;
        case ENOSPC:
        usMsgNum = IDS_ENOSPC ;
            break;
        case ESPIPE:
        usMsgNum = IDS_ESPIPE ;
            break;
        case EROFS:
        usMsgNum = IDS_EROFS ;
            break;
        case EMLINK:
        usMsgNum = IDS_EMLINK ;
            break;
        case EPIPE:
        usMsgNum = IDS_EPIPE ;
            break;
        case EDOM:
        usMsgNum = IDS_EDOM ;
            break;
        case ERANGE:
        usMsgNum = IDS_ERANGE ;
            break;
//        case EUCLEAN:
//        perr = "File system not clean";
//            break;
        case EDEADLK:
        usMsgNum = IDS_EDEADLK ;
            break;
        case ENOMSG:
        usMsgNum = IDS_ENOMSG ;
            break;
        case EIDRM:
        usMsgNum = IDS_EIDRM ;
            break;
        case ECHRNG:
        usMsgNum = IDS_ECHRNG ;
            break;
        case EL2NSYNC:
        usMsgNum = IDS_EL2NSYNC ;
            break;
        case EL3HLT:
        usMsgNum = IDS_EL3HLT ;
            break;
        case EL3RST:
        usMsgNum = IDS_EL3RST ;
            break;
        case ELNRNG:
        usMsgNum = IDS_ELNRNG ;
            break;
        case EUNATCH:
        usMsgNum = IDS_EUNATCH ;
            break;
        case ENOCSI:
        usMsgNum = IDS_ENOCSI ;
            break;
        case EL2HLT:
        usMsgNum = IDS_EL2HLT ;
            break;
        case EBADE:
        usMsgNum = IDS_EBADE ;
            break;
        case EBADR:
        usMsgNum = IDS_EBADR ;
            break;
        case EXFULL:
        usMsgNum = IDS_EXFULL ;
            break;
        case ENOANO:
        usMsgNum = IDS_ENOANO ;
            break;
        case EBADRQC:
        usMsgNum = IDS_EBADRQC ;
            break;
        case EBADSLT:
        usMsgNum = IDS_EBADSLT ;
            break;
        case EBFONT:
        usMsgNum = IDS_EBFONT ;
            break;
        case ENOSTR:
        usMsgNum = IDS_ENOSTR ;
            break;
        case ENODATA:
        usMsgNum = IDS_ENODATA ;
            break;
        case ETIME:
        usMsgNum = IDS_ETIME ;
            break;
        case ENOSR:
        usMsgNum = IDS_ENOSR ;
            break;
        case ENONET:
        usMsgNum = IDS_ENONET ;
            break;
        case ENOPKG:
        usMsgNum = IDS_ENOPKG ;
            break;
        case EREMOTE:
        usMsgNum = IDS_EREMOTE ;
            break;
        case ENOLINK:
        usMsgNum = IDS_ENOLINK ;
            break;
        case EADV:
        usMsgNum = IDS_EADV ;
            break;
        case ESRMNT:
        usMsgNum = IDS_ESRMNT ;
            break;
        case ECOMM:
        usMsgNum = IDS_ECOMM ;
            break;
        case EPROTO:
        usMsgNum = IDS_EPROTO ;
            break;
        case EMULTIHOP:
        usMsgNum = IDS_EMULTIHOP ;
            break;
        case ELBIN:
        usMsgNum = IDS_ELBIN ;
            break;
        case EDOTDOT:
        usMsgNum = IDS_EDOTDOT ;
            break;
        case EBADMSG:
        usMsgNum = IDS_EBADMSG ;
            break;
        case ENOTUNIQ:
        usMsgNum = IDS_ENOTUNIQ ;
            break;
        case EREMCHG:
        usMsgNum = IDS_EREMCHG ;
            break;
        case ELIBACC:
        usMsgNum = IDS_ELIBACC;
            break;
        case ELIBBAD:
        usMsgNum = IDS_ELIBBAD ;
            break;
        case ELIBSCN:
        usMsgNum = IDS_ELIBSCN ;
            break;
        case ELIBMAX:
        usMsgNum = IDS_ELIBMAX ;
            break;
        case ELIBEXEC:
        usMsgNum = IDS_ELIBEXEC ;
            break;
        case ENOTSOCK:
        usMsgNum = IDS_ENOTSOCK ;
            break;
        case EADDRNOTAVAIL:
        usMsgNum = IDS_EADDRNOTAVAIL ;
            break;
        case EADDRINUSE:
        usMsgNum = IDS_EADDRINUSE ;
            break;
        case EAFNOSUPPORT:
        usMsgNum = IDS_EAFNOSUPPORT ;
            break;
        case ESOCKTNOSUPPORT:
        usMsgNum = IDS_ESOCKTNOSUPPORT ;
            break;
        case EPROTONOSUPPORT:
        usMsgNum = IDS_EPROTONOSUPPORT ;
            break;
        case ENOBUFS:
        usMsgNum = IDS_ENOBUFS ;
            break;
        case ETIMEDOUT:
        usMsgNum = IDS_ETIMEDOUT ;
            break;
        case EISCONN:
        usMsgNum = IDS_EISCONN ;
            break;
        case ENOTCONN:
        usMsgNum = IDS_ENOTCONN ;
            break;
        case ENOPROTOOPT:
        usMsgNum = IDS_ENOPROTOOPT ;
            break;
        case ECONNRESET:
        usMsgNum = IDS_ECONNRESET ;
            break;
        case ECONNABORT:
        usMsgNum = IDS_ECONNABORT ;
            break;
        case ENETDOWN:
        usMsgNum = IDS_ENETDOWN ;
            break;
        case ECONNREFUSED:
        usMsgNum = IDS_ECONNREFUSED ;
            break;
        case EHOSTUNREACH:
        usMsgNum = IDS_EHOSTUNREACH ;
            break;
        case EPROTOTYPE:
        usMsgNum = IDS_EPROTOTYPE ;
            break;
        case EOPNOTSUPP:
        usMsgNum = IDS_EOPNOTSUPP ;
            break;
        case ESUBNET:
        usMsgNum = IDS_ESUBNET ;
            break;
        case ENETNOLNK:
        usMsgNum = IDS_ENETNOLNK ;
            break;
        case EBADIOCTL:
        usMsgNum = IDS_EBADIOCTL ;
            break;
        case ERESOURCE:
        usMsgNum = IDS_ERESOURCE ;
            break;
        case EPROTUNR:
        usMsgNum = IDS_EPROTUNR ;
            break;
        case EPORTUNR:
        usMsgNum = IDS_EPORTUNR ;
            break;
        case ENETUNR:
        usMsgNum = IDS_ENETUNR ;
            break;
        case EPACKET:
        usMsgNum = IDS_EPACKET ;
            break;
        case ETYPEREG:
        usMsgNum = IDS_ETYPEREG ;
            break;
        case ENOTINIT:
        usMsgNum = IDS_ENOTINIT ;
            break;
        default:
            if (WSA_perror(yourmsg, lerrno)) {
                return;
            }
        usMsgNum = IDS_UNKNOWN ;
            break;
    }

    if (!(msglen = FormatMessageW(
               FORMAT_MESSAGE_FROM_HMODULE,
               (LPVOID)SockModuleHandle,
               usMsgNum,
               0L,
               perrW,
               MAX_MSGTABLE,
               NULL))) {
    return;
    }

    WideCharToMultiByte(CP_OEMCP,
                            0,
                            perrW,
                            -1,                     
                            perr,
                            sizeof(perr),
                            NULL,
                            NULL);  
    fprintf(stderr, "> %s:%s\n", yourmsg, perr);
    return;
}
