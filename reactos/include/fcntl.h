/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_fcntl_h_
#define __dj_include_fcntl_h_

#ifdef __cplusplus
extern "C" {
#endif

#define _IOREAD   000010
#define _IOWRT    000020
#define _IOMYBUF  000040
#define _IOEOF    000100
#define _IOERR    000200
#define _IOSTRG   000400
#define _IORW     001000
#define _IOAPPEND 002000
#define _IORMONCL 004000  /* remove on close, for temp files */
/* if _flag & _IORMONCL, ._name_to_remove needs freeing */
#define _IOUNGETC 010000  /* there is an ungetc'ed character in the buffer */


#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#define FD_CLOEXEC	0x0001

#define F_DUPFD		1
#define F_GETFD		2
#define F_GETFL		3
#define F_GETLK		4
#define F_SETFD		5
#define F_SETFL		6
#define F_SETLK		7
#define F_SETLKW	8

#define F_UNLCK		0
#define F_RDLCK		1
#define F_WRLCK		2

#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_ACCMODE	0x0003

#define O_BINARY	0x0004	/* must fit in char, reserved by dos */
#define O_TEXT		0x0008	/* must fit in char, reserved by dos */

#define O_RANDOM	0x0010	
#define O_SEQUENTIAL	0x0020	


#define O_TEMPORARY	0x0040	

/* temporary access hint */



/* sequential/random access hints */




#define O_NOINHERIT	0x0080	/* DOS-specific */

#define O_CREAT		0x0100	/* second byte, away from DOS bits */
#define O_EXCL		0x0200
#define O_NOCTTY	0x0400
#define O_TRUNC		0x0800
#define O_APPEND	0x1000
#define O_NONBLOCK	0x2000

#define O_SHORT_LIVED	0x1000	

//#include <sys/types.h>
#include <sys/stat.h>

//typedef int		dev_t;
//typedef int		ino_t;
//typedef int		mode_t;
//typedef int		nlink_t;

#include <io.h>


struct flock {
  off_t	l_len;
  pid_t	l_pid;
  off_t	l_start;
  short	l_type;
  short	l_whence;
};

extern int _fmode; /* O_TEXT or O_BINARY */

//int	open(const char *_path, int _oflag, ...);
//int	creat(const char *_path, int _mode);
int	fcntl(int _fildes, int _cmd, ...);

#ifndef _POSIX_SOURCE


#define S_IREAD		S_IRUSR
#define S_IWRITE	S_IWUSR
#define S_IEXEC		S_IXUSR

/*
 *  For compatibility with other DOS C compilers.
 */

#define _O_RDONLY       O_RDONLY
#define _O_WRONLY       O_WRONLY
#define _O_RDWR         O_RDWR
#define _O_APPEND       O_APPEND
#define _O_CREAT        O_CREAT
#define _O_TRUNC        O_TRUNC
#define _O_EXCL         O_EXCL
#define _O_TEXT         O_TEXT
#define _O_BINARY       O_BINARY
#define _O_NOINHERIT    O_NOINHERIT
#define _O_RANDOM	O_RANDOM
#define _O_SEQUENTIAL	O_RANDOM
#define _O_SHORT_LIVED  O_SHORT_LIVED
#define _O_TEMPORARY 	O_TEMPORARY 

#define _S_IREAD	S_IRUSR
#define _S_IWRITE	S_IWUSR
#define _S_IEXEC	S_IXUSR

/*
 * Support for advanced filesystems (Windows 9x VFAT, NTFS, LFN etc.)
 */

#define _FILESYS_UNKNOWN	0x80000000U
#define _FILESYS_CASE_SENSITIVE	0x0001
#define _FILESYS_CASE_PRESERVED	0x0002
#define _FILESYS_UNICODE	0x0004
#define _FILESYS_LFN_SUPPORTED	0x4000
#define _FILESYS_VOL_COMPRESSED	0x8000

unsigned _get_volume_info (const char *_path, int *_max_file_len, int *_max_path_len, char *_filesystype);
char _use_lfn (const char *_path);
char *_lfn_gen_short_fname (const char *_long_fname, char *_short_fname);

#define _LFN_CTIME	1
#define _LFN_ATIME	2

unsigned _lfn_get_ftime (int _handle, int _which);

char _preserve_fncase (void);
#define _USE_LFN	_use_lfn(0) /* assume it's the same on ALL drives */

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_fcntl_h_ */
