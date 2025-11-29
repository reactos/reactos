/*
 * System I/O definitions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_IO_H
#define __WINE_IO_H

#include <corecrt.h>
#include <corecrt_io.h>

static inline int access(const char* path, int mode) { return _access(path, mode); }
static inline int chmod(const char* path, int mode) { return _chmod(path, mode); }
static inline int chsize(int fd, __msvcrt_long size) { return _chsize(fd, size); }
static inline int close(int fd) { return _close(fd); }
static inline int creat(const char* path, int mode) { return _creat(path, mode); }
static inline int dup(int od) { return _dup(od); }
static inline int dup2(int od, int nd) { return _dup2(od, nd); }
static inline int eof(int fd) { return _eof(fd); }
static inline __msvcrt_long filelength(int fd) { return _filelength(fd); }
static inline int isatty(int fd) { return _isatty(fd); }
static inline int locking(int fd, int mode, __msvcrt_long size) { return _locking(fd, mode, size); }
static inline __msvcrt_long lseek(int fd, __msvcrt_long off, int where) { return _lseek(fd, off, where); }
static inline char* mktemp(char* pat) { return _mktemp(pat); }
static inline int read(int fd, void* buf, unsigned int size) { return _read(fd, buf, size); }
static inline int setmode(int fd, int mode) { return _setmode(fd, mode); }
static inline __msvcrt_long tell(int fd) { return _tell(fd); }
#ifndef _UMASK_DEFINED
static inline int umask(int fd) { return _umask(fd); }
#define _UMASK_DEFINED
#endif
#ifndef _UNLINK_DEFINED
static inline int unlink(const char* path) { return _unlink(path); }
#define _UNLINK_DEFINED
#endif
static inline int write(int fd, const void* buf, unsigned int size) { return _write(fd, buf, size); }

#if defined(__GNUC__) && (__GNUC__ < 4)
_ACRTIMP int __cdecl open(const char*,int,...) __attribute__((alias("_open")));
_ACRTIMP int __cdecl sopen(const char*,int,int,...) __attribute__((alias("_sopen")));
#else
#define open _open
#define sopen _sopen
#endif /* __GNUC__ */

#endif /* __WINE_IO_H */
