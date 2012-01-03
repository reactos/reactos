/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#pragma once
#define SPECSTRINGS_H

#include <sal.h>

#define __field_bcount(size) __notnull __byte_writableTo(size)
#define __field_ecount(size) __notnull __elem_writableTo(size)


#define __deref_in
#define __deref_in_ecount(size)
#define __deref_in_bcount(size)
#define __deref_in_opt
#define __deref_in_ecount_opt(size)
#define __deref_in_bcount_opt(size)
#define __deref_opt_in
#define __deref_opt_in_ecount(size)
#define __deref_opt_in_bcount(size)
#define __deref_opt_in_opt
#define __deref_opt_in_ecount_opt(size)
#define __deref_opt_in_bcount_opt(size)
#define __out_awcount(expr,size)
#define __in_awcount(expr,size)
#define __nullnullterminated
#define __analysis_assume(expr)

//#endif

