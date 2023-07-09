//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       unaligned.hpp
//
//  Contents:   Templatized data types for access to unaligned scalar values.
//
//----------------------------------------------------------------------------

// Implementation of UNALIGNED data values.  This works for simple scalar values.
// This file is heavily macro-based.  The intent is to do this work with arithmetic.
// The functionality may be expressed much more consisely with memcpy, but I need an
// implementation faster than that.

// To create an UNALIGNED type, use the MAKE_UNALIGNED_TYPE macro.  For example, if
// you need an int_UNALIGNED, say
//
//        MAKE_UNALIGNED_TYPE( int, 4 );
//
// Note that you need to specifiy the size of the base type.  This is because the
// amount of work to be done needs to be known by the preprocessor, where sizeof()
// doesn't work.

// To use: The intent is that replace occurances of "type UNALIGNED" in your code
// with "type_UNALIGNED".  It's simply a lexical change.  You could automate this
// change in order to easily convert code.  More importantly, the reverse
// transformation may also be easily automated, making it easier to merge with code
// that doesn't use these types.

// UNALIGNED64: The unaligned 64 types are an optimization.  They should only be used
// for values over four bytes in size (e.g. __int64, double).  They assume that the
// pointers are aligned to 4-bytes, so that access may be done with two 32-bit
// references, instead of 8 8-bit references.

#ifndef __unaligned_h__
#define __unaligned_h__

#if defined (_MSC_VER) || defined(__APOGEE__)

#define MAKE_UNALIGNED_TYPE(base,size)    typedef base UNALIGNED   base##_UNALIGNED

//
// Use MAKE_UNALIGNED64_TYPE for QWORD data that may only be DWORD aligned on 64-bit
// machines.  Only 64-bit architectures pay the penalty for accessing such data.
//

#ifdef SPARC
#define MAKE_UNALIGNED64_TYPE(base,size)  typedef base base##_UNALIGNED64
#else
#define MAKE_UNALIGNED64_TYPE(base,size)  typedef base UNALIGNED base##_UNALIGNED64
#endif // SPARC

#define MAKE_UNALIGNEDPTR_TYPE(base)      typedef base UNALIGNED *base##_UNALIGNEDPTR

#else // defined (_MSC_VER) || defined(__APOGEE__)


#ifdef BIG_ENDIAN

#define SET2(val,data)                          \
    data[0] = (val) >> 8,                       \
    data[1] = (val)

#define GET2(data)                              \
    ( ( data[0] << 8 ) |                        \
      ( data[1] ) )

#define SET4(val,data)                          \
    data[0] = (val) >> 24,                      \
    data[1] = (val) >> 16,                      \
    data[2] = (val) >> 8,                       \
    data[3] = (val)

#define GET4(data)                              \
    ( ( data[0] << 24 ) |                       \
      ( data[1] << 16 ) |                       \
      ( data[2] << 8 ) |                        \
      ( data[3] ) )

#define SET8(val,data)                          \
    data[0] = (val) >> 56,                      \
    data[1] = (val) >> 48,                      \
    data[2] = (val) >> 40,                      \
    data[3] = (val) >> 32                       \
    data[4] = (val) >> 24,                      \
    data[5] = (val) >> 16,                      \
    data[6] = (val) >> 8,                       \
    data[7] = (val)

#define GET8(data)                              \
    ( ( data[0] << 56 ) |                       \
      ( data[1] << 48 ) |                       \
      ( data[2] << 40 ) |                       \
      ( data[3] << 32 ) |                       \
      ( data[4] << 24 ) |                       \
      ( data[5] << 16 ) |                       \
      ( data[6] << 8 ) |                        \
      ( data[7] ) )

#define SET648(val,data)                        \
    data[0] = (val) >> 32,                      \
    data[1] = (val)

#define GET648(data)                            \
    ( ( data[0] << 32 ) |                       \
      ( data[1] ) )

#else

#define SET2(val,data)                          \
    data[1] = (val) >> 8,                       \
    data[0] = (val)

#define GET2(data)                              \
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

#define SET648(val,data)                        \
    data[1] = (val) >> 32,                      \
    data[0] = (val)

#define GET648(data)                            \
    ( ( data[1] << 32 ) |                       \
      ( data[0] ) )

#endif

#define GROUP(type,size,get,set,sfx)                                    \
    Unaligned##sfx##size( type v ) { set##size( v, data ); }            \
    type operator = ( type v )     { set##size( v, data ); return v; }  \
    operator type()           { return get##size( data ); }             \
    type operator += (const type v) { set##size( get##size(data) + v, data); return *this; } \
    type operator -= (const type v) { set##size( get##size(data) - v, data); return *this; } \
    type operator <<= (const type v) { set##size( get##size(data) << v, data); return *this; } \
    type operator *= (const type v) { set##size( get##size(data) * v, data); return *this; } \
    type operator /= (const type v) { set##size( get##size(data) / v, data); return *this; } \
    type operator %= (const type v) { set##size( get##size(data) % v, data); return *this; } \
    type operator >>= (const type v) { set##size( get##size(data) >> v, data); return *this; } \
    type operator ^= (const type v) { set##size( get##size(data) ^ v, data); return *this; } \
    type operator &= (const type v) { set##size( get##size(data) & v, data); return *this; } \
    type operator |= (const type v) { set##size( get##size(data) | v, data); return *this; } \


#define MAKE_TEMPLATE(size,type,sfx)                            \
template<class BASE>                                            \
class Unaligned##sfx##size<BASE>                                \
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

MAKE_TEMPLATE( 2, unsigned char, );
MAKE_TEMPLATE( 4, unsigned char, );
MAKE_TEMPLATE( 8, unsigned char, );
MAKE_TEMPLATE( 8, unsigned int, 64 );

template<class BASE>
class UnalignedPtr<BASE> : Unaligned4<BASE> {
public:
	UnalignedPtr<BASE>(const void * v) : Unaligned4<BASE>((int)v) { }
	void *operator = (void * v)        { SET4((int)v,data); return v; }
	const void *operator = (const void * v)        { SET4((int)v,data); return v; }
	operator BASE()              { return (BASE)GET4(data); }
};

#undef SET2
#undef SET4
#undef SET8
#undef SET648
#undef GROUP
#undef MAKE_TEMPLATE

#define MAKE_UNALIGNED_TYPE(base,size)    typedef Unaligned##size<base>   base##_UNALIGNED
#define MAKE_UNALIGNED64_TYPE(base,size)  typedef Unaligned64##size<base> base##_UNALIGNED64
#define MAKE_UNALIGNEDPTR_TYPE(base)      typedef UnalignedPtr<base> *    base##_UNALIGNEDPTR

#endif  // _MSC_VER

// Predefine some of the basic types.

MAKE_UNALIGNED_TYPE( short, 2 );
MAKE_UNALIGNED_TYPE( int, 4 );
MAKE_UNALIGNED_TYPE( long, 4 );
MAKE_UNALIGNED_TYPE( WORD, 2 );
MAKE_UNALIGNED_TYPE( DWORD, 4 );
MAKE_UNALIGNED_TYPE( USHORT, 4 );
MAKE_UNALIGNED_TYPE( SHORT, 4 );
MAKE_UNALIGNED_TYPE( __int64, 8 );
MAKE_UNALIGNED64_TYPE( __int64, 8 );

#endif __unaligned_h__

