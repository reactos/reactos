/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   1998/12/04  RJJ  Cleaned up and added i386 def checks
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

wchar_t * ___wcstok = NULL;

/* FUNCTIONS *****************************************************************/

wchar_t* wcsdup(wchar_t* src)
{
   wchar_t* dest;
   
   dest = ExAllocatePool(NonPagedPool, (wcslen(src)+1)*2);
   wcscpy(dest,src);
   return(dest);
}

wchar_t * wcscat(wchar_t *dest, const wchar_t *src)
{
  int i, j;
   
  for (j = 0; dest[j] != 0; j++)
    ;
  for (i = 0; src[i] != 0; i++)
    {
      dest[j + i] = src[i];
    }
  dest[j + i] = 0;

  return dest;
}

wchar_t * 
wcschr(const wchar_t *str, wchar_t ch)
{
  while ((*str) != ((wchar_t) 0))
    {
      if ((*str) == ch)
        {
          return str;
        }
      str++;
    }

  return NULL;
}

#if 0
wchar_t towupper(wchar_t w)
{
   if (w < L'A')
     {
	return(w + 'A');
     }
   else     
     {
	return(w);
     }
}
#endif

int wcsicmp(const wchar_t* cs, const wchar_t* ct)
{
   while (*cs != '\0' && *ct != '\0' && wtoupper(*cs) == wtoupper(*ct))
     {
	cs++;
	ct++;
     }
   return *cs - *ct;
}

int wcscmp(const wchar_t *cs, const wchar_t *ct)
{
  while (*cs != '\0' && *ct != '\0' && *cs == *ct)
    {
      cs++;
      ct++;
    }
  return *cs - *ct;
}

wchar_t* wcscpy(wchar_t* str1, const wchar_t* str2)
{
   wchar_t* s = str1;
   DPRINT("wcscpy(str1 %w, str2 %w)\n",str1,str2);
   while ((*str2)!=0)
     {
	*s = *str2;
	s++;
	str2++;
     }
   *s = 0;
   return(str1);
}

size_t wcscspn(const wchar_t *cs, const wchar_t *ct)
{
   UNIMPLEMENTED;
}

size_t wcslen(const wchar_t *s)
{
  unsigned int len = 0;

  while (s[len] != 0) 
    {
      len++;
    }

  return len;
}

wchar_t * 
wcsncat(wchar_t *dest, const wchar_t *src, size_t count)
{
  int i, j;
   
  for (j = 0; dest[j] != 0; j++)
    ;
  for (i = 0; i < count; i++)
    {
      dest[j + i] = src[i];
      if (src[i] == 0)
        {
          return dest;
        }
    }
  dest[j + i] = 0;

  return dest;
}

int 
wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count)
{
UNIMPLEMENTED;
}


wchar_t * 
wcsncpy(wchar_t *dest, const wchar_t *src, size_t count)
{
  int i;
   
  for (i = 0; i < count; i++)
    {
      dest[i] = src[i];
      if (src[i] == 0)
        {
          return dest;
        }
    }
  dest[i] = 0;

  return dest;
}


int wcsnicmp(const wchar_t *cs, const wchar_t *ct, size_t count)
{
   UNIMPLEMENTED;
}


wchar_t * 
wcsrchr(const wchar_t *str, wchar_t ch)
{
  unsigned int len = 0;
  while (str[len] != ((wchar_t)0))
    {
      len++;
    }
   
  for (; len > 0; len--)
    {
      if (str[len-1]==ch)
        {
          return (wchar_t *) &str[len - 1];
        }
    }

  return NULL;
}

size_t 
wcsspn(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

wchar_t * wcsstr(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

wchar_t * wcstok(wchar_t * s,const wchar_t * ct)
{
UNIMPLEMENTED;
}


