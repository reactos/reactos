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
 * @implemented
 */
unsigned int	_statusfp (void)
{

register unsigned short __res;
#ifdef __GNUC__
__asm__ __volatile__ (
	"fstsw	%0 \n\t"
//	"movzwl %ax, %eax"
	:"=a" (__res)
	);
#else
#endif /*__GNUC__*/
	return __res;
}
