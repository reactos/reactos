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
 *
 * copy this swab from wine cvs 2006-05-24
 */
void _swab (char *  src,  char *  dst,  int  sizeToCopy)

{
 if (sizeToCopy > 1)
  {
    sizeToCopy = (unsigned)sizeToCopy >> 1;

    while (sizeToCopy--) {
      char s0 = src[0];
      char s1 = src[1];
      *dst++ = s1;
      *dst++ = s0;
      src = src + 2;
    }
  }
}
