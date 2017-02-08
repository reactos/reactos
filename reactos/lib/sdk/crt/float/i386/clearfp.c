/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/float/i386/clearfp.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

unsigned int _statusfp( void );

/*********************************************************************
 *		_clearfp (MSVCRT.@)
 */
unsigned int CDECL _clearfp(void)
{
  unsigned int retVal = _statusfp();
#if defined(__GNUC__)
  __asm__ __volatile__( "fnclex" );
#else
  __asm fnclex;
#endif
  return retVal;
}

