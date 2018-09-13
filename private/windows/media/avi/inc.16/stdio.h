/***
*stdio.h - definitions/declarations for standard I/O routines
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file defines the structures, values, macros, and functions
*   used by the level 2 I/O ("standard I/O") routines.
*   [ANSI/System V]
*
****/

#ifndef _INC_STDIO

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#define __near      _near
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

#ifndef _VA_LIST_DEFINED
typedef char *va_list;
#define _VA_LIST_DEFINED
#endif 

/* buffered I/O macros */

#define BUFSIZ  512
#ifdef _MT
#define _NFILE  40
#else 
#define _NFILE  20
#endif 
#define EOF (-1)

#ifndef _FILE_DEFINED
#pragma pack(2)
struct _iobuf {
    char *_ptr;
    int   _cnt;
    char *_base;
    char  _flag;
    char  _file;
    };
typedef struct _iobuf FILE;
#pragma pack()
#define _FILE_DEFINED
#endif 


/* _P_tmpnam: Directory where temporary files may be created.
 * L_tmpnam size =  size of _P_tmpdir
 *  + 1 (in case _P_tmpdir does not end in "\\")
 *  + 6 (for the temp number string)
 *  + 1 (for the null terminator)
 */

#define  _P_tmpdir "\\"
#define  L_tmpnam sizeof(_P_tmpdir)+8


/* fseek constants */

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0


/* minimum guaranteed filename length, open file count, and unique
 * tmpnam filenames.
 */

#define FILENAME_MAX 128
#define FOPEN_MAX 18
#define TMP_MAX 32767
#define _SYS_OPEN 20


/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else 
#define NULL    ((void *)0)
#endif 
#endif 


/* declare _iob[] array */

#ifndef _STDIO_DEFINED
extern FILE __near __cdecl _iob[];
#endif 


/* define file position type */

#ifndef _FPOS_T_DEFINED
typedef long fpos_t;
#define _FPOS_T_DEFINED
#endif 


/* standard file pointers */

#ifndef _WINDLL
#define stdin  (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])
#endif 
#ifndef _WINDOWS
#define _stdaux (&_iob[3])
#define _stdprn (&_iob[4])
#endif 


#define _IOREAD     0x01
#define _IOWRT      0x02

#define _IOFBF      0x0
#define _IOLBF      0x40
#define _IONBF      0x04

#define _IOMYBUF    0x08
#define _IOEOF      0x10
#define _IOERR      0x20
#define _IOSTRG     0x40
#define _IORW       0x80


#ifdef _WINDOWS
#ifndef _WINDLL
#ifndef _WINFO_DEFINED
/* interface version number */
#define _QWINVER    0

/* max number of windows */
#define _WFILE      20

/* values for windows screen buffer size */
#define _WINBUFINF  0
#define _WINBUFDEF  -1

/* size/move settings */
#define _WINSIZEMIN 1
#define _WINSIZEMAX 2
#define _WINSIZERESTORE 3
#define _WINSIZECHAR    4

/* size/move query types */
#define _WINMAXREQ  100
#define _WINCURRREQ 101

/* values for closing window */
#define _WINPERSIST 1
#define _WINNOPERSIST   0

/* pseudo file handle for frame window */
#define _WINFRAMEHAND   -1

/* menu items */
#define _WINSTATBAR 1
#define _WINTILE    2
#define _WINCASCADE 3
#define _WINARRANGE 4

/* quickwin exit options */
#define _WINEXITPROMPT      1
#define _WINEXITNOPERSIST   2
#define _WINEXITPERSIST     3

/* open structure */
#pragma pack(2)
struct _wopeninfo {
    unsigned int _version;
    const char __far * _title;
    long _wbufsize;
    };
#pragma pack()

/* size/move structure */
struct _wsizeinfo {
    unsigned int _version;
    unsigned int _type;
    unsigned int _x;
    unsigned int _y;
    unsigned int _h;
    unsigned int _w;
    };
#define _WINFO_DEFINED
#endif 
#endif 
#endif 

/* function prototypes */

#ifndef _STDIO_DEFINED
int __cdecl _filbuf(FILE *);
int __cdecl _flsbuf(int, FILE *);
FILE * __cdecl _fsopen(const char *,
    const char *, int);
void __cdecl clearerr(FILE *);
int __cdecl fclose(FILE *);
int __cdecl _fcloseall(void);
FILE * __cdecl _fdopen(int, const char *);
int __cdecl feof(FILE *);
int __cdecl ferror(FILE *);
int __cdecl fflush(FILE *);
int __cdecl fgetc(FILE *);
#ifndef _WINDLL
int __cdecl _fgetchar(void);
#endif 
int __cdecl fgetpos(FILE *, fpos_t *);
char * __cdecl fgets(char *, int, FILE *);
int __cdecl _fileno(FILE *);
int __cdecl _flushall(void);
FILE * __cdecl fopen(const char *,
    const char *);
int __cdecl fprintf(FILE *, const char *, ...);
int __cdecl fputc(int, FILE *);
#ifndef _WINDLL
int __cdecl _fputchar(int);
#endif 
int __cdecl fputs(const char *, FILE *);
size_t __cdecl fread(void *, size_t, size_t, FILE *);
FILE * __cdecl freopen(const char *,
    const char *, FILE *);
#ifndef _WINDLL
int __cdecl fscanf(FILE *, const char *, ...);
#endif 
int __cdecl fsetpos(FILE *, const fpos_t *);
int __cdecl fseek(FILE *, long, int);
long __cdecl ftell(FILE *);
#ifdef _WINDOWS
#ifndef _WINDLL
FILE * __cdecl _fwopen(struct _wopeninfo *, struct _wsizeinfo *, const char *);
#endif 
#endif 
size_t __cdecl fwrite(const void *, size_t, size_t,
    FILE *);
int __cdecl getc(FILE *);
#ifndef _WINDLL
int __cdecl getchar(void);
char * __cdecl gets(char *);
#endif 
int __cdecl _getw(FILE *);
#ifndef _WINDLL
void __cdecl perror(const char *);
#endif 
#ifndef _WINDLL
int __cdecl printf(const char *, ...);
#endif 
int __cdecl putc(int, FILE *);
#ifndef _WINDLL
int __cdecl putchar(int);
int __cdecl puts(const char *);
#endif 
int __cdecl _putw(int, FILE *);
int __cdecl remove(const char *);
int __cdecl rename(const char *, const char *);
void __cdecl rewind(FILE *);
int __cdecl _rmtmp(void);
#ifndef _WINDLL
int __cdecl scanf(const char *, ...);
#endif 
void __cdecl setbuf(FILE *, char *);
int __cdecl setvbuf(FILE *, char *, int, size_t);
int __cdecl _snprintf(char *, size_t, const char *, ...);
int __cdecl sprintf(char *, const char *, ...);
#ifndef _WINDLL
int __cdecl sscanf(const char *, const char *, ...);
#endif 
char * __cdecl _tempnam(char *, char *);
FILE * __cdecl tmpfile(void);
char * __cdecl tmpnam(char *);
int __cdecl ungetc(int, FILE *);
int __cdecl _unlink(const char *);
int __cdecl vfprintf(FILE *, const char *, va_list);
#ifndef _WINDLL
int __cdecl vprintf(const char *, va_list);
#endif 
int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
int __cdecl vsprintf(char *, const char *, va_list);
#define _STDIO_DEFINED
#endif 

/* macro definitions */

#define feof(_stream)     ((_stream)->_flag & _IOEOF)
#define ferror(_stream)   ((_stream)->_flag & _IOERR)
#define _fileno(_stream)  ((int)(unsigned char)(_stream)->_file)
#define getc(_stream)     (--(_stream)->_cnt >= 0 ? 0xff & *(_stream)->_ptr++ \
    : _filbuf(_stream))
#define putc(_c,_stream)  (--(_stream)->_cnt >= 0 \
    ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))
#ifndef _WINDLL
#define getchar()     getc(stdin)
#define putchar(_c)   putc((_c),stdout)
#endif 

#ifdef _MT
#undef  getc
#undef  putc
#undef  getchar
#undef  putchar
#endif 

#ifndef __STDC__
/* Non-ANSI names for compatibility */

#define P_tmpdir  _P_tmpdir
#define SYS_OPEN  _SYS_OPEN

#ifndef _WINDOWS
#define stdaux    _stdaux
#define stdprn    _stdprn
#endif 

int __cdecl fcloseall(void);
FILE * __cdecl fdopen(int, const char *);
#ifndef _WINDLL
int __cdecl fgetchar(void);
#endif 
int __cdecl fileno(FILE *);
int __cdecl flushall(void);
#ifndef _WINDLL
int __cdecl fputchar(int);
#endif 
int __cdecl getw(FILE *);
int __cdecl putw(int, FILE *);
int __cdecl rmtmp(void);
char * __cdecl tempnam(char *, char *);
int __cdecl unlink(const char *);

#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_STDIO
#endif 
