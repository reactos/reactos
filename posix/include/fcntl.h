/* $Id:
 */
/*
 * fcntl.h
 *
 * file control options. Based on the Single UNIX(r) Specification,
 * Version 2
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
#ifndef __FCNTL_H_INCLUDED__
#define __FCNTL_H_INCLUDED__

/* INCLUDES */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* OBJECTS */

/* TYPES */
/* 
  the structure flock describes a file lock
 */
struct flock
{
 short l_type;   /* type of lock; F_RDLCK, F_WRLCK, F_UNLCK */
 short l_whence; /* flag for starting offset */
 off_t l_start;  /* relative offset in bytes */
 off_t l_len;    /* size; if 0 then until EOF */
 pid_t l_pid;    /* process ID of the process holding the lock;
                   returned with F_GETLK */
};

/* CONSTANTS */
/*
  values for cmd used by fcntl()
 */
enum __fcntl_cmd
{
 F_DUPFD,  /* duplicate file descriptor */
 F_GETFD,  /* get file descriptor flags */
 F_SETFD,  /* set file descriptor flags */
 F_GETFL,  /* get file status flags and file access modes */
 F_SETFL,  /* Set file status flags */
 F_GETLK,  /* get record locking information */
 F_SETLK,  /* set record locking information */
 F_SETLKW, /* set record locking information; wait if blocked */
/* ReactOS-specific */
 F_NEWFD,  /* create new file descriptor */
 F_DELFD,  /* delete file descriptor */
 F_GETALL, /* get a copy of the internal descriptor object */
 F_SETALL, /* initialize internal descriptor object */
 F_GETXP,  /* get file descriptor extra data pointer */
 F_SETXP,  /* set file descriptor extra data pointer */
 F_GETXS,  /* get file descriptor extra data size */
 F_SETXS,  /* set file descriptor extra data size */
 F_GETFH,  /* get file handle */
 F_SETFH   /* set file handle */
};

/* 
  file descriptor flags used for fcntl()
 */
/* Close the file descriptor upon execution of an exec family function. */
#define FD_CLOEXEC (0x00000001) 

/*
  values for l_type used for record locking with fcntl()
 */
/* Shared or read lock. */
#define F_RDLCK (1)
/* Unlock. */
#define F_UNLCK (2)
/* Exclusive or write lock. */
#define F_WRLCK (3)

/*
  file flags used for open()
 */
/* Create file if it does not exist. */
#define O_CREAT  (0x00000001)
/* Exclusive use flag. */
#define O_EXCL   (0x00000002)
/* Do not assign controlling terminal. */
#define O_NOCTTY (0x00000004)
/* Truncate flag. */
#define O_TRUNC  (0x00000008)
/* ReactOS-specific */
/* File must be a directory */
#define _O_DIRFILE (0x00000010)

/*
  file status flags used for open() and fcntl()
 */
/* Set append mode. */
#define O_APPEND   (0x00000100)
/* Write according to synchronised I/O data integrity completion. */
#define O_DSYNC    (0x00000200)
/* Non-blocking mode. */
#define O_NONBLOCK (0x00000400)
/* Synchronised read I/O operations. */
#define O_RSYNC    (0x00000800)
/* Write according to synchronised I/O file integrity completion. */
#define O_SYNC     (0x00001000)

/* 
  file access modes used for open() and fcntl()
 */
/* Open for reading only. */
#define O_RDONLY (0x01000000)
/* Open for reading and writing. */
#define O_RDWR   (0x02000000)
/* Open for writing only. */
#define O_WRONLY (0x04000000)

/* 
  mask for use with file access modes
 */
#define O_ACCMODE (O_RDONLY | O_RDWR | O_WRONLY)

/* PROTOTYPES */
int  creat(const char *, mode_t);
int  fcntl(int, int, ...);
int  open(const char *, int, ...);

int _Wcreat(const wchar_t *, mode_t);
int _Wopen(const wchar_t *, int, ...);

/* MACROS */

#endif /* __FCNTL_H_INCLUDED__ */

/* EOF */

