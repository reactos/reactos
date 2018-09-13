/***
*fcntl.h - file control options used by open()
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines constants for the file control options used
*	by the _open() function.
*	[System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_FCNTL
#define _INC_FCNTL

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#define _O_RDONLY	0x0000	/* open for reading only */
#define _O_WRONLY	0x0001	/* open for writing only */
#define _O_RDWR 	0x0002	/* open for reading and writing */
#define _O_APPEND	0x0008	/* writes done at eof */

#define _O_CREAT	0x0100	/* create and open file */
#define _O_TRUNC	0x0200	/* open and truncate */
#define _O_EXCL 	0x0400	/* open only if file doesn't already exist */

/* O_TEXT files have <cr><lf> sequences translated to <lf> on read()'s,
** and <lf> sequences translated to <cr><lf> on write()'s
*/

#define _O_TEXT 	0x4000	/* file mode is text (translated) */
#define _O_BINARY	0x8000	/* file mode is binary (untranslated) */

/* macro to translate the C 2.0 name used to force binary mode for files */

#define _O_RAW	_O_BINARY

/* Open handle inherit bit */

#define _O_NOINHERIT	0x0080	/* child process doesn't inherit file */

/* Temporary file bit - file is deleted when last handle is closed */

#define _O_TEMPORARY	0x0040	/* temporary file bit */

/* temporary access hint */

#define _O_SHORT_LIVED	0x1000	/* temporary storage file, try not to flush */

/* sequential/random access hints */

#define _O_SEQUENTIAL	0x0020	/* file access is primarily sequential */
#define _O_RANDOM	0x0010	/* file access is primarily random */

#if !__STDC__ || defined(_POSIX_)
/* Non-ANSI names for compatibility */
#define O_RDONLY	_O_RDONLY
#define O_WRONLY	_O_WRONLY
#define O_RDWR		_O_RDWR
#define O_APPEND	_O_APPEND
#define O_CREAT 	_O_CREAT
#define O_TRUNC 	_O_TRUNC
#define O_EXCL		_O_EXCL
#define O_TEXT		_O_TEXT
#define O_BINARY	_O_BINARY
#define O_RAW		_O_BINARY
#define O_TEMPORARY	_O_TEMPORARY
#define O_NOINHERIT	_O_NOINHERIT
#define O_SEQUENTIAL	_O_SEQUENTIAL
#define O_RANDOM	_O_RANDOM
#endif /* __STDC__ */

#endif /* _INC_FCNTL */
