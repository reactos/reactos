/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Standard Annotation Language (SAL) definitions - GCC overrides
 * COPYRIGHT:   2021 - Jérôme Gardou
 */

#pragma once

#ifndef __GNUC__
#error "Not for your compiler!"
#endif

#ifndef __has_attribute
#pragma GCC warning "GCC without __has_attribute, no SAL niceties for you"
#define __has_attribute(__x) 0
#endif

#ifndef _GCC_NO_SAL_ATTRIIBUTES
#if __has_attribute(warn_unused_result)
# undef _Must_inspect_result_
/* FIXME: Not really equivalent */
# define _Must_inspect_result_ __attribute__((__warn_unused_result__))
# undef _Check_return_
/* This one is 1:1 equivalent */
# define _Check_return_ __attribute__((__warn_unused_result__))
#endif
#endif // _GCC_NO_SAL_ATTRIIBUTES
