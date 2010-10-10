/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismbtrl.c
 * PURPOSE:     Checks for a trailing byte
 * PROGRAMERS:
 *              Copyright 1999 Ariadne
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <precomp.h>
#include <mbctype.h>

size_t _mbclen2(const unsigned int s);

//  iskanji2()   : (0x40 <= c <= 0x7E 0x80  <=  c <= 0xFC)

/*
 * @implemented
 */
int _ismbbtrail(unsigned int c)
{
  return (_mbctype[(c&0xff) + 1] & _M2) != 0;
}


/*
 * @implemented
 */
int _ismbstrail( const unsigned char *start, const unsigned char *str)
{
  /* Note: this function doesn't check _ismbbtrail */
  if ((str > start) && _ismbslead(start, str-1))
    return -1;
  else
    return 0;
}
