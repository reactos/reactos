// panic.h
// This file is (C) 2003-2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifndef PANIC_H
#define PANIC_H

void panic ( const char* format, ... );

#define suAssert(expr) if ( !(expr) ) panic ( "%s(%lu): SOCKET ERROR %s\nExpression: %s\n", __FILE__, __LINE__, suErrDesc(SUERRNO), #expr )

#if defined(DEBUG) || defined(_DEBUG)
#  define suVerify(expr) suAssert(expr)
#else
#  define suVerify(expr) expr
#endif

#endif//PANIC_H
