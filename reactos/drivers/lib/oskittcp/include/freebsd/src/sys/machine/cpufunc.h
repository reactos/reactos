/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Functions to provide access to special i386 instructions.
 */

#ifndef _MACHINE_CPUFUNC_H_
#define	_MACHINE_CPUFUNC_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/spl.h>	/* XXX belongs elsewhere */

#ifdef	__GNUC__

#ifdef BDE_DEBUGGER
extern int	bdb_exists;

static __inline int
bdb(void)
{
	if (!bdb_exists)
		return (0);
	__asm __volatile("int $3");
	return (1);
}
#endif /* BDE_DEBUGGER */

static __inline void
disable_intr(void)
{
#ifdef OSKIT
	osenv_intr_disable();
#else
	__asm __volatile("cli" : : : "memory");
#endif
}

static __inline void
enable_intr(void)
{
#ifdef OSKIT
	osenv_intr_enable();
#else
	__asm __volatile("sti");
#endif
}

#define	HAVE_INLINE_FFS

static __inline int
ffs(int mask)
{
	int	result;
	/*
	 * bsfl turns out to be not all that slow on 486's.  It can beaten
	 * using a binary search to reduce to 4 bits and then a table lookup,
	 * but only if the code is inlined and in the cache, and the code
	 * is quite large so inlining it probably busts the cache.
	 *
	 * Note that gcc-2's builtin ffs would be used if we didn't declare
	 * this inline or turn off the builtin.  The builtin is faster but
	 * broken in gcc-2.4.5 and slower but working in gcc-2.5 and 2.6.
	 */
	__asm __volatile("testl %0,%0; je 1f; bsfl %0,%0; incl %0; 1:"
			 : "=r" (result) : "0" (mask));
	return (result);
}

#if __GNUC__ < 2

#define	inb(port)		inbv(port)
#define	outb(port, data)	outbv(port, data)

#else /* __GNUC >= 2 */

/*
 * The following complications are to get around gcc not having a
 * constraint letter for the range 0..255.  We still put "d" in the
 * constraint because "i" isn't a valid constraint when the port
 * isn't constant.  This only matters for -O0 because otherwise
 * the non-working version gets optimized away.
 * 
 * Use an expression-statement instead of a conditional expression
 * because gcc-2.6.0 would promote the operands of the conditional
 * and produce poor code for "if ((inb(var) & const1) == const2)".
 */
#define	inb(port)	({						\
	u_char	_data;							\
	if (__builtin_constant_p((int) (port)) && (port) < 256ul)	\
		_data = inbc(port);					\
	else								\
		_data = inbv(port);					\
	_data; })

#define	outb(port, data) \
	(__builtin_constant_p((int) (port)) && (port) < 256ul \
	 ? outbc(port, data) : outbv(port, data))

static __inline u_char
inbc(u_int port)
{
	u_char	data;

	__asm __volatile("inb %1,%0" : "=a" (data) : "id" ((u_short)(port)));
	return (data);
}

static __inline void
outbc(u_int port, u_char data)
{
	__asm __volatile("outb %0,%1" : : "a" (data), "id" ((u_short)(port)));
}

#endif /* __GNUC <= 2 */

static __inline u_char
inbv(u_int port)
{
	u_char	data;
	/*
	 * We use %%dx and not %1 here because i/o is done at %dx and not at
	 * %edx, while gcc generates inferior code (movw instead of movl)
	 * if we tell it to load (u_short) port.
	 */
	__asm __volatile("inb %%dx,%0" : "=a" (data) : "d" (port));
	return (data);
}

static __inline u_long
inl(u_int port)
{
	u_long	data;

	__asm __volatile("inl %%dx,%0" : "=a" (data) : "d" (port));
	return (data);
}

static __inline void
insb(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; insb"
			 : : "d" (port), "D" (addr), "c" (cnt)
			 : "di", "cx", "memory");
}

static __inline void
insw(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; insw"
			 : : "d" (port), "D" (addr), "c" (cnt)
			 : "di", "cx", "memory");
}

static __inline void
insl(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; insl"
			 : : "d" (port), "D" (addr), "c" (cnt)
			 : "di", "cx", "memory");
}

static __inline u_short
inw(u_int port)
{
	u_short	data;

	__asm __volatile("inw %%dx,%0" : "=a" (data) : "d" (port));
	return (data);
}

static __inline unsigned
loadandclear(u_int *addr)
{
	u_int	result;

	__asm __volatile("xorl %0,%0; xchgl %1,%0"
			 : "=&r" (result) : "m" (*addr));
	return (result);
}

static __inline void
outbv(u_int port, u_char data)
{
	u_char	al;
	/*
	 * Use an unnecessary assignment to help gcc's register allocator.
	 * This make a large difference for gcc-1.40 and a tiny difference
	 * for gcc-2.6.0.  For gcc-1.40, al had to be ``asm("ax")'' for
	 * best results.  gcc-2.6.0 can't handle this.
	 */
	al = data;
	__asm __volatile("outb %0,%%dx" : : "a" (al), "d" (port));
}

static __inline void
outl(u_int port, u_long data)
{
	/*
	 * outl() and outw() aren't used much so we haven't looked at
	 * possible micro-optimizations such as the unnecessary
	 * assignment for them.
	 */
	__asm __volatile("outl %0,%%dx" : : "a" (data), "d" (port));
}

static __inline void
outsb(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; outsb"
			 : : "d" (port), "S" (addr), "c" (cnt)
			 : "si", "cx");
}

static __inline void
outsw(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; outsw"
			 : : "d" (port), "S" (addr), "c" (cnt)
			 : "si", "cx");
}

static __inline void
outsl(u_int port, void *addr, size_t cnt)
{
	__asm __volatile("cld; rep; outsl"
			 : : "d" (port), "S" (addr), "c" (cnt)
			 : "si", "cx");
}

static __inline void
outw(u_int port, u_short data)
{
	__asm __volatile("outw %0,%%dx" : : "a" (data), "d" (port));
}

static __inline void
pmap_update(void)
{
	u_long	temp;
	/*
	 * This should be implemented as load_cr3(rcr3()) when load_cr3()
	 * is inlined.
	 */
	__asm __volatile("movl %%cr3, %0; movl %0, %%cr3" : "=r" (temp)
			 : : "memory");
}

static __inline u_long
rcr2(void)
{
	u_long	data;

	__asm __volatile("movl %%cr2,%0" : "=r" (data));
	return (data);
}

static __inline u_long
read_eflags(void)
{
	u_long	ef;

	__asm __volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}

static __inline void
write_eflags(u_long ef)
{
	__asm __volatile("pushl %0; popfl" : : "r" (ef));
}

/*
 * XXX queue stuff belongs elsewhere.
 */
struct quehead {
	struct quehead *qh_link;
	struct quehead *qh_rlink;
};

static __inline void
insque(void *a, void *b)
{
	struct quehead *element = (struct quehead *)a,
			*head = (struct quehead *)b;

	element->qh_link = head->qh_link;
	element->qh_rlink = head;
	head->qh_link = element;
	element->qh_link->qh_rlink = element;
}

static __inline void
remque(void *a)
{
	struct quehead *element = (struct quehead *)a;

	element->qh_link->qh_rlink = element->qh_rlink;
	element->qh_rlink->qh_link = element->qh_link;
	element->qh_rlink = 0;
}

#else /* !__GNUC__ */

int	bdb		__P((void));
void	disable_intr	__P((void));
void	enable_intr	__P((void));
u_char	inb		__P((u_int port));
u_long	inl		__P((u_int port));
void	insb		__P((u_int port, void *addr, size_t cnt));
void	insl		__P((u_int port, void *addr, size_t cnt));
void	insw		__P((u_int port, void *addr, size_t cnt));
u_short	inw		__P((u_int port));
u_int	loadandclear	__P((u_int *addr));
void	outb		__P((u_int port, u_char data));
void	outl		__P((u_int port, u_long data));
void	outsb		__P((u_int port, void *addr, size_t cnt));
void	outsl		__P((u_int port, void *addr, size_t cnt));
void	outsw		__P((u_int port, void *addr, size_t cnt));
void	outw		__P((u_int port, u_short data));
void	pmap_update	__P((void));
u_long	read_eflags	__P((void));
u_long	rcr2		__P((void));
void	write_eflags	__P((u_long ef));

void	insque		__P((void *a, void *b));
void	remque		__P((void *a));

#endif	/* __GNUC__ */

/*
 * XXX the following declarations document garbage in support.s.
 * gcc hasn't needed _divsi* for years.
 * bcopy[bwx]() was used by pccons but isn't used now.
 */
int	__divsi3	__P((int factor1, int factor2));
u_int	__udivsi3	__P((u_int factor1, u_int factor2));
void	bcopyb		__P((const void *from, void *to, size_t len));
void	bcopyw		__P((const void *from, void *to, size_t len));
void	bcopyx		__P((const void *from, void *to, size_t len,
			     int stride));

#if 0
/*
 * These functions in support.s are declared elsewhere.
 */
void	bcopy		__P((const void *from, void *to, size_t len));
void	blkclr		__P((void *buf, size_t len));
void	bzero		__P((void *buf, size_t len));
int	copyin		__P((void *udaddr, void *kaddr, size_t len));
int	copyinstr	__P((void *udaddr, void *kaddr, size_t len,
			     size_t *lencopied));
int	copyout		__P((void *kaddr, void *udaddr, size_t len));
int	copystr		__P((void *kfaddr, void *kdaddr, size_t len,
			     size_t *lencopied));
int	fubyte		__P((void *base));
int	fuswintr	__P((void *base));
int	fuibyte		__P((void *base));
int	fuword		__P((void *base));
struct	region_descriptor;
void	lgdt		__P((struct region_descriptor *rdp));
void	lidt		__P((struct region_descriptor *rdp));
void	lldt		__P((u_short sel));
/*
 * longjmp() and setjmp() are only used by ddb.  They probably shouldn't
 * shouldn't be supported in the kernel.
 */
#include <setjmp.h>
void	longjmp		__P((jmp_buf jb, int rv));
void	ovbcopy		__P((const void *from, void *to, size_t len);
int	setjmp		__P((jmp_buf jb));
struct soft_segment_descriptor;
union descriptor;
int	ssdtosd		__P((struct soft_segment_descriptor *ssdp,
			     union descriptor *sdp));
int	subyte		__P((void *base, int byte));
int	suibyte		__P((void *base, int byte));
int	suswintr	__P((void *base, int word));
int	suword		__P((void *base, int word));

/*
 * These functions in support.s are declared elsewhere, but never used.
 * A silly amount of effort went into copyoutstr().  It's not worth
 * maintaining, since the string length is usually known so copyout
 * works better, or is easy to find so copyout() can be used.
 */
int	copyoutstr	__P((void *kaddr, void *udaddr, size_t len,
			     size_t *lencopied));
int	fuiword		__P((void *base));
int	suiword		__P((void *base, int word));

/*
 * These functions in support.s are also in libkern.a and are declared in
 * libkern.h.
 * ffs() is built in to gcc-2 and was buggy in gcc-2.4.5 so we may may the
 * buggy version if we don't replace it by an inline.
 */
int	bcmp		__P((const void *b1, const void *b2, size_t length));
int	ffs		__P((int mask));
#endif /* 0 */

/*
 * These variables and functions in support.s are used.
 */
extern u_int atdevbase;	/* offset in virtual memory of ISA io mem */

void	filli		__P((int pat, void *base, size_t cnt));
void	fillw		__P((int /*u_short*/ pat, void *base, size_t cnt));
int	fusword		__P((void *base));
void	load_cr0	__P((u_long cr0));
void	load_cr3	__P((u_long cr3));
void	ltr		__P((u_short sel));
u_int	rcr0		__P((void));
u_long	rcr3		__P((void));
int	rtcin		__P((int val));

/*
 * These functions are NOT in support.s and should be declared elsewhere.
 */
void	Debugger	__P((const char *msg));
u_long	kvtop		__P((void *addr));
typedef void alias_for_inthand_t __P((u_int cs, u_int ef, u_int esp,
				      u_int ss));
void	setidt		__P((int idx, alias_for_inthand_t *func, int typ,
			     int dpl, int selec));

#endif /* !_MACHINE_CPUFUNC_H_ */
