/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbsrchr.c 
 * PURPOSE:     Searches for a character in reverse
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

unsigned char * _mbsrchr(const unsigned char *src, unsigned int val)
{

  char  *s = src;
  short cc = val;
  const char *sp=(char *)0;

  
  while (*s)
  {	
    	if (*(short *)s == cc)
      		sp = s;
    	s+= _mbclen2(*s);
  }
  if (cc == 0)
    	sp = s;
  return (char *)sp;
}