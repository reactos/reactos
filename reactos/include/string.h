/*
 * Adapted from linux for the reactos kernel, march 1998 -- David Welch
 * Removed extern specifier from ___strtok and ___wcstok, june 1998 -- Boudewijn Dekker
 * Added stricmp and strnicmp -- Boudewijn Dekker
 */




#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <types.h>        /* for size_t */


#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif


char * ___strtok; // removed extern specifier 02-06-98, BD

extern char * strcpy(char *,const char *);
extern char * strncpy(char *,const char *, __kernel_size_t);
extern char * strcat(char *, const char *);
extern char * strncat(char *, const char *, __kernel_size_t);
extern char * strchr(const char *,int);
extern char * strrchr(const char *,int);
extern char * strpbrk(const char *,const char *);
extern char * strtok(char *,const char *);
extern char * strstr(const char *,const char *);
extern __kernel_size_t strlen(const char *);
extern __kernel_size_t strnlen(const char *,__kernel_size_t);
extern __kernel_size_t strspn(const char *,const char *);
extern int strcmp(const char *,const char *);
extern int strncmp(const char *,const char *,__kernel_size_t);
extern int stricmp(const char* cs,const char * ct);
extern int strnicmp(const char* cs,const char * ct, size_t count);

extern void * memset(void *,int,__kernel_size_t);
extern void * memcpy(void *,const void *,__kernel_size_t);
extern void * memmove(void *,const void *,__kernel_size_t);
extern void * memscan(void *,int,__kernel_size_t);
extern int memcmp(const void *,const void *,__kernel_size_t);


/*
 * Include machine specific inline routines
 */
#ifndef _I386_STRING_H_
#define _I386_STRING_H_

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
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		Copyright (C) 1991, 1992 Linus Torvalds
 */

#define __HAVE_ARCH_STRCPY
extern inline  char * strcpy(char * dest,const char *src) 
{
__asm__ __volatile__(
	"cld\n"
	"1:\tlodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b"
	: /* no output */
	:"S" (src),"D" (dest):"si","di","ax","memory");
return dest;
}

#define __HAVE_ARCH_STRNCPY
extern inline char * strncpy(char * dest,const char *src,size_t count)
{
__asm__ __volatile__(
	"cld\n"
	"1:\tdecl %2\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"rep\n\t"
	"stosb\n"
	"2:"
	: /* no output */
	:"S" (src),"D" (dest),"c" (count):"si","di","ax","cx","memory");
return dest;
}

#define __HAVE_ARCH_STRCAT
extern inline char * strcat(char * dest,const char * src)
{
__asm__ __volatile__(
	"cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"decl %1\n"
	"1:\tlodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b"
	: /* no output */
	:"S" (src),"D" (dest),"a" (0),"c" (0xffffffff):"si","di","ax","cx");
return dest;
}

#define __HAVE_ARCH_STRNCAT
extern inline char * strncat(char * dest,const char * src,size_t count)
{
__asm__ __volatile__(
	"cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"decl %1\n\t"
	"movl %4,%3\n"
	"1:\tdecl %3\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"stosb\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n"
	"2:\txorl %2,%2\n\t"
	"stosb"
	: /* no output */
	:"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
	:"si","di","ax","cx","memory");
return dest;
}

#define __HAVE_ARCH_STRCMP
extern inline int strcmp(const char * cs,const char * ct)
{
register int __res;
__asm__ __volatile__(
	"cld\n"
	"1:\tlodsb\n\t"
	"scasb\n\t"
	"jne 2f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"jmp 3f\n"
	"2:\tsbbl %%eax,%%eax\n\t"
	"orb $1,%%eax\n"
	"3:"
	:"=a" (__res):"S" (cs),"D" (ct):"si","di");
return __res;
}

#define __HAVE_ARCH_STRNCMP
extern inline int strncmp(const char * cs,const char * ct,size_t count)
{
register int __res;
__asm__ __volatile__(
	"cld\n"
	"1:\tdecl %3\n\t"
	"js 2f\n\t"
	"lodsb\n\t"
	"scasb\n\t"
	"jne 3f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n"
	"2:\txorl %%eax,%%eax\n\t"
	"jmp 4f\n"
	"3:\tsbbl %%eax,%%eax\n\t"
	"orb $1,%%al\n"
	"4:"
	:"=a" (__res):"S" (cs),"D" (ct),"c" (count):"si","di","cx");
return __res;
}

#define __HAVE_ARCH_STRCHR
extern inline char * strchr(const char * s, int c)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movb %%al,%%ah\n"
	"1:\tlodsb\n\t"
	"cmpb %%ah,%%al\n\t"
	"je 2f\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"movl $1,%1\n"
	"2:\tmovl %1,%0\n\t"
	"decl %0"
	:"=a" (__res):"S" (s),"0" (c):"si");
return __res;
}

#define __HAVE_ARCH_STRRCHR
extern inline char * strrchr(const char * s, int c)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movb %%al,%%ah\n"
	"1:\tlodsb\n\t"
	"cmpb %%ah,%%al\n\t"
	"jne 2f\n\t"
	"leal -1(%%esi),%0\n"
	"2:\ttestb %%al,%%al\n\t"
	"jne 1b"
	:"=d" (__res):"0" (0),"S" (s),"a" (c):"ax","si");
return __res;
}

#define __HAVE_ARCH_STRSPN
extern inline size_t strspn(const char * cs, const char * ct)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 1b\n"
	"2:\tdecl %0"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

#define __HAVE_ARCH_STRCSPN
extern inline size_t strcspn(const char * cs, const char * ct)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n"
	"2:\tdecl %0"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res-cs;
}

#define __HAVE_ARCH_STRPBRK
extern inline char * strpbrk(const char * cs,const char * ct)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"movl %%ecx,%%edx\n"
	"1:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 2f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 1b\n\t"
	"decl %0\n\t"
	"jmp 3f\n"
	"2:\txorl %0,%0\n"
	"3:"
	:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	:"ax","cx","dx","di");
return __res;
}

#define __HAVE_ARCH_STRSTR
extern inline char * strstr(const char * cs,const char * ct)
{
register char * __res;
__asm__ __volatile__(
	"cld\n\t" \
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
	"movl %%ecx,%%edx\n"
	"1:\tmovl %4,%%edi\n\t"
	"movl %%esi,%%eax\n\t"
	"movl %%edx,%%ecx\n\t"
	"repe\n\t"
	"cmpsb\n\t"
	"je 2f\n\t"		/* also works for empty string, see above */
	"xchgl %%eax,%%esi\n\t"
	"incl %%esi\n\t"
	"cmpb $0,-1(%%eax)\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"2:"
	:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
	:"cx","dx","di","si");
return __res;
}

#define __HAVE_ARCH_STRLEN
extern inline size_t strlen(const char * s)
{
register int __res;
__asm__ __volatile__(
	"cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"di");
return __res;
}

#define __HAVE_ARCH_STRTOK
extern inline char * strtok(char * s,const char * ct)
{
register char * __res;
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
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"je 7f\n\t"			/* empty delimiter-string */
	"movl %%ecx,%%edx\n"
	"2:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 7f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 2b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 7f\n\t"
	"movl %1,%0\n"
	"3:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 5f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 3b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 5f\n\t"
	"movb $0,(%1)\n\t"
	"incl %1\n\t"
	"jmp 6f\n"
	"5:\txorl %1,%1\n"
	"6:\tcmpb $0,(%0)\n\t"
	"jne 7f\n\t"
	"xorl %0,%0\n"
	"7:\ttestl %0,%0\n\t"
	"jne 8f\n\t"
	"movl %0,%1\n"
	"8:"
	:"=b" (__res),"=S" (___strtok)
	:"0" (___strtok),"1" (s),"g" (ct)
	:"ax","cx","dx","di","memory");
return __res;
}


#define __HAVE_ARCH_STRICMP
extern inline int stricmp(const char* cs,const char * ct)
{
register int __res;


__asm__ __volatile__(
	"cld\n"
	"1:\tmovb (%%esi), %%eax\n\t"
	"movb  (%%edi), %%dl \n\t"
	"cmpb $0x5A, %%al\n\t"
	"ja 2f\t\n"
	"cmpb $0x40, %%al\t\n"
        "jbe 2f\t\n"
        "addb $0x20, %%al\t\n"
	"2:\t cmpb $0x5A, %%dl\t\n"
	"ja 3f\t\n"
	"cmpb $0x40, %%dl\t\n"
        "jbe 3f\t\n"
        "addb $0x20, %%dl\t\n"
	"3:\t inc %%esi\t\n"
	"inc %%edi\t\n"
	"cmpb %%al, %%dl\t\n"
	"jne 4f\n\t"  
	"cmpb $00, %%al\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"jmp 5f\n"
	"4:\tsbbl %%eax,%%eax\n\t"
	"orb $1,%%eax\n"
	"5:"
	:"=a" (__res):"S" (cs),"D" (ct):"si","di");
	
return __res;
}


#define __HAVE_ARCH_STRNICMP
extern inline int strnicmp(const char* cs,const char * ct, size_t count)
{
register int __res;


__asm__ __volatile__(
	"cld\n"
	"1:\t decl %3\n\t"
	"js 6f\n\t"
	"movb (%%esi), %%al\n\t"
	"movb  (%%edi), %%dl \n\t"
	"cmpb $0x5A, %%al\n\t"
	"ja 2f\t\n"
	"cmpb $0x40, %%al\t\n"
        "jbe 2f\t\n"
        "addb $0x20, %%al\t\n"
	"2:\t cmpb $0x5A, %%dl\t\n"
	"ja 3f\t\n"
	"cmpb $0x40, %%dl\t\n"
        "jbe 3f\t\n"
        "addb $0x20, %%dl\t\n"
	"3:\t inc %%esi\t\n"
	"inc %%edi\t\n"
	"cmpb %%al, %%dl\t\n"
	"jne 4f\n\t"  
	"cmpb $00, %%al\n\t"
	"jne 1b\n\t"
	"6:xorl %%eax,%%eax\n\t"
	"jmp 5f\n"
	"4:\tsbbl %%eax,%%eax\n\t"
	"orb $1,%%eax\n"
	"5:"
	:"=a" (__res):"S" (cs),"D" (ct), "c" (count):"si","di", "cx");
	

return __res;
}






extern inline void * __memcpy(void * to, const void * from, size_t n)
{
__asm__ __volatile__(
	"cld\n\t"
	"rep ; movsl\n\t"
	"testb $2,%b1\n\t"
	"je 1f\n\t"
	"movsw\n"
	"1:\ttestb $1,%b1\n\t"
	"je 2f\n\t"
	"movsb\n"
	"2:"
	: /* no output */
	:"c" (n/4), "q" (n),"D" ((long) to),"S" ((long) from)
	: "cx","di","si","memory");
return (to);
}

/*
 * This looks horribly ugly, but the compiler can optimize it totally,
 * as the count is constant.
 */
extern inline void * __constant_memcpy(void * to, const void * from, size_t n)
{
	switch (n) {
		case 0:
			return to;
		case 1:
			*(unsigned char *)to = *(const unsigned char *)from;
			return to;
		case 2:
			*(unsigned short *)to = *(const unsigned short *)from;
			return to;
		case 3:
			*(unsigned short *)to = *(const unsigned short *)from;
			*(2+(unsigned char *)to) = *(2+(const unsigned char *)from);
			return to;
		case 4:
			*(unsigned long *)to = *(const unsigned long *)from;
			return to;
		case 8:
			*(unsigned long *)to = *(const unsigned long *)from;
			*(1+(unsigned long *)to) = *(1+(const unsigned long *)from);
			return to;
		case 12:
			*(unsigned long *)to = *(const unsigned long *)from;
			*(1+(unsigned long *)to) = *(1+(const unsigned long *)from);
			*(2+(unsigned long *)to) = *(2+(const unsigned long *)from);
			return to;
		case 16:
			*(unsigned long *)to = *(const unsigned long *)from;
			*(1+(unsigned long *)to) = *(1+(const unsigned long *)from);
			*(2+(unsigned long *)to) = *(2+(const unsigned long *)from);
			*(3+(unsigned long *)to) = *(3+(const unsigned long *)from);
			return to;
		case 20:
			*(unsigned long *)to = *(const unsigned long *)from;
			*(1+(unsigned long *)to) = *(1+(const unsigned long *)from);
			*(2+(unsigned long *)to) = *(2+(const unsigned long *)from);
			*(3+(unsigned long *)to) = *(3+(const unsigned long *)from);
			*(4+(unsigned long *)to) = *(4+(const unsigned long *)from);
			return to;
	}
#define COMMON(x) \
__asm__("cld\n\t" \
	"rep ; movsl" \
	x \
	: /* no outputs */ \
	: "c" (n/4),"D" ((long) to),"S" ((long) from) \
	: "cx","di","si","memory");

	switch (n % 4) {
		case 0: COMMON(""); return to;
		case 1: COMMON("\n\tmovsb"); return to;
		case 2: COMMON("\n\tmovsw"); return to;
		case 3: COMMON("\n\tmovsw\n\tmovsb"); return to;
	}
#undef COMMON
}

#define __HAVE_ARCH_MEMCPY
#define memcpy(t, f, n) \
(__builtin_constant_p(n) ? \
 __constant_memcpy((t),(f),(n)) : \
 __memcpy((t),(f),(n)))

#define __HAVE_ARCH_MEMMOVE
extern inline void * memmove(void * dest,const void * src, size_t n)
{
if (dest<src)
__asm__ __volatile__(
	"cld\n\t"
	"rep\n\t"
	"movsb"
	: /* no output */
	:"c" (n),"S" (src),"D" (dest)
	:"cx","si","di");
else
__asm__ __volatile__(
	"std\n\t"
	"rep\n\t"
	"movsb\n\t"
	"cld"
	: /* no output */
	:"c" (n),
	 "S" (n-1+(const char *)src),
	 "D" (n-1+(char *)dest)
	:"cx","si","di","memory");
return dest;
}

#define memcmp __builtin_memcmp

#define __HAVE_ARCH_MEMCHR
extern inline void * memchr(const void * cs,int c,size_t count)
{
register void * __res;
if (!count)
	return NULL;
__asm__ __volatile__(
	"cld\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 1f\n\t"
	"movl $1,%0\n"
	"1:\tdecl %0"
	:"=D" (__res):"a" (c),"D" (cs),"c" (count)
	:"cx");
return __res;
}

extern inline void * __memset_generic(void * s, char c,size_t count)
{
__asm__ __volatile__(
	"cld\n\t"
	"rep\n\t"
	"stosb"
	: /* no output */
	:"a" (c),"D" (s),"c" (count)
	:"cx","di","memory");
return s;
}

/* we might want to write optimized versions of these later */
#define __constant_count_memset(s,c,count) __memset_generic((s),(c),(count))

/*
 * memset(x,0,y) is a reasonably common thing to do, so we want to fill
 * things 32 bits at a time even when we don't know the size of the
 * area at compile-time..
 */
extern inline void * __constant_c_memset(void * s, unsigned long c, size_t count)
{
__asm__ __volatile__(
	"cld\n\t"
	"rep ; stosl\n\t"
	"testb $2,%b1\n\t"
	"je 1f\n\t"
	"stosw\n"
	"1:\ttestb $1,%b1\n\t"
	"je 2f\n\t"
	"stosb\n"
	"2:"
	: /* no output */
	:"a" (c), "q" (count), "c" (count/4), "D" ((long) s)
	:"cx","di","memory");
return (s);	
}

/* Added by Gertjan van Wingerde to make minix and sysv module work */
#define __HAVE_ARCH_STRNLEN
extern inline size_t strnlen(const char * s, size_t count)
{
register int __res;
__asm__ __volatile__(
	"movl %1,%0\n\t"
	"jmp 2f\n"
	"1:\tcmpb $0,(%0)\n\t"
	"je 3f\n\t"
	"incl %0\n"
	"2:\tdecl %2\n\t"
	"cmpl $-1,%2\n\t"
	"jne 1b\n"
	"3:\tsubl %1,%0"
	:"=a" (__res)
	:"c" (s),"d" (count)
	:"dx");
return __res;
}
/* end of additional stuff */

/*
 * This looks horribly ugly, but the compiler can optimize it totally,
 * as we by now know that both pattern and count is constant..
 */
extern inline void * __constant_c_and_count_memset(void * s, unsigned long pattern, size_t count)
{
	switch (count) {
		case 0:
			return s;
		case 1:
			*(unsigned char *)s = pattern;
			return s;
		case 2:
			*(unsigned short *)s = pattern;
			return s;
		case 3:
			*(unsigned short *)s = pattern;
			*(2+(unsigned char *)s) = pattern;
			return s;
		case 4:
			*(unsigned long *)s = pattern;
			return s;
	}
#define COMMON(x) \
__asm__("cld\n\t" \
	"rep ; stosl" \
	x \
	: /* no outputs */ \
	: "a" (pattern),"c" (count/4),"D" ((long) s) \
	: "cx","di","memory")

	switch (count % 4) {
		case 0: COMMON(""); return s;
		case 1: COMMON("\n\tstosb"); return s;
		case 2: COMMON("\n\tstosw"); return s;
		case 3: COMMON("\n\tstosw\n\tstosb"); return s;
	}
#undef COMMON
}

#define __constant_c_x_memset(s, c, count) \
(__builtin_constant_p(count) ? \
 __constant_c_and_count_memset((s),(c),(count)) : \
 __constant_c_memset((s),(c),(count)))

#define __memset(s, c, count) \
(__builtin_constant_p(count) ? \
 __constant_count_memset((s),(c),(count)) : \
 __memset_generic((s),(c),(count)))

#define __HAVE_ARCH_MEMSET
#define memset(s, c, count) \
(__builtin_constant_p(c) ? \
 __constant_c_x_memset((s),(0x01010101UL*(unsigned char)c),(count)) : \
 __memset((s),(c),(count)))

/*
 * find the first occurrence of byte 'c', or 1 past the area if none
 */
#define __HAVE_ARCH_MEMSCAN
extern inline void * memscan(void * addr, int c, size_t size)
{
	if (!size)
		return addr;
	__asm__("cld
		repnz; scasb
		jnz 1f
		dec %%edi
1:		"
		: "=D" (addr), "=c" (size)
		: "0" (addr), "1" (size), "a" (c));
	return addr;
}





#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_STRING_H_ */


