#include <stdlib.h>

#if defined(__GNUC__)
static unsigned long long next = 0;
#else
static unsigned __int64 next = 0;
#endif

/*
 * @implemented
 */
int __cdecl rand(void)
{
#if defined(__GNUC__)
	next = next * 0x5deece66dLL + 11;
#else
	next = next * 0x5deece66di64 + 11;
#endif
	return (int)((next >> 16) & RAND_MAX);
}


/*
 * @implemented
 */
void __cdecl srand(unsigned seed)
{
	next = seed;
}
