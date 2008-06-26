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

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void CDECL _fpreset(void)
{
#if defined(__GNUC__) && defined(__i386__)
  __asm__ __volatile__( "fninit" );
#else
  FIXME(":Not Implemented!\n");
#endif
}
