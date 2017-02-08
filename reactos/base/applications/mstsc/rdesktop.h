/*
   rdesktop: A Remote Desktop Protocol client.
   Master include file
   Copyright (C) Matthew Chapman 1999-2008

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h> /* winsock2.h first */
#include <mmsystem.h>
#include <time.h>
#define DIR int
#else /* WIN32 */
#include <dirent.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/types.h>
#include <unistd.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* WIN32 */
//#include <limits.h>		/* PATH_MAX */
#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif

#define VERSION "1.8.3"

/* standard exit codes */
#ifndef EX_OK
#define EX_OK           0
#endif
#ifndef EX_USAGE
#define EX_USAGE        64
#endif
#ifndef EX_DATAERR
#define EX_DATAERR      65
#endif
#ifndef EX_NOINPUT
#define EX_NOINPUT      66
#endif
#ifndef EX_NOUSER
#define EX_NOUSER       67
#endif
#ifndef EX_NOHOST
#define EX_NOHOST       68
#endif
#ifndef EX_UNAVAILABLE
#define EX_UNAVAILABLE  69
#endif
#ifndef EX_SOFTWARE
#define EX_SOFTWARE     70
#endif
#ifndef EX_OSERR
#define EX_OSERR        71
#endif
#ifndef EX_OSFILE
#define EX_OSFILE       72
#endif
#ifndef EX_CANTCREAT
#define EX_CANTCREAT    73
#endif
#ifndef EX_IOERR
#define EX_IOERR        74
#endif
#ifndef EX_TEMPFAIL
#define EX_TEMPFAIL     75
#endif
#ifndef EX_PROTOCOL
#define EX_PROTOCOL     76
#endif
#ifndef EX_NOPERM
#define EX_NOPERM       77
#endif
#ifndef EX_CONFIG
#define EX_CONFIG       78
#endif

/* rdesktop specific exit codes, lined up with disconnect PDU reasons */
#define EXRD_API_DISCONNECT 1
#define EXRD_API_LOGOFF 2
#define EXRD_IDLE_TIMEOUT 3
#define EXRD_LOGON_TIMEOUT 4
#define EXRD_REPLACED 5
#define EXRD_OUT_OF_MEM 6
#define EXRD_DENIED 7
#define EXRD_DENIED_FIPS 8
#define EXRD_INSUFFICIENT_PRIVILEGES 9
#define EXRD_FRESH_CREDENTIALS_REQUIRED 10
#define EXRD_RPC_DISCONNECT_BY_USER 11
#define EXRD_DISCONNECT_BY_USER 12
#define EXRD_LIC_INTERNAL 16
#define EXRD_LIC_NOSERVER 17
#define EXRD_LIC_NOLICENSE 18
#define EXRD_LIC_MSG 19
#define EXRD_LIC_HWID 20
#define EXRD_LIC_CLIENT 21
#define EXRD_LIC_NET 22
#define EXRD_LIC_PROTO 23
#define EXRD_LIC_ENC 24
#define EXRD_LIC_UPGRADE 25
#define EXRD_LIC_NOREMOTE 26

/* other exit codes */
#define EXRD_WINDOW_CLOSED 62
#define EXRD_UNKNOWN 63

#ifdef WITH_DEBUG
#define DEBUG(args)	printf args;
#else
#define DEBUG(args)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(args) printf args;
#else
#define DEBUG_KBD(args)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(args) printf args;
#else
#define DEBUG_RDP5(args)
#endif

#ifdef WITH_DEBUG_CLIPBOARD
#define DEBUG_CLIPBOARD(args) printf args;
#else
#define DEBUG_CLIPBOARD(args)
#endif

#ifdef WITH_DEBUG_SOUND
#define DEBUG_SOUND(args) printf args;
#else
#define DEBUG_SOUND(args)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(args) printf args;
#else
#define DEBUG_CHANNEL(args)
#endif

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(args) printf args;
#else
#define DEBUG_SCARD(args)
#endif

#define STRNCPY(dst,src,n)	{ strncpy(dst,src,n-1); dst[n-1] = 0; }

#ifndef MIN
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))
#endif

/* timeval macros */
#ifndef timerisset
#define timerisset(tvp)\
         ((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)\
        ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
        (tvp)->tv_sec == (uvp)->tv_sec &&\
        (tvp)->tv_usec cmp (uvp)->tv_usec)
#endif
#ifndef timerclear
#define timerclear(tvp)\
        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

/* If configure does not define the endianess, try
   to find it out */
#if !defined(L_ENDIAN) && !defined(B_ENDIAN)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define L_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define B_ENDIAN
#else
#error Unknown endianness. Edit rdesktop.h.
#endif
#endif /* B_ENDIAN, L_ENDIAN from configure */

/* No need for alignment on x86 and amd64 */
#if !defined(NEED_ALIGN)
#if !(defined(__x86__) || defined(__x86_64__) || \
      defined(__AMD64__) || defined(_M_IX86) || \
      defined(__i386__))
#define NEED_ALIGN
#endif
#endif

#include "parse.h"
#include "constants.h"
#include "types.h"

#ifndef MAKE_PROTO
#include "proto.h"
#endif
