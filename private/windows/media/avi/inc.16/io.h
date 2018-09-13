/***
*io.h - declarations for low-level file handling and I/O functions
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file contains the function declarations for the low-level
*   file handling and I/O functions.
*
****/

#ifndef _INC_IO

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

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

int __cdecl _access(const char *, int);
int __cdecl _chmod(const char *, int);
int __cdecl _chsize(int, long);
int __cdecl _close(int);
int __cdecl _commit(int);
int __cdecl _creat(const char *, int);
int __cdecl _dup(int);
int __cdecl _dup2(int, int);
int __cdecl _eof(int);
long __cdecl _filelength(int);
int __cdecl _isatty(int);
int __cdecl _locking(int, int, long);
long __cdecl _lseek(int, long, int);
char * __cdecl _mktemp(char *);
int __cdecl _open(const char *, int, ...);
int __cdecl _read(int, void *, unsigned int);
int __cdecl remove(const char *);
int __cdecl rename(const char *, const char *);
int __cdecl _setmode(int, int);
int __cdecl _sopen(const char *, int, int, ...);
long __cdecl _tell(int);
int __cdecl _umask(int);
int __cdecl _unlink(const char *);
int __cdecl _write(int, const void *, unsigned int);
#ifdef _WINDOWS
#ifndef _WINDLL
int __cdecl _wabout(char *);
int __cdecl _wclose(int, int);
int __cdecl _wgetexit(void);
int __cdecl _wgetfocus(void);
long __cdecl _wgetscreenbuf(int);
int __cdecl _wgetsize(int, int, struct _wsizeinfo *);
int __cdecl _wmenuclick(int);
int __cdecl _wopen(struct _wopeninfo *, struct _wsizeinfo *, int);
int __cdecl _wsetexit(int);
int __cdecl _wsetfocus(int);
int __cdecl _wsetscreenbuf(int, long);
int __cdecl _wsetsize(int, struct _wsizeinfo *);
void __cdecl _wyield(void);
#endif 
#endif 

#ifndef __STDC__
/* Non-ANSI names for compatibility */
int __cdecl access(const char *, int);
int __cdecl chmod(const char *, int);
int __cdecl chsize(int, long);
int __cdecl close(int);
int __cdecl creat(const char *, int);
int __cdecl dup(int);
int __cdecl dup2(int, int);
int __cdecl eof(int);
long __cdecl filelength(int);
int __cdecl isatty(int);
int __cdecl locking(int, int, long);
long __cdecl lseek(int, long, int);
char * __cdecl mktemp(char *);
int __cdecl open(const char *, int, ...);
int __cdecl read(int, void *, unsigned int);
int __cdecl setmode(int, int);
int __cdecl sopen(const char *, int, int, ...);
long __cdecl tell(int);
int __cdecl umask(int);
int __cdecl unlink(const char *);
int __cdecl write(int, const void *, unsigned int);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_IO
#endif 
