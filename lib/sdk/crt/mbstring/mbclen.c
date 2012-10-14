/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbclen.c
 * PURPOSE:      Determines the length of a multi byte character
 * PROGRAMERS:
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <mbstring.h>
#include <stdlib.h>

int isleadbyte(int byte);

/*
 * @implemented
 */
size_t _mbclen(const unsigned char *s)
{
  return _ismbblead(*s) ? 2 : 1;
}

size_t _mbclen2(const unsigned int s)
{
  return (_ismbblead(s>>8) && _ismbbtrail(s&0x00FF)) ? 2 : 1;
}

/*
 * assume MB_CUR_MAX == 2
 *
 * @implemented
 */
int mblen( const char *str, size_t size )
{
  if (str && *str && size)
  {
    return !isleadbyte(*str) ? 1 : (size>1 ? 2 : -1);
  }
  return 0;
}
