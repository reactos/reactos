/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbstrlen.c
 * PURPOSE:      Determines the length of a multi byte string, current locale
 * PROGRAMERS:
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <mbstring.h>
#include <stdlib.h>

#ifdef _LIBCNT_
unsigned short *NlsLeadByteInfo;
#define isleadbyte(c) NlsLeadByteInfo[c]
#else
int isleadbyte(int byte);
#endif

/*
 * @implemented
 */
size_t __cdecl _mbstrlen( const char *str )
{
  size_t len = 0;
  while(*str)
  {
    /* FIXME: According to the documentation we are supposed to test for
     * multi-byte character validity. Whatever that means
     */
    str += isleadbyte((unsigned char)*str) ? 2 : 1;
    len++;
  }
  return len;
}
