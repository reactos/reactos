/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/string/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   1998/12/04  RJJ  Cleaned up and added i386 def checks
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wstring.h>

#include <internal/debug.h>

wchar_t * ___wcstok = NULL;

/* FUNCTIONS *****************************************************************/

wchar_t * 
wcscat(wchar_t *dest, const wchar_t *src)
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

int 
wcscmp(const wchar_t *cs, const wchar_t *ct)
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
   while ((*str2)!=0)
     {
	*s = *str2;
	s++;
	str2++;
     }
   *s = 0;
   return(str1);
}

#if 0

size_t 
wcscspn(const wchar_t *cs, const wchar_t *ct)
{
  register wchar_t * __res;
  __asm__ __volatile__(
    "cld\n\t"
    "movl %4,%%edi\n\t"
    "repne\n\t"
    "scasw\n\t"
    "notl %%ecx\n\t"
    "decl %%ecx\n\t"
    "movl %%ecx,%%edx\n"
    "1:\tlodsw\n\t"
    "testw %%eax,%%eax\n\t"
    "je 2f\n\t"
    "movl %4,%%edi\n\t"
    "movl %%edx,%%ecx\n\t"
    "repne\n\t"
    "scasw\n\t"
    "jne 1b\n"
    "2:\tdecl %0"
    : "=S" (__res)
    : "a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
    : "eax","ecx","edx","edi");

  return __res-cs;
}

#else

size_t 
wcscspn(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

#endif

#ifdef i386

int 
wcsicmp(const wchar_t *cs,const wchar_t *ct)
{
  register int __res;

  __asm__ __volatile__(
    "cld\n"
    "1:\tmovw (%%esi), %%eax\n\t"
    "movw  (%%edi), %%edx \n\t"
    "cmpw $0x5A, %%eax\n\t"
    "ja 2f\t\n"
    "cmpw $0x40, %%eax\t\n"
    "jbe 2f\t\n"
    "addw $0x20, %%eax\t\n"
    "2:\t cmpw $0x5A, %%edx\t\n"
    "ja 3f\t\n"
    "cmpw $0x40, %%edx\t\n"
    "jbe 3f\t\n"
    "addw $0x20, %%edx\t\n"
    "3:\t inc %%esi\t\n"
    "inc %%esi\t\n"
    "inc %%edi\t\n"
    "inc %%edi\t\n"
    "cmpw %%eax, %%edx\t\n"
    "jne 4f\n\t"  
    "cmpw $00, %%eax\n\t"
    "jne 1b\n\t"
    "xorl %%eax,%%eax\n\t"
    "jmp 5f\n"
    "4:\tsbbl %%eax,%%eax\n\t"
    "orw $1,%%eax\n"
    "5:"
    : "=a" (__res)
    : "S" (cs),"D" (ct)
    : "esi","edi");

  return __res;
}

#else

int 
wcsicmp(const wchar_t *cs,const wchar_t *ct)
{
UNIMPLEMENTED;
}

#endif

size_t 
wcslen(const wchar_t *s)
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

#ifdef i386

int 
wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count)
{
  register int __res;

  __asm__ __volatile__(
    "cld\n"
    "1:\tdecl %3\n\t"
    "js 2f\n\t"
    "lodsw\n\t"
    "scasw\n\t"
    "jne 3f\n\t"
    "testw %%eax,%%eax\n\t"
    "jne 1b\n"
    "2:\txorl %%eax,%%eax\n\t"
    "jmp 4f\n"
    "3:\tsbbl %%eax,%%eax\n\t"
    "orw $1,%%eax\n"
    "4:"
    : "=a" (__res)
    : "S" (cs), "D" (ct), "c" (count)
    : "esi","edi","ecx");

  return __res;
}

#else

int 
wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count)
{
UNIMPLEMENTED;
}

#endif

wchar_t* wcsncpy(wchar_t *dest, const wchar_t *src, size_t count)
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

#ifdef i386

int 
wcsnicmp(const wchar_t *cs,const wchar_t *ct, size_t count)
{
  register int __res;

  __asm__ __volatile__(
    "cld\n"
    "1:\t decl %3\n\t"
    "js 6f\n\t"
    "movw (%%esi), %%eax\n\t"
    "movw  (%%edi), %%edx \n\t"
    "cmpw $0x5A, %%eax\n\t"
    "ja 2f\t\n"
    "cmpw $0x40, %%eax\t\n"
    "jbe 2f\t\n"
    "addw $0x20, %%eax\t\n"
    "2:\t cmpw $0x5A, %%edx\t\n"
    "ja 3f\t\n"
    "cmpw $0x40, %%edx\t\n"
    "jbe 3f\t\n"
    "addw $0x20, %%edx\t\n"
    "3:\t inc %%esi\t\n"
    "inc %%esi\t\n"
    "inc %%edi\t\n"
    "inc %%edi\t\n"
    "cmpw %%eax, %%edx\t\n"
    "jne 4f\n\t"  
    "cmpw $00, %%eax\n\t"
    "jne 1b\n\t"
    "6:xorl %%eax,%%eax\n\t"
    "jmp 5f\n"
    "4:\tsbbl %%eax,%%eax\n\t"
    "orw $1,%%eax\n"
    "5:"
    : "=a" (__res)
    : "S" (cs), "D" (ct), "c" (count)
    : "esi", "edi", "ecx");

  return __res;
}

#else

int 
wcsnicmp(const wchar_t *cs,const wchar_t *ct, size_t count)
{
UNIMPLEMENTED;
}

#endif

#ifdef i386

size_t 
wcsnlen(const wchar_t *s, size_t count)
{
  register int __res;
  __asm__ __volatile__(
    "movl %1,%0\n\t"
    "jmp 2f\n"
    "1:\tcmpw $0,(%0)\n\t"
    "je 3f\n\t"
    "incl %0\n"
    "2:\tdecl %2\n\t"
    "cmpl $-1,%2\n\t"
    "jne 1b\n"
    "3:\tsubl %1,%0"
    : "=a" (__res)
    : "c" (s), "d" (count)
    : "edx");

  return __res;
}

#else

size_t 
wcsnlen(const wchar_t *s, size_t count)
{
UNIMPLEMENTED;
}

#endif

#ifdef i386

wchar_t *
wcspbrk(const wchar_t *cs, const wchar_t *ct)
{
  register wchar_t * __res;
  __asm__ __volatile__(
    "cld\n\t"
    "movl %4,%%edi\n\t"
    "repne\n\t"
    "scasw\n\t"
    "notl %%ecx\n\t"
    "decl %%ecx\n\t"
    "movl %%ecx,%%edx\n"
    "1:\tlodsw\n\t"
    "testw %%eax,%%eax\n\t"
    "je 2f\n\t"
    "movl %4,%%edi\n\t"
    "movl %%edx,%%ecx\n\t"
    "repne\n\t"
    "scasw\n\t"
    "jne 1b\n\t"
    "decl %0\n\t"
    "jmp 3f\n"
    "2:\txorl %0,%0\n"
    "3:"
    : "=S" (__res)
    : "a" (0), "c" (0xffffffff), "0" (cs), "g" (ct)
    : "eax", "ecx", "edx", "edi");

  return __res;
}

#else

wchar_t *
wcspbrk(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

#endif

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

#ifdef i386

size_t 
wcsspn(const wchar_t *cs, const wchar_t *ct)
{
  register wchar_t * __res;

  __asm__ __volatile__(
    "cld\n\t"
    "movl %4,%%edi\n\t"
    "repne\n\t"
    "scasw\n\t"
    "notl %%ecx\n\t"
    "decl %%ecx\n\t"
    "movl %%ecx,%%edx\n"
    "1:\tlodsw\n\t"
    "testw %%eax,%%eax\n\t"
    "je 2f\n\t"
    "movl %4,%%edi\n\t"
    "movl %%edx,%%ecx\n\t"
    "repne\n\t"
    "scasb\n\t"
    "je 1b\n"
    "2:\tdecl %0"
    : "=S" (__res)
    : "a" (0), "c" (0xffffffff), "0" (cs), "g" (ct)
    : "eax", "ecx", "edx", "edi");

  return __res-cs;
}

#else

size_t 
wcsspn(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

#endif

#ifdef i386

wchar_t * 
wcsstr(const wchar_t *cs, const wchar_t *ct)
{
  register wchar_t * __res;

  __asm__ __volatile__(
    "cld\n\t" \
    "movl %4,%%edi\n\t"
    "repne\n\t"
    "scasw\n\t"
    "notl %%ecx\n\t"
    "decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
    "movl %%ecx,%%edx\n"
    "1:\tmovl %4,%%edi\n\t"
    "movl %%esi,%%eax\n\t"
    "movl %%edx,%%ecx\n\t"
    "repe\n\t"
    "cmpsw\n\t"
    "je 2f\n\t"		/* also works for empty string, see above */
    "xchgl %%eax,%%esi\n\t"
    "incl %%esi\n\t"
    "cmpw $0,-1(%%eax)\n\t"
    "jne 1b\n\t"
    "xorl %%eax,%%eax\n\t"
    "2:"
    : "=a" (__res)
    : "0" (0), "c" (0xffffffff), "S" (cs), "g" (ct)
    : "ecx", "edx", "edi", "esi");

  return __res;
}

#else

wchar_t * 
wcsstr(const wchar_t *cs, const wchar_t *ct)
{
UNIMPLEMENTED;
}

#endif

size_t wstrlen(const wchar_t *s)
{
    return wcslen(s);
}

#ifdef i386

wchar_t * 
wcstok(wchar_t * s,const wchar_t * ct)
{
  register wchar_t * __res;

  __asm__ __volatile__(
    "testl %1,%1\n\t"
    "jne 1f\n\t"
    "testl %0,%0\n\t"
    "je 8f\n\t"
    "movl %0,%1\n"
    "1:\txorl %0,%0\n\t"
    "movl $-1,%%ecx\n\t"
    "xorl %%eax,%%eax\n\t"
    "cld\n\t"
    "movl %4,%%edi\n\t"
    "repnz\n\t"
    "scasw\n\t"
    "notl %%ecx\n\t"
    "decl %%ecx\n\t"
    "decl %%ecx\n\t"
    "je 7f\n\t"			/* empty delimiter-string */
    "movl %%ecx,%%edx\n"
    "2:\tlodsw\n\t"
    "testw %%eax,%%eax\n\t"
    "je 7f\n\t"
    "movl %4,%%edi\n\t"
    "movl %%edx,%%ecx\n\t"
    "repne\n\t"
    "scasw\n\t"
    "je 2b\n\t"
    "decl %1\n\t"
    "decl %1\n\t"  
    "cmpw $0,(%1)\n\t"
    "je 7f\n\t"
    "movl %1,%0\n"
    "3:\tlodsw\n\t"
    "testw %%eax,%%eax\n\t"
    "je 5f\n\t"
    "movl %4,%%edi\n\t"
    "movl %%edx,%%ecx\n\t"
    "repne\n\t"
    "scasw\n\t"
    "jne 3b\n\t"
    "decl %1\n\t"
    "decl %1\n\t"  
    "decl %1\n\t"  
    "decl %1\n\t" 
    "cmpw $0,(%1)\n\t"
    "je 5f\n\t"
    "movw $0,(%1)\n\t"
    "incl %1\n\t"
    "incl %1\n\t" 
    "jmp 6f\n"
    "5:\txorl %1,%1\n"
    "6:\tcmpw $0,(%0)\n\t"
    "jne 7f\n\t"
    "xorl %0,%0\n"
    "7:\ttestl %0,%0\n\t"
    "jne 8f\n\t"
    "movl %0,%1\n"
    "8:"
    : "=b" (__res), "=S" (___wcstok)
    : "0" (___wcstok), "1" (s), "g" (ct)
    : "eax", "ecx", "edx", "edi", "memory");

  return __res;
}

#else

wchar_t * 
wcstok(wchar_t * s,const wchar_t * ct)
{
UNIMPLEMENTED;
}

#endif


