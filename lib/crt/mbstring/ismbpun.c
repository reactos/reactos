/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

#include <mbstring.h>
#include <mbctype.h>
#include <ctype.h>


/*
 * @implemented
 */
int _ismbbpunct(unsigned int c)
{
// (0xA1 <= c <= 0xA6)
  return (ispunct(c) || _ismbbkana(c));
}

 //iskana()     :(0xA1 <= c <= 0xDF)
 //iskpun()     :(0xA1 <= c <= 0xA6)
 //iskmoji()    :(0xA7 <= c <= 0xDF)
