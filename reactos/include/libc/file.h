/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#ifndef __dj_include_libc_file_h__
#define __dj_include_libc_file_h__

#include <stdio.h>
#include <fcntl.h>


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

int __set_errno(int err);

char __is_text_file(FILE *p);

int __fileno_alloc(void *hFile, int mode);

int _doprnt(const char *fmt, va_list args, FILE *f);
int _doscan(FILE *iop, const char *fmt, void **argp);
int _dowscan(FILE *iop, const wchar_t *fmt, void **argp);


int __fileno_dup2( int handle1, int handle2 );
int __fileno_setmode(int _fd, int _newmode);
int __fileno_close(int _fd);


#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* __dj_include_libc_file_h__ */
