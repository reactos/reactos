/* $Id: string.c,v 1.3 2000/06/29 23:35:51 dwelch Exp $
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
#include <ddk/cctypes.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

void RtlAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
/*
 * FUNCTION: Convert an ANSI string to it's Unicode equivalent
 */
{
   int i;
   
   for (i=0; (i<Length && Source[i] != ' '); i++)
     {
	Dest[i] = Source[i];
     }
   Dest[i]=0;
}

void RtlCatAnsiToUnicode(PWSTR Dest, PCH Source, ULONG Length)
/*
 * FUNCTION: Appends a converted ANSI to Unicode string to the end of an
 *           existing Unicode string
 */
{
   ULONG i;
   
   while((*Dest)!=0)
     {
	Dest++;
     }
   for (i=0; (i<Length && Source[i] != ' '); i++)
     {
	Dest[i] = Source[i];
     }
   Dest[i]=0;
}

void vfat_initstr(wchar_t *wstr, ULONG wsize)
/*
 * FUNCTION: Initialize a string for use with a long file name
 */
{
  int i;
  wchar_t nc=0;
  for(i=0; i<wsize; i++)
  {
    *wstr=nc;
    wstr++;
  }
  wstr=wstr-wsize;
}

wchar_t * vfat_wcsncat(wchar_t * dest, const wchar_t * src,size_t wstart, size_t wcount)
/*
 * FUNCTION: Append a string for use with a long file name
 */
{
   int i;

   dest+=wstart;
   for(i=0; i<wcount; i++)
   {
     *dest=src[i];
     dest++;
   }
   dest=dest-(wcount+wstart);

   return dest;
}

wchar_t * vfat_wcsncpy(wchar_t * dest, const wchar_t *src,size_t wcount)
/*
 * FUNCTION: Copy a string for use with long file names
 */
{
 int i;
   
   for (i=0;i<wcount;i++)
   {
     dest[i]=src[i];
     if(!dest[i]) break;
   }
   return(dest);
}

wchar_t * vfat_movstr(wchar_t *src, ULONG dpos,
                      ULONG spos, ULONG len)
/*
 * FUNCTION: Move the characters in a string to a new position in the same
 *           string
 */
{
 int i;

  if(dpos<=spos)
  {
    for(i=0; i<len; i++)
    {
      src[dpos++]=src[spos++];
    }
  }
  else
  {
    dpos+=len-1;
    spos+=len-1;
    for(i=0; i<len; i++)
    {
      src[dpos--]=src[spos--];
    }
  }

  return(src);
}

BOOLEAN wstrcmpi(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare to wide character strings
 * return TRUE if s1==s2
 */
{
   while (towlower(*s1)==towlower(*s2))
     {
	if ((*s1)==0 && (*s2)==0)
	  {
	     return(TRUE);
	  }
	
	s1++;
	s2++;	
     }
   return(FALSE);
}

BOOLEAN wstrcmpjoki(PWSTR s1, PWSTR s2)
/*
 * FUNCTION: Compare to wide character strings, s2 with jokers (* or ?)
 * return TRUE if s1 like s2
 */
{
   while ((*s2=='?')||(towlower(*s1)==towlower(*s2)))
   {
      if ((*s1)==0 && (*s2)==0)
        return(TRUE);
      s1++;
      s2++;	
   }
   if(*s2=='*')
   {
     s2++;
     while (*s1)
       if (wstrcmpjoki(s1,s2)) return TRUE;
       else s1++;
   }
   if ((*s1)==0 && (*s2)==0)
        return(TRUE);
   return(FALSE);
}

