/***
*fcntl.h - file control options used by open()
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines constants for the file control options used
*   by the open() function.
*   [System V]
*
****/

#ifndef _INC_FCNTL

#define _O_RDONLY   0x0000  /* open for reading only */
#define _O_WRONLY   0x0001  /* open for writing only */
#define _O_RDWR     0x0002  /* open for reading and writing */
#define _O_APPEND   0x0008  /* writes done at eof */

#define _O_CREAT    0x0100  /* create and open file */
#define _O_TRUNC    0x0200  /* open and truncate */
#define _O_EXCL     0x0400  /* open only if file doesn't already exist */

/* _O_TEXT files have <cr><lf> sequences translated to <lf> on read()'s,
** and <lf> sequences translated to <cr><lf> on write()'s
*/

#define _O_TEXT     0x4000  /* file mode is text (translated) */
#define _O_BINARY   0x8000  /* file mode is binary (untranslated) */

/* macro to translate the C 2.0 name used to force binary mode for files */

#define _O_RAW  _O_BINARY

/* Open handle inherit bit */

#define _O_NOINHERIT    0x0080  /* child process doesn't inherit file */

#ifndef __STDC__
/* Non-ANSI names for compatibility */
#define O_RDONLY    _O_RDONLY
#define O_WRONLY    _O_WRONLY
#define O_RDWR      _O_RDWR
#define O_APPEND    _O_APPEND
#define O_CREAT     _O_CREAT
#define O_TRUNC     _O_TRUNC
#define O_EXCL      _O_EXCL
#define O_TEXT      _O_TEXT
#define O_BINARY    _O_BINARY
#define O_NOINHERIT _O_NOINHERIT
#endif 

#define _INC_FCNTL
#endif 
