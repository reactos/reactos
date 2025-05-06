/*
 * File definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_FCNTL_H
#define __WINE_FCNTL_H

#include <corecrt.h>

#define _O_RDONLY      0
#define _O_WRONLY      1
#define _O_RDWR        2
#define _O_ACCMODE     (_O_RDONLY|_O_WRONLY|_O_RDWR)
#define _O_APPEND      0x0008
#define _O_RANDOM      0x0010
#define _O_SEQUENTIAL  0x0020
#define _O_TEMPORARY   0x0040
#define _O_NOINHERIT   0x0080
#define _O_CREAT       0x0100
#define _O_TRUNC       0x0200
#define _O_EXCL        0x0400
#define _O_SHORT_LIVED 0x1000
#define _O_TEXT        0x4000
#define _O_BINARY      0x8000
#define _O_RAW         _O_BINARY

#define _O_WTEXT       0x10000
#define _O_U16TEXT     0x20000
#define _O_U8TEXT      0x40000

#define O_RDONLY    _O_RDONLY
#define O_WRONLY    _O_WRONLY
#define O_RDWR      _O_RDWR
#define O_ACCMODE   _O_ACCMODE
#define O_APPEND    _O_APPEND
#define O_RANDOM    _O_RANDOM
#define O_SEQUENTIAL _O_SEQUENTIAL
#define O_TEMPORARY _O_TEMPORARY
#define O_NOINHERIT _O_NOINHERIT
#define O_CREAT     _O_CREAT
#define O_TRUNC     _O_TRUNC
#define O_EXCL      _O_EXCL
#define O_TEXT      _O_TEXT
#define O_BINARY    _O_BINARY
#define O_RAW       _O_BINARY

#endif /* __WINE_FCNTL_H */
