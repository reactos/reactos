/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wstring.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

wchar_t* wcsrchr(const wchar_t* str, wchar_t ch)
{
   unsigned int len = 0;
   while (str[len]!=((wchar_t)0))
     {
	len++;
     }
   
   for (;len>0;len--)
     {
	if (str[len-1]==ch)
	  {
	     return(&str[len-1]);
	  }
     }
   return(NULL);
}

wchar_t* wcschr(wchar_t* str, wchar_t ch)
{
   while ((*str)!=((wchar_t)0))
     {
	if ((*str)==ch)
	  {
	     return(str);
	  }
	str++;
     }
   return(NULL);
}

wchar_t * wcscpy(wchar_t * str1,const wchar_t * str2)
{
   while ( (*str1)==(*str2) )
     {
	str1++;
	str2++;
	if ( (*str1)==((wchar_t)0) && (*str1)==((wchar_t)0) )
	  {
	     return(0);
	  }
     }
   return( (*str1) - (*str2) );
}

unsigned long wstrlen(PWSTR s)
{
        WCHAR c=' ';
        unsigned int len=0;

        while(c!=0) {
                c=*s;
                s++;
                len++;
        };
        s-=len;

        return len-1;
}

inline int wcscmp(const wchar_t* cs,const wchar_t * ct)
{
register int __res;
__asm__ __volatile__(
	"cld\n"
	"1:\tlodsw\n\t"
	"scasw\n\t"
	"jne 2f\n\t"
	"testw %%eax,%%eax\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"jmp 3f\n"
	"2:\tsbbl %%eax,%%eax\n\t"
	"orw $1,%%eax\n"
	"3:"
	:"=a" (__res):"S" (cs),"D" (ct):"esi","edi");
return __res;
}

#ifdef __MACHINE_STRING_FUNCTIONS
/*
 * Include machine specific inline routines
 */
//#ifndef _I386_STRING_H_
//#define _I386_STRING_H_

/*
 * On a 486 or Pentium, we are better off not using the
 * byte string operations. But on a 386 or a PPro the
 * byte string ops are faster than doing it by hand
 * (MUCH faster on a Pentium).
 *
 * Also, the byte strings actually work correctly. Forget
 * the i486 routines for now as they may be broken..
 */

#if FIXED_486_STRING && (CPU == 486 || CPU == 586)
  #include <asm/string-486.h>
#else

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially wcstok,wcsstr,wcs[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		Copyright (C) 1991, 1992 Linus Torvalds
 */



#define __HAVE_ARCH_WCSCPY
extern inline wchar_t * wcscpy(wchar_t * dest,const wchar_t *src)
{
__asm__ __volatile__(
	"cld\n"
	"1:\tlodsw\n\t"
	"stosw\n\t"
	"testw %%eax,%%eax\n\t"
	"jne 1b"
	: /* no output */
	:"S" (src),"D" (dest):"esi","edi","eax","memory");
return dest;
}

#define __HAVE_ARCH_WCSNCPY
inline wchar_t * wcsncpy(wchar_t * dest,const wchar_t *src,size_t count)
{
__asm__ __volatile__(
	"cld\n"
	"1:\tdecl %2\n\t"
	"js 2f\n\t"
	"lodsw\n\t"
	"stosw\n\t"
	"testw %%eax,%%eax\n\t"
	"jne 1b\n\t"
	"rep\n\t"
	"stosw\n"
	"2:"
	: /* no output */
	:"S" (src),"D" (dest),"c" (count):"esi","edi","eax","ecx","memory");
return dest;
}

#define __HAVE_ARCH_WCSCAT
inline wchar_t * wcscat(wchar_t * dest,const wchar_t * src)
{
__asm__ __volatile__(
	"cld\n\t"
	"repnz\n\t"  
	"scasw\n\t"  
	"decl %1\n"
	"decl %1\n\t"  
	"1:\tlodsw\n\t" 
	"stosw\n\t"	
	"testw %%eax,%%eax\n\t"
	"jne 1b"
	: /* no output */
	:"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"esi","edi","eax","ecx");
return dest;
}

#define __HAVE_ARCH_WCSNCAT
inline wchar_t * wcsncat(wchar_t * dest,const wchar_t * src,size_t count)
{
__asm__ __volatile__(
	"cld\n\t"
	"repnz\n\t"
	"scasw\n\t"
	"decl %1\n\t"
	"movl %4,%3\n"
	"decl %1\n\t" 
	"1:\tdecl %3\n\t"
	"js 2f\n\t"
	"lodsw\n\t"
	"stosw\n\t"
	"testw %%eax,%%eax\n\t"
	"jne 1b\n"
	"2:\txorl %2,%2\n\t"
	"stosw"
	: /* no output */
	:"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
	:"esi","edi","eax","ecx","memory");
return dest;
}

#define __HAVE_ARCH_WCSCMP

#define __HAVE_ARCH_WCSNCMP
inline int wcsncmp(const wchar_t * cs,const wchar_t * ct,size_t count)
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
	:"=a" (__res):"S" (cs),"D" (ct),"c" (count):"esi","edi","ecx");
return __res;
}

#define __HAVE_ARCH_WCSCHR
inline wchar_t * wcschr(const wchar_t * s, int c)
{
register wchar_t * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movw %%eax,%%edx\n"
	"1:\tlodsw\n\t"
	"cmpw %%edx,%%eax\n\t"
	"je 2f\n\t"
	"testw %%eax,%%eax\n\t"
	"jne 1b\n\t"
	"movl $1,%1\n"
	"2:\tmovl %1,%0\n\t"
	"decl %0\n\t"
	"decl %0\n\t"
	:"=a" (__res):"S" (s),"0" (c):"esi");
return __res;
}

#define __HAVE_ARCH_WCSRCHR
inline wchar_t * wcsrchr(const wchar_t * s, int c)
{
register wchar_t * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movw %%eax,%%edx\n"
	"1:\tlodsw\n\t"
	"cmpw %%edx,%%eax\n\t"
	"jne 2f\n\t"
	"leal -2(%%esi),%0\n"
	"2:\ttestw %%eax,%%eax\n\t"
	"jne 1b"
	:"=d" (__res):"0" (0),"S" (s),"a" (c):"eax","esi");
return __res;
}

#define __HAVE_ARCH_WCSSPN
inline size_t wcsspn(const wchar_t * cs, const wchar_t * ct)
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
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"eax","ecx","edx","edi");
return __res-cs;
}

#define __HAVE_ARCH_WCSCSPN
inline size_t wcscspn(const wchar_t * cs, const wchar_t * ct)
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
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"eax","ecx","edx","edi");
return __res-cs;
}

#define __HAVE_ARCH_STRPBRK
inline wchar_t * wcspbrk(const wchar_t * cs,const wchar_t * ct)
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
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"eax","ecx","edx","edi");
return __res;
}

#define __HAVE_ARCH_WCSSTR
inline wchar_t * wcsstr(const wchar_t * cs,const wchar_t * ct)
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
	:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
	:"ecx","edx","edi","esi");
return __res;
}


#define __HAVE_ARCH_WCSLEN
inline size_t wcslen(const wchar_t * s)
{
register int __res;
__asm__ __volatile__(
	"cld\n\t"
	"repne\n\t"
	"scasw\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"edi");
return __res;
}



#define __HAVE_ARCH_WCSTOK


inline wchar_t * wcstok(wchar_t * s,const wchar_t * ct)
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
	:"=b" (__res),"=S" (___wcstok)
	:"0" (___wcstok),"1" (s),"g" (ct)
	:"eax","ecx","edx","edi","memory");

return __res;
}


#define __HAVE_ARCH_WCSNNLEN
inline size_t wcsnlen(const wchar_t * s, size_t count)
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
	:"=a" (__res)
	:"c" (s),"d" (count)
	:"edx");
return __res;
}



#define __HAVE_ARCH_WCSICMP
inline int wcsicmp(const wchar_t* cs,const wchar_t * ct)
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
	:"=a" (__res):"S" (cs),"D" (ct):"esi","edi");
	
return __res;
}


#define __HAVE_ARCH_WCSNICMP
inline int wcsnicmp(const wchar_t* cs,const wchar_t * ct, size_t count)
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
	:"=a" (__res):"S" (cs),"D" (ct), "c" (count):"esi","edi", "ecx");
	

return __res;
}

#endif
#endif
