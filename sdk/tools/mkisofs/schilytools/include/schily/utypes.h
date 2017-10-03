/* @(#)utypes.h	1.36 13/09/14 Copyright 1997-2013 J. Schilling */
/*
 *	Definitions for some user defined types
 *
 *	Copyright (c) 1997-2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_UTYPES_H
#define	_SCHILY_UTYPES_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * uypes.h is based on inttypes.h
 */
#ifndef	_SCHILY_INTTYPES_H
#include <schily/inttypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Several unsigned cardinal types
 */
typedef	unsigned long	Ulong;
typedef	unsigned int	Uint;
typedef	unsigned short	Ushort;
typedef	unsigned char	Uchar;

/*
 * The IBM AIX C-compiler seems to be the only compiler on the world
 * which does not allow to use unsigned char bit fields as a hint
 * for packed bit fields. Define a pesical type to avoid warnings.
 * The packed attribute is honored wit unsigned int in this case too.
 */
#if	defined(_AIX) && !defined(__GNUC__)

typedef unsigned int	Ucbit;

#else

typedef unsigned char	Ucbit;

#endif

#ifndef	CHAR_MIN
#define	CHAR_MIN	TYPE_MINVAL(char)
#endif
#ifndef	CHAR_MAX
#define	CHAR_MAX	TYPE_MAXVAL(char)
#endif
#ifndef	UCHAR_MAX
#define	UCHAR_MAX	TYPE_MAXVAL(unsigned char)
#endif

#ifndef	SHRT_MIN
#define	SHRT_MIN	TYPE_MINVAL(short)
#endif
#ifndef	SHRT_MAX
#define	SHRT_MAX	TYPE_MAXVAL(short)
#endif
#ifndef	USHRT_MAX
#define	USHRT_MAX	TYPE_MAXVAL(unsigned short)
#endif

#ifndef	INT_MIN
#define	INT_MIN		TYPE_MINVAL(int)
#endif
#ifndef	INT_MAX
#define	INT_MAX		TYPE_MAXVAL(int)
#endif
#ifndef	UINT_MAX
#define	UINT_MAX	TYPE_MAXVAL(unsigned int)
#endif

#ifndef	LONG_MIN
#define	LONG_MIN	TYPE_MINVAL(long)
#endif
#ifndef	LONG_MAX
#define	LONG_MAX	TYPE_MAXVAL(long)
#endif
#ifndef	ULONG_MAX
#define	ULONG_MAX	TYPE_MAXVAL(unsigned long)
#endif

#define	OFF_T_MIN	TYPE_MINVAL(off_t)
#define	OFF_T_MAX	TYPE_MAXVAL(off_t)

#define	UID_T_MIN	TYPE_MINVAL(uid_t)
#define	UID_T_MAX	TYPE_MAXVAL(uid_t)

#define	GID_T_MIN	TYPE_MINVAL(gid_t)
#define	GID_T_MAX	TYPE_MAXVAL(gid_t)

#define	PID_T_MIN	TYPE_MINVAL(pid_t)
#define	PID_T_MAX	TYPE_MAXVAL(pid_t)

#define	MODE_T_MIN	TYPE_MINVAL(mode_t)
#define	MODE_T_MAX	TYPE_MAXVAL(mode_t)

#define	TIME_T_MIN	TYPE_MINVAL(time_t)
#define	TIME_T_MAX	TYPE_MAXVAL(time_t)

#define	CADDR_T_MIN	TYPE_MINVAL(caddr_t)
#define	CADDR_T_MAX	TYPE_MAXVAL(caddr_t)

#define	DADDR_T_MIN	TYPE_MINVAL(daddr_t)
#define	DADDR_T_MAX	TYPE_MAXVAL(daddr_t)

#define	DEV_T_MIN	TYPE_MINVAL(dev_t)
#define	DEV_T_MAX	TYPE_MAXVAL(dev_t)

#define	MAJOR_T_MIN	TYPE_MINVAL(major_t)
#define	MAJOR_T_MAX	TYPE_MAXVAL(major_t)

#define	MINOR_T_MIN	TYPE_MINVAL(minor_t)
#define	MINOR_T_MAX	TYPE_MAXVAL(minor_t)

#define	INO_T_MIN	TYPE_MINVAL(ino_t)
#define	INO_T_MAX	TYPE_MAXVAL(ino_t)

#define	NLINK_T_MIN	TYPE_MINVAL(nlink_t)
#define	NLINK_T_MAX	TYPE_MAXVAL(nlink_t)

#define	BLKSIZE_T_MIN	TYPE_MINVAL(blksize_t)
#define	BLKSIZE_T_MAX	TYPE_MAXVAL(blksize_t)

#define	BLKCNT_T_MIN	TYPE_MINVAL(blkcnt_t)
#define	BLKCNT_T_MAX	TYPE_MAXVAL(blkcnt_t)

#define	CLOCK_T_MIN	TYPE_MINVAL(clock_t)
#define	CLOCK_T_MAX	TYPE_MAXVAL(clock_t)

#define	SOCKLEN_T_MIN	TYPE_MINVAL(socklen_t)
#define	SOCKLEN_T_MAX	TYPE_MAXVAL(socklen_t)

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_UTYPES_H */
