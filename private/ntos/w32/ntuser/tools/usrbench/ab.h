// ab.h

// Input:
// A1: A or W: Ansi, Wide
// A2: A or W

// macros: (where x is 1 or 2)
// IFx(ansi,wide)
// AWx:         adds suffix (A or W)
// Lx(a)        makes widechar

#if !defined(A1) || !defined(A2)
#error A1 and A2 must be defined as A or W
#endif

#ifdef AW1

#undef AW1
#undef AW2
#undef IF1
#undef IF2
#undef L1
#undef L2
#undef FAW

#endif

#ifndef CONCAT
#define CONCAT(a,b) a##b
#endif

#if A1

#define IF1(a,w)    w
#define AW1(x)   x##W
#define L1(x)    CONCAT(L, x)

#else

#define IF1(a,w)    a
#define AW1(x)  x##A
#define L1(x)   x

#endif

#if A2

#define IF2(a,w)    w
#define AW2(x)  x##W
#define L2(x)   CONCAT(L, x)

#else

#define IF2(a,w)    a
#define AW2(x)  x##A
#define L2(x)   x

#endif

#if A1
#if A2
#define FAW(x)  x##WW
#else
#define FAW(x)  x##WA
#endif
#else
#if A2
#define FAW(x)  x##AW
#else
#define FAW(x)  x##AA
#endif
#endif
