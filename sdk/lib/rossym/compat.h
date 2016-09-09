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

#define offsetof(x,y) FIELD_OFFSET(x,y)
#define nil (0)

#define nelem(arr) (sizeof((arr)[0]) / sizeof((arr)))

int readn(void *fd, char *buf, ulong len);
int seek(void *fd, ulong off, int mode);

void *RosSymAllocMemZero(ulong num, ulong size);
void *RosSymRealloc(void *mem, ulong newsize);
void xfree(void *v);

#define werrstr(str, ...) DPRINT(str "\n" ,##__VA_ARGS__)
#define malloc(x) RosSymAllocMem(x)
#define mallocz(x,y) RosSymAllocMemZero(x,y)
#define free(x) xfree(x)
#define USED(x) (*((char *)&(x)) ^= 0)
#define memset(x,y,z) RtlZeroMemory(x,z)

#endif/*_LIBMACH_COMPAT_H_*/
