#include <msvcrt/float.h>

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
