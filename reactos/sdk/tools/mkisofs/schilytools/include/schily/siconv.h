/* @(#)siconv.h	1.5 10/12/20 Copyright 2007-2010 J. Schilling */
/*
 *	Definitions fur users of libsiconv
 *
 *	Copyright (c) 2007-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef _SCHILY_SICONV_H
#define	_SCHILY_SICONV_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif
#ifndef _SCHILY_ICONV_H
#include <schily/iconv.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif


typedef struct siconv_table	siconvt_t;
struct siconv_table {
	char		*sic_name;		/* SICONV charset name	*/
	UInt16_t	*sic_cs2uni;		/* Charset -> Unicode	*/
	UInt8_t		**sic_uni2cs;		/* Unicode -> Charset	*/
	iconv_t		sic_cd2uni;		/* iconv Charset -> Unicode */
	iconv_t		sic_uni2cd;		/* iconv Unicode -> Charset */
	siconvt_t	*sic_alt;		/* alternate iconv tab	*/
	siconvt_t	*sic_next;		/* Next table		*/
	int		sic_refcnt;		/* Reference count	*/
};

#define	use_iconv(t)	((t)->sic_cd2uni != NULL)

#define	sic_c2uni(t, c)	((t)->sic_cs2uni[c])
#define	sic_uni2c(t, c)	((t)->sic_uni2cs[((c) >> 8) & 0xFF][(c) & 0xFF])

extern int		sic_list		__PR((FILE *));
extern siconvt_t	*sic_open		__PR((char *));
extern const char	*sic_base		__PR((void));
extern int 		sic_close		__PR((siconvt_t *));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SICONV_H */
