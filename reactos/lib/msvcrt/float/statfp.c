#include <msvcrti.h>


unsigned int	_statusfp (void)
{	

register unsigned short __res;

__asm__ __volatile__ (
	"fstsw	%0 \n\t"
//	"movzwl %ax, %eax"
	:"=a" (__res)
	);
	return __res;
}
