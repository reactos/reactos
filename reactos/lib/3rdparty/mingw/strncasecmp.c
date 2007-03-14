/*
 * strncasecmp.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Oldnames from ANSI header string.h
 *
 * Some wrapper functions for those old name functions whose appropriate
 * equivalents are not simply underscore prefixed.
 *
 */

#include <string.h>

int
strncasecmp (const char *sz1, const char *sz2, size_t sizeMaxCompare)
{
  return _strnicmp (sz1, sz2, sizeMaxCompare);
}

