/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include "precomp.h"

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
