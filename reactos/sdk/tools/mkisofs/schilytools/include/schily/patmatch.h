/* @(#)patmatch.h	1.16 17/07/02 Copyright 1985,1993-2017 J. Schilling */

#ifndef	_SCHILY_PATMATCH_H
#define	_SCHILY_PATMATCH_H
/*
 *	Definitions for the pattern matching functions.
 *
 *	Copyright (c) 1985,1993-2017 J. Schilling
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
/*
 *	The pattern matching functions are based on the algorithm
 *	presented by Martin Richards in:
 *
 *	"A Compact Function for Regular Expression Pattern Matching",
 *	Software-Practice and Experience, Vol. 9, 527-534 (1979)
 *
 *	Several changes have been made to the original source which has been
 *	written in BCPL:
 *
 *	'/'	is replaced by	'!'		(to allow UNIX filenames)
 *	'(',')' are replaced by	'{', '}'
 *	'\''	is replaced by	'\\'		(UNIX compatible quote)
 *
 *	Character classes have been added to allow "[<character list>]"
 *	to be used.
 *	Start of line '^' and end of line '$' have been added.
 *
 *	Any number in the following comment is zero or more occurrencies
 */
#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define	ALT	'!'	/* Alternation in match i.e. this!that!the_other */
#define	REP	'#'	/* Any number of occurrences of the following expr */
#define	NIL	'%'	/* Empty string (exactly nothing) */
#define	STAR	'*'	/* Any number of any character (equivalent of #?) */
#define	ANY	'?'	/* Any one character */
#define	QUOTE	'\\'	/* Quotes the next character */
#define	LBRACK	'{'	/* Begin of precedence grouping */
#define	RBRACK	'}'	/* End of precedence grouping */
#define	LCLASS	'['	/* Begin of character set */
#define	RCLASS	']'	/* End of character set */
#define	NOT	'^'	/* If first in set: invert set content */
#define	RANGE	'-'	/* Range notation in sets */
#define	START	'^'	/* Begin of a line */
#define	END	'$'	/* End of a line */

/*
 * A list of case statements that may be used for a issimple() or ispattern()
 * funtion that checks whether a string conrtains characters that need the
 * pattern matcher.
 *
 * Note that this list does not contain NOT or RANGE because you need
 * LCLASS and RCLASS in addition.
 */
#define	casePAT	case ALT: case REP: case NIL: case STAR: case ANY:	\
		case QUOTE: case LBRACK: case RBRACK:			\
		case LCLASS: case RCLASS: case START: case END:


#define	MAXPAT	128	/* Max length of pattern for opatmatch()/opatlmatch() */

extern	int	    patcompile	__PR((const unsigned char *__pat, int __patlen,
						int *__aux));

extern	unsigned char *opatmatch	__PR((const unsigned char *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt));
extern	unsigned char *opatlmatch __PR((const unsigned char *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt));
extern	unsigned char *patmatch	__PR((const unsigned char *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt, int __state[]));
extern	unsigned char *patlmatch __PR((const unsigned char *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt, int __state[]));

#ifdef	__cplusplus
}
#endif

#ifdef	_SCHILY_WCHAR_H

#ifdef	__cplusplus
extern "C" {
#endif

extern	int	patwcompile	__PR((const wchar_t *__pat, int __patlen,
						int *__aux));
extern	wchar_t	*patwmatch	__PR((const wchar_t *__pat, const int *__aux,
						const  wchar_t *__str,
						int __soff, int __slen,
						int __alt, int __state[]));
extern	wchar_t	*patwlmatch	__PR((const wchar_t *__pat, const int *__aux,
						const  wchar_t *__str,
						int __soff, int __slen,
						int __alt, int __state[]));

extern	unsigned char *patmbmatch __PR((const wchar_t *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt, int __state[]));
extern	unsigned char *patmblmatch __PR((const wchar_t *__pat,
						const int *__aux,
						const  unsigned char *__str,
						int __soff, int __slen,
						int __alt, int __state[]));


#ifdef	__cplusplus
}
#endif
#endif	/* _SCHILY_WCHAR_H */

#endif	/* _SCHILY_PATMATCH_H */
