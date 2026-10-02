// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//------------------------------------------------------------------------------
#pragma once

//
// ARRAY_SIZE uses NT provided type safe version
//
#define ARRAY_SIZE(A)   ARRAYSIZE(A)

//
// ARRAY_COMMA_ELEM_COUNT and ARRAY_COMMA_BYTE_COUNT are useful to prevent
// typos when calling a method that accepts a buffer parameter followed by
// its length or size, respectively.
//
#define ARRAY_COMMA_ELEM_COUNT(A) ((A)), ARRAYSIZE((A))
#define ARRAY_COMMA_BYTE_COUNT(A) ((A)), sizeof((A))


// Bit twiddling trick.  If f is non-zero, this will resolve to dwFlag.  If not,
// it will resolve to 0.
#define BOOLFLAG(f, dwFlag)  ((DWORD)(-(LONG)!!(f)) & (dwFlag))

// Compile time "Assert".  The compiler can't allocate an array of 0 elements.
// If the expression inside [ ] is false, the array size will be zero!
#define COMPILE_TIME_ASSERT(x,y)  typedef int _farf_##x[sizeof(x) == (y)]
#define COMPILE_TIME_ASSERT_1(x,y)  typedef int _farf_1_##x[(x) == (y)]
#define COMPILE_TIME_OFFSETOF_ASSERT(c1,c2,m)  typedef int _farf_2_##c1##m[(offsetof(c1,m)) == (offsetof(c2,m))]

// Put this in a private section and don't implement these methods and the
// linker will stop you from copying.  Doesn't work with template classes
#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&)

// The following macro reduces code by telling the compiler that the 'default'
// case in a switch statement should never be reached:
#if DBG
    #define NO_DEFAULT(Message) RIP(Message); __assume(0)
#else
    #define NO_DEFAULT(Message) __assume(0)
#endif


// Min and Max templates -------------------------------------------------------
// Warning, Arguments must be cast to same types for template instantiation

#ifdef min
#undef min
#endif

template < class T > inline T min ( T a, T b ) { return a < b ? a : b; }

#ifdef max
#undef max
#endif

template < class T > inline T max ( T a, T b ) { return a > b ? a : b; }


#define ReleaseInterface(x) if (x) {(x)->Release(); (x) = NULL; }
#define SetInterface(x,y) {(x)=(y); if (y) {(y)->AddRef();}}
#define ReplaceInterface(x,y) {if (x) {(x)->Release();} (x)=(y); if (y) {(y)->AddRef();}}

#define ReleaseInterfaceNoNULL(x) do { if (x) { (x)->Release(); } } while (UNCONDITIONAL_EXPR(false))

#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }


