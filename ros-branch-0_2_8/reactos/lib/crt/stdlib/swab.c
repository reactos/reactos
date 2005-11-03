#include <stdlib.h>

/*
 * @implemented
 */
void _swab (const char* caFrom, char* caTo, size_t sizeToCopy)
{
  if (sizeToCopy > 1)
  {
    sizeToCopy = sizeToCopy >> 1;

    while (sizeToCopy--) {
      *caTo++ = caFrom[1];
      *caTo++ = *caFrom++;
      caFrom++;
    }
  }
}
