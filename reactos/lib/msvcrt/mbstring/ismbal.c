/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismbal.c
 * PURPOSE:     Checks for alphabetic multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

int _ismbbalpha(unsigned char c)
{
  return (isalpha(c) || _ismbbkalnum(c));
}

