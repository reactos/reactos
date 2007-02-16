/*
 * locking.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Constants for the mode parameter of the locking function.
 *
 */

#ifndef	_LOCKING_H_
#define	_LOCKING_H_

/* All the headers include this file. */
#include <_mingw.h>

#define	_LK_UNLCK	0	/* Unlock */
#define	_LK_LOCK	1	/* Lock */
#define	_LK_NBLCK	2	/* Non-blocking lock */
#define	_LK_RLCK	3	/* Lock for read only */
#define	_LK_NBRLCK	4	/* Non-blocking lock for read only */

#ifndef	NO_OLDNAMES
#define	LK_UNLCK	_LK_UNLCK
#define	LK_LOCK		_LK_LOCK
#define	LK_NBLCK	_LK_NBLCK
#define	LK_RLCK		_LK_RLCK
#define	LK_NBRLCK	_LK_NBRLCK
#endif	/* Not NO_OLDNAMES */

#endif	/* Not _LOCKING_H_ */
