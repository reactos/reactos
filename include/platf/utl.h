_ONCE

/*************************************************************************
 utl.h - basic utility macros
 Jose M. Catena Gomez, diwaves.com
*************************************************************************/

// extract an smaller type from a larger type and byte offset
#define partp(typ, ptr, pos) (*(((typ *)ptr) + pos))

// buld larger types from smaller ones
#define mk16(l,h) ((i16u)((i8u)(l)) | (((i16u)((i8u)(h))) << 8))
#define mk32(l,h) ((i32u)((i16u)(l)) | (((i32u)((i16u)(h))) << 16))

// already in windef.h
#ifndef min
#define min(a,b) ((a<b)?a:b)
#endif
#ifndef max
#define max(a,b) ((a>b)?a:b)
#endif

// some other implementations of these are much worse
// specially when they use multiplication
#define round_down(n, align) (n & ~(align-1))
#define round_up(n, align) ((n + (align-1)) & ~(align-1))

#define elemcnt(x) (sizeof(x) / sizeof(x[0]))
#define fldoffset(typ, fld) (&((typ *)0)->fld)
#define fldaddr(addr, typ, fld) (&(((typ)(addr))->fld))
#define fld(addr, typ, fl) (((typ)(addr))->fl)

// string lengh / size in bytes conversion
#ifdef _UNICODE
#define cntctob(c) (c<<1)
#define cntbtoc(b) (b>>1)
#else
#define cntctob(c) (c)
#define cntbtoc(b) (b)
#endif
#define _tcssiz(tcs) (cntctob(_tcslen(tcs)))

