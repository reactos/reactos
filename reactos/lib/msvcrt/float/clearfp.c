#include <msvcrti.h>


unsigned int _clearfp (void)
{
  unsigned short __res = _statusfp();

__asm__ __volatile__ (
	"fclex \n\t"
	);

  return __res;
}

