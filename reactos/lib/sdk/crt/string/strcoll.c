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

/* Compare S1 and S2, returning less than, equal to or
   greater than zero if the collated form of S1 is lexicographically
   less than, equal to or greater than the collated form of S2.  */


/*
 * @unimplemented
 */
int strcoll(const char* s1, const char* s2)
{
    return strcmp(s1, s2);
}

/*
 * @implemented
 */
int _stricoll(const char* s1, const char* s2)
{
  /* FIXME: handle collates */
  TRACE("str1 %s str2 %s\n", debugstr_a(s1), debugstr_a(s2));
  return lstrcmpiA( s1, s2 );
}

