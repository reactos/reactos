/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#define __CRT__NO_INLINE
#include <string.h>

#undef strncasecmp
int strncasecmp (const char *, const char *, size_t);

int
strncasecmp (const char *sz1,const char *sz2,size_t sizeMaxCompare)
{
  return _strnicmp (sz1,sz2,sizeMaxCompare);
}
