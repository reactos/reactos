// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------
//

//
//  Abstract:
//
//      Includes SAL defines not in the public sal.h.
//

#include <codeanalysis/sourceannotations.h>  // TEMPORARY INCLUDE

#if !defined(MIDL_PASS) && defined(_PREFAST_) && _MSC_VER >= 1000 

#ifndef __inner_range
#define __inner_range(lb,ub)      __declspec("SAL_range(" #lb "," #ub ")")
#endif

#ifndef __inexpressible_readableTo
#define __inexpressible_readableTo(size)  __declspec("SAL_readableTo(inexpressibleCount('" #size "'))")    
#endif

#ifndef __inexpressible_writableTo
#define __inexpressible_writableTo(size)  __declspec("SAL_writableTo(inexpressibleCount('" #size "'))")
#endif

#ifndef __out_range
#define __out_range(lb,ub)        __post __inner_range(lb,ub)
#endif

#ifndef __in_range
#define __in_range(lb,ub)                      __pre __inner_range(lb,ub)
#endif

#ifndef __range
#define __range(lb,ub)                      __inner_range(lb,ub)
#endif

#ifndef __deref_out_range
#define __deref_out_range(lb,ub)  __post __deref __inner_range(lb,ub)    
#endif

#ifndef __in_xcount
#define __in_xcount(x)            __in __pre __inexpressible_readableTo(size)
#endif

#ifndef __RPC_unique_pointer
#define __RPC_unique_pointer                            __maybenull
#endif

#ifndef __RPC__deref_out
#define __RPC__deref_out                            __deref_out
#endif

#ifndef __RPC__deref_out_opt
#define __RPC__deref_out_opt                        __RPC__deref_out
#endif

#ifndef __RPC__inout
#define __RPC__inout                                __inout
#endif

#ifndef __RPC__out
#define __RPC__out                                  __out
#endif

#ifndef __RPC__in
#define __RPC__in                                  __in
#endif

#ifndef __RPC__in_opt
#define __RPC__in_opt                               __RPC__in __pre __exceptthat __maybenull
#endif

#ifndef __RPC__in_ecount
#define __RPC__in_ecount(size)                      __RPC__in __pre __elem_readableTo(size)
#endif

#ifndef __RPC__in_ecount_full
#define __RPC__in_ecount_full(size)                 __RPC__in_ecount(size)
#endif

#ifndef __RPC__in_ecount_full_opt
#define __RPC__in_ecount_full_opt(size)             __RPC__in_ecount_full(size) __pre __exceptthat  __maybenull
#endif

#ifndef __RPC__inout_ecount_part_opt
#define __RPC__inout_ecount_part_opt(size, length) __inout_ecount_part_opt(size, length)
#endif

#ifndef __RPC__inout_opt
#define __RPC__inout_opt                            __inout_opt
#endif

#ifndef __RPC__inout_ecount_part
#define __RPC__inout_ecount_part(size, length)      __inout_ecount_part(size, length)
#endif

#ifndef __RPC__inout_ecount_full
#define __RPC__inout_ecount_full(size)              __RPC__inout_ecount_part(size, size)
#endif

#ifndef __RPC__deref_opt_inout_opt
#define __RPC__deref_opt_inout_opt                      __deref_opt_inout_opt
#endif

#ifndef __RPC__out_ecount
#define __RPC__out_ecount(size)                     __out_ecount(size)  __post  __elem_writableTo(size)
#endif

#ifndef __RPC__out_ecount_part
#define __RPC__out_ecount_part(size, length)        __RPC__out_ecount(size)  __post  __elem_readableTo(length)
#endif

#ifndef __RPC__out_ecount_full
#define __RPC__out_ecount_full(size)                __RPC__out_ecount_part(size, size)
#endif

#ifndef __deref_xcount
#define __deref_xcount(size)                                    __ecount(1) __post __elem_readableTo(1) __post __deref __notnull __post __deref __inexpressible_writableTo(size)
#endif

#ifndef __deref_out_xcount
#define __deref_out_xcount(size)                                __deref_xcount(size) __post __deref __valid __refparam
#endif

#ifndef __deref_out_xcount_opt
#define __deref_out_xcount_opt(size)                            __deref_out_xcount(size)                    __post __deref __exceptthat __maybenull
#endif

#ifndef __out_ecount_full
#define __out_ecount_full(size) __out_ecount(size)
#endif

#ifndef __in_ecount_opt
#define __in_ecount_opt(size) __in_ecount(size) __exceptthat __maybenull
#endif

#ifndef __in_opt
#define __in_opt __in __exceptthat __maybenull
#endif

#ifndef __out_ecount_part
#define __out_ecount_part(size,length) __out_ecount(size)
#endif

#ifndef __out_bcount_part
#define __out_bcount_part(size,length) __out_bcount(size)
#endif

#ifndef __out_ecount_opt
#define __out_ecount_opt(size) __out_ecount(size) __exceptthat __maybenull
#endif

#ifndef __field_ecount_opt
#define __field_ecount_opt(size)            __maybenull __elem_writableTo(size)
#endif

#ifndef __field_ecount
#define __field_ecount(size)                __notnull __elem_writableTo(size)
#endif

#ifndef __field_range
#define __field_range(lb,ub)                __range(lb,ub)
#endif

#else

#ifndef __out_range
#define __out_range(lb,ub)       
#endif

#ifndef __in_range
#define __in_range(lb,ub)
#endif


#ifndef __range
#define __range(lb,ub)
#endif

#ifndef __deref_out_range
#define __deref_out_range(lb,ub)      
#endif

#ifndef __in_xcount
#define __in_xcount(x)        
#endif

#ifndef __RPC__in_ecount_full
#define __RPC__in_ecount_full(size)
#endif

#ifndef __RPC__in_ecount_full_opt
#define __RPC__in_ecount_full_opt(size) 
#endif

#ifndef __RPC_unique_pointer
#define __RPC_unique_pointer
#endif

#ifndef __RPC__deref_out_opt
#define __RPC__deref_out_opt 
#endif

#ifndef __RPC__inout
#define __RPC__inout                                   
#endif

#ifndef __RPC__out
#define __RPC__out
#endif

#ifndef __RPC__in
#define __RPC__in
#endif

#ifndef __RPC__in_opt
#define __RPC__in_opt 
#endif

#ifndef __RPC__inout_ecount_part_opt
#define __RPC__inout_ecount_part_opt(size, length)
#endif

#ifndef __RPC__inout_opt
#define __RPC__inout_opt
#endif

#ifndef __RPC__inout_ecount_full
#define __RPC__inout_ecount_full(size)
#endif

#ifndef __RPC__deref_opt_inout_opt
#define __RPC__deref_opt_inout_opt
#endif

#ifndef __RPC__out_ecount_full
#define __RPC__out_ecount_full(size)
#endif

#ifndef __deref_xcount
#define __deref_xcount(size)
#endif

#ifndef __deref_out_xcount
#define __deref_out_xcount(size)
#endif

#ifndef __deref_out_xcount_opt
#define __deref_out_xcount_opt(size) 
#endif

#ifndef __in_ecount_opt
#define __in_ecount_opt(size) 
#endif

#ifndef __in_opt
#define __in_opt
#endif

#ifndef __out_ecount_part
#define __out_ecount_part(size,length) 
#endif

#ifndef __out_bcount_part
#define __out_bcount_part(size,length) 
#endif

#ifndef __out_ecount_opt
#define __out_ecount_opt(size) 
#endif

#ifndef __field_ecount_opt
#define __field_ecount_opt(size)  
#endif

#ifndef __field_ecount
#define __field_ecount(size) 
#endif

#ifndef __field_range
#define __field_range(lb,ub) 
#endif

#ifndef __deref_xcount
#define __deref_xcount(size)
#endif

#ifndef __out_ecount_full
#define __out_ecount_full(size) 
#endif

#endif




