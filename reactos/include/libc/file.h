/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#ifndef __dj_include_libc_file_h__
#define __dj_include_libc_file_h__

#include <stdio.h>
#include <fcntl.h>


//#include <libc/dosio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#ifndef _IORMONCL
#define _IORMONCL 004000  /* remove on close, for temp files */
#endif
/* if _flag & _IORMONCL, ._name_to_remove needs freeing */

#ifndef _IOUNGETC
#define _IOUNGETC 010000  /* there is an ungetc'ed character in the buffer */
#endif

int	_flsbuf(int, FILE*);
int	_filbuf(FILE *);
void	_fwalk(void (*)(FILE *));



char __is_text_file(FILE *p);

int _doprnt(const char *fmt, va_list args, FILE *f);
int _doscan(FILE *iop, const char *fmt, void **argp);

void *filehnd(int fileno);
int __fileno_dup2( int handle1, int handle2 );
int __fileno_setmode(int _fd, int _newmode);
int	__fileno_close(int _fd);

#undef  fileno
#define fileno(f)	(f->_file)
#undef  feof
#define feof(f)		(((f)->_flag&_IOEOF)!=0)
#undef  ferror
#define ferror(f)	(((f)->_flag&_IOERR)!=0)

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* __dj_include_libc_file_h__ */
