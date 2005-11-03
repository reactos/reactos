/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stat.h	8.6 (Berkeley) 3/8/94
 */

#ifndef _SYS_STAT_H_
#define	_SYS_STAT_H_

/*
 * XXX we need this for struct timespec.  We get miscellaneous namespace
 * pollution with it.  struct timespace itself is namespace pollution if
 * _POSIX_SOURCE is defined.
 */
#include <sys/time.h>

#ifndef _POSIX_SOURCE
struct ostat {
	unsigned short	st_dev;		/* inode's device */
	ino_t	st_ino;			/* inode's number */
	mode_t	st_mode;		/* inode protection mode */
	nlink_t	st_nlink;		/* number of hard links */
	unsigned short	st_uid;		/* user ID of the file's owner */
	unsigned short	st_gid;		/* group ID of the file's group */
	unsigned short	st_rdev;	/* device type */
	long	st_size;		/* file size, in bytes */
	struct	timespec st_atimespec;	/* time of last access */
	struct	timespec st_mtimespec;	/* time of last data modification */
	struct	timespec st_ctimespec;	/* time of last file status change */
	long	st_blksize;		/* optimal blocksize for I/O */
	long	st_blocks;		/* blocks allocated for file */
	unsigned long	st_flags;	/* user defined flags for file */
	unsigned long	st_gen;		/* file generation number */
};
#endif /* !_POSIX_SOURCE */

//#ifndef _MSC_VER
struct stat {
	dev_t	st_dev;			/* inode's device */
	ino_t	st_ino;			/* inode's number */
	mode_t	st_mode;		/* inode protection mode */
	nlink_t	st_nlink;		/* number of hard links */
	uid_t	st_uid;			/* user ID of the file's owner */
	gid_t	st_gid;			/* group ID of the file's group */
	dev_t	st_rdev;		/* device type */
	struct	timespec st_atimespec;	/* time of last access */
	struct	timespec st_mtimespec;	/* time of last data modification */
	struct	timespec st_ctimespec;	/* time of last file status change */
	off_t	st_size;		/* file size, in bytes */
	quad_t	st_blocks;		/* blocks allocated for file */
	unsigned long	st_blksize;	/* optimal blocksize for I/O */
	unsigned long	st_flags;	/* user defined flags for file */
	unsigned long	st_gen;		/* file generation number */
	long	st_lspare;
	quad_t	st_qspare[2];
};
//#endif//_MSC_VER
#define st_atime st_atimespec.ts_sec
#define st_mtime st_mtimespec.ts_sec
#define st_ctime st_ctimespec.ts_sec

#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */
#ifndef _POSIX_SOURCE
#define	S_ISTXT	0001000			/* sticky bit */
#endif

#define	S_IRWXU	0000700			/* RWX mask for owner */
#define	S_IRUSR	0000400			/* R for owner */
#define	S_IWUSR	0000200			/* W for owner */
#define	S_IXUSR	0000100			/* X for owner */

#ifndef _POSIX_SOURCE
#define	S_IREAD		S_IRUSR
#define	S_IWRITE	S_IWUSR
#define	S_IEXEC		S_IXUSR
#endif

#define	S_IRWXG	0000070			/* RWX mask for group */
#define	S_IRGRP	0000040			/* R for group */
#define	S_IWGRP	0000020			/* W for group */
#define	S_IXGRP	0000010			/* X for group */

#define	S_IRWXO	0000007			/* RWX mask for other */
#define	S_IROTH	0000004			/* R for other */
#define	S_IWOTH	0000002			/* W for other */
#define	S_IXOTH	0000001			/* X for other */

#ifndef _POSIX_SOURCE
#define	S_IFMT	 0170000		/* type of file mask */
#define	S_IFIFO	 0010000		/* named pipe (fifo) */
#define	S_IFCHR	 0020000		/* character special */
#define	S_IFDIR	 0040000		/* directory */
#define	S_IFBLK	 0060000		/* block special */
#define	S_IFREG	 0100000		/* regular */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */
#define	S_ISVTX	 0001000		/* save swapped text even after use */
#endif

#define	S_ISDIR(m)	(((m) & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	(((m) & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	(((m) & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	(((m) & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	(((m) & 0170000) == 0010000 || \
			 ((m) & 0170000) == 0140000)	/* fifo or socket */
#ifndef _POSIX_SOURCE
#define	S_ISLNK(m)	(((m) & 0170000) == 0120000)	/* symbolic link */
#define	S_ISSOCK(m)	(((m) & 0170000) == 0010000 || \
			 ((m) & 0170000) == 0140000)	/* fifo or socket */
#endif

#ifndef _POSIX_SOURCE
#define	ACCESSPERMS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* 0777 */
							/* 7777 */
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
							/* 0666 */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define S_BLKSIZE	512		/* block size used in the stat struct */

/*
 * Definitions of flags stored in file flags word.
 *
 * Super-user and owner changeable flags.
 */
#define	UF_SETTABLE	0x0000ffff	/* mask of owner changeable flags */
#define	UF_NODUMP	0x00000001	/* do not dump file */
#define	UF_IMMUTABLE	0x00000002	/* file may not be changed */
#define	UF_APPEND	0x00000004	/* writes to file may only append */
/*
 * Super-user changeable flags.
 */
#define	SF_SETTABLE	0xffff0000	/* mask of superuser changeable flags */
#define	SF_ARCHIVED	0x00010000	/* file is archived */
#define	SF_IMMUTABLE	0x00020000	/* file may not be changed */
#define	SF_APPEND	0x00040000	/* writes to file may only append */

#ifdef KERNEL
/*
 * Shorthand abbreviations of above.
 */
#define	APPEND		(UF_APPEND | SF_APPEND)
#define	IMMUTABLE	(UF_IMMUTABLE | SF_IMMUTABLE)
#endif

#endif /* !_POSIX_SOURCE */

#ifdef KERNEL
void    cvtstat __P((struct stat *, struct ostat *));
#else /* KERNEL */
#include <sys/cdefs.h>

__BEGIN_DECLS
int	chmod __P((const char *, mode_t));
int	fstat __P((int, struct stat *));
int	mkdir __P((const char *, mode_t));
int	mkfifo __P((const char *, mode_t));
int	stat __P((const char *, struct stat *));
mode_t	umask __P((mode_t));
#ifndef _POSIX_SOURCE
int	chflags __P((const char *, u_long));
int	fchflags __P((int, u_long));
int	fchmod __P((int, mode_t));
int	lstat __P((const char *, struct stat *));
#endif
__END_DECLS

#endif /* KERNEL */

#endif /* !_SYS_STAT_H_ */
