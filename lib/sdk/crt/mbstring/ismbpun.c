/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismbpun.c
 * PURPOSE:
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */

#include <precomp.h>


/*
 * @implemented
 */
int _ismbbpunct(unsigned int c)
{
// (0xA1 <= c <= 0xA6)
    return (_mbctype[c & 0xff] & _MBPUNCT);
}

 //iskana()     :(0xA1 <= c <= 0xDF)
 //iskpun()     :(0xA1 <= c <= 0xA6)
 //iskmoji()    :(0xA7 <= c <= 0xDF)
