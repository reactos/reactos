/* @(#)intcvt.h	1.8 13/10/26 Copyright 1986-2013 J. Schilling */
/*
 *	Definitions for conversion to/from integer data types of various size.
 *
 *	Copyright (c) 1986-2013 J. Schilling
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

#ifndef	_SCHILY_INTCVT_H
#define	_SCHILY_INTCVT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#define	i_to_2_byte(a, i)	(((Uchar *)(a))[0] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[1] = (i) & 0xFF)

#define	i_to_3_byte(a, i)	(((Uchar *)(a))[0] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[2] = (i) & 0xFF)

#define	i_to_4_byte(a, i)	(((Uchar *)(a))[0] = ((i) >> 24)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[2] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[3] = (i) & 0xFF)



#define	a_to_byte(a)		(((Int8_t *)a)[0])

#define	a_to_u_byte(a)		((UInt8_t) \
				(((Uchar *)a)[0]		& 0xFF))

#define	a_to_u_2_byte(a)	((UInt16_t) \
				((((Uchar *)a)[1]		& 0xFF) | \
				    (((Uchar *)a)[0] << 8	& 0xFF00)))

#define	a_to_2_byte(a)		(int)(Int16_t)a_to_u_2_byte(a)

#define	a_to_u_3_byte(a)	((Ulong) \
				((((Uchar *)a)[2]		& 0xFF) | \
				    (((Uchar *)a)[1] << 8	& 0xFF00) | \
				    (((Uchar *)a)[0] << 16	& 0xFF0000)))

#define	a_to_3_byte(a)		a_to_u_3_byte(a) /* XXX signed version? */

#ifdef	__STDC__
#	define	__TOP_4BYTE	0xFF000000UL
#else
#	define	__TOP_4BYTE	0xFF000000
#endif

#define	a_to_u_4_byte(a)	((Ulong) \
				((((Uchar*)a)[3]		& 0xFF) | \
				    (((Uchar*)a)[2] << 8	& 0xFF00) | \
				    (((Uchar*)a)[1] << 16	& 0xFF0000) | \
				    (((Uchar*)a)[0] << 24	& __TOP_4BYTE)))

#define	a_to_4_byte(a)		(long)(Int32_t)a_to_u_4_byte(a)

/*
 * Little Endian versions of above macros
 */
#define	li_to_2_byte(a, i)	(((Uchar *)(a))[1] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[0] = (i) & 0xFF)

#define	li_to_3_byte(a, i)	(((Uchar *)(a))[2] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[0] = (i) & 0xFF)

#define	li_to_4_byte(a, i)	(((Uchar *)(a))[3] = ((i) >> 24)& 0xFF,\
				    ((Uchar *)(a))[2] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[0] = (i) & 0xFF)


#define	la_to_u_2_byte(a)	((UInt16_t) \
				((((Uchar*)a)[0]		& 0xFF) | \
				    (((Uchar*)a)[1] << 8	& 0xFF00)))

#define	la_to_2_byte(a)		(int)(Int16_t)la_to_u_2_byte(a)

#define	la_to_u_3_byte(a)	((Ulong) \
				((((Uchar*)a)[0]		& 0xFF) | \
				    (((Uchar*)a)[1] << 8	& 0xFF00) | \
				    (((Uchar*)a)[2] << 16	& 0xFF0000)))

#define	la_to_3_byte(a)		la_to_u_3_byte(a) /* XXX signed version? */

#define	la_to_u_4_byte(a)	((Ulong) \
				((((Uchar*)a)[0]		& 0xFF) | \
				    (((Uchar*)a)[1] << 8	& 0xFF00) | \
				    (((Uchar*)a)[2] << 16	& 0xFF0000) | \
				    (((Uchar*)a)[3] << 24	& __TOP_4BYTE)))

#define	la_to_4_byte(a)		(long)(Int32_t)la_to_u_4_byte(a)

#endif	/* _SCHILY_INTCVT_H */
