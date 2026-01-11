#ifndef _LIBMACH_COMPAT_H_
#define _LIBMACH_COMPAT_H_

/* BSD like types */
typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

typedef unsigned short u16int;
typedef short s16int;
typedef unsigned int u32int;
typedef int s32int;
typedef unsigned long long u64int;
typedef long long s64int;

#ifndef _WIN32
typedef ulong size_t;
#endif

DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlRaiseStatus(IN NTSTATUS Status);

#undef assert
#define assert(x) do { \
    if (!(x)) { \
        werrstr("(%s:%d) assertion " #x " failed\n", __FILE__, __LINE__); \
        RtlRaiseStatus(STATUS_ASSERTION_FAILURE); \
    } \
    } while (0)
#ifndef offsetof
#define offsetof(x,y) FIELD_OFFSET(x,y)
#endif
#define nil (0)

#define nelem(arr) (sizeof((arr)[0]) / sizeof((arr)))

int readn(void *fd, char *buf, ulong len);
int seek(void *fd, ulong off, int mode);

void *RosSymAllocMemZero(ulong num, ulong size);
void *RosSymRealloc(void *mem, ulong newsize);
void xfree(void *v);

/*
 * werrstr is disabled in kernel mode because rossym may be called from
 * inside the kernel debugger, which holds KdpDebuggerLock. Calling DbgPrint
 * would try to reacquire the lock causing SPIN_LOCK_ALREADY_OWNED.
 */
#ifdef _NTSYSTEM_
#define werrstr(x, ...) ((void)0)
#else
#define werrstr(x, ...) printf("(%s:%d) " x "\n",__FILE__,__LINE__,##__VA_ARGS__)
#endif

#define malloc(x) RosSymAllocMem(x)
#define mallocz(x,y) RosSymAllocMemZero(x,y)
#define free(x) xfree(x)
#define USED(x) (*((char *)&(x)) ^= 0)

#endif/*_LIBMACH_COMPAT_H_*/
