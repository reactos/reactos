/* $Id: string.c,v 1.10 2003/07/24 19:00:42 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/string.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN wstrcmpjoki(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare two wide character strings, s2 with jokers (* or ?)
 * return TRUE if s1 like s2
 */
{
   while ((*s2=='*')||(*s2=='?')||(towlower(*s1)==towlower(*s2)))
   {
      if ((*s1)==0 && (*s2)==0)
        return(TRUE);
      if(*s2=='*')
      {
	s2++;
        while (*s1)
        if (wstrcmpjoki(s1,s2)) return TRUE;
         else s1++;
      }
      else
      {
        s1++;
        s2++;
      }
   }
   if ((*s2)=='.')
   {
   	for (;((*s2)=='.')||((*s2)=='*')||((*s2)=='?');s2++) {}
   }
   if ((*s1)==0 && (*s2)==0)
        return(TRUE);
   return(FALSE);
}

PWCHAR  
vfatGetNextPathElement (PWCHAR  pFileName)
{
  if (*pFileName == L'\0')
  {
    return  0;
  }

  while (*pFileName != L'\0' && *pFileName != L'\\')
  {
    pFileName++;
  }

  return  pFileName;
}

void
vfatWSubString (PWCHAR pTarget, const PWCHAR pSource, size_t pLength)
{
  wcsncpy (pTarget, pSource, pLength);
  pTarget [pLength] = L'\0';
}

BOOL  
vfatIsFileNameValid (PWCHAR pFileName)
{
  PWCHAR  c;

  c = pFileName;
  while (*c != 0)
  {
    if (*c == L'*' || *c == L'?')
    {
      return FALSE;
    }
    c++;
  }

  return  TRUE;
}


