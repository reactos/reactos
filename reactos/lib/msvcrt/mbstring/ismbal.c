/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/hanzen.c
 * PURPOSE:     Checks for alphabetic multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <msvcrti.h>


int _ismbbalpha(unsigned int c)
{
  return (isalpha(c) || _ismbbkalnum(c));
}

