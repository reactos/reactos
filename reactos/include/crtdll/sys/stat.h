/*
 * stat.h
 *
 * Symbolic constants for opening and creating files, also stat, fstat and
 * chmod functions.
 *
 * This file is part of the Mingw32 package.
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
 * $Revision: 1.6 $
 * $Author: ekohl $
 * $Date: 2001/07/03 13:14:18 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _STAT_H_
#define _STAT_H_

#include <crtdll/sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Constants for the stat st_mode member.
 */
#define	S_IFIFO		0x1000	/* FIFO */
#define	S_IFCHR		0x2000	/* Character */
#define	S_IFBLK		0x3000	/* Block */
#define	S_IFDIR		0x4000	/* Directory */
#define	S_IFREG		0x8000	/* Regular */

#define	S_IFMT		0xF000	/* File type mask */

#define	S_IEXEC		0x0040
#define	S_IWRITE	0x0080
#define	S_IREAD		0x0100

#define	S_ISDIR(m)	((m) & S_IFDIR)
#define	S_ISFIFO(m)	((m) & S_IFIFO)
#define	S_ISCHR(m)	((m) & S_IFCHR)
#define	S_ISBLK(m)	((m) & S_IFBLK)
#define	S_ISREG(m)	((m) & S_IFREG)

#define	S_IRWXU		(S_IREAD | S_IWRITE | S_IEXEC)
#define	S_IXUSR		S_IEXEC
#define	S_IWUSR		S_IWRITE
#define	S_IRUSR		S_IREAD

#define	_S_IEXEC	S_IEXEC
#define _S_IREAD  	S_IREAD
#define _S_IWRITE 	S_IWRITE

/*
 * The structure manipulated and returned by stat and fstat.
 *
 * NOTE: If called on a directory the values in the time fields are not only
 * invalid, they will cause localtime et. al. to return NULL. And calling
 * asctime with a NULL pointer causes an Invalid Page Fault. So watch it!
 */
struct stat
{
	short	st_dev;		/* Equivalent to drive number 0=A 1=B ... */
	short	st_ino;		/* Always zero ? */
	short	st_mode;	/* See above constants */
	short	st_nlink;	/* Number of links. */
	int	st_uid;		/* User: Maybe significant on NT ? */
	short	st_gid;		/* Group: Ditto */
	short	st_rdev;	/* Seems useless (not even filled in) */
	long	st_size;	/* File size in bytes */
	time_t	st_atime;	/* Accessed date (always 00:00 hrs local
				 * on FAT) */
	time_t	st_mtime;	/* Modified time */
	time_t	st_ctime;	/* Creation time */
};


int	_fstat (int nFileNo, struct stat* pstat);
int	_chmod (const char* szPath, int nMode);
int	_stat (const char* szPath, struct stat* pstat);


#ifndef	_NO_OLDNAMES

#define	fstat(nFileNo, pstat)   _fstat(nFileNo, pstat)
#define	chmod(szPath,nMode) 	_chmod(szPath,nMode)
#define	stat(szPath,pstat) 	_stat(szPath,pstat)

#endif	/* Not _NO_OLDNAMES */


#ifdef	__cplusplus
}
#endif

#endif	/* Not _STAT_H_ */

#endif	/* Not __STRICT_ANSI__ */
