/*
 * Standard I/O definitions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDIO_H
#define __WINE_STDIO_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

/* file._flag flags */
#ifndef USE_MSVCRT_PREFIX
#define _IOREAD          0x0001
#define _IOWRT           0x0002
#define _IOMYBUF         0x0008
#define _IOEOF           0x0010
#define _IOERR           0x0020
#define _IOSTRG          0x0040
#define _IORW            0x0080
#define _IOAPPEND        0x0200
#else
#define MSVCRT__IOREAD   0x0001
#define MSVCRT__IOWRT    0x0002
#define MSVCRT__IOMYBUF  0x0008
#define MSVCRT__IOEOF    0x0010
#define MSVCRT__IOERR    0x0020
#define MSVCRT__IOSTRG   0x0040
#define MSVCRT__IORW     0x0080
#define MSVCRT__IOAPPEND 0x0200
#endif /* USE_MSVCRT_PREFIX */

#ifndef NULL
#ifdef  __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#ifndef USE_MSVCRT_PREFIX

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* more file._flag flags, but these conflict with Unix */
#define _IOFBF    0x0000
#define _IONBF    0x0004
#define _IOLBF    0x0040

#define EOF       (-1)
#define FILENAME_MAX 260
#define TMP_MAX   0x7fff
#define FOPEN_MAX 20
#define L_tmpnam  260

#define BUFSIZ    512

#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif

#else

/* more file._flag flags, but these conflict with Unix */
#define MSVCRT__IOFBF    0x0000
#define MSVCRT__IONBF    0x0004
#define MSVCRT__IOLBF    0x0040

#define MSVCRT_FILENAME_MAX 260
#define MSVCRT_TMP_MAX   0x7fff

#define MSVCRT_EOF       (-1)

#define MSVCRT_BUFSIZ    512

#endif /* USE_MSVCRT_PREFIX */

#ifndef MSVCRT_FILE_DEFINED
#define MSVCRT_FILE_DEFINED
typedef struct MSVCRT(_iobuf)
{
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
} MSVCRT(FILE);
#endif  /* MSVCRT_FILE_DEFINED */

#ifndef MSVCRT_FPOS_T_DEFINED
typedef long MSVCRT(fpos_t);
#define MSVCRT_FPOS_T_DEFINED
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef MSVCRT_WCTYPE_T_DEFINED
typedef MSVCRT(wchar_t) MSVCRT(wint_t);
typedef MSVCRT(wchar_t) MSVCRT(wctype_t);
#define MSVCRT_WCTYPE_T_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSVCRT_STDIO_DEFINED
MSVCRT(FILE)*        MSVCRT(__p__iob)(void);
#define _iob               (__p__iob())
#endif /* MSVCRT_STDIO_DEFINED */

#ifndef USE_MSVCRT_PREFIX
#define stdin              (_iob+STDIN_FILENO)
#define stdout             (_iob+STDOUT_FILENO)
#define stderr             (_iob+STDERR_FILENO)
#endif /* USE_MSVCRT_PREFIX */

#ifndef MSVCRT_STDIO_DEFINED
#define MSVCRT_STDIO_DEFINED
int         _fcloseall(void);
//MSVCRT(FILE)* _fdopen(int,const char*);
int         _fgetchar(void);
int         _filbuf(MSVCRT(FILE*));
//int         _fileno(MSVCRT(FILE)*);
int         _flsbuf(int,MSVCRT(FILE)*);
int         _flushall(void);
int         _fputchar(int);
MSVCRT(FILE)* _fsopen(const char*,const char*,int);
int         _getmaxstdio(void);
//int         _getw(MSVCRT(FILE)*);
//int         _pclose(MSVCRT(FILE)*);
//MSVCRT(FILE)* _popen(const char*,const char*);
//int         _putw(int,MSVCRT(FILE)*);
int         _rmtmp(void);
int         _setmaxstdio(int);
int         _snprintf(char*,MSVCRT(size_t),const char*,...);
char*       _tempnam(const char*,const char*);
int         _unlink(const char*);
int         _vsnprintf(char*,MSVCRT(size_t),const char*,va_list);

void        MSVCRT(clearerr)(MSVCRT(FILE)*);
int         MSVCRT(fclose)(MSVCRT(FILE)*);
int         MSVCRT(feof)(MSVCRT(FILE)*);
int         MSVCRT(ferror)(MSVCRT(FILE)*);
int         MSVCRT(fflush)(MSVCRT(FILE)*);
int         MSVCRT(fgetc)(MSVCRT(FILE)*);
int         MSVCRT(fgetpos)(MSVCRT(FILE)*,MSVCRT(fpos_t)*);
char*       MSVCRT(fgets)(char*,int,MSVCRT(FILE)*);
MSVCRT(FILE)* MSVCRT(fopen)(const char*,const char*);
int         MSVCRT(fprintf)(MSVCRT(FILE)*,const char*,...);
int         MSVCRT(fputc)(int,MSVCRT(FILE)*);
int         MSVCRT(fputs)(const char*,MSVCRT(FILE)*);
MSVCRT(size_t) MSVCRT(fread)(void*,MSVCRT(size_t),MSVCRT(size_t),MSVCRT(FILE)*);
MSVCRT(FILE)* MSVCRT(freopen)(const char*,const char*,MSVCRT(FILE)*);
int         MSVCRT(fscanf)(MSVCRT(FILE)*,const char*,...);
int         MSVCRT(fseek)(MSVCRT(FILE)*,long,int);
int         MSVCRT(fsetpos)(MSVCRT(FILE)*,MSVCRT(fpos_t)*);
long        MSVCRT(ftell)(MSVCRT(FILE)*);
MSVCRT(size_t) MSVCRT(fwrite)(const void*,MSVCRT(size_t),MSVCRT(size_t),MSVCRT(FILE)*);
int         MSVCRT(getc)(MSVCRT(FILE)*);
int         MSVCRT(getchar)(void);
char*       MSVCRT(gets)(char*);
void        MSVCRT(perror)(const char*);
int         MSVCRT(printf)(const char*,...);
int         MSVCRT(putc)(int,MSVCRT(FILE)*);
int         MSVCRT(putchar)(int);
int         MSVCRT(puts)(const char*);
int         MSVCRT(remove)(const char*);
int         MSVCRT(rename)(const char*,const char*);
void        MSVCRT(rewind)(MSVCRT(FILE)*);
int         MSVCRT(scanf)(const char*,...);
void        MSVCRT(setbuf)(MSVCRT(FILE)*,char*);
int         MSVCRT(setvbuf)(MSVCRT(FILE)*,char*,int,MSVCRT(size_t));
int         MSVCRT(sprintf)(char*,const char*,...);
int         MSVCRT(sscanf)(const char*,const char*,...);
MSVCRT(FILE)* MSVCRT(tmpfile)(void);
char*       MSVCRT(tmpnam)(char*);
int         MSVCRT(ungetc)(int,MSVCRT(FILE)*);
int         MSVCRT(vfprintf)(MSVCRT(FILE)*,const char*,va_list);
int         MSVCRT(vprintf)(const char*,va_list);
int         MSVCRT(vsprintf)(char*,const char*,va_list);

#ifndef MSVCRT_WSTDIO_DEFINED
#define MSVCRT_WSTDIO_DEFINED
MSVCRT(wint_t)  _fgetwchar(void);
MSVCRT(wint_t)  _fputwchar(MSVCRT(wint_t));
MSVCRT(wchar_t)*_getws(MSVCRT(wchar_t)*);
int             _putws(const MSVCRT(wchar_t)*);
int             _snwprintf(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,...);
int             _vsnwprintf(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,va_list);
//MSVCRT(FILE)*   _wfdopen(int,const MSVCRT(wchar_t)*);
//MSVCRT(FILE)*   _wfopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
//MSVCRT(FILE)*   _wfreopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
//MSVCRT(FILE)*   _wfsopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,int);
void            _wperror(const MSVCRT(wchar_t)*);
//MSVCRT(FILE)*   _wpopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             _wremove(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wtempnam(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wtmpnam(MSVCRT(wchar_t)*);

MSVCRT(wint_t)  MSVCRT(fgetwc)(MSVCRT(FILE)*);
MSVCRT(wchar_t)*MSVCRT(fgetws)(MSVCRT(wchar_t)*,int,MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(fputwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
int             MSVCRT(fputws)(const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
int             MSVCRT(fwprintf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,...);
int             MSVCRT(fputws)(const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
int             MSVCRT(fwscanf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,...);
MSVCRT(wint_t)  MSVCRT(getwc)(MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(getwchar)(void);
MSVCRT(wchar_t)*MSVCRT(getws)(MSVCRT(wchar_t)*);
MSVCRT(wint_t)  MSVCRT(putwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(putwchar)(MSVCRT(wint_t));
int             MSVCRT(putws)(const MSVCRT(wchar_t)*);
int             MSVCRT(swprintf)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int             MSVCRT(swscanf)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
MSVCRT(wint_t)  MSVCRT(ungetwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
int             MSVCRT(vfwprintf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(vswprintf)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(vwprintf)(const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(wprintf)(const MSVCRT(wchar_t)*,...);
int             MSVCRT(wscanf)(const MSVCRT(wchar_t)*,...);
#endif /* MSVCRT_WSTDIO_DEFINED */

#endif /* MSVCRT_STDIO_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
static inline MSVCRT(FILE)* fdopen(int fd, const char *mode) { return _fdopen(fd, mode); }
static inline int fgetchar(void) { return _fgetchar(); }
static inline int fileno(MSVCRT(FILE)* file) { return _fileno(file); }
static inline int fputchar(int c) { return _fputchar(c); }
static inline int pclose(MSVCRT(FILE)* file) { return _pclose(file); }
static inline MSVCRT(FILE)* popen(const char* command, const char* mode) { return _popen(command, mode); }
static inline char* tempnam(const char *dir, const char *prefix) { return _tempnam(dir, prefix); }
#ifndef MSVCRT_UNLINK_DEFINED
static inline int unlink(const char* path) { return _unlink(path); }
#define MSVCRT_UNLINK_DEFINED
#endif
static inline int vsnprintf(char *buffer, size_t size, const char *format, va_list args) { return _vsnprintf(buffer,size,format,args); }

static inline MSVCRT(wint_t) fgetwchar(void) { return _fgetwchar(); }
static inline MSVCRT(wint_t) fputwchar(MSVCRT(wint_t) wc) { return _fputwchar(wc); }
static inline int getw(MSVCRT(FILE)* file) { return _getw(file); }
static inline int putw(int val, MSVCRT(FILE)* file) { return _putw(val, file); }
static inline MSVCRT(FILE)* wpopen(const MSVCRT(wchar_t)* command,const MSVCRT(wchar_t)* mode) { return _wpopen(command, mode); }
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_STDIO_H */
