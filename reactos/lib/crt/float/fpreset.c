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

/*
 * @unimplemented
 */
void _fpreset(void)
{
  /* FIXME: This causes an exception */
//	__asm__ __volatile__("fninit\n\t");
  return;
}
