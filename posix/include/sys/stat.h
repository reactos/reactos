/* $Id: stat.h,v 1.2 2002/02/20 09:17:56 hyperion Exp $
 */
/*
 * sys/stat.h
 *
 * data returned by the stat() function. Conforming to the Single
 * UNIX(r) Specification Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
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
 */
#ifndef __SYS_STAT_H_INCLUDED__
#define __SYS_STAT_H_INCLUDED__

/* INCLUDES */
#include <sys/types.h>

/* OBJECTS */

/* TYPES */
struct stat
{
 dev_t     st_dev;     /* ID of device containing file */
 ino_t     st_ino;     /* file serial number */
 mode_t    st_mode;    /* mode of file (see below) */
 nlink_t   st_nlink;   /* number of links to the file */
 uid_t     st_uid;     /* user ID of file */
 gid_t     st_gid;     /* group ID of file */
 dev_t     st_rdev;    /* device ID (if file is character or block special) */
 off_t     st_size;    /* file size in bytes (if file is a regular file) */
 time_t    st_atime;   /* time of last access */
 time_t    st_mtime;   /* time of last data modification */
 time_t    st_ctime;   /* time of last status change */
 blksize_t st_blksize; /* a filesystem-specific preferred I/O block size for
                          this object.  In some filesystem types, this may
			  vary from file to file */
 blkcnt_t  st_blocks;  /* number of blocks allocated for this object */
};

/* CONSTANTS */
/*
  file type
 */
#define S_IFBLK (1) /* block special */
#define S_IFCHR (2) /* character special */
#define S_IFIFO (3) /* FIFO special */
#define S_IFREG (4) /* regular */
#define S_IFDIR (5) /* directory */
#define S_IFLNK (6) /* symbolic link */

/* type of file */
#define S_IFMT  (S_IFBLK | S_IFCHR | S_IFIFO | S_IFREG | S_IFDIR | S_IFLNK)

/*
  file mode bits
 */
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR) /* read, write, execute/search by owner */
#define S_IRUSR (0x00000001)                  /* read permission, owner */
#define S_IWUSR (0x00000002)                  /* write permission, owner */
#define S_IXUSR (0x00000004)                  /* execute/search permission, owner */

#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP) /* read, write, execute/search by group */
#define S_IRGRP (0x00000010)                  /* read permission, group */
#define S_IWGRP (0x00000020)                  /* write permission, group */
#define S_IXGRP (0x00000040)                  /* execute/search permission, group */

#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH) /* read, write, execute/search by others */
#define S_IROTH (0x00000100)                  /* read permission, others */
#define S_IWOTH (0x00000200)                  /* write permission, others */
#define S_IXOTH (0x00000400)                  /* execute/search permission, others */

#define S_ISUID (0x00001000)                  /* set-user-ID on execution */
#define S_ISGID (0x00002000)                  /* set-group-ID on execution */
#define S_ISVTX (0x00004000)                  /* on directories, restricted deletion flag */

/*
  the following macros will test whether a file is of the specified type
 */
#define	S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define	S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define	S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define	S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define	S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define	S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define	S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* shared memory, semaphores and message queues are unlikely to be ever
   implemented as files */
#define S_TYPEISMQ(buf)  (0) /* Test for a message queue */
#define S_TYPEISSEM(buf) (0) /* Test for a semaphore */
#define S_TYPEISSHM(buf) (0) /* Test for a shared memory object */

/* PROTOTYPES */
int    chmod(const char *, mode_t);
int    fchmod(int, mode_t);
int    fstat(int, struct stat *);
int    lstat(const char *, struct stat *);
int    mkdir(const char *, mode_t);
int    mkfifo(const char *, mode_t);
int    mknod(const char *, mode_t, dev_t);
int    stat(const char *, struct stat *);
mode_t umask(mode_t);

/* MACROS */

#endif /* __SYS_STAT_H_INCLUDED__ */

/* EOF */

