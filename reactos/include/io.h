/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_io_h_
#define __dj_include_io_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#include <sys/types.h>
#include <internal/types.h>


/*
 *  For compatibility with other DOS C compilers.
 */

#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */


struct _finddata_t {
  char reserved[21] __attribute__((packed));
  unsigned char attrib __attribute__((packed));
  unsigned short time_create __attribute__((packed));
  unsigned short time_access __attribute__((packed));
  unsigned short time_write __attribute__((packed));
  unsigned long size __attribute__((packed));
  char name[256] __attribute__((packed));
};

int		chsize(int handle, long size);
int		close(int _fd);
int		_close(int _fd);
int		_creat(const char *_path, int _attrib);
unsigned int   _commit(int _handle);
ssize_t		crlf2nl(char *_buffer, ssize_t _length);
int		_dos_lock(int _fd, long _offset, long _length);
long		filelength(int _handle);
long  		_findfirst(char *_name, struct _finddata_t *_result);
int  		_findnext(long handle, struct _finddata_t  *_result);
int  		_findclose(long handle);
short		_get_dev_info(int _arg);
int		lock(int _fd, long _offset, long _length);
int		_open(const char *_path, int _oflag, ...);
size_t		_read(int _fd, void *_buf,size_t _nbyte);
int		setmode(int _fd, int _newmode);
int		_setmode(int _fd, int _newmode);
off_t		tell(int _fd);
int		_dos_unlock(int _fd, long _offset, long _length);
int		unlock(int _fd, long _offset, long _length);
size_t		_write(int _fd, const void *_buf, size_t _nbyte);
int	        _chmod(const char *_path, int _func, ...);
void		_flush_disk_cache(void);

int _dup( int handle);
int _dup2( int handle1, int handle2 );
long _lseek(int _filedes, long _offset, int _whence);
int _open_osfhandle ( void *osfhandle, int flags );

#define open _open
#define dup _dup
#define dup2 _dup2
#define lseek _lseek
#define open_osfhandle _open_osfhandle




#define sopen(path, access, shflag, mode) \
	open((path), (access)|(shflag), (mode))

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_io_h_ */
