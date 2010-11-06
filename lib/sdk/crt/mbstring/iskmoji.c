/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/iskmoji.c
 * PURPOSE:
 * PROGRAMER:
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted  from PROJECT C Library
 *
 */

#include <mbctype.h>

int _ismbbkalpha(unsigned char c)
{
 return (0xA7 <= c && c <= 0xDF);
}
