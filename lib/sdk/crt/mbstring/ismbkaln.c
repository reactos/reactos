/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismbkaln.c
 * PURPOSE:
 * PROGRAMER:
 * UPDATE HISTORY:
 *              12/04/99: Ariadne Created
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>
/*
 * @implemented
 */
int _ismbbkalnum( unsigned int c )
{
  return (_mbctype[c & 0xff] & _MKMOJI);
}
