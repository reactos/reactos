/*
 * share.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Constants for file sharing functions.
 *
 */

#ifndef	_SHARE_H_
#define	_SHARE_H_

/* All the headers include this file. */
#include <_mingw.h>

#define _SH_COMPAT	0x00	/* Compatibility */
#define	_SH_DENYRW	0x10	/* Deny read/write */
#define	_SH_DENYWR	0x20	/* Deny write */
#define	_SH_DENYRD	0x30	/* Deny read */
#define	_SH_DENYNO	0x40	/* Deny nothing */

#ifndef _NO_OLDNAMES

/* Non ANSI names */
#define SH_DENYRW _SH_DENYRW
#define SH_DENYWR _SH_DENYWR
#define SH_DENYRD _SH_DENYRD
#define SH_DENYNO _SH_DENYNO

#endif	/* Not _NO_OLDNAMES */

#endif	/* Not _SHARE_H_ */
