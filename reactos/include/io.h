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


#define F_OK	0x01
#define R_OK	0x02
#define W_OK	0x04
#define X_OK	0x08
#define D_OK	0x10


struct _finddata_t {
  char reserved[21] __attribute__((packed));
  unsigned char attrib __attribute__((packed));
  unsigned short time_create __attribute__((packed));
  unsigned short time_access __attribute__((packed));
  unsigned short time_write __attribute__((packed));
  unsigned long size __attribute__((packed));
  char name[256] __attribute__((packed));
};

int 		_access( const char *_path, int _amode );
int		_chmod(const char *filename, int func);
int		_chsize(int _fd, long size);
int		_close(int _fd);
int		_creat(const char *_path, int _attrib);
unsigned int    _commit(int _fd);
int 		_dup(int _fd);
int 		_dup2( int _fd1, int _fd2 );
int 		_eof( int _fd );
long		_filelength(int _fd);
long  		_findfirst(char *_name, struct _finddata_t *_result);
int  		_findnext(long handle, struct _finddata_t  *_result);
int  		_findclose(long handle);
void *		_get_osfhandle(int fileno);
int 		_locking( int _fd, int mode, long nbytes );
off_t		_lseek(int _fd, off_t _offset, int _whence);
char *		_mktemp (char *_template);
int		_open(const char *_path, int _oflag, ...);
int 		_open_osfhandle ( void *osfhandle, int flags );
int 		_pipe(int _fildes[2], unsigned int size, int mode );
size_t		_read(int _fd, void *_buf,size_t _nbyte);
int		remove(const char *fn);
int		rename(const char *old, const char *new);
int		_setmode(int _fd, int _newmode);
int		_sopen(const char *path, int access, int shflag, ...);
off_t		_tell(int _fd);
mode_t		_umask(mode_t newmask);
int		_unlink(const char *_path);
//int		unlock(int _fd, long _offset, long _length);
size_t		_write(int _fd, const void *_buf, size_t _nbyte);


#define access			_access            
#define chmod                  _chmod             
#define chsize                 _chsize            
#define close                  _close             
#define creat                  _creat             
#define commit                 _commit            
#define dup                    _dup               
#define dup2                   _dup2              
#define eof                    _eof               
#define filelength             _filelength        
#define findfirst              _findfirst         
#define findnext               _findnext          
#define findclose              _findclose         
#define get_osfhandle          _get_osfhandle     
#define locking                _locking           
#define lseek                  _lseek             
#define mktemp                 _mktemp            
#define open                   _open              
#define open_osfhandle         _open_osfhandle    
#define pipe                   _pipe              
#define read                   _read              
#define setmode                _setmode           
#define sopen(path, access, shflag, mode) \
		_open((path), (access)|(shflag), (mode))            
#define tell                   _tell              
#define umask                  _umask             
#define unlink                 _unlink            
#define unlock                  unlock             
#define write                  _write       


#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_io_h_ */
