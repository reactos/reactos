#include <float.h>

void    _fpreset (void)
{
	__asm__ __volatile__("fninit\n\t");
	return;
}