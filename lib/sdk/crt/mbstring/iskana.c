/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/iskana.c
 * PURPOSE:     Checks for kana character
 * PROGRAMER:
 * UPDATE HISTORY:
 *              12/04/99:  Ariadne, Taiji Yamada Created
 *              05/30/08: Samuel Serapion adapted  from PROJECT C Library
 *
 */

#include <precomp.h>

/*
 * @implemented
 */
int _ismbbkana(unsigned int c)
{
  return (_mbctype[c & 0xff] & _MBKANA);
}
