// verify.h
// This code is (C) 2003-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#ifndef VERIFY_H
#define VERIFY_H

//#include <assert.h>

#ifdef ASSERT
#undef ASSERT
#endif//ASSERT

#include "panic.h"

#if defined(DEBUG) || defined(_DEBUG)
inline void AssertHandler ( bool b, const char* str )
{
	if ( !b )
		panic ( str );
}
#  define ASSERT(x) AssertHandler((x) ? true : false, #x )
#else
#  define ASSERT(x)
#endif

#ifdef verify
#undef verify
#endif//verify

#if defined(DEBUG) || defined(_DEBUG)
#  define verify(x) ASSERT(x)
#else
#  define verify(x) x
#endif

#endif//VERIFY_H
