/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/hanzen.c
 * PURPOSE:     Checks for kana character
 * PROGRAMER:   Boudewijn Dekker, Taiji Yamada
 * UPDATE HISTORY:
		Modified from Taiji Yamada japanese code system utilities
 *              12/04/99: Created
 */
#include <msvcrt/mbstring.h>
#include <msvcrt/mbctype.h>

int _ismbbkana(unsigned char c)
{
  return ((_jctype+1)[(unsigned char)(c)] & (_KNJ_M|_KNJ_P));
}
