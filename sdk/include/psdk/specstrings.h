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

#define __deref_in                          __in _Pre_ __deref __deref __readonly
#define __deref_in_ecount(size)             __deref_in _Pre_ __deref __elem_readableTo(size)
#define __deref_in_bcount(size)             __deref_in _Pre_ __deref __byte_readableTo(size)
#define __deref_in_opt                      __deref_in _Pre_ __deref __exceptthat __maybenull
#define __deref_in_ecount_opt(size)         __deref_in_ecount(size) _Pre_ __deref __exceptthat __maybenull
#define __deref_in_bcount_opt(size)         __deref_in_bcount(size) _Pre_ __deref __exceptthat __maybenull
#define __deref_opt_in                      __deref_in __exceptthat __maybenull
#define __deref_opt_in_ecount(size)         __deref_in_ecount(size) __exceptthat __maybenull
#define __deref_opt_in_bcount(size)         __deref_in_bcount(size) __exceptthat __maybenull
#define __deref_opt_in_opt                  __deref_in_opt __exceptthat __maybenull
#define __deref_opt_in_ecount_opt(size)     __deref_in_ecount_opt(size) __exceptthat __maybenull
#define __deref_opt_in_bcount_opt(size)     __deref_in_bcount_opt(size) __exceptthat __maybenull

#undef __nullnullterminated
#define __nullnullterminated                __inexpressible_readableTo("string terminated by two nulls") __nullterminated

#if (_MSC_VER >= 1000) && !defined(__midl) && defined(_PREFAST_)

#define __analysis_noreturn                 __declspec(noreturn)

#define __inner_data_source(src_raw)        _SA_annotes1(SAL_untrusted_data_source, src_raw)

#else // (_MSC_VER >= 1000) && !defined(__midl) && defined(_PREFAST_)

#define __analysis_noreturn

#define __inner_data_source(src_raw)

#endif // (_MSC_VER >= 1000) && !defined(__midl) && defined(_PREFAST_)

#define __field_ecount(size)                __notnull __elem_writableTo(size)
#define __field_bcount(size)                __notnull __byte_writableTo(size)

#define __field_bcount_full(size)           __notnull __byte_writableTo(size) __byte_readableTo(size)

#define __out_awcount(expr, size)           _Pre_ __notnull __byte_writableTo((expr) ? (size) : (size) * 2) _Post_ __valid __refparam
#define __in_awcount(expr, size)            _Pre_ __valid _Pre_ _Notref_ __deref __readonly __byte_readableTo((expr) ? (size) : (size) * 2)
#define __post_invalid                      _Post_ __notvalid

#define __in_data_source(src_sym)           _Pre_ __inner_data_source(#src_sym)
#define __out_data_source(src_sym)          _Post_ __inner_data_source(#src_sym)
#define __kernel_entry                      __inner_control_entrypoint(UserToKernel)

#include <driverspecs.h>
