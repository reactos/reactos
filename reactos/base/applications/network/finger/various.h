// Various things you need when porting BSD and GNU utilities to
// Win32.

#ifndef VARIOUS_H
#define VARIOUS_H


typedef float f4byte_t;
typedef double f8byte_t;
typedef long uid_t;  // SunOS 5.5

#define __P(x) x

/* utmp.h */
#define UT_LINESIZE 8
#define UT_HOSTSIZE 16

/* stat.h */
#define  S_ISREG(mode)   (((mode)&0xF000) == 0x8000)
#define  S_ISDIR(mode)   (((mode)&0xF000) == 0x4000)

#undef MIN //take care of windows default
#undef MAX //take care of windows default
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define bcopy(s1, s2, n)  memmove(s2, s1, n)
#define bcmp(s1, s2, n)  (memcmp(s1, s2, n) != 0)
#define bzero(s, n)  memset(s, 0, n)

#define index(s, c)  strchr(s, c)
#define rindex(s, c)  strrchr(s, c)

void netfinger(char *);

#endif
