/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_sys_stat_h_
#define __dj_include_sys_stat_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#define S_ISBLK(m)	(((m) & 0xf000) == 0x1000)
#define S_ISCHR(m)	(((m) & 0xf000) == 0x2000)
#define S_ISDIR(m)	(((m) & 0xf000) == 0x3000)
#define S_ISFIFO(m)	(((m) & 0xf000) == 0x4000)
#define S_ISREG(m)	(((m) & 0xf000) == 0x0000)

#define S_ISUID		0x80000000
#define S_ISGID		0x40000000

#define S_IRUSR		00400
#define S_IRGRP		00040
#define S_IROTH		00004
#define S_IWUSR		00200
#define S_IWGRP		00020
#define S_IWOTH		00002
#define S_IXUSR		00100
#define S_IXGRP		00010
#define S_IXOTH		00001
#define S_IRWXU		00700
#define S_IRWXG		00070
#define S_IRWXO		00007

#include <sys/types.h>
#include <internal/types.h>
//#include <sys/djtypes.h>
__DJ_time_t
#undef __DJ_time_t
#define __DJ_time_t

struct stat {
  time_t	st_atime;
  time_t	st_ctime;
  dev_t		st_dev;
  gid_t		st_gid;
  ino_t		st_ino;
  mode_t	st_mode;
  time_t	st_mtime;
  nlink_t	st_nlink;
  off_t		st_size;
  off_t		st_blksize;
  uid_t		st_uid;
  dev_t		st_rdev; /* unused */
};

int	chmod(const char *_path, mode_t _mode);
int	fstat(int _fildes, struct stat *_buf);
//int	mkdir(const char *_path, mode_t _mode);
int	mkfifo(const char *_path, mode_t _mode);
int	stat(const char *_path, struct stat *_buf);
mode_t	umask(mode_t _cmask);

#ifndef _POSIX_SOURCE

/* POSIX.1 doesn't mention these at all */

#define S_IFMT		0xf000

#define S_IFREG		0x0000
#define S_IFBLK		0x1000
#define S_IFCHR		0x2000
#define S_IFDIR		0x3000
#define S_IFIFO		0x4000
#define S_IFFIFO	S_IFIFO

#define S_IFLABEL	0x5000
#define S_ISLABEL(m)	(((m) & 0xf000) == 0x5000)

void	        _fixpath(const char *, char *);
unsigned short  _get_magic(const char *, int);
int             _is_executable(const char *, int, const char *);
int		mknod(const char *_path, mode_t _mode, dev_t _dev);
char          * _truename(const char *, char *);

/* Bit-mapped variable _djstat_flags describes what expensive
   f?stat() features our application needs.  If you don't need a
   feature, set its bit in the variable.  By default, all the
   bits are cleared (i.e., you get the most expensive code).  */
#define _STAT_INODE         1   /* should we bother getting inode numbers? */
#define _STAT_EXEC_EXT      2   /* get execute bits from file extension? */
#define _STAT_EXEC_MAGIC    4   /* get execute bits from magic signature? */
#define _STAT_DIRSIZE       8   /* compute directory size? */
#define _STAT_ROOT_TIME  0x10   /* try to get root dir time stamp? */
#define _STAT_WRITEBIT   0x20   /* fstat() needs write bit? */

extern unsigned short   _djstat_flags;

/* Bit-mapped variable _djstat_fail_bits describes which individual
   undocumented features f?stat() failed to use.  To get a human-
   readable description of the bits, call _djstat_describe_lossage(). */
#define _STFAIL_SDA         1   /* Get SDA call failed */
#define _STFAIL_OSVER       2   /* Unsupported DOS version */
#define _STFAIL_BADSDA      4   /* Bad pointer to SDA */
#define _STFAIL_TRUENAME    8   /* _truename() failed */
#define _STFAIL_HASH     0x10   /* inode defaults to hashing */
#define _STFAIL_LABEL    0x20   /* Root dir, but no volume label */
#define _STFAIL_DCOUNT   0x40   /* dirent_count ridiculously large */
#define _STFAIL_WRITEBIT 0x80   /* fstat() failed to get write access bit */
#define _STFAIL_DEVNO   0x100   /* fstat() failed to get device number */
#define _STFAIL_BADSFT  0x200   /* SFT entry found, but can't be trusted */
#define _STFAIL_SFTIDX  0x400   /* bad SFT index in JFT */
#define _STFAIL_SFTNF   0x800   /* file entry not found in SFT array */

extern unsigned short   _djstat_fail_bits;

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_sys_stat_h_ */
