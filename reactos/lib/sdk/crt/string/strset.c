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
 */
char* CDECL _strnset(char* str, int value, size_t len)
{
  if (len > 0 && str)
    while (*str && len--)
      *str++ = value;
  return str;
}

/*
 * @implemented
 */
char* CDECL _strset(char* str, int value)
{
  char *ptr = str;
  while (*ptr)
    *ptr++ = value;

  return str;
}
