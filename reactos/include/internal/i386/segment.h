#ifndef _ASM_SEGMENT_H
#define _ASM_SEGMENT_H

#define USER_CS            (0x8+0x3)
#define USER_DS            (0x10+0x3)
#define ZERO_DS            0x18
#define KERNEL_CS          0x20
#define KERNEL_DS          0x28

#ifndef __ASSEMBLY__

/*
 * Uh, these should become the main single-value transfer routines..
 * They automatically use the right size if we just have the right
 * pointer type..
 */
#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) ((__typeof__(*(ptr)))__get_user((ptr),sizeof(*(ptr))))

/*
 * This is a silly but good way to make sure that
 * the __put_user function is indeed always optimized,
 * and that we use the correct sizes..
 */
extern int bad_user_access_length(void);

/*
 * dummy pointer type structure.. gcc won't try to do something strange
 * this way..
 */
struct __segment_dummy { unsigned long a[100]; };
#define __sd(x) ((struct __segment_dummy *) (x))
#define __const_sd(x) ((const struct __segment_dummy *) (x))

static inline void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
		case 1:
			__asm__ ("movb %b1,%%fs:%0"
				:"=m" (*__sd(y))
				:"iq" ((unsigned char) x), "m" (*__sd(y)));
			break;
		case 2:
			__asm__ ("movw %w1,%%fs:%0"
				:"=m" (*__sd(y))
				:"ir" ((unsigned short) x), "m" (*__sd(y)));
			break;
		case 4:
			__asm__ ("movl %1,%%fs:%0"
				:"=m" (*__sd(y))
				:"ir" (x), "m" (*__sd(y)));
			break;
		default:
			bad_user_access_length();
	}
}

static inline unsigned long __get_user(const void * y, int size)
{
	unsigned long result;

	switch (size) {
		case 1:
			__asm__ ("movb %%fs:%1,%b0"
				:"=q" (result)
				:"m" (*__const_sd(y)));
			return (unsigned char) result;
		case 2:
			__asm__ ("movw %%fs:%1,%w0"
				:"=r" (result)
				:"m" (*__const_sd(y)));
			return (unsigned short) result;
		case 4:
			__asm__ ("movl %%fs:%1,%0"
				:"=r" (result)
				:"m" (*__const_sd(y)));
			return result;
		default:
			return bad_user_access_length();
	}
}

static inline void __generic_memcpy_tofs(void * to, const void * from, unsigned long n)
{
    __asm__ volatile
	("	cld
		push %%es
		push %%fs
		cmpl $3,%0
		pop %%es
		jbe 1f
		movl %%edi,%%ecx
		negl %%ecx
		andl $3,%%ecx
		subl %%ecx,%0
		rep; movsb
		movl %0,%%ecx
		shrl $2,%%ecx
		rep; movsl
		andl $3,%0
	1:	movl %0,%%ecx
		rep; movsb
		pop %%es"
	:"=abd" (n)
	:"0" (n),"D" ((long) to),"S" ((long) from)
	:"cx","di","si");
}

static inline void __constant_memcpy_tofs(void * to, const void * from, unsigned long n)
{
	switch (n) {
		case 0:
			return;
		case 1:
			__put_user(*(const char *) from, (char *) to, 1);
			return;
		case 2:
			__put_user(*(const short *) from, (short *) to, 2);
			return;
		case 3:
			__put_user(*(const short *) from, (short *) to, 2);
			__put_user(*(2+(const char *) from), 2+(char *) to, 1);
			return;
		case 4:
			__put_user(*(const int *) from, (int *) to, 4);
			return;
		case 8:
			__put_user(*(const int *) from, (int *) to, 4);
			__put_user(*(1+(const int *) from), 1+(int *) to, 4);
			return;
		case 12:
			__put_user(*(const int *) from, (int *) to, 4);
			__put_user(*(1+(const int *) from), 1+(int *) to, 4);
			__put_user(*(2+(const int *) from), 2+(int *) to, 4);
			return;
		case 16:
			__put_user(*(const int *) from, (int *) to, 4);
			__put_user(*(1+(const int *) from), 1+(int *) to, 4);
			__put_user(*(2+(const int *) from), 2+(int *) to, 4);
			__put_user(*(3+(const int *) from), 3+(int *) to, 4);
			return;
	}
#define COMMON(x) \
__asm__("cld\n\t" \
	"push %%es\n\t" \
	"push %%fs\n\t" \
	"pop %%es\n\t" \
	"rep ; movsl\n\t" \
	x \
	"pop %%es" \
	: /* no outputs */ \
	:"c" (n/4),"D" ((long) to),"S" ((long) from) \
	:"cx","di","si")

	switch (n % 4) {
		case 0:
			COMMON("");
			return;
		case 1:
			COMMON("movsb\n\t");
			return;
		case 2:
			COMMON("movsw\n\t");
			return;
		case 3:
			COMMON("movsw\n\tmovsb\n\t");
			return;
	}
#undef COMMON
}

static inline void __generic_memcpy_fromfs(void * to, const void * from, unsigned long n)
{
    __asm__ volatile
	("	cld
		cmpl $3,%0
		jbe 1f
		movl %%edi,%%ecx
		negl %%ecx
		andl $3,%%ecx
		subl %%ecx,%0
		fs; rep; movsb
		movl %0,%%ecx
		shrl $2,%%ecx
		fs; rep; movsl
		andl $3,%0
	1:	movl %0,%%ecx
		fs; rep; movsb"
	:"=abd" (n)
	:"0" (n),"D" ((long) to),"S" ((long) from)
	:"cx","di","si", "memory");
}

static inline void __constant_memcpy_fromfs(void * to, const void * from, unsigned long n)
{
	switch (n) {
		case 0:
			return;
		case 1:
			*(char *)to = __get_user((const char *) from, 1);
			return;
		case 2:
			*(short *)to = __get_user((const short *) from, 2);
			return;
		case 3:
			*(short *) to = __get_user((const short *) from, 2);
			*((char *) to + 2) = __get_user(2+(const char *) from, 1);
			return;
		case 4:
			*(int *) to = __get_user((const int *) from, 4);
			return;
		case 8:
			*(int *) to = __get_user((const int *) from, 4);
			*(1+(int *) to) = __get_user(1+(const int *) from, 4);
			return;
		case 12:
			*(int *) to = __get_user((const int *) from, 4);
			*(1+(int *) to) = __get_user(1+(const int *) from, 4);
			*(2+(int *) to) = __get_user(2+(const int *) from, 4);
			return;
		case 16:
			*(int *) to = __get_user((const int *) from, 4);
			*(1+(int *) to) = __get_user(1+(const int *) from, 4);
			*(2+(int *) to) = __get_user(2+(const int *) from, 4);
			*(3+(int *) to) = __get_user(3+(const int *) from, 4);
			return;
	}
#define COMMON(x) \
__asm__("cld\n\t" \
	"rep ; fs ; movsl\n\t" \
	x \
	: /* no outputs */ \
	:"c" (n/4),"D" ((long) to),"S" ((long) from) \
	:"cx","di","si","memory")

	switch (n % 4) {
		case 0:
			COMMON("");
			return;
		case 1:
			COMMON("fs ; movsb");
			return;
		case 2:
			COMMON("fs ; movsw");
			return;
		case 3:
			COMMON("fs ; movsw\n\tfs ; movsb");
			return;
	}
#undef COMMON
}

#define memcpy_fromfs(to, from, n) \
(__builtin_constant_p(n) ? \
 __constant_memcpy_fromfs((to),(from),(n)) : \
 __generic_memcpy_fromfs((to),(from),(n)))

#define memcpy_tofs(to, from, n) \
(__builtin_constant_p(n) ? \
 __constant_memcpy_tofs((to),(from),(n)) : \
 __generic_memcpy_tofs((to),(from),(n)))

/*
 * These are deprecated..
 *
 * Use "put_user()" and "get_user()" with the proper pointer types instead.
 */

#define get_fs_byte(addr) __get_user((const unsigned char *)(addr),1)
#define get_fs_word(addr) __get_user((const unsigned short *)(addr),2)
#define get_fs_long(addr) __get_user((const unsigned int *)(addr),4)

#define put_fs_byte(x,addr) __put_user((x),(unsigned char *)(addr),1)
#define put_fs_word(x,addr) __put_user((x),(unsigned short *)(addr),2)
#define put_fs_long(x,addr) __put_user((x),(unsigned int *)(addr),4)

#ifdef WE_REALLY_WANT_TO_USE_A_BROKEN_INTERFACE

static inline unsigned short get_user_word(const short *addr)
{
	return __get_user(addr, 2);
}

static inline unsigned char get_user_byte(const char * addr)
{
	return __get_user(addr,1);
}

static inline unsigned long get_user_long(const int *addr)
{
	return __get_user(addr, 4);
}

static inline void put_user_byte(char val,char *addr)
{
	__put_user(val, addr, 1);
}

static inline void put_user_word(short val,short * addr)
{
	__put_user(val, addr, 2);
}

static inline void put_user_long(unsigned long val,int * addr)
{
	__put_user(val, addr, 4);
}

#endif

/*
 * Someone who knows GNU asm better than I should double check the following.
 * It seems to work, but I don't know if I'm doing something subtly wrong.
 * --- TYT, 11/24/91
 * [ nothing wrong here, Linus: I just changed the ax to be any reg ]
 */

static inline unsigned long get_fs(void)
{
	unsigned long _v;
	__asm__("mov %%fs,%w0":"=r" (_v):"0" (0));
	return _v;
}

static inline unsigned long get_ds(void)
{
	unsigned long _v;
	__asm__("mov %%ds,%w0":"=r" (_v):"0" (0));
	return _v;
}

static inline void set_fs(unsigned long val)
{
	__asm__ __volatile__("mov %w0,%%fs": /* no output */ :"r" (val));
}

static inline void set_ds(unsigned long val)
{
        __asm__ __volatile__("mov %w0,%%ds": /* no output */ :"r" (val));
}


#endif /* __ASSEMBLY__ */

#endif /* _ASM_SEGMENT_H */
