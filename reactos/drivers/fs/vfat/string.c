/* $Id: string.c,v 1.12 2003/10/11 17:51:56 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/string.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

const WCHAR *long_illegals = L"\"*\\<>/?:|";

BOOLEAN 
vfatIsLongIllegal(WCHAR c)
{
  return wcschr(long_illegals, c) ? TRUE : FALSE;
}

BOOLEAN wstrcmpjoki(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare two wide character strings, s2 with jokers (* or ?)
 * return TRUE if s1 like s2
 */
{
   while ((*s2==L'*')||(*s2==L'?')||(RtlUpcaseUnicodeChar(*s1)==RtlUpcaseUnicodeChar(*s2)))
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
   if ((*s2)==L'.')
   {
   	for (;((*s2)==L'.')||((*s2)==L'*')||((*s2)==L'?');s2++) {}
   }
   if ((*s1)==0 && (*s2)==0)
        return(TRUE);
   return(FALSE);
}





