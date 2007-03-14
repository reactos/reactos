/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/iskana.c
 * PURPOSE:     Checks for kana character
 * PROGRAMER:   Ariadne, Taiji Yamada
 * UPDATE HISTORY:
		Modified from Taiji Yamada japanese code system utilities
 *              12/04/99: Created
 */
#include <mbstring.h>
#include <mbctype.h>
#include <internal/mbstring.h>

/*
 * @implemented
 */
int _ismbbkana(unsigned int c)
{
  return ((_mbctype+1)[(unsigned char)(c)] & (_KNJ_M|_KNJ_P));
}
