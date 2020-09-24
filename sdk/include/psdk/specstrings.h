/*
 * specstrings.h
 *
 * Standard Annotation Language (SAL) definitions
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#define SPECSTRINGS_H

#include <sal.h>
#include <driverspecs.h>

#define __field_bcount(size) __notnull __byte_writableTo(size)
#define __field_bcount_full(size) __notnull __byte_writableTo(size) __byte_readableTo(size)
#define __field_ecount(size) __notnull __elem_writableTo(size)
#define __post_invalid _Post_ __notvalid

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
#define __in_data_source(src_sym)
#define __kernel_entry
#define __range(lb,ub)
#define __in_bound
#define __out_bound
#define __in_range(lb,ub)
#define __out_range(lb,ub)
#define __deref_in_range(lb,ub)
#define __deref_out_range(lb,ub)

#if (_MSC_VER >= 1000) && !defined(__midl) && defined(_PREFAST_)

#define __inner_data_source(src_raw)        _SA_annotes1(SAL_untrusted_data_source,src_raw)
#define __out_data_source(src_sym)          _Post_ __inner_data_source(#src_sym)
#define __analysis_noreturn __declspec(noreturn)

#else

#define __out_data_source(src_sym)
#define __analysis_noreturn

#endif

#if defined(_PREFAST_) && defined(_PFT_SHOULD_CHECK_RETURN)
#define _Check_return_opt_ _Check_return_
#else
#define _Check_return_opt_
#endif

#if defined(_PREFAST_) && defined(_PFT_SHOULD_CHECK_RETURN_WAT)
#define _Check_return_wat_ _Check_return_
#else
#define _Check_return_wat_
#endif
