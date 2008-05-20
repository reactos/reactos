/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismbkaln.c
 * PURPOSE:
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <mbctype.h>
#include <ctype.h>
#include <internal/mbstring.h>
/*
 * @implemented
 */
int _ismbbkalnum( unsigned int c )
{
  return ((_mbctype+1)[(unsigned char)(c)] & (_KNJ_P));
}
