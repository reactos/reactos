/*
 * fcntl.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Access constants for _open. Note that the permissions constants are
 * in sys/stat.h (ick).
 *
 */
#ifndef _FCNTL_H_
#define _FCNTL_H_

/* All the headers include this file. */
#include <_mingw.h>

/*
 * It appears that fcntl.h should include io.h for compatibility...
 */
#include <io.h>

/* Specifiy one of these flags to define the access mode. */
#define	_O_RDONLY	0
#define _O_WRONLY	1
#define _O_RDWR		2

/* Mask for access mode bits in the _open flags. */
#define _O_ACCMODE	(_O_RDONLY|_O_WRONLY|_O_RDWR)

#define	_O_APPEND	0x0008	/* Writes will add to the end of the file. */

#define	_O_RANDOM	0x0010
#define	_O_SEQUENTIAL	0x0020
#define	_O_TEMPORARY	0x0040	/* Make the file dissappear after closing.
				 * WARNING: Even if not created by _open! */
#define	_O_NOINHERIT	0x0080

#define	_O_CREAT	0x0100	/* Create the file if it does not exist. */
#define	_O_TRUNC	0x0200	/* Truncate the file if it does exist. */
#define	_O_EXCL		0x0400	/* Open only if the file does not exist. */

#define _O_SHORT_LIVED  0x1000

/* NOTE: Text is the default even if the given _O_TEXT bit is not on. */
#define	_O_TEXT		0x4000	/* CR-LF in file becomes LF in memory. */
#define	_O_BINARY	0x8000	/* Input and output is not translated. */
#define	_O_RAW		_O_BINARY

#ifndef	_NO_OLDNAMES

/* POSIX/Non-ANSI names for increased portability */
#define	O_RDONLY	_O_RDONLY
#define O_WRONLY	_O_WRONLY
#define O_RDWR		_O_RDWR
#define O_ACCMODE	_O_ACCMODE
#define	O_APPEND	_O_APPEND
#define	O_CREAT		_O_CREAT
#define	O_TRUNC		_O_TRUNC
#define	O_EXCL		_O_EXCL
#define	O_TEXT		_O_TEXT
#define	O_BINARY	_O_BINARY
#define	O_TEMPORARY	_O_TEMPORARY
#define O_NOINHERIT	_O_NOINHERIT
#define O_SEQUENTIAL	_O_SEQUENTIAL
#define	O_RANDOM	_O_RANDOM

#endif	/* Not _NO_OLDNAMES */

#endif	/* Not _FCNTL_H_ */
