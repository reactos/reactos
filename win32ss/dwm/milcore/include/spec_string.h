// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Abstract:        Additional SAL macros not provided by SpecStrings.h
//

#pragma once
#if !defined(__midl) && defined(_PREFAST_) && _MSC_VER >= 1000

// ---------------------------------------------------------------------------
// In Out Parameter Annotations with Different Opt Specifications
#define __deref_inout_ecount_inopt(size)    __deref_inout_ecount(size)    __pre __deref __exceptthat __maybenull
#define __deref_inout_ecount_outopt(size)   __deref_inout_ecount(size)    __post __deref __exceptthat __maybenull


// ---------------------------------------------------------------------------
// Readonly Result Buffer Annotations
//
// ***NOTE***: Pre-fix/fast does not currently support the notion of returning
// a read-only buffer. When it does, we can change the definition of __returnro
// appropriately.
//
// _returnro/_outro annoations are similar to _out annotations except the
// caller may not modify the buffer post call.
//
// These annotations are often useful when the callee is providing special
// access via return to a buffer.  Such buffers are often still owned by the
// callee and attention should be paid to their use in the caller.
//
// ---------------------------------------------------------------------------
// Readonly Result Buffer Annotation Examples
//
//     __returnro RECT const *GetRectRef(); bool
//     GetRectRef(__deref_outro_ecount(1) RECT const **ppOut);
//
//          The caller would have readonly access to a RECT provided by
//          GetRectRef, and quite likey GetRectRef still controls the lifetime
//          of the buffer although the annotation doesn't specify either way.
//
// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Helper Macro Definitions
//
// These express behavior common to many of the high-level annotations.
// DO NOT USE THESE IN YOUR CODE.
// ---------------------------------------------------------------------------
//
// __pre commented out to work around prefast assert bug.
#define __inner_pre_only_writableTo(sizeType, size)  /*__pre*/ __##sizeType##_writableTo(size)

// ---------------------------------------------------------------------------
//
// Readonly result buffers of multiple elements/bytes
// ---------------------------------------------------------------------------

#define __returnro_ecount(esize)       __notnull __post __valid __post __elem_readableTo(esize)
#define __returnro_bcount(bsize)       __notnull __post __valid __post __byte_readableTo(bsize)
#define __returnro_xcount(xsize)       __notnull __post __valid __post __inexpressible_readableTo(xsize)

#define __returnro_ecount_opt(esize)   __returnro_ecount(esize)   __exceptthat __maybenull
#define __returnro_bcount_opt(bsize)   __returnro_bcount(bsize)   __exceptthat __maybenull
#define __returnro_xcount_opt(xsize)   __returnro_xcount(xsize)   __exceptthat __maybenull

#define __outro_ecount(esize)       __notnull __inner_pre_only_writableTo(elem, esize) __post __valid __post __elem_readableTo(esize)
#define __outro_bcount(bsize)       __notnull __inner_pre_only_writableTo(byte, bsize) __post __valid __post __byte_readableTo(bsize)
#define __outro_xcount(xsize)       __notnull __inner_pre_only_writableTo(inexpressible, xsize) __post __valid __post __inexpressible_readableTo(xsize)

#define __outro_ecount_opt(esize)   __outro_ecount(esize)   __exceptthat __maybenull
#define __outro_bcount_opt(bsize)   __outro_bcount(bsize)   __exceptthat __maybenull
#define __outro_xcount_opt(xsize)   __outro_xcount(xsize)   __exceptthat __maybenull

#define __deref_outro_ecount(esize)       __ecount(1)  __post __elem_readableTo(1)  __post __deref __notnull  __post __deref __valid  __post __deref __inner_pre_only_writableTo(elem, esize)
#define __deref_outro_bcount(bsize)       __ecount(1)  __post __elem_readableTo(1)  __post __deref __notnull  __post __deref __valid  __post __deref __inner_pre_only_writableTo(byte, bsize)
#define __deref_outro_xcount(xsize)       __ecount(1)  __post __elem_readableTo(1)  __post __deref __notnull  __post __deref __valid  __post __deref __inner_pre_only_writableTo(inexpressible, xsize)

#define __deref_outro_ecount_opt(esize)   __deref_outro_ecount(esize)   __post __deref __exceptthat __maybenull
#define __deref_outro_bcount_opt(bsize)   __deref_outro_bcount(bsize)   __post __deref __exceptthat __maybenull
#define __deref_outro_xcount_opt(xsize)   __deref_outro_xcount(xsize)   __post __deref __exceptthat __maybenull


// ---------------------------------------------------------------------------
//
// Readonly result buffers of single elements - implicit _ecount(1)
//

#define __returnro                  __returnro_ecount(1)
#define __returnro_opt              __returnro              __exceptthat __maybenull
#define __outro                     __outro_ecount(1)
#define __outro_opt                 __outro                 __exceptthat __maybenull



// ---------------------------------------------------------------------------
// Pointer Buffer Annotations
//
// _pcount annotations are similar to _deref except that that instead of
// and implict single pointer (or reference) _pcount has an explicit count of
// pointers (multiple references are not allowed by the C++ language).
//
// A common need for _pcount annotation is multidimensional arrays, where the
// structure consists of an array of pointers each of which points to an
// array of elements.  In this case psize would represent the first order
// dimension to the array and esize or bsize would represnt the second order
// dimension to yield a  psize x esize  or  psize x bsize  array, respectively.
//
// The annotation breakdown table now has pcount added to the Level compoment.
//
// |-------------|------------|---------|--------|----------|---------------|
// |   Level     |   Usage    |  Size   | Output | Optional |  Parameters   |
// |-------------|------------|---------|--------|----------|---------------|
// | <>          | <>         | <>      | <>     | <>       | <>            |
// | _deref      | _in        | _ecount | _full  | _opt     | (size)        |
// | _deref_opt  | _out       | _bcount | _part  |          | (size,length) |
// | _pcount     | _inout     |         |        |          |               |
// | _pcount_opt |            |         |        |          |               |
// |             |            |         |        |          |               |
// |-------------|------------|---------|--------|----------|---------------|
//
// Level: Describes the buffer pointer's level of indirection from the
//          parameter or return value 'p'.
//
// <>          : p is the buffer pointer.
// _deref      : *p is the buffer pointer. p must not be NULL.
// _deref_opt  : *p may be the buffer pointer. p may be NULL, in which case
//                the rest of the annotation is ignored.
// _pcount     : *p is the first point in the buffer of psize pointers.
//                p must not be NULL.
// _pcount_opt : *p may be the first point in the buffer of psize pointers.
//                p may be NULL, in which case the rest of the annotation
//                is ignored.
//
//   One difference from _deref is that _pcount will have an explicit usage
//   prefix of _in, _out, or _inout; whereas _deref has an implicit usage
//   derived from the remainder of the annotation.
// 
//   The remaining portion of the annotations apply to each pointer in the
//   buffer of pointers.
//
// Note because validity of pointer types are understood fully what would be
// annotated as valid for a non-pointer type is explicitly annotated as
// notnull for the pointers in a buffer of pointers.
//
// Simplified _pcount annotations in the absence of an _ecount or _bcount
// there is an implied _ecount(1).  These simplified _pcount annotations
// should be used when there is a buffer of pointers, but the pointers
// themselves do reference anything more complex than a single element.
//
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// Helper Macro Definitions
//
// These express behavior common to many of the high-level annotations.
// DO NOT USE THESE IN YOUR CODE.
// ---------------------------------------------------------------------------

#define __inner_in_pcount(psize)          __in_ecount(psize)  __pre __deref __notnull
#define __inner_in_pcount_opt(psize)      __in_ecount_opt(psize)  __pre __deref __notnull

#define __inner_inout_pcount(psize)       __inout_ecount(psize)  __pre __deref __notnull  __post __deref __valid


// ---------------------------------------------------------------------------
//
// In buffer of pointers to buffers of elements
//

//  Nothing optional

#define __in_pcount_in_ecount(psize,esize)                      __inner_in_pcount(psize)  __pre __deref __elem_readableTo(esize)  __pre __deref __valid  __pre __deref __deref __readonly

#define __in_pcount_ecount(psize,esize)                         __inner_in_pcount(psize)  __pre __deref __elem_writableTo(esize)

#define __in_pcount_out_ecount(psize,esize)                     __in_pcount_ecount(psize,esize)  __post __deref __valid
#define __in_pcount_out_ecount_part(psize,esize,elength)        __in_pcount_out_ecount(psize,esize)  __post __deref __elem_readableTo(elength)
#define __in_pcount_out_ecount_full(psize,esize)                __in_pcount_out_ecount_part(psize,esize,esize)

#define __in_pcount_inout_ecount(psize,esize)                   __in_pcount_out_ecount(psize,esize)  __pre __deref __valid
#define __in_pcount_inout_ecount_part(psize,esize,elength)      __in_pcount_out_ecount_part(psize,esize,elength)  __pre __deref __valid  __pre __deref __elem_readableTo(elength)
#define __in_pcount_inout_ecount_full(psize,esize)              __in_pcount_inout_ecount_part(psize,esize,esize)

//  Dereferenced buffer(s) are optional

#define __in_pcount_in_ecount_opt(psize,esize)                  __in_pcount_in_ecount(psize,esize)                  __deref __exceptthat __maybenull

#define __in_pcount_ecount_opt(psize,esize)                     __in_pcount_ecount(psize,esize)                     __deref __exceptthat __maybenull

#define __in_pcount_out_ecount_opt(psize,esize)                 __in_pcount_out_ecount(psize,esize)                 __deref __exceptthat __maybenull
#define __in_pcount_out_ecount_part_opt(psize,esize,elength)    __in_pcount_out_ecount_part(psize,esize,elength)    __deref __exceptthat __maybenull
#define __in_pcount_out_ecount_full_opt(psize,esize)            __in_pcount_out_ecount_full(psize,esize)            __deref __exceptthat __maybenull

#define __in_pcount_inout_ecount_opt(psize,esize)               __in_pcount_inout_ecount(psize,esize)               __deref __exceptthat __maybenull
#define __in_pcount_inout_ecount_part_opt(psize,esize,elength)  __in_pcount_inout_ecount_part(psize,esize,elength)  __deref __exceptthat __maybenull
#define __in_pcount_inout_ecount_full_opt(psize,esize)          __in_pcount_inout_ecount_full(psize,esize)          __deref __exceptthat __maybenull

//  Buffer of pointers is optional

// ...
#define __in_pcount_opt_ecount(psize,esize)                     __in_pcount_opt(psize)  __pre __deref __elem_writableTo(esize)
// ...

//  Everything is optional

// ...
#define __in_pcount_opt_ecount_opt(psize,esize)                 __in_pcount_opt_ecount(psize,esize)  __deref __exceptthat __maybenull
// ...


// ---------------------------------------------------------------------------
//
// In buffer of pointers to single elements
//

#define __in_pcount_in(psize)                                   __in_pcount_in_ecount(psize,1)
#define __in_pcount(psize)                                      __in_pcount_ecount(psize,1)
#define __in_pcount_out(psize)                                  __in_pcount_out_ecount_full(psize,1)
#define __in_pcount_inout(psize)                                __in_pcount_inout_ecount_full(psize,1)

#define __in_pcount_in_opt(psize)                               __in_pcount_in_ecount_opt(psize,1)
#define __in_pcount_out_opt(psize)                              __in_pcount_out_ecount_full_opt(psize,1)
#define __in_pcount_inout_opt(psize)                            __in_pcount_inout_ecount_full_opt(psize,1)

#define __in_pcount_opt_in(psize)                               __in_pcount_in_ecount(psize,1)              __exceptthat __maybenull
#define __in_pcount_opt(psize)                                  __in_pcount_ecount(psize,1)                 __exceptthat __maybenull
#define __in_pcount_opt_out(psize)                              __in_pcount_out_ecount_full(psize,1)        __exceptthat __maybenull
#define __in_pcount_opt_inout(psize)                            __in_pcount_inout_ecount_full(psize,1)      __exceptthat __maybenull

#define __in_pcount_opt_in_opt(psize)                           __in_pcount_in_ecount_opt(psize,1)          __exceptthat __maybenull
#define __in_pcount_opt_out_opt(psize)                          __in_pcount_out_ecount_full_opt(psize,1)    __exceptthat __maybenull
#define __in_pcount_opt_inout_opt(psize)                        __in_pcount_inout_ecount_full_opt(psize,1)  __exceptthat __maybenull


// ---------------------------------------------------------------------------
//
// In buffer of pointers to buffers of bytes
//

//  Nothing optional

#define __in_pcount_in_bcount(psize,bsize)                      __inner_in_pcount(psize)  __pre __deref __byte_readableTo(bsize)  __pre __deref __valid  __pre __deref __deref __readonly

#define __in_pcount_bcount(psize,bsize)                         __inner_in_pcount(psize)  __pre __deref __byte_writableTo(bsize)

#define __in_pcount_out_bcount(psize,bsize)                     __in_pcount_bcount(psize,bsize)  __post __deref __valid
#define __in_pcount_out_bcount_part(psize,bsize,blength)        __in_pcount_out_bcount(psize,bsize)  __post __deref __byte_readableTo(blength)
#define __in_pcount_out_bcount_full(psize,bsize)                __in_pcount_out_bcount_part(psize,bsize,bsize)

#define __in_pcount_inout_bcount(psize,bsize)                   __in_pcount_out_bcount(psize,bsize)  __pre __deref __valid
#define __in_pcount_inout_bcount_part(psize,bsize,blength)      __in_pcount_out_bcount_part(psize,bsize,blength)  __pre __deref __valid  __pre __deref __byte_readableTo(blength)
#define __in_pcount_inout_bcount_full(psize,bsize)              __in_pcount_inout_bcount_part(psize,bsize,bsize)


// ...
#define __in_pcount_bcount_opt(psize,bsize)                     __in_pcount_bcount(psize,bsize)      __deref __exceptthat __maybenull
#define __in_pcount_opt_bcount(psize,bsize)                     __in_pcount_opt(psize)  __pre __deref __byte_writableTo(bsize)
#define __in_pcount_opt_bcount_opt(psize,bsize)                 __in_pcount_opt_bcount(psize,bsize)  __deref __exceptthat __maybenull
// ...


// ---------------------------------------------------------------------------
//
// Out buffer of pointers to buffers of elements
//

// Dereferenced buffers are not initialized (just allocated)

#define __out_pcount_ecount(psize,esize)                                __ecount(psize)  __post __deref __notnull  __post __deref __elem_writableTo(esize)
#define __out_pcount_part_ecount(psize,plength,esize)                   __out_pcount_ecount(psize,esize) __post __elem_readableTo(plength)
#define __out_pcount_full_ecount(psize,esize)                           __out_pcount_part_ecount(psize,psize,esize)

// Dereferenced buffers are initialized

#define __out_pcount_out_ecount(psize,esize)                            __out_pcount_ecount(psize,esize)  __post __deref __valid
#define __out_pcount_out_ecount_part(psize,esize,elength)               __out_pcount_out_ecount(psize,esize)  __post __deref __elem_readableTo(elength)
#define __out_pcount_out_ecount_full(psize,esize)                       __out_pcount_out_ecount_part(psize,esize,esize)

#define __out_pcount_part_out_ecount(psize,plength,esize)               __out_pcount_part_ecount(psize,plength,esize) __post __deref __valid
#define __out_pcount_part_out_ecount_part(psize,plength,esize,elength)  __out_pcount_part_out_ecount(psize,plength,esize) __post __deref __elem_readableTo(elength)
#define __out_pcount_part_out_ecount_full(psize,plength,esize)          __out_pcount_part_out_ecount_part(psize,plength,esize,esize)

#define __out_pcount_full_out_ecount(psize,esize)                       __out_pcount_part_out_ecount(psize,psize,esize)
#define __out_pcount_full_out_ecount_part(psize,esize,elength)          __out_pcount_part_out_ecount_part(psize,psize,esize,elength)
#define __out_pcount_full_out_ecount_full(psize,esize)                  __out_pcount_part_out_ecount_full(psize,psize,esize)


// ---------------------------------------------------------------------------
//
// Out buffer of pointers to single elements
//

// Dereferenced elements are not initialized (just allocated)

#define __out_pcount(psize)                                             __out_pcount_ecount(psize,1)
#define __out_pcount_part(psize,plength)                                __out_pcount_part_ecount(psize,plength,1)
#define __out_pcount_full(psize)                                        __out_pcount_part(psize,psize)

// Dereferenced elements are initialized

#define __out_pcount_out(psize)                                         __out_pcount_out_ecount_full(psize,1)
#define __out_pcount_part_out(psize,plength)                            __out_pcount_part_out_ecount_full(psize,plength,1)
#define __out_pcount_full_out(psize)                                    __out_pcount_part_out(psize,psize)


// ---------------------------------------------------------------------------
//
// Out buffer of pointers to buffers of bytes
//

// Dereferenced buffers are not initialized (just allocated)

#define __out_pcount_bcount(psize,bsize)                                __ecount(psize)  __post __deref __notnull  __post __deref __byte_writableTo(bsize)
#define __out_pcount_part_bcount(psize,plength,bsize)                   __out_pcount_bcount(psize,bsize) __post __byte_readableTo(plength)
#define __out_pcount_full_bcount(psize,bsize)                           __out_pcount_part_bcount(psize,psize,bsize)

// Dereferenced buffers are initialized

#define __out_pcount_out_bcount(psize,bsize)                            __out_pcount_bcount(psize,bsize)  __post __deref __valid
#define __out_pcount_out_bcount_part(psize,bsize,blength)               __out_pcount_out_bcount(psize,bsize)  __post __deref __byte_readableTo(blength)
#define __out_pcount_out_bcount_full(psize,bsize)                       __out_pcount_out_bcount_part(psize,bsize,bsize)

#define __out_pcount_part_out_bcount(psize,plength,bsize)               __out_pcount_part_bcount(psize,plength,bsize) __post __deref __valid
#define __out_pcount_part_out_bcount_part(psize,plength,bsize,blength)  __out_pcount_part_out_bcount(psize,plength,bsize) __post __deref __byte_readableTo(blength)
#define __out_pcount_part_out_bcount_full(psize,plength,bsize)          __out_pcount_part_out_bcount_part(psize,plength,bsize,bsize)

#define __out_pcount_full_out_bcount(psize,bsize)                       __out_pcount_part_out_bcount(psize,psize,bsize)
#define __out_pcount_full_out_bcount_part(psize,bsize,blength)          __out_pcount_part_out_bcount_part(psize,psize,bsize,blength)
#define __out_pcount_full_out_bcount_full(psize,bsize)                  __out_pcount_part_out_bcount_full(psize,psize,bsize)



#define __out_pcount_out_out_ecount(psize,esize)                        __out_pcount_out(psize)  __post __deref __deref __notnull  __post __deref __byte_writableTo(esize)  __post __deref __deref __valid
#define __out_pcount_out_out_ecount_part(psize,esize,elength)           __out_pcount_out_out_ecount(psize,esize)  __post __deref __deref __byte_readableTo(elength)
#define __out_pcount_out_out_ecount_full(psize,esize)                   __out_pcount_out_out_ecount_part(psize,esize,esize)



// ---------------------------------------------------------------------------
//
// InOut buffer of pointers to buffers ...
//

#define __inout_pcount_in_ecount(psize,esize)
#define __inout_pcount_in(psize)
#define __inout_pcount_in_bcount(psize,bsize)       __inner_inout_pcount(psize)  __pre __deref __byte_readableTo(bsize)  __post __deref __byte_readableTo(bsize)


#else


#define __deref_inout_ecount_inopt(size)
#define __deref_inout_ecount_outopt(size)
#define __returnro_ecount(esize)
#define __returnro_bcount(bsize)
#define __returnro_xcount(xsize)
#define __returnro_ecount_opt(esize)
#define __returnro_bcount_opt(bsize)
#define __returnro_xcount_opt(xsize)
#define __outro_ecount(esize)
#define __outro_bcount(bsize)
#define __outro_xcount(xsize)
#define __outro_ecount_opt(esize)
#define __outro_bcount_opt(bsize)
#define __outro_xcount_opt(xsize)
#define __deref_outro_ecount(esize)
#define __deref_outro_bcount(bsize)
#define __deref_outro_xcount(xsize)
#define __deref_outro_ecount_opt(esize)
#define __deref_outro_bcount_opt(bsize)
#define __deref_outro_xcount_opt(xsize)
#define __returnro
#define __returnro_opt
#define __outro  
#define __outro_opt 
#define __in_pcount_in_ecount(psize,esize)
#define __in_pcount_ecount(psize,esize)   
#define __in_pcount_out_ecount(psize,esize)
#define __in_pcount_out_ecount_part(psize,esize,elength)
#define __in_pcount_out_ecount_full(psize,esize)        
#define __in_pcount_inout_ecount(psize,esize)           
#define __in_pcount_inout_ecount_part(psize,esize,elength) 
#define __in_pcount_inout_ecount_full(psize,esize)          
#define __in_pcount_in_ecount_opt(psize,esize)              
#define __in_pcount_ecount_opt(psize,esize)                 
#define __in_pcount_out_ecount_opt(psize,esize)              
#define __in_pcount_out_ecount_part_opt(psize,esize,elength)  
#define __in_pcount_out_ecount_full_opt(psize,esize)           
#define __in_pcount_inout_ecount_opt(psize,esize)              
#define __in_pcount_inout_ecount_part_opt(psize,esize,elength) 
#define __in_pcount_inout_ecount_full_opt(psize,esize)         
#define __in_pcount_opt_ecount(psize,esize)                    
#define __in_pcount_opt_ecount_opt(psize,esize)                
#define __in_pcount_in(psize)                                  
#define __in_pcount(psize)                                     
#define __in_pcount_out(psize)                                 
#define __in_pcount_inout(psize)                               
#define __in_pcount_in_opt(psize)                              
#define __in_pcount_out_opt(psize)                             
#define __in_pcount_inout_opt(psize)                           
#define __in_pcount_opt_in(psize)                              
#define __in_pcount_opt(psize)                                 
#define __in_pcount_opt_out(psize)                             
#define __in_pcount_opt_inout(psize)                           
#define __in_pcount_opt_in_opt(psize)                          
#define __in_pcount_opt_out_opt(psize)                         
#define __in_pcount_opt_inout_opt(psize)                       
#define __in_pcount_in_bcount(psize,bsize)                     
#define __in_pcount_bcount(psize,bsize)                        
#define __in_pcount_out_bcount(psize,bsize)                    
#define __in_pcount_out_bcount_part(psize,bsize,blength)       
#define __in_pcount_out_bcount_full(psize,bsize)               
#define __in_pcount_inout_bcount(psize,bsize)                  
#define __in_pcount_inout_bcount_part(psize,bsize,blength)     
#define __in_pcount_inout_bcount_full(psize,bsize)             
#define __in_pcount_bcount_opt(psize,bsize)                    
#define __in_pcount_opt_bcount(psize,bsize)                    
#define __in_pcount_opt_bcount_opt(psize,bsize)                
#define __out_pcount_ecount(psize,esize)                       
#define __out_pcount_part_ecount(psize,plength,esize)          
#define __out_pcount_full_ecount(psize,esize)                  
#define __out_pcount_out_ecount(psize,esize)                   
#define __out_pcount_out_ecount_part(psize,esize,elength)      
#define __out_pcount_out_ecount_full(psize,esize)              
#define __out_pcount_part_out_ecount(psize,plength,esize)      
#define __out_pcount_part_out_ecount_part(psize,plength,esize,elength)  
#define __out_pcount_part_out_ecount_full(psize,plength,esize)          
#define __out_pcount_full_out_ecount(psize,esize)                       
#define __out_pcount_full_out_ecount_part(psize,esize,elength)          
#define __out_pcount_full_out_ecount_full(psize,esize)                  
#define __out_pcount(psize)                                             
#define __out_pcount_part(psize,plength)                                
#define __out_pcount_full(psize)                                        
#define __out_pcount_out(psize)                                         
#define __out_pcount_part_out(psize,plength)                            
#define __out_pcount_full_out(psize)                                    
#define __out_pcount_bcount(psize,bsize)                                
#define __out_pcount_part_bcount(psize,plength,bsize)                   
#define __out_pcount_full_bcount(psize,bsize)                           
#define __out_pcount_out_bcount(psize,bsize)                            
#define __out_pcount_out_bcount_part(psize,bsize,blength)               
#define __out_pcount_out_bcount_full(psize,bsize)                       
#define __out_pcount_part_out_bcount(psize,plength,bsize)            
#define __out_pcount_part_out_bcount_part(psize,plength,bsize,blength)
#define __out_pcount_part_out_bcount_full(psize,plength,bsize)        
#define __out_pcount_full_out_bcount(psize,bsize)                     
#define __out_pcount_full_out_bcount_part(psize,bsize,blength)        
#define __out_pcount_full_out_bcount_full(psize,bsize)                
#define __out_pcount_out_out_ecount(psize,esize)                      
#define __out_pcount_out_out_ecount_part(psize,esize,elength)         
#define __out_pcount_out_out_ecount_full(psize,esize)                 
#define __inout_pcount_in_ecount(psize,esize)
#define __inout_pcount_in(psize)
#define __inout_pcount_in_bcount(psize,bsize)

#endif



