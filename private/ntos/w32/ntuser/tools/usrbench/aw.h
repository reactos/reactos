// aw.h

// IF(ansi, widechar)
// AW(x)        add suffix (A or W)
// L(x)         makes widechar when WIDE is defined

#ifdef AW
#undef AW
#endif
#ifdef IF
#undef IF
#endif
#ifdef L
#undef L
#endif

#ifndef CONCAT
#define CONCAT(a,b) a##b
#endif

#if defined(WIDE) && (WIDE)

#define IF(a,w) w
#define AW(x)   x##W
#define L(x)    CONCAT(L, x)

#else

#define IF(a,w) a
#define AW(x)   x##A
#define L(x)    x

#endif

