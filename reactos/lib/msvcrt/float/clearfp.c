#include <msvcrt/float.h>

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

