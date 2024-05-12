/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
/*
 * This file is part of the Mingw32 package.
 *
 * unistd.h maps (roughly) to io.h
 */

#ifndef __STRICT_ANSI__

#include <io.h>
#include <process.h>

#define __UNISTD_GETOPT__
#include <getopt.h>
#undef __UNISTD_GETOPT__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FTRUNCATE_DEFINED
#define FTRUNCATE_DEFINED
/* This is defined as a real library function to allow autoconf
   to verify its existence. */
int ftruncate(int, _off_t);
__CRT_INLINE int ftruncate(int __fd, _off_t __length)
{
  return _chsize (__fd, __length);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
