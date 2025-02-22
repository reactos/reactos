/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/wstring/wcscoll.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @unimplemented
 */
int CDECL _wcsncoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return wcsncmp(s1,s2,c);
}

/*
 * @unimplemented
 */
int CDECL _wcsnicoll (const wchar_t *s1, const wchar_t *s2, size_t c)
{
  /* FIXME: handle collates */
  return _wcsnicmp(s1,s2,c);
}
