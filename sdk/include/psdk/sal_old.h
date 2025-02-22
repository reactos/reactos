/*
 * sal_old.h
 *
 * Old style Standard Annotation Language (SAL) definitions
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
#define __specstrings

#ifdef __cplusplus
#ifndef __nothrow
#define __nothrow __declspec(nothrow)
#endif
#else
#ifndef __nothrow
#define __nothrow
#endif
#endif


#if defined(_PREFAST_) && !defined(__midl)

#define SPECSTRINGIZE(x) #x
#define __byte_readableTo(size) __declspec("SAL_readableTo(byteCount("SPECSTRINGIZE(size)"))")
#define __byte_writableTo(size) __declspec("SAL_writableTo(byteCount("SPECSTRINGIZE(size)"))")
#define __deref __declspec("SAL_deref")
#define __elem_readableTo(size) __declspec("SAL_readableTo(elementCount("SPECSTRINGIZE(size)"))")
#define __elem_writableTo(size) __declspec("SAL_writableTo(elementCount("SPECSTRINGIZE(size)"))")
#define __exceptthat __declspec("SAL_except")
#define __execeptthat __exceptthat
#define __inner_blocksOn(resource) __declspec("SAL_blocksOn("SPECSTRINGIZE(resource)")")
#define __inner_checkReturn __declspec("SAL_checkReturn")
#define __inner_control_entrypoint(category) __declspec("SAL_entrypoint(controlEntry, "SPECSTRINGIZE(category)")")
#define __inner_data_entrypoint(category) __declspec("SAL_entrypoint(dataEntry, "SPECSTRINGIZE(category)")")
#define __inner_fallthrough __FallThrough();
#define __inner_fallthrough_dec __inline __nothrow void __FallThrough() {}
#define __inner_override __declspec("__override")
#define __inner_success(expr) __declspec("SAL_success("SPECSTRINGIZE(expr)")")
#define __maybenull __declspec("SAL_maybenull")
#define __maybereadonly __declspec("SAL_maybereadonly")
#define __maybevalid __declspec("SAL_maybevalid")
#define __notnull __declspec("SAL_notnull")
#define __notreadonly __declspec("SAL_notreadonly")
#define __notvalid __declspec("SAL_notvalid")
#define __null __declspec("SAL_null")
#define __post __declspec("SAL_post")
#define __postcond(expr) __post
#define __pre __declspec("SAL_pre")
#define __precond(expr) __pre
#define __readableTo(extent) __declspec("SAL_readableTo("SPECSTRINGIZE(extent)")")
#define __readonly __declspec("SAL_readonly")
#define __refparam __deref __notreadonly
#define __valid __declspec("SAL_valid")
#define __writableTo(size) __declspec("SAL_writableTo("SPECSTRINGIZE(size)")")

#else

#define __byte_readableTo(size)
#define __byte_writableTo(size)
#define __deref
#define __elem_readableTo(size)
#define __elem_writableTo(size)
#define __exceptthat
#define __execeptthat
#define __inner_blocksOn(resource)
#define __inner_checkReturn
#define __inner_control_entrypoint(category)
#define __inner_data_entrypoint(category)
#define __inner_fallthrough
#define __inner_fallthrough_dec
#define __inner_override
#define __inner_success(expr)
#define __inner_typefix(ctype)
#define __maybenull
#define __maybereadonly
#define __maybevalid
#define __notnull
#define __notreadonly
#define __notvalid
#define __null
#define __post
#define __postcond(expr)
#define __pre
#define __precond(expr)
#define __readableTo(extent)
#define __readonly
#define __refparam
#define __valid
#define __writableTo(size)

#endif /* defined(_PREFAST_) && !defined(__midl) */

#define __bcount_opt(size) __bcount(size) __exceptthat __maybenull
#define __bcount(size) __notnull __byte_writableTo(size)
#define __blocksOn(resource) __inner_blocksOn(resource)
#define __callback __inner_callback
#define __checkReturn __inner_checkReturn
#define __control_entrypoint(category) __inner_control_entrypoint(category)
#define __data_entrypoint(category) __inner_data_entrypoint(category)
#define __deref_bcount_opt(size) __deref_bcount(size) __post __deref __exceptthat __maybenull
#define __deref_bcount(size) __ecount(1) __post __elem_readableTo(1) __post __deref __notnull __post __deref __byte_writableTo(size)
#define __deref_ecount_opt(size) __deref_ecount(size) __post __deref __exceptthat __maybenull
#define __deref_ecount(size) __ecount(1) __post __elem_readableTo(1) __post __deref __notnull __post __deref __elem_writableTo(size)
#define __deref_inout __notnull __elem_readableTo(1) __pre __deref __valid __post __deref __valid __refparam
#define __deref_inout_bcount_full_opt(size) __deref_inout_bcount_full(size) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_bcount_full(size) __deref_inout_bcount_part(size,size)
#define __deref_inout_bcount_nz_opt(size) __deref_inout_bcount_opt(size)
#define __deref_inout_bcount_nz(size) __deref_inout_ecount(size)
#define __deref_inout_bcount_opt(size) __deref_inout_bcount(size) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_bcount_part_opt(size,length) __deref_inout_bcount_part(size,length) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_bcount_part(size,length) __deref_inout_bcount(size) __pre __deref __byte_readableTo(length) __post __deref __byte_readableTo(length)
#define __deref_inout_bcount_z_opt(size) __deref_inout_bcount_opt(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_inout_bcount_z(size) __deref_inout_bcount(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_inout_bcount(size) __deref_inout __pre __deref __byte_writableTo(size) __post __deref __byte_writableTo(size)
#define __deref_inout_ecount_full_opt(size) __deref_inout_ecount_full(size) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_ecount_full(size) __deref_inout_ecount_part(size,size)
#define __deref_inout_ecount_nz_opt(size) __deref_inout_ecount_opt(size)
#define __deref_inout_ecount_nz(size) __deref_inout_ecount(size)
#define __deref_inout_ecount_opt(size) __deref_inout_ecount(size) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_ecount_part_opt(size,length) __deref_inout_ecount_part(size,length) __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_ecount_part(size,length) __deref_inout_ecount(size) __pre __deref __elem_readableTo(length) __post __deref __elem_readableTo(length)
#define __deref_inout_ecount_z_opt(size) __deref_inout_ecount_opt(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_inout_ecount_z(size) __deref_inout_ecount(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_inout_ecount(size) __deref_inout __pre __deref __elem_writableTo(size) __post __deref __elem_writableTo(size)
#define __deref_inout_nz __deref_inout
#define __deref_inout_nz_opt __deref_inout_opt
#define __deref_inout_opt __deref_inout __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull
#define __deref_inout_z __deref_inout __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_inout_z_opt __deref_inout_opt __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_bcount_opt(size) __deref_bcount_opt(size) __exceptthat __maybenull
#define __deref_opt_bcount(size) __deref_bcount(size) __exceptthat __maybenull
#define __deref_opt_ecount_opt(size) __deref_ecount_opt(size) __exceptthat __maybenull
#define __deref_opt_ecount(size) __deref_ecount(size) __exceptthat __maybenull
#define __deref_opt_inout __deref_inout __exceptthat __maybenull
#define __deref_opt_inout_bcount_full_opt(size) __deref_inout_bcount_full_opt(size) __exceptthat __maybenull
#define __deref_opt_inout_bcount_full(size) __deref_inout_bcount_full(size) __exceptthat __maybenull
#define __deref_opt_inout_bcount_nz_opt(size) __deref_opt_inout_bcount_opt(size)
#define __deref_opt_inout_bcount_nz(size) __deref_opt_inout_bcount(size)
#define __deref_opt_inout_bcount_opt(size) __deref_inout_bcount_opt(size) __exceptthat __maybenull
#define __deref_opt_inout_bcount_part_opt(size,length) __deref_inout_bcount_part_opt(size,length) __exceptthat __maybenull
#define __deref_opt_inout_bcount_part(size,length) __deref_inout_bcount_part(size,length) __exceptthat __maybenull
#define __deref_opt_inout_bcount_z_opt(size) __deref_opt_inout_bcount_opt(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_inout_bcount_z(size) __deref_opt_inout_bcount(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_inout_bcount(size) __deref_inout_bcount(size) __exceptthat __maybenull
#define __deref_opt_inout_ecount_full_opt(size) __deref_inout_ecount_full_opt(size) __exceptthat __maybenull
#define __deref_opt_inout_ecount_full(size) __deref_inout_ecount_full(size) __exceptthat __maybenull
#define __deref_opt_inout_ecount_nz_opt(size) __deref_opt_inout_ecount_opt(size)
#define __deref_opt_inout_ecount_nz(size) __deref_opt_inout_ecount(size)
#define __deref_opt_inout_ecount_opt(size) __deref_inout_ecount_opt(size) __exceptthat __maybenull
#define __deref_opt_inout_ecount_part_opt(size,length) __deref_inout_ecount_part_opt(size,length) __exceptthat __maybenull
#define __deref_opt_inout_ecount_part(size,length) __deref_inout_ecount_part(size,length) __exceptthat __maybenull
#define __deref_opt_inout_ecount_z_opt(size) __deref_opt_inout_ecount_opt(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_inout_ecount_z(size) __deref_opt_inout_ecount(size) __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_inout_ecount(size) __deref_inout_ecount(size) __exceptthat __maybenull
#define __deref_opt_inout_nz __deref_opt_inout
#define __deref_opt_inout_nz_opt __deref_opt_inout_opt
#define __deref_opt_inout_opt __deref_inout_opt __exceptthat __maybenull
#define __deref_opt_inout_z __deref_opt_inout __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_inout_z_opt __deref_opt_inout_opt __pre __deref __nullterminated __post __deref __nullterminated
#define __deref_opt_out __deref_out __exceptthat __maybenull
#define __deref_opt_out_bcount_full_opt(size) __deref_out_bcount_full_opt(size) __exceptthat __maybenull
#define __deref_opt_out_bcount_full(size) __deref_out_bcount_full(size) __exceptthat __maybenull
#define __deref_opt_out_bcount_nz_opt(size) __deref_opt_out_bcount_opt(size)
#define __deref_opt_out_bcount_opt(size) __deref_out_bcount_opt(size) __exceptthat __maybenull
#define __deref_opt_out_bcount_part_opt(size,length) __deref_out_bcount_part_opt(size,length) __exceptthat __maybenull
#define __deref_opt_out_bcount_part(size,length) __deref_out_bcount_part(size,length) __exceptthat __maybenull
#define __deref_opt_out_bcount_z_opt(size) __deref_opt_out_bcount_opt(size) __post __deref __nullterminated
#define __deref_opt_out_bcount(size) __deref_out_bcount(size) __exceptthat __maybenull
#define __deref_opt_out_ecount_full_opt(size) __deref_out_ecount_full_opt(size) __exceptthat __maybenull
#define __deref_opt_out_ecount_full(size) __deref_out_ecount_full(size) __exceptthat __maybenull
#define __deref_opt_out_ecount_nz_opt(size) __deref_opt_out_ecount_opt(size)
#define __deref_opt_out_ecount_opt(size) __deref_out_ecount_opt(size) __exceptthat __maybenull
#define __deref_opt_out_ecount_part_opt(size,length) __deref_out_ecount_part_opt(size,length) __exceptthat __maybenull
#define __deref_opt_out_ecount_part(size,length) __deref_out_ecount_part(size,length) __exceptthat __maybenull
#define __deref_opt_out_ecount_z_opt(size) __deref_opt_out_ecount_opt(size) __post __deref __nullterminated
#define __deref_opt_out_ecount(size) __deref_out_ecount(size) __exceptthat __maybenull
#define __deref_opt_out_nz_opt __deref_opt_out_opt
#define __deref_opt_out_opt __deref_out_opt __exceptthat __maybenull
#define __deref_opt_out_z __deref_opt_out __post __deref __nullterminated
#define __deref_opt_out_z_opt __post __deref __valid __refparam __exceptthat __maybenull __pre __deref __exceptthat __maybenull __post __deref __exceptthat __maybenull __post __deref __nullterminated
#define __deref_out __deref_ecount(1) __post __deref __valid __refparam
#define __deref_out_bcount_full_opt(size) __deref_out_bcount_full(size) __post __deref __exceptthat __maybenull
#define __deref_out_bcount_full(size) __deref_out_bcount_part(size,size)
#define __deref_out_bcount_nz_opt(size) __deref_out_bcount_opt(size)
#define __deref_out_bcount_nz(size) __deref_out_ecount(size)
#define __deref_out_bcount_opt(size) __deref_out_bcount(size) __post __deref __exceptthat __maybenull
#define __deref_out_bcount_part_opt(size,length) __deref_out_bcount_part(size,length) __post __deref __exceptthat __maybenull
#define __deref_out_bcount_part(size,length) __deref_out_bcount(size) __post __deref __byte_readableTo(length)
#define __deref_out_bcount_z_opt(size) __deref_out_bcount_opt(size) __post __deref __nullterminated
#define __deref_out_bcount_z(size) __deref_out_ecount(size) __post __deref __nullterminated
#define __deref_out_bcount(size) __deref_bcount(size) __post __deref __valid __refparam
#define __deref_out_ecount_full_opt(size) __deref_out_ecount_full(size) __post __deref __exceptthat __maybenull
#define __deref_out_ecount_full(size) __deref_out_ecount_part(size,size)
#define __deref_out_ecount_nz_opt(size) __deref_out_ecount_opt(size)
#define __deref_out_ecount_nz(size) __deref_out_ecount(size)
#define __deref_out_ecount_opt(size) __deref_out_ecount(size) __post __deref __exceptthat __maybenull
#define __deref_out_ecount_part_opt(size,length) __deref_out_ecount_part(size,length) __post __deref __exceptthat __maybenull
#define __deref_out_ecount_part(size,length) __deref_out_ecount(size) __post __deref __elem_readableTo(length)
#define __deref_out_ecount_z_opt(size) __deref_out_ecount_opt(size) __post __deref __nullterminated
#define __deref_out_ecount_z(size) __deref_out_ecount(size) __post __deref __nullterminated
#define __deref_out_ecount(size) __deref_ecount(size) __post __deref __valid __refparam
#define __deref_out_nz __deref_out
#define __deref_out_nz_opt __deref_out_opt
#define __deref_out_range(x,y) /* FIXME */
#define __deref_out_opt __deref_out __post __deref __exceptthat __maybenull
#define __deref_out_z __post __deref __valid __refparam __post __deref __nullterminated
#define __deref_out_z_opt __post __deref __valid __refparam __execeptthat __maybenull __post __deref __nullterminated
#define __ecount_opt(size) __ecount(size) __exceptthat __maybenull
#define __ecount(size) __notnull __elem_writableTo(size)
#define __format_string
#define __in __pre __valid __pre __deref __readonly
#define __in_bcount_nz_opt(size) __in_bcount_opt(size)
#define __in_bcount_nz(size) __in_bcount(size)
#define __in_bcount_opt(size) __in_bcount(size) __exceptthat __maybenull
#define __in_bcount_z_opt(size) __in_bcount_opt(size) __pre __nullterminated
#define __in_bcount_z(size) __in_bcount(size) __pre __nullterminated
#define __in_bcount(size) __in __pre __byte_readableTo(size)
#define __in_ecount_nz_opt(size) __in_ecount_opt(size)
#define __in_ecount_nz(size) __in_ecount(size)
#define __in_ecount_opt(size) __in_ecount(size) __exceptthat __maybenull
#define __in_ecount_z_opt(size) __in_ecount_opt(size) __pre __nullterminated
#define __in_ecount_z(size) __in_ecount(size) __pre __nullterminated
#define __in_ecount(size) __in __pre __elem_readableTo(size)
#define __in_nz __in
#define __in_nz_opt __in_opt
#define __in_opt __in __exceptthat __maybenull
#define __in_z __in __pre __nullterminated
#define __in_z_opt __in_opt __pre __nullterminated
#define __inout __pre __valid __post __valid __refparam
#define __inout_bcount_full_opt(size) __inout_bcount_full(size) __exceptthat __maybenull
#define __inout_bcount_full(size) __inout_bcount_part(size,size)
#define __inout_bcount_nz_opt(size) __inout_bcount_opt(size)
#define __inout_bcount_nz(size) __inout_bcount(size)
#define __inout_bcount_opt(size) __inout_bcount(size) __exceptthat __maybenull
#define __inout_bcount_part_opt(size,length) __inout_bcount_part(size,length) __exceptthat __maybenull
#define __inout_bcount_part(size,length) __out_bcount_part(size,length) __pre __valid __pre __byte_readableTo(length)
#define __inout_bcount_z_opt(size) __inout_bcount_opt(size)
#define __inout_bcount_z(size) __inout_bcount(size) __pre __nullterminated __post __nullterminated
#define __inout_bcount(size) __out_bcount(size) __pre __valid
#define __inout_ecount_full_opt(size) __inout_ecount_full(size) __exceptthat __maybenull
#define __inout_ecount_full(size) __inout_ecount_part(size,size)
#define __inout_ecount_nz_opt(size) __inout_ecount_opt(size)
#define __inout_ecount_nz(size) __inout_ecount(size)
#define __inout_ecount_opt(size) __inout_ecount(size) __exceptthat __maybenull
#define __inout_ecount_part_opt(size,length) __inout_ecount_part(size,length) __exceptthat __maybenull
#define __inout_ecount_part(size,length) __out_ecount_part(size,length) __pre __valid __pre __elem_readableTo(length)
#define __inout_ecount_z_opt(size) __inout_ecount_opt(size) __pre __nullterminated __post __nullterminated
#define __inout_ecount_z(size) __inout_ecount(size) __pre __nullterminated __post __nullterminated
#define __inout_ecount(size) __out_ecount(size) __pre __valid
#define __inout_nz __inout
#define __inout_nz_opt __inout_opt
#define __inout_opt __inout __exceptthat __maybenull
#define __inout_z __inout __pre __nullterminated __post __nullterminated
#define __inout_z_opt __inout_opt __pre __nullterminated __post __nullterminated
#define __nullnullterminated
#define __nullterminated __readableTo(sentinel(0))
#define __out __ecount(1) __post __valid __refparam
#define __out_bcount_full_opt(size) __out_bcount_full(size) __exceptthat __maybenull
#define __out_bcount_full_z_opt(size) __out_bcount_full_opt(size) __post __nullterminated
#define __out_bcount_full_z(size) __out_bcount_full(size) __post __nullterminated
#define __out_bcount_full(size) __out_bcount_part(size,size)
#define __out_bcount_nz_opt(size) __out_bcount_opt(size) __post __nullterminated
#define __out_bcount_nz(size) __bcount(size) __post __valid __refparam
#define __out_bcount_opt(size) __out_bcount(size) __exceptthat __maybenull
#define __out_bcount_part_opt(size,length) __out_bcount_part(size,length) __exceptthat __maybenull
#define __out_bcount_part_z_opt(size,length) __out_bcount_part_opt(size,length) __post __nullterminated
#define __out_bcount_part_z(size,length) __out_bcount_part(size,length) __post __nullterminated
#define __out_bcount_part(size,length) __out_bcount(size) __post __byte_readableTo(length)
#define __out_bcount_z_opt(size) __out_bcount_opt(size) __post __nullterminated
#define __out_bcount_z(size) __bcount(size) __post __valid __refparam __post __nullterminated
#define __out_bcount(size) __bcount(size) __post __valid __refparam
#define __out_ecount_full_opt(size) __out_ecount_full(size) __exceptthat __maybenull
#define __out_ecount_full_z_opt(size) __out_ecount_full_opt(size) __post __nullterminated
#define __out_ecount_full_z(size) __out_ecount_full(size) __post __nullterminated
#define __out_ecount_full(size) __out_ecount_part(size,size)
#define __out_ecount_nz_opt(size) __out_ecount_opt(size) __post __nullterminated
#define __out_ecount_nz(size) __ecount(size) __post __valid __refparam
#define __out_ecount_opt(size) __out_ecount(size) __exceptthat __maybenull
#define __out_ecount_part_opt(size,length) __out_ecount_part(size,length) __exceptthat __maybenull
#define __out_ecount_part_z_opt(size,length) __out_ecount_part_opt(size,length) __post __nullterminated
#define __out_ecount_part_z(size,length) __out_ecount_part(size,length) __post __nullterminated
#define __out_ecount_part(size,length) __out_ecount(size) __post __elem_readableTo(length)
#define __out_ecount_z_opt(size) __out_ecount_opt(size) __post __nullterminated
#define __out_ecount_z(size) __ecount(size) __post __valid __refparam __post __nullterminated
#define __out_ecount(size) __ecount(size) __post __valid __refparam
#define __out_nz __post __valid __refparam __post
#define __out_nz_opt __post __valid __refparam __post __exceptthat __maybenull
#define __out_opt __out __exceptthat __maybenull
#define __out_z __post __valid __refparam __post __nullterminated
#define __out_z_opt __post __valid __refparam __post __nullterminated __exceptthat __maybenull
#define __override __inner_override
#define __reserved __pre __null
#define __success(expr) __inner_success(expr)
#define __typefix(ctype) __inner_typefix(ctype)

#ifndef __fallthrough
__inner_fallthrough_dec
#define __fallthrough __inner_fallthrough
#endif

