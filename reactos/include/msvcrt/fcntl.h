/*
 * fcntl.h
 *
 * Access constants for _open. Note that the permissions constants are
 * in sys/stat.h (ick).
 *
 * This code is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: chorns $
 * $Date: 2002/09/08 10:22:30 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* added _O_RANDOM_O_SEQUENTIAL	_O_SHORT_LIVED*/	
/* changed fmode_dll */

#ifndef _FCNTL_H_
#define _FCNTL_H_

/*
 * It appears that fcntl.h should include io.h for compatibility...
 */
#include <msvcrt/io.h>

/*
 * This variable determines the default file mode.
 * TODO: Which flags work?
 */
#if 0
#if __MSVCRT__
extern unsigned int* __imp__fmode;
#define	_fmode	(*__imp__fmode)
#else
/* CRTDLL */
extern unsigned int* _fmode_dll;
#define	_fmode	(*_fmode_dll)
#endif
#endif /* 0 */

/*
 * NOTE: This is the correct definition to build crtdll.
 *       It is NOT valid outside of crtdll.
 */
extern unsigned int* _fmode_dll;
extern unsigned int _fmode;


/* Specifiy one of these flags to define the access mode. */
#define	_O_RDONLY	0
#define _O_WRONLY	1
#define _O_RDWR		2

/* Mask for access mode bits in the _open flags. */
#define _O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)

#define	_O_APPEND	0x0008	/* Writes will add to the end of the file. */
#define	_O_CREAT	0x0100	/* Create the file if it does not exist. */
#define	_O_TRUNC	0x0200	/* Truncate the file if it does exist. */
#define	_O_EXCL		0x0400	/* Open only if the file does not exist. */

/* NOTE: Text is the default even if the given _O_TEXT bit is not on. */
#define	_O_TEXT		0x4000	/* CR-LF in file becomes LF in memory. */
#define	_O_BINARY	0x8000	/* Input and output is not translated. */
#define	_O_RAW		_O_BINARY

#define	_O_TEMPORARY	0x0040	/* Make the file dissappear after closing.
				 * WARNING: Even if not created by _open! */
#define	_O_NOINHERIT	0x0080


#define _O_RANDOM	0x0010
#define _O_SEQUENTIAL	_O_RANDOM
#define _O_SHORT_LIVED	0x1000	

#ifndef __STRICT_ANSI__
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
#define	O_NOINHERIT	_O_NOINHERIT

#define O_RANDOM	_O_RANDOM
#define O_SEQUENTIAL	_O_RANDOM
#define O_SHORT_LIVED   _O_SHORT_LIVED

#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
extern "C" {
#endif

int	_setmode (int nHandle, int nAccessMode);

#ifndef	_NO_OLDNAMES
int	setmode (int nHandle, int nAccessMode);
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not __STRICT_ANSI__ */
#endif	/* Not _FCNTL_H_ */
