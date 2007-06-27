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

unsigned int _statusfp( void );

/*
 * @implemented
 */
unsigned int _clearfp (void)
{
  unsigned short __res = _statusfp();
#ifdef __GNUC__
__asm__ __volatile__ (
	"fclex \n\t"
	);
#else
#endif /*__GNUC__*/
  return __res;
}

