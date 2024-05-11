//
// fcntl.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// File control options used by _open().
//
#pragma once
#ifndef _INC_FCNTL // include guard for 3rd party interop
#define _INC_FCNTL

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#define _O_RDONLY      0x0000  // open for reading only
#define _O_WRONLY      0x0001  // open for writing only
#define _O_RDWR        0x0002  // open for reading and writing
#define _O_APPEND      0x0008  // writes done at eof

#define _O_CREAT       0x0100  // create and open file
#define _O_TRUNC       0x0200  // open and truncate
#define _O_EXCL        0x0400  // open only if file doesn't already exist

// O_TEXT files have <cr><lf> sequences translated to <lf> on read()'s and <lf>
// sequences translated to <cr><lf> on write()'s

#define _O_TEXT        0x4000  // file mode is text (translated)
#define _O_BINARY      0x8000  // file mode is binary (untranslated)
#define _O_WTEXT       0x10000 // file mode is UTF16 (translated)
#define _O_U16TEXT     0x20000 // file mode is UTF16 no BOM (translated)
#define _O_U8TEXT      0x40000 // file mode is UTF8  no BOM (translated)

// macro to translate the C 2.0 name used to force binary mode for files
#define _O_RAW _O_BINARY

#define _O_NOINHERIT   0x0080  // child process doesn't inherit file
#define _O_TEMPORARY   0x0040  // temporary file bit (file is deleted when last handle is closed)
#define _O_SHORT_LIVED 0x1000  // temporary storage file, try not to flush
#define _O_OBTAIN_DIR  0x2000  // get information about a directory
#define _O_SEQUENTIAL  0x0020  // file access is primarily sequential
#define _O_RANDOM      0x0010  // file access is primarily random



#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
    #define O_RDONLY     _O_RDONLY
    #define O_WRONLY     _O_WRONLY
    #define O_RDWR       _O_RDWR
    #define O_APPEND     _O_APPEND
    #define O_CREAT      _O_CREAT
    #define O_TRUNC      _O_TRUNC
    #define O_EXCL       _O_EXCL
    #define O_TEXT       _O_TEXT
    #define O_BINARY     _O_BINARY
    #define O_RAW        _O_BINARY
    #define O_TEMPORARY  _O_TEMPORARY
    #define O_NOINHERIT  _O_NOINHERIT
    #define O_SEQUENTIAL _O_SEQUENTIAL
    #define O_RANDOM     _O_RANDOM
#endif

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS

#endif // _INC_FCNTL
