/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_OSCALLS
#define _INC_OSCALLS

#ifndef _CRTBLD
#error ERROR: Use of C runtime library internal header file.
#endif

#include <crtdefs.h>

#ifdef NULL
#undef NULL
#endif

#define NOMINMAX

#define _WIN32_FUSION 0x0100
#include <windows.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

typedef struct _FTIME
{
  unsigned short twosecs : 5;
  unsigned short minutes : 6;
  unsigned short hours : 5;
} FTIME;

typedef FTIME *PFTIME;

typedef struct _FDATE
{
  unsigned short day : 5;
  unsigned short month : 4;
  unsigned short year : 7;
} FDATE;

typedef FDATE *PFDATE;

#endif
