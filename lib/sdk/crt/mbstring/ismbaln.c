/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismbaln.c
 * PURPOSE:
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */


#include <precomp.h>

int _ismbbkalnum( unsigned int c );

/*
 * @implemented
 */
int _ismbbalnum(unsigned int c)
{
  return (isalnum(c) || _ismbbkalnum(c));
}


