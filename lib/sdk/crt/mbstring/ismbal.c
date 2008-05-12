/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismbal.c
 * PURPOSE:     Checks for alphabetic multibyte character
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <mbctype.h>
#include <ctype.h>

/*
 * @implemented
 */
int _ismbbalpha(unsigned int c)
{
  return (isalpha(c) || _ismbbkalnum(c));
}

