/* $Id: strdup.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/string/strdup.c
 * PURPOSE:     Duplicate a string
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              21/01/2002: Created
 */

#include <string.h>
#include <stdlib.h>
#include <psx/debug.h>

char *strdup(const char *s1)
{
 char *pchRetBuf;
 int   nStrLen;

 HINT("strdup() is inefficient - consider dropping zero-terminated strings");

 if (s1 == 0)
  return 0;

 nStrLen = strlen(s1);

 /* allocate enough buffer space for s1 and the null terminator */
 pchRetBuf = (char *) malloc(nStrLen + 1);

 if (pchRetBuf == 0)
  /* memory allocation failed */
  return 0;

 /* copy the string */
 strcpy(pchRetBuf, s1);

 return pchRetBuf;

}

/* EOF */

