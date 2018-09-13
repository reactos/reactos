//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       lendian.hpp
//
//  Contents:   Templatized data types for access to unaligned scalar values.
//
//----------------------------------------------------------------------------

// Implementation of LENDIAN data values.  This works for simple scalar values.
// This file is heavily macro-based.  The intent is to do this work with arithmetic.
// The functionality may be expressed much more consisely with memcpy, but I need an
// implementation faster than that.

// To create an LENDIAN type, use the MAKE_LENDIAN_TYPE macro.  For example, if
// you need an int_LENDIAN, say
//
//        MAKE_LENDIAN_TYPE( int, 4 );
//
// Note that you need to specifiy the size of the base type.  This is because the
// amount of work to be done needs to be known by the preprocessor, where sizeof()
// doesn't work.

// To use: The intent is that replace occurances of "type LENDIAN" in your code
// with "type_LENDIAN".  It's simply a lexical change.  You could automate this
// change in order to easily convert code.  More importantly, the reverse
// transformation may also be easily automated, making it easier to merge with code
// that doesn't use these types.

// LENDIAN64: The unaligned 64 types are an optimization.  They should only be used
// for values over four bytes in size (e.g. __int64, double).  They assume that the
// pointers are aligned to 4-bytes, so that access may be done with two 32-bit
// references, instead of 8 8-bit references.

#ifndef __lendian_h__
#define __lendian_h__

#if defined(_MSC_VER) && !defined(BIG_ENDIAN)

#define MAKE_LENDIAN_UNALIGNED_TYPE(base,size)    typedef base UNALIGNED   base##_LENDIAN_UNALIGNED
#define MAKE_LENDIAN_TYPE(base,size)              typedef base             base##_LENDIAN_UNALIGNED; \
                                                  typedef base   base##_LENDIAN \

//
// Use MAKE_UNALIGNED64_TYPE for QWORD data that may only be DWORD aligned on 64-bit 
// machines.  Only 64-bit architectures pay the penalty for accessing such data.
//   
#if defined(ALPHA)
#  define MAKE_LENDIAN64_UNALIGNED_TYPE(base,size)  typedef base UNALIGNED base##_LENDIAN64_UNALIGNED
#else
#  define MAKE_LENDIAN64_UNALIGNED_TYPE(base,size)  typedef base base##_LENDIAN64_UNALIGNED
#endif
#define MAKE_LENDIAN64_TYPE(base,size)  typedef base base##_LENDIAN64

#define MAKE_LENDIANPTR_UNALIGNED_TYPE(base)      typedef base UNALIGNED *base##_LENDIANPTR_UNALIGNED
#define MAKE_LENDIANPTR_TYPE(base)                typedef base *base##_LENDIANPTR

#else // _MSC_VER

#define SET2(val,data)                          \
    data[1] = (val) >> 8,                       \
    data[0] = (val)

#define GET2(data)                          \
    ( ( data[1] << 8 ) |                        \
      ( data[0] ) )

#define SET4(val,data)                          \
    data[3] = (val) >> 24,                      \
    data[2] = (val) >> 16,                      \
    data[1] = (val) >> 8,                       \
    data[0] = (val)

#define GET4(data)                              \
    ( ( data[3] << 24 ) |                       \
      ( data[2] << 16 ) |                       \
      ( data[1] << 8 ) |                        \
      ( data[0] ) )

#define SET8(val,data)                          \
    data[7] = (val) >> 56,                      \
    data[6] = (val) >> 48,                      \
    data[5] = (val) >> 40,                      \
    data[4] = (val) >> 32,                      \
    data[3] = (val) >> 24,                      \
    data[2] = (val) >> 16,                      \
    data[1] = (val) >> 8,                       \
    data[0] = (val)

#define GET8(data)                              \
    ( ( data[7] << 56 ) |                       \
      ( data[6] << 48 ) |                       \
      ( data[5] << 40 ) |                       \
      ( data[4] << 32 ) |                       \
      ( data[3] << 24 ) |                       \
      ( data[2] << 16 ) |                       \
      ( data[1] << 8 ) |                        \
      ( data[0] ) )

#if defined( ux10 )
#define SET02(x,y)  SET2(x,y)
#define GET02(x)    GET2(x)
#define SET04(x,y)  SET4(x,y)
#define GET04(x)    GET4(x)
#define SET08(x,y)  SET8(x,y)
#define GET08(x)    GET8(x)
#endif

#define SET648(val,data)                        \
    data[1] = (val) >> 32,                      \
    data[0] = (val)

#define GET648(data)                            \
    ( ( data[1] << 32 ) |                       \
      ( data[0] ) )

#define GROUP(type,size,get,set,sfx)                                                            \
    LEndian##sfx##size( type v ) { set##size( v, data ); }                                      \
    type operator = ( type v )   { set##size( v, data ); return v; }                            \
    operator type()              { return get##size( data ); }                                  \
    type operator += (const type v) { set##size( get##size(data) + v, data); return *this; }    \
    type operator -= (const type v) { set##size( get##size(data) - v, data); return *this; }    \
    type operator <<= (const type v) { set##size( get##size(data) << v, data); return *this; }  \
    type operator <= (const type v) { set##size( get##size(data) < v, data); return *this; }    \
    type operator >= (const type v) { set##size( get##size(data) > v, data); return *this; }    \
    type operator *= (const type v) { set##size( get##size(data) * v, data); return *this; }    \
    type operator /= (const type v) { set##size( get##size(data) / v, data); return *this; }    \
    type operator %= (const type v) { set##size( get##size(data) % v, data); return *this; }    \
    type operator >>= (const type v) { set##size( get##size(data) >> v, data); return *this; }  \
    type operator ^= (const type v) { set##size( get##size(data) ^ v, data); return *this; }    \
    type operator &= (const type v) { set##size( get##size(data) & v, data); return *this; }    \
    type operator |= (const type v) { set##size( get##size(data) | v, data); return *this; }


#define MAKE_TEMPLATE(size,type,sfx)                            \
template<class BASE>                                            \
class LEndian##sfx##size                                        \
{                                                               \
  protected:                                                    \
    type data[ size / sizeof(type) ];                           \
                                                                \
  public:                                                       \
                                                                \
    GROUP( signed char,      size, GET##sfx, SET##sfx, sfx )    \
    GROUP( short,            size, GET##sfx, SET##sfx, sfx )    \
    GROUP( int,              size, GET##sfx, SET##sfx, sfx )    \
    GROUP( wchar_t,          size, GET##sfx, SET##sfx, sfx )    \
    GROUP( long,             size, GET##sfx, SET##sfx, sfx )    \
    GROUP( __int64,          size, GET##sfx, SET##sfx, sfx )    \
    GROUP( unsigned char,    size, GET##sfx, SET##sfx, sfx )    \
    GROUP( unsigned short,   size, GET##sfx, SET##sfx, sfx )    \
    GROUP( unsigned int,     size, GET##sfx, SET##sfx, sfx )    \
    GROUP( unsigned long,    size, GET##sfx, SET##sfx, sfx )    \
    GROUP( unsigned __int64, size, GET##sfx, SET##sfx, sfx )    \
}


#if defined( ux10 )
MAKE_TEMPLATE( 2, unsigned char, 0);
MAKE_TEMPLATE( 4, unsigned char, 0);
MAKE_TEMPLATE( 8, unsigned char, 0);
#else
MAKE_TEMPLATE( 2, unsigned char, );
MAKE_TEMPLATE( 4, unsigned char, );
MAKE_TEMPLATE( 8, unsigned char, );
#endif
MAKE_TEMPLATE( 8, unsigned int, 64 );

#if defined( ux10 )
template<class BASE>
class LEndianPtr : LEndian04<BASE> {
public:
	LEndianPtr(const void * v) : LEndian04<BASE>((int)v) { }
	void *operator = (void * v)        { SET4((int)v,data); return v; }
	const void *operator = (const void * v)        { SET4((int)v,data); return v; }
	operator BASE()              { return (BASE)GET4(data); }
};

#else

template<class BASE>
class LEndianPtr : LEndian4<BASE> {
public:
	LEndianPtr(const void * v) : LEndian4<BASE>((int)v) { }
	void *operator = (void * v)        { SET4((int)v,data); return v; }
	const void *operator = (const void * v)        { SET4((int)v,data); return v; }
	operator BASE()              { return (BASE)GET4(data); }
};

#endif


#undef SET2
#undef SET4
#undef SET8
#undef SET648
#undef GROUP
#undef MAKE_TEMPLATE

#if defined( ux10 )
#define MAKE_LENDIAN_TYPE(base,size)            \
   typedef LEndian0##size<base>   base##_LENDIAN; \
   typedef LEndian0##size<base>   base##_LENDIAN_UNALIGNED
#else
#define MAKE_LENDIAN_TYPE(base,size)            \
   typedef LEndian##size<base>   base##_LENDIAN; \
   typedef LEndian##size<base>   base##_LENDIAN_UNALIGNED
#endif

#define MAKE_LENDIAN64_TYPE(base,size)                          \
  typedef LEndian64##size<base> base##_LENDIAN64;                \
  typedef LEndian64##size<base> base##_LENDIAN64_UNALIGNED

#define MAKE_LENDIANPTR_TYPE(base)                              \
  typedef LEndianPtr<base>      base##_LENDIANPTR;               \
  typedef LEndianPtr<base>      base##_LENDIANPTR_UNALIGNED

#endif  // _MSC_VER

// Predefine some of the basic types.

MAKE_LENDIAN_TYPE( short, 2 );
MAKE_LENDIAN_TYPE( int, 4 );
MAKE_LENDIAN_TYPE( long, 4 );
MAKE_LENDIAN_TYPE( WORD, 2 );
MAKE_LENDIAN_TYPE( DWORD, 4 );
MAKE_LENDIAN_TYPE( USHORT, 4 );
MAKE_LENDIAN_TYPE( SHORT, 4 );
MAKE_LENDIAN_TYPE( __int64, 8 );
MAKE_LENDIAN64_TYPE( __int64, 8 );

#endif // __lendian_h__

