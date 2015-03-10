/*
 * sal.h
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
#define __ATTR_SAL
#define _SAL_VERSION 20

/* HACK: gcc's C++ headers conflict with oldstyle macros */
#if !defined(__cplusplus) || !defined(__GNUC__)
#include <sal_old.h>
#endif

/* Disable expansion for MIDL and RC */
#if defined(MIDL_PASS) || defined(__midl) || defined(RC_INVOKED) // [
#undef _PREFAST_
#endif // ]

/* Check for deprecated use of declspecs for sal */
#if defined(_USE_DECLSPECS_FOR_SAL) && _USE_DECLSPECS_FOR_SAL // [
#pragma message("declspecs for sal are deprecated.")
#ifdef _PREFAST_ // [
#error Support for _USE_DECLSPECS_FOR_SAL is dropped
#endif // ] _PREFAST_
#endif // ] _USE_DECLSPECS_FOR_SAL

#undef _USE_DECLSPECS_FOR_SAL
#undef _USE_ATTRIBUTES_FOR_SAL
#define _USE_DECLSPECS_FOR_SAL 0
#define _USE_ATTRIBUTES_FOR_SAL 1

#ifdef _PREFAST_ // [

#if _MSC_VER < 1610 // [
#error broken
#endif // ]

#pragma warning(disable:6320) /* disable warning about SEH filter */
#pragma warning(disable:28247) /* duplicated model file annotations */
#pragma warning(disable:28251) /* Inconsistent annotation */

/******************************************************************************/
//#include "codeanalysis\sourceannotations.h"

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86))
#define _W64 __w64
#else
#define _W64
#endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef _W64 unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#pragma push_macro("_SA")
#pragma push_macro("_REPEATABLE")
#pragma push_macro("_SRC_ANNO")
#pragma push_macro("_SRC_ANNO_REPEATABLE")
#pragma push_macro("_CPLUSPLUSONLY")

#ifdef __cplusplus
namespace vc_attributes {
#define _SA(id) id
#define _REPEATABLE [repeatable]
#define _CPLUSPLUSONLY(x) x
#else  // !__cplusplus
#define _SA(id) SA_##id
#define _REPEATABLE
#define _CPLUSPLUSONLY(x)
#endif  // !__cplusplus

#define _SRC_ANNO(x) [source_annotation_attribute(_SA(All))] struct x
#define _SRC_ANNO_REPEATABLE(x) _REPEATABLE _SRC_ANNO(x)

typedef enum _SA(YesNoMaybe)
{
    _SA(No) = 0x0fff0001,
    _SA(Maybe) = 0x0fff0010,
    _SA(Yes) = 0x0fff0100
} _SA(YesNoMaybe);

typedef enum _SA(AccessType)
{
    _SA(NoAccess) = 0,
    _SA(Read) = 1,
    _SA(Write) = 2,
    _SA(ReadWrite) = 3
} _SA(AccessType);

#ifndef SAL_NO_ATTRIBUTE_DECLARATIONS

_SRC_ANNO_REPEATABLE(PreAttribute)
{
    _CPLUSPLUSONLY(PreAttribute();)
    unsigned int Deref;
    _SA(YesNoMaybe) Valid;
    _SA(YesNoMaybe) Null;
    _SA(YesNoMaybe) Tainted;
    _SA(AccessType) Access;
    unsigned int Notref;
    size_t ValidElementsConst;
    size_t ValidBytesConst;
    const wchar_t* ValidElements;
    const wchar_t* ValidBytes;
    const wchar_t* ValidElementsLength;
    const wchar_t* ValidBytesLength;
    size_t WritableElementsConst;
    size_t WritableBytesConst;
    const wchar_t* WritableElements;
    const wchar_t* WritableBytes;
    const wchar_t* WritableElementsLength;
    const wchar_t* WritableBytesLength;
    size_t ElementSizeConst;
    const wchar_t* ElementSize;
    _SA(YesNoMaybe) NullTerminated;
    const wchar_t* Condition;
};

_SRC_ANNO_REPEATABLE(PostAttribute)
{
    _CPLUSPLUSONLY(PostAttribute();)
    unsigned int Deref;
    _SA(YesNoMaybe) Valid;
    _SA(YesNoMaybe) Null;
    _SA(YesNoMaybe) Tainted;
    _SA(AccessType) Access;
    unsigned int Notref;
    size_t ValidElementsConst;
    size_t ValidBytesConst;
    const wchar_t* ValidElements;
    const wchar_t* ValidBytes;
    const wchar_t* ValidElementsLength;
    const wchar_t* ValidBytesLength;
    size_t WritableElementsConst;
    size_t WritableBytesConst;
    const wchar_t* WritableElements;
    const wchar_t* WritableBytes;
    const wchar_t* WritableElementsLength;
    const wchar_t* WritableBytesLength;
    size_t ElementSizeConst;
    const wchar_t* ElementSize;
    _SA(YesNoMaybe) NullTerminated;
    _SA(YesNoMaybe) MustCheck;
    const wchar_t* Condition;
};

_SRC_ANNO(FormatStringAttribute)
{
    _CPLUSPLUSONLY(FormatStringAttribute();)
    const wchar_t* Style;
    const wchar_t* UnformattedAlternative;
};

_SRC_ANNO_REPEATABLE(InvalidCheckAttribute)
{
    _CPLUSPLUSONLY(InvalidCheckAttribute();)
    long Value;
};

_SRC_ANNO(SuccessAttribute)
{
    _CPLUSPLUSONLY(SuccessAttribute();)
    const wchar_t* Condition;
};

_SRC_ANNO_REPEATABLE(PreBoundAttribute)
{
    _CPLUSPLUSONLY(PreBoundAttribute();)
    unsigned int Deref;
};

_SRC_ANNO_REPEATABLE(PostBoundAttribute)
{
    _CPLUSPLUSONLY(PostBoundAttribute();)
    unsigned int Deref;
};

_SRC_ANNO_REPEATABLE(PreRangeAttribute)
{
    _CPLUSPLUSONLY(PreRangeAttribute();)
    unsigned int Deref;
    const char* MinVal;
    const char* MaxVal;
};

_SRC_ANNO_REPEATABLE(PostRangeAttribute)
{
    _CPLUSPLUSONLY(PostRangeAttribute();)
    unsigned int Deref;
    const char* MinVal;
    const char* MaxVal;
};

_SRC_ANNO_REPEATABLE(DerefAttribute)
{
    _CPLUSPLUSONLY(DerefAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(NotrefAttribute)
{
    _CPLUSPLUSONLY(NotrefAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(AnnotesAttribute)
{
    _CPLUSPLUSONLY(AnnotesAttribute();)
    wchar_t *Name;
    wchar_t *p1;
    wchar_t *p2;
    wchar_t *p3;
    wchar_t *p4;
    wchar_t *p5;
    wchar_t *p6;
    wchar_t *p7;
    wchar_t *p8;
    wchar_t *p9;
};

_SRC_ANNO_REPEATABLE(AtAttribute)
{
    _CPLUSPLUSONLY(AtAttribute();)
    wchar_t *p1;
};

_SRC_ANNO_REPEATABLE(AtBufferAttribute)
{
    _CPLUSPLUSONLY(AtBufferAttribute();)
    wchar_t *p1;
    wchar_t *p2;
    wchar_t *p3;
};

_SRC_ANNO_REPEATABLE(WhenAttribute)
{
    _CPLUSPLUSONLY(WhenAttribute();)
    wchar_t *p1;
};

_SRC_ANNO_REPEATABLE(TypefixAttribute)
{
    _CPLUSPLUSONLY(TypefixAttribute();)
    wchar_t *p1;
};

_SRC_ANNO_REPEATABLE(ContextAttribute)
{
    _CPLUSPLUSONLY(ContextAttribute();)
    wchar_t *p1;
};

_SRC_ANNO_REPEATABLE(ExceptAttribute)
{
    _CPLUSPLUSONLY(ExceptAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(PreOpAttribute)
{
    _CPLUSPLUSONLY(PreOpAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(PostOpAttribute)
{
    _CPLUSPLUSONLY(PostOpAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(BeginAttribute)
{
    _CPLUSPLUSONLY(BeginAttribute();)
    int unused;
};

_SRC_ANNO_REPEATABLE(EndAttribute)
{
    _CPLUSPLUSONLY(EndAttribute();)
    int unused;
};

#endif // !SAL_NO_ATTRIBUTE_DECLARATIONS

#pragma pop_macro("_CPLUSPLUSONLY")
#pragma pop_macro("_SRC_ANNO_REPEATABLE")
#pragma pop_macro("_SRC_ANNO")
#pragma pop_macro("_REPEATABLE")
#pragma pop_macro("_SA")

#ifdef __cplusplus
}; // namespace vc_attributes

#define SA_All All
#define SA_Class Class
#define SA_Constructor Constructor
#define SA_Delegate Delegate
#define SA_Enum Enum
#define SA_Event Event
#define SA_Field Field
#define SA_GenericParameter GenericParameter
#define SA_Interface Interface
#define SA_Method Method
#define SA_Module Module
#define SA_Parameter Parameter
#define SA_Property Property
#define SA_ReturnValue ReturnValue
#define SA_Struct Struct
#define SA_Typedef Typedef

#define _vc_attributes_(x) ::vc_attributes::x
typedef ::vc_attributes::YesNoMaybe SA_YesNoMaybe;
const ::vc_attributes::YesNoMaybe SA_Yes   = ::vc_attributes::Yes;
const ::vc_attributes::YesNoMaybe SA_No    = ::vc_attributes::No;
const ::vc_attributes::YesNoMaybe SA_Maybe = ::vc_attributes::Maybe;
typedef ::vc_attributes::AccessType SA_AccessType;
const ::vc_attributes::AccessType SA_NoAccess  = ::vc_attributes::NoAccess;
const ::vc_attributes::AccessType SA_Read      = ::vc_attributes::Read;
const ::vc_attributes::AccessType SA_Write     = ::vc_attributes::Write;
const ::vc_attributes::AccessType SA_ReadWrite = ::vc_attributes::ReadWrite;

#else // !__cplusplus

#define _vc_attributes_(x) struct x
typedef struct PreAttribute PreAttribute;
typedef struct PostAttribute PostAttribute;

#endif // !__cplusplus

#if !defined(__cplusplus) || !defined(SAL_NO_ATTRIBUTE_DECLARATIONS)
typedef _vc_attributes_(PreAttribute)          SA_Pre;
typedef _vc_attributes_(PostAttribute)         SA_Post;
typedef _vc_attributes_(FormatStringAttribute) SA_FormatString;
typedef _vc_attributes_(InvalidCheckAttribute) SA_InvalidCheck;
typedef _vc_attributes_(SuccessAttribute)      SA_Success;
typedef _vc_attributes_(PreBoundAttribute)     SA_PreBound;
typedef _vc_attributes_(PostBoundAttribute)    SA_PostBound;
typedef _vc_attributes_(PreRangeAttribute)     SA_PreRange;
typedef _vc_attributes_(PostRangeAttribute)    SA_PostRange;
typedef _vc_attributes_(DerefAttribute)        SAL_deref;
typedef _vc_attributes_(NotrefAttribute)       SAL_notref;
typedef _vc_attributes_(PreOpAttribute)        SAL_pre;
typedef _vc_attributes_(PostOpAttribute)       SAL_post;
typedef _vc_attributes_(ExceptAttribute)       SAL_except;
typedef _vc_attributes_(AtAttribute)           SAL_at;
typedef _vc_attributes_(AtBufferAttribute)     SAL_at_buffer;
typedef _vc_attributes_(WhenAttribute)         SAL_when;
typedef _vc_attributes_(BeginAttribute)        SAL_begin;
typedef _vc_attributes_(EndAttribute)          SAL_end;
typedef _vc_attributes_(TypefixAttribute)      SAL_typefix;
typedef _vc_attributes_(AnnotesAttribute)      SAL_annotes;
typedef _vc_attributes_(ContextAttribute)      SAL_context;
#endif

#ifdef _MANAGED
#ifdef CODE_ANALYSIS
#define SA_SUPPRESS_MESSAGE(category, id, ...) [::System::Diagnostics::CodeAnalysis::SuppressMessage(category, id, __VA_ARGS__)]
#define CA_SUPPRESS_MESSAGE(...) [System::Diagnostics::CodeAnalysis::SuppressMessage(__VA_ARGS__)]
#define CA_GLOBAL_SUPPRESS_MESSAGE(...) [assembly:System::Diagnostics::CodeAnalysis::SuppressMessage(__VA_ARGS__)]
#else // !CODE_ANALYSIS
#define SA_SUPPRESS_MESSAGE(category, id, ...)
#define CA_SUPPRESS_MESSAGE(...)
#define CA_GLOBAL_SUPPRESS_MESSAGE(...)
#endif // !CODE_ANALYSIS
#endif  // _MANAGED

/******************************************************************************/

enum __SAL_YesNo {_SAL_notpresent, _SAL_no, _SAL_maybe, _SAL_yes, _SAL_default};

#define _SA_SPECSTRIZE(x) #x

#define _SA_annotes0(n)                             [SAL_annotes(Name=#n)]
#define _SA_annotes1(n,pp1)                         [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1))]
#define _SA_annotes2(n,pp1,pp2)                     [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1), p2=_SA_SPECSTRIZE(pp2))]
#define _SA_annotes3(n,pp1,pp2,pp3)                 [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1), p2=_SA_SPECSTRIZE(pp2), p3=_SA_SPECSTRIZE(pp3))]

#define _SAL2_NAME(Name)                            _SA_annotes3(SAL_name, #Name, "", "2")
#define _SAL11_NAME(Name)                           _SA_annotes3(SAL_name, #Name, "", "1.1")

#define _Pre_                                       [SAL_pre]
#define _Post_                                      [SAL_post]
#define _Deref_impl_                                [SAL_deref]
#define _Notref_impl_                               [SAL_notref]
#define __inner_exceptthat                          [SAL_except]
#define __inner_typefix(ctype)                      [SAL_typefix(p1=_SA_SPECSTRIZE(ctype))]
#define _Group_(annos)                              [SAL_begin] annos [SAL_end]
#define _When_(expr, annos)                         [SAL_when(p1=_SA_SPECSTRIZE(expr))] _Group_(annos)
#define _At_(target, annos)                         [SAL_at(p1=_SA_SPECSTRIZE(target))] _Group_(annos)
#define _At_buffer_(target, iter, bound, annos)     [SAL_at_buffer(p1=_SA_SPECSTRIZE(target), p2=_SA_SPECSTRIZE(iter), p3=_SA_SPECSTRIZE(bound))] [SAL_begin] annos [SAL_end]
#define _On_failure_(annos)                         [SAL_context(p1="SAL_failed")] _Group_(_Post_ _Group_(annos))
#define _Always_(annos)                             _Group_(annos) _On_failure_(annos)

#define _Analysis_noreturn_                                         _SAL2_NAME(_Analysis_noreturn_) [SAL_annotes(Name="SAL_terminates")]
#define _Analysis_assume_(expr)                                     __assume(expr)
#define __analysis_assume(expr)                                     __assume(expr)

#define _Check_return_                                              _SAL2_NAME(_Check_return_) [SA_Post(MustCheck=SA_Yes)]
#define _COM_Outptr_                                                _SAL2_NAME(_COM_Outptr_) _Group_(_Outptr_ _On_failure_(_Deref_post_null_))
#define _COM_Outptr_opt_                                            _SAL2_NAME(_COM_Outptr_opt_) _Group_(_Outptr_opt_ _On_failure_(_Deref_post_null_))
#define _COM_Outptr_opt_result_maybenull_                           _SAL2_NAME(_COM_Outptr_opt_result_maybenull_) _Group_(_Outptr_opt_result_maybenull_ _On_failure_(_Deref_post_null_))
#define _COM_Outptr_result_maybenull_                               _SAL2_NAME(_COM_Outptr_result_maybenull_) _Group_(_Outptr_opt_result_maybenull_ _On_failure_(_Deref_post_null_))
#define _Const_                                                     _SAL2_NAME(_Const_) [SA_Pre(Access=SA_Read,Notref=1)]
#define _Deref_in_bound_                                            _SAL2_NAME(_Deref_in_bound_) [SA_PreBound(Deref=1)]
//#define _Deref_in_range_(lb,ub)
//#define _Deref_inout_bound_
//#define _Deref_inout_z_
//#define _Deref_inout_z_bytecap_c_(size)
//#define _Deref_inout_z_cap_c_(size)
//#define _Deref_opt_out_
//#define _Deref_opt_out_opt_
//#define _Deref_opt_out_opt_z_
//#define _Deref_opt_out_z_
#define _Deref_out_                                                 _SAL2_NAME(_Deref_out_) _Group_(_Out_ _Deref_post_valid_)
//#define _Deref_out_bound_
//#define _Deref_out_opt_
//#define _Deref_out_opt_z_
#define _Deref_out_range_(lb,ub)                                    _SAL2_NAME(_Deref_out_range_) _Group_(_Post_ [SAL_notref] [SAL_deref] [SAL_annotes(Name="SAL_range", p1=_SA_SPECSTRIZE(lb), p2=_SA_SPECSTRIZE(ub))])
#define _Deref_out_z_                                               _SAL11_NAME(_Deref_out_z_) _Group_(_Out_ _Deref_post_z_)
//#define _Deref_out_z_bytecap_c_(size)
//#define _Deref_out_z_cap_c_(size)
#define _Deref_post_bytecap_(size)                                  _SAL11_NAME(_Deref_post_bytecap_) _Group_([SA_Post(Deref=1,Null=SA_No,Notref=1)] [SA_Post(Deref=1,WritableBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Deref_post_bytecap_c_(size)
//#define _Deref_post_bytecap_x_(size)
#define _Deref_post_bytecount_(size)
//#define _Deref_post_bytecount_c_(size)
//#define _Deref_post_bytecount_x_(size)
//#define _Deref_post_cap_(size)
//#define _Deref_post_cap_c_(size)
//#define _Deref_post_cap_x_(size)
//#define _Deref_post_count_(size)
//#define _Deref_post_count_c_(size)
//#define _Deref_post_count_x_(size)
//#define _Deref_post_maybenull_
//#define _Deref_post_notnull_
#define _Deref_post_null_                                           _SAL11_NAME(_Deref_post_null_) _Group_([SA_Post(Deref=1,Null=SA_Yes,Notref=1)] )
//#define _Deref_post_opt_bytecap_(size)
//#define _Deref_post_opt_bytecap_c_(size)
//#define _Deref_post_opt_bytecap_x_(size)
//#define _Deref_post_opt_bytecount_(size)
//#define _Deref_post_opt_bytecount_c_(size)
//#define _Deref_post_opt_bytecount_x_(size)
//#define _Deref_post_opt_cap_(size)
//#define _Deref_post_opt_cap_c_(size)
//#define _Deref_post_opt_cap_x_(size)
//#define _Deref_post_opt_count_(size)
//#define _Deref_post_opt_count_c_(size)
//#define _Deref_post_opt_count_x_(size)
#define _Deref_post_opt_valid_                                      _SAL11_NAME(_Deref_post_opt_valid_) _Group_([SA_Post(Deref=1,Null=SA_Maybe,Notref=1)] [SA_Post(Valid=SA_Yes)])
//#define _Deref_post_opt_valid_bytecap_(size)
//#define _Deref_post_opt_valid_bytecap_c_(size)
//#define _Deref_post_opt_valid_bytecap_x_(size)
//#define _Deref_post_opt_valid_cap_(size)
//#define _Deref_post_opt_valid_cap_c_(size)
//#define _Deref_post_opt_valid_cap_x_(size)
#define _Deref_post_opt_z_                                          _SAL11_NAME(_Deref_post_opt_z_) _Group_([SA_Post(Deref=1,Null=SA_Maybe,Notref=1)] [SA_Post(Deref=1,NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)])
//#define _Deref_post_opt_z_bytecap_(size)
//#define _Deref_post_opt_z_bytecap_c_(size)
//#define _Deref_post_opt_z_bytecap_x_(size)
//#define _Deref_post_opt_z_cap_(size)
//#define _Deref_post_opt_z_cap_c_(size)
//#define _Deref_post_opt_z_cap_x_(size)
#define _Deref_post_valid_                                          _SAL2_NAME(_Deref_post_valid_) _Group_([SA_Post(Deref=1,Null=SA_No,Notref=1)] [SA_Post(Valid=SA_Yes)])
//#define _Deref_post_valid_bytecap_(size)
//#define _Deref_post_valid_bytecap_c_(size)
//#define _Deref_post_valid_bytecap_x_(size)
//#define _Deref_post_valid_cap_(size)
//#define _Deref_post_valid_cap_c_(size)
//#define _Deref_post_valid_cap_x_(size)
#define _Deref_post_z_                                              _SAL11_NAME(_Deref_post_z_) _Group_([SA_Post(Deref=1,Null=SA_No,Notref=1)] [SA_Post(Deref=1,NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)])
//#define _Deref_post_z_bytecap_(size)
//#define _Deref_post_z_bytecap_c_(size)
//#define _Deref_post_z_bytecap_x_(size)
//#define _Deref_post_z_cap_(size)
//#define _Deref_post_z_cap_c_(size)
//#define _Deref_post_z_cap_x_(size)
//#define _Deref_pre_bytecap_(size)
//#define _Deref_pre_bytecap_c_(size)
//#define _Deref_pre_bytecap_x_(size)
#define _Deref_pre_bytecount_(size)                                 _SAL11_NAME(_Deref_pre_bytecount_) _Group_([SA_Pre(Deref=1,Null=SA_No,Notref=1)] [SA_Pre(Deref=1,ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Deref_pre_bytecount_c_(size)
//#define _Deref_pre_bytecount_x_(size)
//#define _Deref_pre_cap_(size)
//#define _Deref_pre_cap_c_(size)
//#define _Deref_pre_cap_x_(size)
//#define _Deref_pre_count_(size)
//#define _Deref_pre_count_c_(size)
//#define _Deref_pre_count_x_(size)
//#define _Deref_pre_invalid_
//#define _Deref_pre_maybenull_
//#define _Deref_pre_notnull_
//#define _Deref_pre_null_
#define _Deref_pre_opt_bytecap_(size)                               _SAL11_NAME(_Pre_opt_bytecap_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Deref_pre_opt_bytecap_c_(size)
//#define _Deref_pre_opt_bytecap_x_(size)
//#define _Deref_pre_opt_bytecount_(size)
//#define _Deref_pre_opt_bytecount_c_(size)
//#define _Deref_pre_opt_bytecount_x_(size)
//#define _Deref_pre_opt_cap_(size)
//#define _Deref_pre_opt_cap_c_(size)
//#define _Deref_pre_opt_cap_x_(size)
//#define _Deref_pre_opt_count_(size)
//#define _Deref_pre_opt_count_c_(size)
//#define _Deref_pre_opt_count_x_(size)
#define _Deref_pre_opt_valid_                                       _SAL11_NAME(_Deref_pre_opt_valid_) _Group_([SA_Pre(Deref=1,Null=SA_Maybe,Notref=1)] [SA_Pre(Valid=SA_Yes)])
//#define _Deref_pre_opt_valid_bytecap_(size)
//#define _Deref_pre_opt_valid_bytecap_c_(size)
//#define _Deref_pre_opt_valid_bytecap_x_(size)
//#define _Deref_pre_opt_valid_cap_(size)
//#define _Deref_pre_opt_valid_cap_c_(size)
//#define _Deref_pre_opt_valid_cap_x_(size)
#define _Deref_pre_opt_z_                                           _SAL11_NAME(_Deref_pre_opt_z_) _Group_([SA_Pre(Deref=1,Null=SA_Maybe,Notref=1)] [SA_Pre(Deref=1,NullTerminated=SA_Yes)] [SA_Pre(Valid=SA_Yes)])
//#define _Deref_pre_opt_z_bytecap_(size)
//#define _Deref_pre_opt_z_bytecap_c_(size)
//#define _Deref_pre_opt_z_bytecap_x_(size)
//#define _Deref_pre_opt_z_cap_(size)
//#define _Deref_pre_opt_z_cap_c_(size)
//#define _Deref_pre_opt_z_cap_x_(size)
#define _Deref_pre_readonly_                                        _SAL2_NAME(_Deref_pre_readonly_) _Group_([SA_Pre(Deref=1,Access=SA_Read,Notref=1)])
//#define _Deref_pre_valid_
//#define _Deref_pre_valid_bytecap_(size)
//#define _Deref_pre_valid_bytecap_c_(size)
//#define _Deref_pre_valid_bytecap_x_(size)
//#define _Deref_pre_valid_cap_(size)
//#define _Deref_pre_valid_cap_c_(size)
//#define _Deref_pre_valid_cap_x_(size)
//#define _Deref_pre_writeonly_
#define _Deref_pre_z_                                               _SAL11_NAME(_Deref_pre_z_) _Group_([SA_Pre(Deref=1,Null=SA_No,Notref=1)] [SA_Pre(Deref=1,NullTerminated=SA_Yes)] [SA_Pre(Valid=SA_Yes)])
//#define _Deref_pre_z_bytecap_(size)
//#define _Deref_pre_z_bytecap_c_(size)
//#define _Deref_pre_z_bytecap_x_(size)
//#define _Deref_pre_z_cap_(size)
//#define _Deref_pre_z_cap_c_(size)
//#define _Deref_pre_z_cap_x_(size)
//#define _Deref_prepost_bytecap_(size)
//#define _Deref_prepost_bytecap_x_(size)
//#define _Deref_prepost_bytecount_(size)
//#define _Deref_prepost_bytecount_x_(size)
//#define _Deref_prepost_cap_(size)
//#define _Deref_prepost_cap_x_(size)
//#define _Deref_prepost_count_(size)
//#define _Deref_prepost_count_x_(size)
//#define _Deref_prepost_opt_bytecap_(size)
//#define _Deref_prepost_opt_bytecap_x_(size)
//#define _Deref_prepost_opt_bytecount_(size)
//#define _Deref_prepost_opt_bytecount_x_(size)
//#define _Deref_prepost_opt_cap_(size)
//#define _Deref_prepost_opt_cap_x_(size)
//#define _Deref_prepost_opt_count_(size)
//#define _Deref_prepost_opt_count_x_(size)
#define _Deref_prepost_opt_valid_                                   _SAL11_NAME(_Deref_prepost_opt_valid_) _Group_(_Deref_pre_opt_valid_ _Deref_post_opt_valid_)
//#define _Deref_prepost_opt_valid_bytecap_(size)
//#define _Deref_prepost_opt_valid_bytecap_x_(size)
//#define _Deref_prepost_opt_valid_cap_(size)
//#define _Deref_prepost_opt_valid_cap_x_(size)
#define _Deref_prepost_opt_z_                                       _SAL11_NAME(_Deref_prepost_opt_z_) _Group_(_Deref_pre_opt_z_ _Deref_post_opt_z_)
//#define _Deref_prepost_opt_z_bytecap_(size)
//#define _Deref_prepost_opt_z_cap_(size)
//#define _Deref_prepost_valid_
//#define _Deref_prepost_valid_bytecap_(size)
//#define _Deref_prepost_valid_bytecap_x_(size)
//#define _Deref_prepost_valid_cap_(size)
//#define _Deref_prepost_valid_cap_x_(size)
#define _Deref_prepost_z_                                           _SAL11_NAME(_Deref_prepost_z_) _Group_(_Deref_pre_z_ _Deref_post_z_)
//#define _Deref_prepost_z_bytecap_(size)
//#define _Deref_prepost_z_cap_(size)
//#define _Deref_ret_bound_
//#define _Deref_ret_opt_z_
//#define _Deref_ret_range_(lb,ub)
//#define _Deref_ret_z_
//#define _Deref2_pre_readonly_
#define _Field_range_(min,max)                                      _SAL2_NAME(_Field_range_) _Group_(_SA_annotes2(SAL_range,min,max))
#define _Field_size_(size)                                          _SAL2_NAME(_Field_size_) _Group_(_Notnull_ _Writable_elements_(size))
#define _Field_size_opt_(size)                                      _SAL2_NAME(_Field_size_opt_) _Group_(_Maybenull_ _Writable_elements_(size))
#define _Field_size_bytes_(size)                                    _SAL2_NAME(_Field_size_bytes_) _Group_(_Notnull_ _Writable_bytes_(size))
//#define _Field_size_bytes_full_(size)
//#define _Field_size_bytes_full_opt_(size)
#define _Field_size_bytes_opt_(size)                                _SAL2_NAME(_Field_size_bytes_opt_) _Group_(_Maybenull_ _Writable_bytes_(size))
#define _Field_size_bytes_part_(size,count)                         _SAL2_NAME(_Field_size_bytes_part_) _Group_(_Notnull_ _Writable_bytes_(size) _Readable_bytes_(count))
#define _Field_size_bytes_part_opt_(size, count)                    _SAL2_NAME(_Field_size_bytes_part_opt_) _Group_(_Maybenull_ _Writable_bytes_(size) _Readable_bytes_(count))
//#define _Field_size_full_(size)
//#define _Field_size_full_opt_(size)
#define _Field_size_part_(size,count)                               _SAL2_NAME(_Field_size_part_) _Group_(_Notnull_ _Writable_elements_(size) _Readable_elements_(count))
//#define _Field_size_part_opt_(size, count)
//#define _Field_z_
#define _Function_class_(x)                                         _SA_annotes1(SAL_functionClassNew, _SA_SPECSTRIZE(x))
#define _In_                                                        _SAL2_NAME(_In_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(Valid=SA_Yes)] [SA_Pre(Deref=1,Access=SA_Read,Notref=1)])
//#define _In_bound_
#define _In_bytecount_(size)                                        _SAL11_NAME(_In_bytecount_) _Group_(_Pre_bytecount_(size) _Deref_pre_readonly_)
//#define _In_bytecount_c_(size)
//#define _In_bytecount_x_(size)
#define _In_count_(size)                                            _SAL11_NAME(_In_count_) _Group_(_Pre_count_(size) _Deref_pre_readonly_)
#define _In_count_c_(size)                                          _SAL11_NAME(_In_count_c_) _Group_(_Pre_count_c_(size) _Deref_pre_readonly_)
//#define _In_count_x_(size)
//#define _In_defensive_(annotes)
#define _In_opt_                                                    _SAL2_NAME(_In_opt_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(Valid=SA_Yes)] _Deref_pre_readonly_)
#define _In_opt_bytecount_(size)                                    _SAL11_NAME(_In_opt_bytecount_) _Group_(_Pre_opt_bytecount_(size) _Deref_pre_readonly_)
//#define _In_opt_bytecount_c_(size)
//#define _In_opt_bytecount_x_(size)
//#define _In_opt_count_(size)
//#define _In_opt_count_c_(size)
//#define _In_opt_count_x_(size)
//#define _In_opt_ptrdiff_count_(size)
#define _In_opt_z_                                                  _SAL2_NAME(_In_opt_z_) _Group_(_In_opt_ [SA_Pre(NullTerminated=SA_Yes)])
//#define _In_opt_z_bytecount_(size)
//#define _In_opt_z_bytecount_c_(size)
#define _In_opt_z_count_(size)                                      _SAL11_NAME(_In_opt_z_count_) _Group_(_Pre_opt_z_ _Pre_opt_count_(size) _Deref_pre_readonly_)
//#define _In_opt_z_count_c_(size)
//#define _In_ptrdiff_count_(size)
#define _In_range_(lb,ub)                                           _SAL2_NAME(_In_range_) _Group_([SAL_pre] [SAL_annotes(Name="SAL_range", p1=_SA_SPECSTRIZE(lb), p2=_SA_SPECSTRIZE(ub))])
#define _In_reads_(size)                                            _SAL2_NAME(_In_reads_) _Group_(_Pre_count_(size) _Deref_pre_readonly_)
#define _In_reads_bytes_(size)                                      _SAL2_NAME(_In_reads_bytes_) _Group_(_Pre_bytecount_(size) _Deref_pre_readonly_)
#define _In_reads_bytes_opt_(size)                                  _SAL2_NAME(_In_reads_bytes_opt_) _Group_(_Pre_opt_bytecount_(size) _Deref_pre_readonly_)
#define _In_reads_opt_(size)                                        _SAL2_NAME(_In_reads_opt_) _Group_(_Pre_opt_count_(size) _Deref_pre_readonly_)
//#define _In_reads_opt_z_(size)
#define _In_reads_or_z_(size)                                       _SAL2_NAME(_In_reads_or_z_) _Group_(_In_ _When_(_String_length_(_Curr_) < (size), _Pre_z_) _When_(_String_length_(_Curr_) >= (size), [SA_Pre(ValidElements="\n" _SA_SPECSTRIZE(size))]))
//#define _In_reads_to_ptr_(ptr)
//#define _In_reads_to_ptr_opt_(ptr)
//#define _In_reads_to_ptr_opt_z_(ptr)
//#define _In_reads_to_ptr_z_(ptr)
#define _In_reads_z_(size)                                          _SAL2_NAME(_In_reads_z_) _Group_(_In_reads_(size) _Pre_z_)
#define _In_z_                                                      _SAL2_NAME(_In_z_) _Group_(_In_ [SA_Pre(NullTerminated=SA_Yes)])
#define _In_z_bytecount_(size)                                      _SAL11_NAME(_In_z_bytecount_) _Group_(_Pre_z_ _Pre_bytecount_(size) _Deref_pre_readonly_)
//#define _In_z_bytecount_c_(size)
#define _In_z_count_(size)                                          _SAL11_NAME(_In_z_count_) _Group_(_Pre_z_ _Pre_count_(size) _Deref_pre_readonly_)
//#define _In_z_count_c_(size)
#define _Inout_                                                     _SAL2_NAME(_Inout_) _Group_(_Prepost_valid_)
#define _Inout_bytecap_(size)                                       _SAL11_NAME(_Inout_bytecap_) _Group_(_Pre_valid_bytecap_(size) _Post_valid_)
//#define _Inout_bytecap_c_(size)
//#define _Inout_bytecap_x_(size)
#define _Inout_bytecount_(size)                                     _SAL11_NAME(_Inout_bytecount_) _Group_(_Prepost_bytecount_(size))
//#define _Inout_bytecount_c_(size)
//#define _Inout_bytecount_x_(size)
//#define _Inout_cap_(size)
//#define _Inout_cap_c_(size)
//#define _Inout_cap_x_(size)
//#define _Inout_count_(size)
//#define _Inout_count_c_(size)
//#define _Inout_count_x_(size)
//#define _Inout_defensive_(annotes)
#define _Inout_opt_                                                 _SAL2_NAME(_Inout_opt_) _Group_(_Prepost_opt_valid_)
//#define _Inout_opt_bytecap_(size)
//#define _Inout_opt_bytecap_c_(size)
//#define _Inout_opt_bytecap_x_(size)
//#define _Inout_opt_bytecount_(size)
//#define _Inout_opt_bytecount_c_(size)
//#define _Inout_opt_bytecount_x_(size)
//#define _Inout_opt_cap_(size)
//#define _Inout_opt_cap_c_(size)
//#define _Inout_opt_cap_x_(size)
//#define _Inout_opt_count_(size)
//#define _Inout_opt_count_c_(size)
//#define _Inout_opt_count_x_(size)
//#define _Inout_opt_ptrdiff_count_(size)
#define _Inout_opt_z_                                               _SAL2_NAME(_Inout_opt_z_) _Group_(_Prepost_opt_z_)
//#define _Inout_opt_z_bytecap_(size)
//#define _Inout_opt_z_bytecap_c_(size)
//#define _Inout_opt_z_bytecap_x_(size)
//#define _Inout_opt_z_bytecount_(size)
//#define _Inout_opt_z_bytecount_c_(size)
//#define _Inout_opt_z_cap_(size)
//#define _Inout_opt_z_cap_c_(size)
//#define _Inout_opt_z_cap_x_(size)
//#define _Inout_opt_z_count_(size)
//#define _Inout_opt_z_count_c_(size)
//#define _Inout_ptrdiff_count_(size)
#define _Inout_updates_(size)                                       _SAL2_NAME(_Inout_updates_) _Group_(_Pre_cap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)] )
//#define _Inout_updates_all_(size)
//#define _Inout_updates_all_opt_(size)
#define _Inout_updates_bytes_(size)                                 _SAL2_NAME(_Inout_updates_bytes_) _Group_(_Pre_bytecap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)] )
//#define _Inout_updates_bytes_all_(size)
//#define _Inout_updates_bytes_all_opt_(size)
#define _Inout_updates_bytes_opt_(size)                             _SAL2_NAME(_Inout_updates_bytes_opt_) _Group_(_Pre_opt_bytecap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)])
#define _Inout_updates_bytes_to_(size,count)                        _SAL2_NAME(_Inout_updates_bytes_to_) _Group_(_Out_writes_bytes_to_(size,count) [SA_Pre(Valid=SA_Yes)] [SA_Pre(ValidBytes="\n" _SA_SPECSTRIZE(count))])
#define _Inout_updates_bytes_to_opt_(size,count)                    _SAL2_NAME(_Inout_updates_bytes_to_opt_) _Group_(_Out_writes_bytes_to_opt_(size,count) [SA_Pre(Valid=SA_Yes)] [SA_Pre(ValidBytes="\n" _SA_SPECSTRIZE(count))])
#define _Inout_updates_opt_(size)                                   _SAL2_NAME(_Inout_updates_opt_) _Group_(_Pre_opt_cap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)])
#define _Inout_updates_opt_z_(size)                                 _SAL2_NAME(_Inout_updates_opt_z_) _Group_(_Pre_opt_cap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)] [SA_Pre(NullTerminated=SA_Yes)] [SA_Post(NullTerminated=SA_Yes)])
#define _Inout_updates_to_(size,count)                              _SAL2_NAME(_Inout_updates_to_) _Group_(_Out_writes_to_(size,count) [SA_Pre(Valid=SA_Yes)] [SA_Pre(ValidElements="\n" _SA_SPECSTRIZE(count))])
//#define _Inout_updates_to_opt_(size,count)
#define _Inout_updates_z_(size)                                     _SAL2_NAME(_Inout_updates_z_) _Group_(_Pre_cap_(size) [SA_Pre(Valid=SA_Yes)] [SA_Post(Valid=SA_Yes)] [SA_Pre(NullTerminated=SA_Yes)] [SA_Post(NullTerminated=SA_Yes)])
#define _Inout_z_                                                   _SAL2_NAME(_Inout_z_) _Group_(_Prepost_z_)
//#define _Inout_z_bytecap_(size)
//#define _Inout_z_bytecap_c_(size)
//#define _Inout_z_bytecap_x_(size)
//#define _Inout_z_bytecount_(size)
//#define _Inout_z_bytecount_c_(size)
//#define _Inout_z_cap_(size)
//#define _Inout_z_cap_c_(size)
//#define _Inout_z_cap_x_(size)
//#define _Inout_z_count_(size)
//#define _Inout_z_count_c_(size)
#define _Interlocked_operand_                                       [SAL_pre] [SAL_annotes(Name="SAL_interlocked")]
#define _Literal_                                                   _SAL2_NAME(_Literal_) _Group_([SAL_pre] [SAL_annotes(Name="SAL_constant", p1="__yes")])
#define _Maybenull_                                                 [SAL_annotes(Name="SAL_null", p1="__maybe")]
#define _Maybevalid_                                                [SAL_annotes(Name="SAL_valid", p1="__maybe")]
//#define _Maybe_raises_SEH_exception
#define _Must_inspect_result_                                       _SAL2_NAME(_Must_inspect_result_) _Group_(_Post_ [SAL_annotes(Name="SAL_mustInspect")] [SA_Post(MustCheck=SA_Yes)])
#define _Notliteral_                                                _SAL2_NAME(_Notliteral_) _Group_([SAL_pre] [SAL_annotes(Name="SAL_constant", p1="__no")] )
#define _Notnull_                                                   [SAL_annotes(Name="SAL_null", p1="__no")]
//#define _Notref_
//#define _Notvalid_
#define _Null_                                                     [SAL_annotes(Name="SAL_null", p1="__yes")]
#define _Null_terminated_                                           _SAL2_NAME(_Null_terminated_) _Group_([SAL_annotes(Name="SAL_nullTerminated", p1="__yes")])
#define _NullNull_terminated_                                       _SAL2_NAME(_NullNull_terminated_) _Group_([SAL_annotes(Name="SAL_nullTerminated", p1="__yes")] [SAL_annotes(Name="SAL_readableTo", p1="inexpressibleCount(\"NullNull terminated string\")")])
//#define _On_failure_(annos)
#define _Out_                                                       _SAL2_NAME(_Out_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)])
//#define _Out_bound_
#define _Out_bytecap_(size)                                         _SAL11_NAME(_Out_bytecap_) _Group_(_Pre_bytecap_(size) [SA_Post(Valid=SA_Yes)])
//#define _Out_bytecap_c_(size)
//#define _Out_bytecap_post_bytecount_(cap,count)
//#define _Out_bytecap_x_(size)
//#define _Out_bytecapcount_(capcount)
//#define _Out_bytecapcount_x_(capcount)
#define _Out_cap_(size)                                             _SAL11_NAME(_Out_cap_) _Group_(_Pre_cap_(size) [SA_Post(Valid=SA_Yes)])
//#define _Out_cap_c_(size)
//#define _Out_cap_m_(mult,size)
//#define _Out_cap_post_count_(cap,count)
//#define _Out_cap_x_(size)
//#define _Out_capcount_(capcount)
//#define _Out_capcount_x_(capcount)
//#define _Out_defensive_(annotes)
#define _Out_opt_                                                   _SAL2_NAME(_Out_opt_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)])
#define _Out_opt_bytecap_(size)                                     _SAL11_NAME(_Out_opt_bytecap_) _Group_(_Pre_opt_bytecap_(size) [SA_Post(Valid=SA_Yes)])
//#define _Out_opt_bytecap_c_(size)
//#define _Out_opt_bytecap_post_bytecount_(cap,count)
//#define _Out_opt_bytecap_x_(size)
//#define _Out_opt_bytecapcount_(capcount)
//#define _Out_opt_bytecapcount_x_(capcount)
#define _Out_opt_cap_(size)                                         _SAL11_NAME(_Out_opt_cap_) _Group_(_Pre_opt_cap_(size) [SA_Post(Valid=SA_Yes)])
//#define _Out_opt_cap_c_(size)
//#define _Out_opt_cap_m_(mult,size)
//#define _Out_opt_cap_post_count_(cap,count)
//#define _Out_opt_cap_x_(size)
//#define _Out_opt_capcount_(capcount)
//#define _Out_opt_capcount_x_(capcount)
//#define _Out_opt_ptrdiff_cap_(size)
#define _Out_opt_z_bytecap_(size)                                   _SAL11_NAME(_Out_opt_z_bytecap_) _Group_(_Pre_opt_bytecap_(size) [SA_Post(Valid=SA_Yes)] _Post_z_)
//#define _Out_opt_z_bytecap_c_(size)
//#define _Out_opt_z_bytecap_post_bytecount_(cap,count)
//#define _Out_opt_z_bytecap_x_(size)
//#define _Out_opt_z_bytecapcount_(capcount)
//#define _Out_opt_z_cap_(size)
//#define _Out_opt_z_cap_c_(size)
//#define _Out_opt_z_cap_m_(mult,size)
//#define _Out_opt_z_cap_post_count_(cap,count)
//#define _Out_opt_z_cap_x_(size)
//#define _Out_opt_z_capcount_(capcount)
//#define _Out_ptrdiff_cap_(size)
#define _Out_range_(lb,ub)                                          _SAL2_NAME(_Out_range_) _Group_(_Post_ _SA_annotes2(SAL_range, lb, ub))
#define _Out_writes_(size)                                          _SAL2_NAME(_Out_writes_) _Group_(_Pre_cap_(size) [SA_Post(Valid=SA_Yes)])
#define _Out_writes_all_(size)                                      _SAL2_NAME(_Out_writes_all_) _Group_(_Out_writes_to_(_Old_(size), _Old_(size)))
#define _Out_writes_all_opt_(size)                                  _SAL2_NAME(_Out_writes_all_opt_) _Group_(_Out_writes_to_opt_(_Old_(size), _Old_(size)))
#define _Out_writes_bytes_(size)                                    _SAL2_NAME(_Out_writes_bytes_) _Group_(_Pre_bytecap_(size) [SA_Post(Valid=SA_Yes)])
#define _Out_writes_bytes_all_(size)                                _SAL2_NAME(_Out_writes_bytes_all_) _Group_(_Out_writes_bytes_to_(_Old_(size), _Old_(size)))
#define _Out_writes_bytes_all_opt_(size)                            _SAL2_NAME(_Out_writes_bytes_all_opt_) _Group_(_Out_writes_bytes_to_opt_(_Old_(size), _Old_(size)))
#define _Out_writes_bytes_opt_(size)                                _SAL2_NAME(_Out_writes_bytes_opt_) _Group_(_Pre_opt_bytecap_(size) [SA_Post(Valid=SA_Yes)])
#define _Out_writes_bytes_to_(size,count)                           _SAL2_NAME(_Out_writes_bytes_to_) _Group_(_Pre_bytecap_(size) [SA_Post(Valid=SA_Yes)] _Post_bytecount_(count))
#define _Out_writes_bytes_to_opt_(size,count)                       _SAL2_NAME(_Out_writes_bytes_to_opt_) _Group_(_Pre_opt_bytecap_(size) [SA_Post(Valid=SA_Yes)] _Post_bytecount_(count) )
#define _Out_writes_opt_(size)                                      _SAL2_NAME(_Out_writes_opt_) _Group_(_Pre_opt_cap_(size) [SA_Post(Valid=SA_Yes)])
#define _Out_writes_opt_z_(size)                                    _SAL2_NAME(_Out_writes_opt_z_) _Group_(_Pre_opt_cap_(size) [SA_Post(Valid=SA_Yes)] _Post_z_)
#define _Out_writes_to_(size,count)                                 _SAL2_NAME(_Out_writes_to_) _Group_(_Pre_cap_(size) [SA_Post(Valid=SA_Yes)] _Post_count_(count))
#define _Out_writes_to_opt_(size,count)                             _SAL2_NAME(_Out_writes_to_opt_) _Group_(_Pre_opt_cap_(size) [SA_Post(Valid=SA_Yes)] _Post_count_(count))
//#define _Out_writes_to_ptr_(ptr)
//#define _Out_writes_to_ptr_opt_(ptr)
//#define _Out_writes_to_ptr_opt_z_(ptr)
//#define _Out_writes_to_ptr_z_(ptr)
#define _Out_writes_z_(size)                                        _SAL2_NAME(_Out_writes_z_) _Group_(_Pre_cap_(size) [SA_Post(Valid=SA_Yes)] _Post_z_)
#define _Out_z_bytecap_(size)                                       _SAL11_NAME(_Out_z_bytecap_) _Group_(_Pre_bytecap_(size) [SA_Post(Valid=SA_Yes)] _Post_z_)
//#define _Out_z_bytecap_c_(size)
#define _Out_z_bytecap_post_bytecount_(cap,count)                   _SAL11_NAME(_Out_z_bytecap_post_bytecount_) _Group_(_Pre_bytecap_(cap) [SA_Post(Valid=SA_Yes)] _Post_z_bytecount_(count))
//#define _Out_z_bytecap_x_(size)
//#define _Out_z_bytecapcount_(capcount)
//#define _Out_z_cap_(size)
//#define _Out_z_cap_c_(size)
//#define _Out_z_cap_m_(mult,size)
//#define _Out_z_cap_post_count_(cap,count)
//#define _Out_z_cap_x_(size)
//#define _Out_z_capcount_(capcount)
#define _Outptr_                                                    _SAL2_NAME(_Outptr_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,ValidElements="\n""1")])
#define _Outptr_opt_                                                _SAL2_NAME(_Outptr_opt_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,ValidElements="\n""1")])
#define _Outptr_opt_result_buffer_(size)                            _SAL2_NAME(_Outptr_opt_result_buffer_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,WritableElements="\n" _SA_SPECSTRIZE(size))])
//#define _Outptr_opt_result_buffer_all_(size)
//#define _Outptr_opt_result_buffer_all_maybenull_(size)
//#define _Outptr_opt_result_buffer_maybenull_(size)
//#define _Outptr_opt_result_buffer_to_(size, count)
//#define _Outptr_opt_result_buffer_to_maybenull_(size, count)
#define _Outptr_opt_result_bytebuffer_(size)                        _SAL2_NAME(_Outptr_opt_result_bytebuffer_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,WritableBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Outptr_opt_result_bytebuffer_all_(size)
#define _Outptr_opt_result_bytebuffer_all_maybenull_(size)          _SAL2_NAME(_Outptr_opt_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_Maybe,Notref=1,ValidBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Outptr_opt_result_bytebuffer_maybenull_(size)
//#define _Outptr_opt_result_bytebuffer_to_(size, count)
//#define _Outptr_opt_result_bytebuffer_to_maybenull_(size, count)
#define _Outptr_opt_result_maybenull_                               _SAL2_NAME(_Outptr_opt_result_maybenull_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_Maybe,Notref=1,ValidElements="\n""1")])
//#define _Outptr_opt_result_maybenull_z_
//#define _Outptr_opt_result_nullonfailure_
//#define _Outptr_opt_result_z_
#define _Outptr_result_buffer_(size)                                _SAL2_NAME(_Outptr_result_buffer_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,WritableElements="\n" _SA_SPECSTRIZE(size))] )
//#define _Outptr_result_buffer_all_(size)
//#define _Outptr_result_buffer_all_maybenull_(size)
#define _Outptr_result_buffer_maybenull_(size)                      _SAL2_NAME(_Outptr_result_buffer_maybenull_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_Maybe,Notref=1,WritableElements="\n" _SA_SPECSTRIZE(size))])
//#define _Outptr_result_buffer_to_(size, count)
//#define _Outptr_result_buffer_to_maybenull_(size, count)
#define _Outptr_result_bytebuffer_(size)                            _SAL2_NAME(_Outptr_result_bytebuffer_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,WritableBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Outptr_result_bytebuffer_all_(size)
//#define _Outptr_result_bytebuffer_all_maybenull_(size)
#define _Outptr_result_bytebuffer_maybenull_(size)                  _SAL2_NAME(_Outptr_result_bytebuffer_maybenull_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_Maybe,Notref=1,WritableBytes="\n" _SA_SPECSTRIZE(size))])
#define _Outptr_result_bytebuffer_to_(size, count)                  _SAL2_NAME(_Outptr_result_bytebuffer_to_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_No,Notref=1,WritableBytes="\n" _SA_SPECSTRIZE(size), ValidBytes="\n" _SA_SPECSTRIZE(count))])
//#define _Outptr_result_bytebuffer_to_maybenull_(size, count)
#define _Outptr_result_maybenull_                                   _SAL2_NAME(_Outptr_result_maybenull_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] [SA_Post(Deref=1,Null=SA_Maybe,Notref=1,ValidElements="\n""1")] )
//#define _Outptr_result_maybenull_z_
#define _Outptr_result_nullonfailure_                               _SAL2_NAME(_Outptr_result_nullonfailure_) _Group_(_Outptr_ [SAL_context(p1="SAL_failed")] _Group_([SAL_post] _Deref_post_null_) )
#define _Outptr_result_z_                                           _SAL2_NAME(_Outptr_result_z_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElementsConst=1,Notref=1)] [SA_Post(Valid=SA_Yes)] _Deref_post_z_)
//#define _Outref_
//#define _Outref_result_buffer_(size)
//#define _Outref_result_buffer_all_(size)
//#define _Outref_result_buffer_all_maybenull_(size)
//#define _Outref_result_buffer_maybenull_(size)
//#define _Outref_result_buffer_to_(size, count)
//#define _Outref_result_buffer_to_maybenull_(size, count)
//#define _Outref_result_bytebuffer_(size)
//#define _Outref_result_bytebuffer_all_(size)
//#define _Outref_result_bytebuffer_all_maybenull_(size)
//#define _Outref_result_bytebuffer_maybenull_(size)
//#define _Outref_result_bytebuffer_to_(size, count)
//#define _Outref_result_bytebuffer_to_maybenull_(size, count)
//#define _Outref_result_maybenull_
//#define _Outref_result_nullonfailure_
#define _Points_to_data_                                            _SAL2_NAME(_Points_to_data_) _Group_([SAL_pre] [SAL_at(p1="*_Curr_")] _Group_([SAL_annotes(Name="SAL_mayBePointer", p1="__no")]) )
#define _Post_bytecap_(size)                                        _SAL11_NAME(_Post_bytecap_) _Group_([SA_Post(WritableBytes="\n" _SA_SPECSTRIZE(size))])
#define _Post_bytecount_(size)                                      _SAL11_NAME(_Post_bytecount_) _Group_([SA_Post(ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Post(Valid=SA_Yes)])
//#define _Post_bytecount_c_(size)
//#define _Post_bytecount_x_(size)
//#define _Post_cap_(size)
#define _Post_count_(size)                                         _SAL11_NAME(_Post_count_) _Group_([SA_Post(ValidElements="\n" _SA_SPECSTRIZE(size))] [SA_Post(Valid=SA_Yes)] )
//#define _Post_count_c_(size)
//#define _Post_count_x_(size)
//#define _Post_defensive_
#define _Post_equal_to_(expr)                                       _SAL2_NAME(_Post_equal_to_) _Group_(_Out_range_(==,expr))
#define _Post_invalid_                                              _SAL2_NAME(_Post_invalid_) _Group_([SA_Post(Deref=1,Valid=SA_No)])
#define _Post_maybenull_                                            _SAL2_NAME(_Post_maybenull_) _Group_([SA_Post(Null=SA_Maybe)])
#define _Post_maybez_                                               _SAL11_NAME(_Post_maybez_) _Group_([SA_Post(NullTerminated=SA_Maybe)])
#define _Post_notnull_                                              _SAL2_NAME(_Post_notnull_) _Group_([SA_Post(Null=SA_No)])
#define _Post_null_                                                 _SAL2_NAME(_Post_null_) _Group_([SA_Post(Null=SA_Yes)])
#define _Post_ptr_invalid_                                          _SAL2_NAME(_Post_ptr_invalid_) _Group_([SA_Post(Valid=SA_No)])
#define _Post_readable_byte_size_(size)                             _SAL2_NAME(_Post_readable_byte_size_) _Group_([SA_Post(ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Post(Valid=SA_Yes)])
#define _Post_readable_size_(size)                                  _SAL2_NAME(_Post_readable_size_) _Group_([SA_Post(ValidElements="\n" _SA_SPECSTRIZE(size))] [SA_Post(Valid=SA_Yes)])
#define _Post_satisfies_(cond)                                      _SAL2_NAME(_Post_satisfies_) _Group_([SAL_post] [SAL_annotes(Name="SAL_satisfies", p1=_SA_SPECSTRIZE(cond))])
#define _Post_valid_                                                _SAL2_NAME(_Post_valid_) _Group_([SA_Post(Valid=SA_Yes)])
#define _Post_writable_byte_size_(size)                             _SAL2_NAME(_Post_writable_byte_size_) _Group_([SA_Post(WritableBytes="\n" _SA_SPECSTRIZE(size))] )
//#define _Post_writable_size_(size)
#define _Post_z_                                                    _SAL2_NAME(_Post_z_) _Group_([SA_Post(NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)])
#define _Post_z_bytecount_(size)                                    _SAL11_NAME(_Post_z_bytecount_) _Group_([SA_Post(NullTerminated=SA_Yes,ValidBytes="\n" _SA_SPECSTRIZE(size) )] [SA_Post(Valid=SA_Yes)])
//#define _Post_z_bytecount_c_(size)
//#define _Post_z_bytecount_x_(size)
//#define _Post_z_count_(size)
//#define _Post_z_count_c_(size)
//#define _Post_z_count_x_(size)
#define _Pre_bytecap_(size)                                         _SAL11_NAME(_Pre_bytecap_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableBytes="\n" _SA_SPECSTRIZE(size) )])
//#define _Pre_bytecap_c_(size)
//#define _Pre_bytecap_x_(size)
#define _Pre_bytecount_(size)                                       _SAL11_NAME(_Pre_bytecount_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Pre_bytecount_c_(size)
//#define _Pre_bytecount_x_(size)
#define _Pre_cap_(size)                                             _SAL11_NAME(_Pre_cap_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableElements="\n" _SA_SPECSTRIZE(size) )])
//#define _Pre_cap_c_(size)
//#define _Pre_cap_c_one_
//#define _Pre_cap_for_(param)
//#define _Pre_cap_m_(mult,size)
//#define _Pre_cap_x_(size)
#define _Pre_count_(size)                                           _SAL11_NAME(_Pre_count_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(ValidElements="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)])
#define _Pre_count_c_(size)                                         _SAL11_NAME(_Pre_count_c_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(ValidElementsConst=size)] [SA_Pre(Valid=SA_Yes)])
//#define _Pre_count_x_(size)
//#define _Pre_defensive_
//#define _Pre_equal_to_(expr)
//#define _Pre_invalid_
#define _Pre_maybenull_                                             _SAL2_NAME(_Pre_maybenull_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)])
#define _Pre_notnull_                                               _SAL2_NAME(_Pre_notnull_) _Group_([SA_Pre(Null=SA_No,Notref=1)])
//#define _Pre_null_
#define _Pre_opt_bytecap_(size)                                     _SAL11_NAME(_Pre_opt_bytecap_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableBytes="\n" _SA_SPECSTRIZE(size))] )
//#define _Pre_opt_bytecap_c_(size)
//#define _Pre_opt_bytecap_x_(size)
#define _Pre_opt_bytecount_(size)                                   _SAL11_NAME(_Pre_opt_bytecount_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Pre_opt_bytecount_c_(size)
//#define _Pre_opt_bytecount_x_(size)
#define _Pre_opt_cap_(size)                                         _SAL11_NAME(_Pre_opt_cap_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(WritableElements="\n" _SA_SPECSTRIZE(size))] )
//#define _Pre_opt_cap_c_(size)
//#define _Pre_opt_cap_c_one_
//#define _Pre_opt_cap_for_(param)
//#define _Pre_opt_cap_m_(mult,size)
//#define _Pre_opt_cap_x_(size)
#define _Pre_opt_count_(size)                                       _SAL11_NAME(_Pre_opt_count_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(ValidElements="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Pre_opt_count_c_(size)
//#define _Pre_opt_count_x_(size)
//#define _Pre_opt_ptrdiff_cap_(ptr)
//#define _Pre_opt_ptrdiff_count_(ptr)
#define _Pre_opt_valid_                                             _SAL2_NAME(_Pre_opt_valid_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(Valid=SA_Yes)])
//#define _Pre_opt_valid_bytecap_(size)
//#define _Pre_opt_valid_bytecap_c_(size)
//#define _Pre_opt_valid_bytecap_x_(size)
//#define _Pre_opt_valid_cap_(size)
//#define _Pre_opt_valid_cap_c_(size)
//#define _Pre_opt_valid_cap_x_(size)
#define _Pre_opt_z_                                                 _SAL11_NAME(_Pre_opt_z_) _Group_([SA_Pre(Null=SA_Maybe,Notref=1)] [SA_Pre(NullTerminated=SA_Yes)] [SA_Pre(Valid=SA_Yes)])
//#define _Pre_opt_z_bytecap_(size)
//#define _Pre_opt_z_bytecap_c_(size)
//#define _Pre_opt_z_bytecap_x_(size)
//#define _Pre_opt_z_cap_(size)
//#define _Pre_opt_z_cap_c_(size)
//#define _Pre_opt_z_cap_x_(size)
//#define _Pre_ptrdiff_cap_(ptr)
//#define _Pre_ptrdiff_count_(ptr)
//#define _Pre_readable_byte_size_(size)
#define _Pre_readable_size_(size)                                   _SAL2_NAME(_Pre_readable_size_) _Group_([SA_Pre(ValidElements="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Pre_readonly_
#define _Pre_satisfies_(cond)                                       _SAL2_NAME(_Pre_satisfies_) _Group_([SAL_pre] [SAL_annotes(Name="SAL_satisfies", p1=_SA_SPECSTRIZE(cond))])
#define _Pre_unknown_
#define _Pre_valid_                                                 _SAL2_NAME(_Pre_valid_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(Valid=SA_Yes)] )
#define _Pre_valid_bytecap_(size)                                   _SAL11_NAME(_Pre_valid_bytecap_) _Group_([SA_Pre(Null=SA_No,Notref=1)] [SA_Pre(WritableBytes="\n" _SA_SPECSTRIZE(size))] [SA_Pre(Valid=SA_Yes)] )
//#define _Pre_valid_bytecap_c_(size)
//#define _Pre_valid_bytecap_x_(size)
//#define _Pre_valid_cap_(size)
//#define _Pre_valid_cap_c_(size)
//#define _Pre_valid_cap_x_(size)
//#define _Pre_writable_byte_size_(size)
//#define _Pre_writable_size_(size)
//#define _Pre_writeonly_
#define _Pre_z_                                                     _SAL2_NAME(_Pre_z_) _Group_([SA_Pre(NullTerminated=SA_Yes)] [SA_Pre(Valid=SA_Yes)])
//#define _Pre_z_bytecap_(size)
//#define _Pre_z_bytecap_c_(size)
//#define _Pre_z_bytecap_x_(size)
//#define _Pre_z_cap_(size)
//#define _Pre_z_cap_c_(size)
//#define _Pre_z_cap_x_(size)
#define _Prepost_bytecount_(size)                                   _SAL11_NAME(_Prepost_bytecount_) _Group_(_Pre_bytecount_(size) _Post_bytecount_(size))
//#define _Prepost_bytecount_c_(size)
//#define _Prepost_bytecount_x_(size)
//#define _Prepost_count_(size)
//#define _Prepost_count_c_(size)
//#define _Prepost_count_x_(size)
//#define _Prepost_opt_bytecount_(size)
//#define _Prepost_opt_bytecount_c_(size)
//#define _Prepost_opt_bytecount_x_(size)
//#define _Prepost_opt_count_(size)
//#define _Prepost_opt_count_c_(size)
//#define _Prepost_opt_count_x_(size)
#define _Prepost_opt_valid_                                         _SAL2_NAME(_Prepost_opt_valid_) _Group_(_Pre_opt_valid_ _Post_valid_)
#define _Prepost_opt_z_                                             _SAL11_NAME(_Prepost_opt_z_) _Group_(_Pre_opt_z_ _Post_z_)
#define _Prepost_valid_                                             _SAL11_NAME(_Prepost_valid_) _Group_(_Pre_valid_ _Post_valid_)
#define _Prepost_z_                                                 _SAL2_NAME(_Prepost_z_) _Group_(_Pre_z_ _Post_z_)
#define _Printf_format_string_                                      _SAL2_NAME(_Printf_format_string_) _Group_([SA_FormatString(Style="printf")] )
//#define _Raises_SEH_exception_
#define _Maybe_raises_SEH_exception_
#define _Readable_bytes_(size)                                      _SAL2_NAME(_Readable_bytes_) _Group_(_SA_annotes1(SAL_readableTo, byteCount(size)))
#define _Readable_elements_(size)                                   _SAL2_NAME(_Readable_elements_) _Group_([SAL_annotes(Name="SAL_readableTo", p1="elementCount(size)")])
#define _Reserved_                                                  _SAL2_NAME(_Reserved_) _Group_([SA_Pre(Null=SA_Yes)])
//#define _Result_nullonfailure_
//#define _Result_zeroonfailure_
#define __inner_callback                                            [SAL_annotes(Name="__callback")]
//#define _Ret_
//#define _Ret_bound_
//#define _Ret_bytecap_(size)
//#define _Ret_bytecap_c_(size)
//#define _Ret_bytecap_x_(size)
//#define _Ret_bytecount_(size)
//#define _Ret_bytecount_c_(size)
//#define _Ret_bytecount_x_(size)
//#define _Ret_cap_(size)
//#define _Ret_cap_c_(size)
//#define _Ret_cap_x_(size)
//#define _Ret_count_(size)
//#define _Ret_count_c_(size)
//#define _Ret_count_x_(size)
#define _Ret_maybenull_                                             _SAL2_NAME(_Ret_maybenull_) _Group_([SA_Post(Null=SA_Maybe)])
//#define _Ret_maybenull_z_
#define _Ret_notnull_                                               _SAL2_NAME(_Ret_notnull_) _Group_([SA_Post(Null=SA_No)])
//#define _Ret_null_
#define _Ret_opt_                                                   _Ret_opt_valid_
#define _Ret_opt_bytecap_(size)                                     _SAL11_NAME(_Ret_opt_bytecap_) _Group_([SA_Post(Null=SA_Maybe,Notref=1)] [SA_Post(WritableBytes="\n" _SA_SPECSTRIZE(size))])
//#define _Ret_opt_bytecap_c_(size)
//#define _Ret_opt_bytecap_x_(size)
#define _Ret_opt_bytecount_(size)                                   _SAL11_NAME(_Ret_opt_bytecount_) _Group_([SA_Post(Null=SA_Maybe,Notref=1)] [SA_Post(ValidBytes="\n" _SA_SPECSTRIZE(size))] [SA_Post(Valid=SA_Yes)])
//#define _Ret_opt_bytecount_c_(size)
//#define _Ret_opt_bytecount_x_(size)
//#define _Ret_opt_cap_(size)
//#define _Ret_opt_cap_c_(size)
//#define _Ret_opt_cap_x_(size)
//#define _Ret_opt_count_(size)
//#define _Ret_opt_count_c_(size)
//#define _Ret_opt_count_x_(size)
#define _Ret_opt_valid_                                             _SAL11_NAME(_Ret_opt_valid_) _Group_([SA_Post(Null=SA_Maybe,Notref=1)] [SA_Post(Valid=SA_Yes)])
#define _Ret_opt_z_                                                 _SAL11_NAME(_Ret_opt_z_) _Group_([SA_Post(Null=SA_Maybe,NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)] )
//#define _Ret_opt_z_bytecap_(size)
//#define _Ret_opt_z_bytecount_(size)
//#define _Ret_opt_z_cap_(size)
//#define _Ret_opt_z_count_(size)
#define _Ret_range_(lb,ub)                                          _SAL2_NAME(_Ret_range_) _Group_([SAL_post] [SAL_annotes(Name="SAL_range", p1=_SA_SPECSTRIZE(lb), p2=_SA_SPECSTRIZE(ub))] )
//#define _Ret_valid_
//#define _Ret_writes_(size)
//#define _Ret_writes_bytes_(size)
//#define _Ret_writes_bytes_maybenull_(size)
//#define _Ret_writes_bytes_to_(size,count)
//#define _Ret_writes_bytes_to_maybenull_(size,count)
//#define _Ret_writes_maybenull_(size)
#define _Ret_writes_maybenull_z_(size)                              _SAL2_NAME(_Ret_writes_maybenull_z_) _Group_([SA_Post(Null=SA_Maybe,ValidElements="\n" _SA_SPECSTRIZE(size),NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)])
//#define _Ret_writes_to_(size,count)
//#define _Ret_writes_to_maybenull_(size,count)
//#define _Ret_writes_z_(size)
#define _Ret_z_                                                     _SAL2_NAME(_Ret_z_) _Group_([SA_Post(Null=SA_No,NullTerminated=SA_Yes)] [SA_Post(Valid=SA_Yes)])
//#define _Ret_z_bytecap_(size)
//#define _Ret_z_bytecount_(size)
//#define _Ret_z_cap_(size)
//#define _Ret_z_count_(size)
#define _Return_type_success_(expr)                                 _SAL2_NAME(_Return_type_success_) _Group_([SA_Success(Condition=_SA_SPECSTRIZE(expr))])
#define _Scanf_format_string_                                       _SAL2_NAME(_Scanf_format_string_) _Group_([SA_FormatString(Style="scanf")])
#define _Scanf_s_format_string_                                     _SAL2_NAME(_Scanf_s_format_string_) _Group_([SA_FormatString(Style="scanf_s")])
#define _Struct_size_bytes_(size)                                   _SAL2_NAME(_Struct_size_bytes_) _Group_(_Writable_bytes_(byteCount(size)))
#define _Success_(expr)                                             _SAL2_NAME(_Success_) _Group_([SA_Success(Condition=_SA_SPECSTRIZE(expr))])
#define _Unchanged_(expr)                                           _SAL2_NAME(_Unchanged_) _Group_([SAL_at(p1=_SA_SPECSTRIZE(expr))] _Group_(_Post_equal_to_(expr) _Const_))
//#define _Use_decl_annotations_
#define _Valid_                                                     [SAL_annotes(Name="SAL_valid", p1="__yes")]
#define _Writable_bytes_(size)                                      _SAL2_NAME(_Writable_bytes_) _Group_(_SA_annotes1(SAL_writableTo, byteCount(size)))
#define _Writable_elements_(size)                                   _SAL2_NAME(_Writable_elements_) _Group_(_SA_annotes1(SAL_writableTo, elementCount(size)))

#define __In_impl_ [SA_Pre(Valid=SA_Yes)] [SA_Pre(Deref=1, Notref=1, Access=SA_Read)]
#define __AuToQuOtE _SA_annotes0(SAL_AuToQuOtE)

#define __ANNOTATION(fun) _SA_annotes0(SAL_annotation) void __SA_##fun
__ANNOTATION(SAL_useHeader(void));
__ANNOTATION(SAL_bound(void));
__ANNOTATION(SAL_allocator(void));
__ANNOTATION(SAL_file_parser(__AuToQuOtE __In_impl_ char *, __In_impl_ char *));
__ANNOTATION(SAL_source_code_content(__In_impl_ char *));
__ANNOTATION(SAL_analysisHint(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_untrusted_data_source(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_untrusted_data_source_this(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_validated(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_validated_this(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_encoded(void));
__ANNOTATION(SAL_adt(__AuToQuOtE __In_impl_ char *, __AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_add_adt_property(__AuToQuOtE __In_impl_ char *, __AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_remove_adt_property(__AuToQuOtE __In_impl_ char *, __AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_transfer_adt_property_from(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_post_type(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_volatile(void));
__ANNOTATION(SAL_nonvolatile(void));
__ANNOTATION(SAL_entrypoint(__AuToQuOtE __In_impl_ char *, __AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_blocksOn(__In_impl_ void*));
__ANNOTATION(SAL_mustInspect(void));
__ANNOTATION(SAL_TypeName(__AuToQuOtE __In_impl_ char *));
__ANNOTATION(SAL_interlocked(void);)
#pragma warning (suppress: 28227 28241)
__ANNOTATION(SAL_name(__In_impl_ char *, __In_impl_ char *, __In_impl_ char *);)

#define __PRIMOP(type, fun) _SA_annotes0(SAL_primop) type __SA_##fun;
__PRIMOP(char *, _Macro_value_(__In_impl_ char *));
__PRIMOP(int, _Macro_defined_(__In_impl_ char *));
__PRIMOP(char *, _Strstr_(__In_impl_ char *, __In_impl_ char *));

__ANNOTATION(SAL_functionClassNew(__In_impl_ char*);)
__PRIMOP(int, _In_function_class_(__In_impl_ char*);)

#define _In_function_class_(x) _In_function_class_(#x)
#define _Called_from_function_class_(x) _In_function_class_(x)

#else // ] [ !_PREFAST_

/* Dummys */
#define __inner_exceptthat
#define __inner_typefix(ctype)
#define _Always_(annos)
#define _Analysis_noreturn_
#define _Analysis_assume_(expr) ((void)0)
#define __analysis_assume(expr) ((void)0)
#define _At_(target, annos)
#define _At_buffer_(target, iter, bound, annos)
#define _Check_return_
#define _COM_Outptr_
#define _COM_Outptr_opt_
#define _COM_Outptr_opt_result_maybenull_
#define _COM_Outptr_result_maybenull_
#define _Const_
#define _Deref_in_bound_
#define _Deref_in_range_(lb,ub)
#define _Deref_inout_bound_
#define _Deref_inout_z_
#define _Deref_inout_z_bytecap_c_(size)
#define _Deref_inout_z_cap_c_(size)
#define _Deref_opt_out_
#define _Deref_opt_out_opt_
#define _Deref_opt_out_opt_z_
#define _Deref_opt_out_z_
#define _Deref_out_
#define _Deref_out_bound_
#define _Deref_out_opt_
#define _Deref_out_opt_z_
#define _Deref_out_range_(lb,ub)
#define _Deref_out_z_
#define _Deref_out_z_bytecap_c_(size)
#define _Deref_out_z_cap_c_(size)
#define _Deref_post_bytecap_(size)
#define _Deref_post_bytecap_c_(size)
#define _Deref_post_bytecap_x_(size)
#define _Deref_post_bytecount_(size)
#define _Deref_post_bytecount_c_(size)
#define _Deref_post_bytecount_x_(size)
#define _Deref_post_cap_(size)
#define _Deref_post_cap_c_(size)
#define _Deref_post_cap_x_(size)
#define _Deref_post_count_(size)
#define _Deref_post_count_c_(size)
#define _Deref_post_count_x_(size)
#define _Deref_post_maybenull_
#define _Deref_post_notnull_
#define _Deref_post_null_
#define _Deref_post_opt_bytecap_(size)
#define _Deref_post_opt_bytecap_c_(size)
#define _Deref_post_opt_bytecap_x_(size)
#define _Deref_post_opt_bytecount_(size)
#define _Deref_post_opt_bytecount_c_(size)
#define _Deref_post_opt_bytecount_x_(size)
#define _Deref_post_opt_cap_(size)
#define _Deref_post_opt_cap_c_(size)
#define _Deref_post_opt_cap_x_(size)
#define _Deref_post_opt_count_(size)
#define _Deref_post_opt_count_c_(size)
#define _Deref_post_opt_count_x_(size)
#define _Deref_post_opt_valid_
#define _Deref_post_opt_valid_bytecap_(size)
#define _Deref_post_opt_valid_bytecap_c_(size)
#define _Deref_post_opt_valid_bytecap_x_(size)
#define _Deref_post_opt_valid_cap_(size)
#define _Deref_post_opt_valid_cap_c_(size)
#define _Deref_post_opt_valid_cap_x_(size)
#define _Deref_post_opt_z_
#define _Deref_post_opt_z_bytecap_(size)
#define _Deref_post_opt_z_bytecap_c_(size)
#define _Deref_post_opt_z_bytecap_x_(size)
#define _Deref_post_opt_z_cap_(size)
#define _Deref_post_opt_z_cap_c_(size)
#define _Deref_post_opt_z_cap_x_(size)
#define _Deref_post_valid_
#define _Deref_post_valid_bytecap_(size)
#define _Deref_post_valid_bytecap_c_(size)
#define _Deref_post_valid_bytecap_x_(size)
#define _Deref_post_valid_cap_(size)
#define _Deref_post_valid_cap_c_(size)
#define _Deref_post_valid_cap_x_(size)
#define _Deref_post_z_
#define _Deref_post_z_bytecap_(size)
#define _Deref_post_z_bytecap_c_(size)
#define _Deref_post_z_bytecap_x_(size)
#define _Deref_post_z_cap_(size)
#define _Deref_post_z_cap_c_(size)
#define _Deref_post_z_cap_x_(size)
#define _Deref_pre_bytecap_(size)
#define _Deref_pre_bytecap_c_(size)
#define _Deref_pre_bytecap_x_(size)
#define _Deref_pre_bytecount_(size)
#define _Deref_pre_bytecount_c_(size)
#define _Deref_pre_bytecount_x_(size)
#define _Deref_pre_cap_(size)
#define _Deref_pre_cap_c_(size)
#define _Deref_pre_cap_x_(size)
#define _Deref_pre_count_(size)
#define _Deref_pre_count_c_(size)
#define _Deref_pre_count_x_(size)
#define _Deref_pre_invalid_
#define _Deref_pre_maybenull_
#define _Deref_pre_notnull_
#define _Deref_pre_null_
#define _Deref_pre_opt_bytecap_(size)
#define _Deref_pre_opt_bytecap_c_(size)
#define _Deref_pre_opt_bytecap_x_(size)
#define _Deref_pre_opt_bytecount_(size)
#define _Deref_pre_opt_bytecount_c_(size)
#define _Deref_pre_opt_bytecount_x_(size)
#define _Deref_pre_opt_cap_(size)
#define _Deref_pre_opt_cap_c_(size)
#define _Deref_pre_opt_cap_x_(size)
#define _Deref_pre_opt_count_(size)
#define _Deref_pre_opt_count_c_(size)
#define _Deref_pre_opt_count_x_(size)
#define _Deref_pre_opt_valid_
#define _Deref_pre_opt_valid_bytecap_(size)
#define _Deref_pre_opt_valid_bytecap_c_(size)
#define _Deref_pre_opt_valid_bytecap_x_(size)
#define _Deref_pre_opt_valid_cap_(size)
#define _Deref_pre_opt_valid_cap_c_(size)
#define _Deref_pre_opt_valid_cap_x_(size)
#define _Deref_pre_opt_z_
#define _Deref_pre_opt_z_bytecap_(size)
#define _Deref_pre_opt_z_bytecap_c_(size)
#define _Deref_pre_opt_z_bytecap_x_(size)
#define _Deref_pre_opt_z_cap_(size)
#define _Deref_pre_opt_z_cap_c_(size)
#define _Deref_pre_opt_z_cap_x_(size)
#define _Deref_pre_readonly_
#define _Deref_pre_valid_
#define _Deref_pre_valid_bytecap_(size)
#define _Deref_pre_valid_bytecap_c_(size)
#define _Deref_pre_valid_bytecap_x_(size)
#define _Deref_pre_valid_cap_(size)
#define _Deref_pre_valid_cap_c_(size)
#define _Deref_pre_valid_cap_x_(size)
#define _Deref_pre_writeonly_
#define _Deref_pre_z_
#define _Deref_pre_z_bytecap_(size)
#define _Deref_pre_z_bytecap_c_(size)
#define _Deref_pre_z_bytecap_x_(size)
#define _Deref_pre_z_cap_(size)
#define _Deref_pre_z_cap_c_(size)
#define _Deref_pre_z_cap_x_(size)
#define _Deref_prepost_bytecap_(size)
#define _Deref_prepost_bytecap_x_(size)
#define _Deref_prepost_bytecount_(size)
#define _Deref_prepost_bytecount_x_(size)
#define _Deref_prepost_cap_(size)
#define _Deref_prepost_cap_x_(size)
#define _Deref_prepost_count_(size)
#define _Deref_prepost_count_x_(size)
#define _Deref_prepost_opt_bytecap_(size)
#define _Deref_prepost_opt_bytecap_x_(size)
#define _Deref_prepost_opt_bytecount_(size)
#define _Deref_prepost_opt_bytecount_x_(size)
#define _Deref_prepost_opt_cap_(size)
#define _Deref_prepost_opt_cap_x_(size)
#define _Deref_prepost_opt_count_(size)
#define _Deref_prepost_opt_count_x_(size)
#define _Deref_prepost_opt_valid_
#define _Deref_prepost_opt_valid_bytecap_(size)
#define _Deref_prepost_opt_valid_bytecap_x_(size)
#define _Deref_prepost_opt_valid_cap_(size)
#define _Deref_prepost_opt_valid_cap_x_(size)
#define _Deref_prepost_opt_z_
#define _Deref_prepost_opt_z_bytecap_(size)
#define _Deref_prepost_opt_z_cap_(size)
#define _Deref_prepost_valid_
#define _Deref_prepost_valid_bytecap_(size)
#define _Deref_prepost_valid_bytecap_x_(size)
#define _Deref_prepost_valid_cap_(size)
#define _Deref_prepost_valid_cap_x_(size)
#define _Deref_prepost_z_
#define _Deref_prepost_z_bytecap_(size)
#define _Deref_prepost_z_cap_(size)
#define _Deref_ret_bound_
#define _Deref_ret_opt_z_
#define _Deref_ret_range_(lb,ub)
#define _Deref_ret_z_
#define _Deref2_pre_readonly_
#define _Field_range_(min,max)
#define _Field_size_(size)
#define _Field_size_bytes_(size)
#define _Field_size_bytes_full_(size)
#define _Field_size_bytes_full_opt_(size)
#define _Field_size_bytes_opt_(size)
#define _Field_size_bytes_part_(size, count)
#define _Field_size_bytes_part_opt_(size, count)
#define _Field_size_full_(size)
#define _Field_size_full_opt_(size)
#define _Field_size_opt_(size)
#define _Field_size_part_(size, count)
#define _Field_size_part_opt_(size, count)
#define _Field_z_
#define _Function_class_(x)
#define _Group_(annos)
#define _In_
#define _In_bound_
#define _In_bytecount_(size)
#define _In_bytecount_c_(size)
#define _In_bytecount_x_(size)
#define _In_count_(size)
#define _In_count_c_(size)
#define _In_count_x_(size)
#define _In_defensive_(annotes)
#define _In_opt_
#define _In_opt_bytecount_(size)
#define _In_opt_bytecount_c_(size)
#define _In_opt_bytecount_x_(size)
#define _In_opt_count_(size)
#define _In_opt_count_c_(size)
#define _In_opt_count_x_(size)
#define _In_opt_ptrdiff_count_(size)
#define _In_opt_z_
#define _In_opt_z_bytecount_(size)
#define _In_opt_z_bytecount_c_(size)
#define _In_opt_z_count_(size)
#define _In_opt_z_count_c_(size)
#define _In_ptrdiff_count_(size)
#define _In_range_(lb,ub)
#define _In_reads_(size)
#define _In_reads_bytes_(size)
#define _In_reads_bytes_opt_(size)
#define _In_reads_opt_(size)
#define _In_reads_opt_z_(size)
#define _In_reads_or_z_(size)
#define _In_reads_to_ptr_(ptr)
#define _In_reads_to_ptr_opt_(ptr)
#define _In_reads_to_ptr_opt_z_(ptr)
#define _In_reads_to_ptr_z_(ptr)
#define _In_reads_z_(size)
#define _In_z_
#define _In_z_bytecount_(size)
#define _In_z_bytecount_c_(size)
#define _In_z_count_(size)
#define _In_z_count_c_(size)
#define _Inout_
#define _Inout_bytecap_(size)
#define _Inout_bytecap_c_(size)
#define _Inout_bytecap_x_(size)
#define _Inout_bytecount_(size)
#define _Inout_bytecount_c_(size)
#define _Inout_bytecount_x_(size)
#define _Inout_cap_(size)
#define _Inout_cap_c_(size)
#define _Inout_cap_x_(size)
#define _Inout_count_(size)
#define _Inout_count_c_(size)
#define _Inout_count_x_(size)
#define _Inout_defensive_(annotes)
#define _Inout_opt_
#define _Inout_opt_bytecap_(size)
#define _Inout_opt_bytecap_c_(size)
#define _Inout_opt_bytecap_x_(size)
#define _Inout_opt_bytecount_(size)
#define _Inout_opt_bytecount_c_(size)
#define _Inout_opt_bytecount_x_(size)
#define _Inout_opt_cap_(size)
#define _Inout_opt_cap_c_(size)
#define _Inout_opt_cap_x_(size)
#define _Inout_opt_count_(size)
#define _Inout_opt_count_c_(size)
#define _Inout_opt_count_x_(size)
#define _Inout_opt_ptrdiff_count_(size)
#define _Inout_opt_z_
#define _Inout_opt_z_bytecap_(size)
#define _Inout_opt_z_bytecap_c_(size)
#define _Inout_opt_z_bytecap_x_(size)
#define _Inout_opt_z_bytecount_(size)
#define _Inout_opt_z_bytecount_c_(size)
#define _Inout_opt_z_cap_(size)
#define _Inout_opt_z_cap_c_(size)
#define _Inout_opt_z_cap_x_(size)
#define _Inout_opt_z_count_(size)
#define _Inout_opt_z_count_c_(size)
#define _Inout_ptrdiff_count_(size)
#define _Inout_updates_(size)
#define _Inout_updates_all_(size)
#define _Inout_updates_all_opt_(size)
#define _Inout_updates_bytes_(size)
#define _Inout_updates_bytes_all_(size)
#define _Inout_updates_bytes_all_opt_(size)
#define _Inout_updates_bytes_opt_(size)
#define _Inout_updates_bytes_to_(size,count)
#define _Inout_updates_bytes_to_opt_(size,count)
#define _Inout_updates_opt_(size)
#define _Inout_updates_opt_z_(size)
#define _Inout_updates_to_(size,count)
#define _Inout_updates_to_opt_(size,count)
#define _Inout_updates_z_(size)
#define _Inout_z_
#define _Inout_z_bytecap_(size)
#define _Inout_z_bytecap_c_(size)
#define _Inout_z_bytecap_x_(size)
#define _Inout_z_bytecount_(size)
#define _Inout_z_bytecount_c_(size)
#define _Inout_z_cap_(size)
#define _Inout_z_cap_c_(size)
#define _Inout_z_cap_x_(size)
#define _Inout_z_count_(size)
#define _Inout_z_count_c_(size)
#define _Interlocked_operand_
#define _Literal_
#define _Maybenull_
#define _Maybevalid_
#define _Maybe_raises_SEH_exception
#define _Must_inspect_result_
#define _Notliteral_
#define _Notnull_
#define _Notref_
#define _Notvalid_
#define _Null_
#define _Null_terminated_
#define _NullNull_terminated_
#define _On_failure_(annos)
#define _Out_
#define _Out_bound_
#define _Out_bytecap_(size)
#define _Out_bytecap_c_(size)
#define _Out_bytecap_post_bytecount_(cap,count)
#define _Out_bytecap_x_(size)
#define _Out_bytecapcount_(capcount)
#define _Out_bytecapcount_x_(capcount)
#define _Out_cap_(size)
#define _Out_cap_c_(size)
#define _Out_cap_m_(mult,size)
#define _Out_cap_post_count_(cap,count)
#define _Out_cap_x_(size)
#define _Out_capcount_(capcount)
#define _Out_capcount_x_(capcount)
#define _Out_defensive_(annotes)
#define _Out_opt_
#define _Out_opt_bytecap_(size)
#define _Out_opt_bytecap_c_(size)
#define _Out_opt_bytecap_post_bytecount_(cap,count)
#define _Out_opt_bytecap_x_(size)
#define _Out_opt_bytecapcount_(capcount)
#define _Out_opt_bytecapcount_x_(capcount)
#define _Out_opt_cap_(size)
#define _Out_opt_cap_c_(size)
#define _Out_opt_cap_m_(mult,size)
#define _Out_opt_cap_post_count_(cap,count)
#define _Out_opt_cap_x_(size)
#define _Out_opt_capcount_(capcount)
#define _Out_opt_capcount_x_(capcount)
#define _Out_opt_ptrdiff_cap_(size)
#define _Out_opt_z_bytecap_(size)
#define _Out_opt_z_bytecap_c_(size)
#define _Out_opt_z_bytecap_post_bytecount_(cap,count)
#define _Out_opt_z_bytecap_x_(size)
#define _Out_opt_z_bytecapcount_(capcount)
#define _Out_opt_z_cap_(size)
#define _Out_opt_z_cap_c_(size)
#define _Out_opt_z_cap_m_(mult,size)
#define _Out_opt_z_cap_post_count_(cap,count)
#define _Out_opt_z_cap_x_(size)
#define _Out_opt_z_capcount_(capcount)
#define _Out_ptrdiff_cap_(size)
#define _Out_range_(lb,ub)
#define _Out_writes_(size)
#define _Out_writes_all_(size)
#define _Out_writes_all_opt_(size)
#define _Out_writes_bytes_(size)
#define _Out_writes_bytes_all_(size)
#define _Out_writes_bytes_all_opt_(size)
#define _Out_writes_bytes_opt_(size)
#define _Out_writes_bytes_to_(size,count)
#define _Out_writes_bytes_to_opt_(size,count)
#define _Out_writes_opt_(size)
#define _Out_writes_opt_z_(size)
#define _Out_writes_to_(size,count)
#define _Out_writes_to_opt_(size,count)
#define _Out_writes_to_ptr_(ptr)
#define _Out_writes_to_ptr_opt_(ptr)
#define _Out_writes_to_ptr_opt_z_(ptr)
#define _Out_writes_to_ptr_z_(ptr)
#define _Out_writes_z_(size)
#define _Out_z_bytecap_(size)
#define _Out_z_bytecap_c_(size)
#define _Out_z_bytecap_post_bytecount_(cap,count)
#define _Out_z_bytecap_x_(size)
#define _Out_z_bytecapcount_(capcount)
#define _Out_z_cap_(size)
#define _Out_z_cap_c_(size)
#define _Out_z_cap_m_(mult,size)
#define _Out_z_cap_post_count_(cap,count)
#define _Out_z_cap_x_(size)
#define _Out_z_capcount_(capcount)
#define _Outptr_
#define _Outptr_opt_
#define _Outptr_opt_result_buffer_(size)
#define _Outptr_opt_result_buffer_all_(size)
#define _Outptr_opt_result_buffer_all_maybenull_(size)
#define _Outptr_opt_result_buffer_maybenull_(size)
#define _Outptr_opt_result_buffer_to_(size, count)
#define _Outptr_opt_result_buffer_to_maybenull_(size, count)
#define _Outptr_opt_result_bytebuffer_(size)
#define _Outptr_opt_result_bytebuffer_all_(size)
#define _Outptr_opt_result_bytebuffer_all_maybenull_(size)
#define _Outptr_opt_result_bytebuffer_maybenull_(size)
#define _Outptr_opt_result_bytebuffer_to_(size, count)
#define _Outptr_opt_result_bytebuffer_to_maybenull_(size, count)
#define _Outptr_opt_result_maybenull_
#define _Outptr_opt_result_maybenull_z_
#define _Outptr_opt_result_nullonfailure_
#define _Outptr_opt_result_z_
#define _Outptr_result_buffer_(size)
#define _Outptr_result_buffer_all_(size)
#define _Outptr_result_buffer_all_maybenull_(size)
#define _Outptr_result_buffer_maybenull_(size)
#define _Outptr_result_buffer_to_(size, count)
#define _Outptr_result_buffer_to_maybenull_(size, count)
#define _Outptr_result_bytebuffer_(size)
#define _Outptr_result_bytebuffer_all_(size)
#define _Outptr_result_bytebuffer_all_maybenull_(size)
#define _Outptr_result_bytebuffer_maybenull_(size)
#define _Outptr_result_bytebuffer_to_(size, count)
#define _Outptr_result_bytebuffer_to_maybenull_(size, count)
#define _Outptr_result_maybenull_
#define _Outptr_result_maybenull_z_
#define _Outptr_result_nullonfailure_
#define _Outptr_result_z_
#define _Outref_
#define _Outref_result_buffer_(size)
#define _Outref_result_buffer_all_(size)
#define _Outref_result_buffer_all_maybenull_(size)
#define _Outref_result_buffer_maybenull_(size)
#define _Outref_result_buffer_to_(size, count)
#define _Outref_result_buffer_to_maybenull_(size, count)
#define _Outref_result_bytebuffer_(size)
#define _Outref_result_bytebuffer_all_(size)
#define _Outref_result_bytebuffer_all_maybenull_(size)
#define _Outref_result_bytebuffer_maybenull_(size)
#define _Outref_result_bytebuffer_to_(size, count)
#define _Outref_result_bytebuffer_to_maybenull_(size, count)
#define _Outref_result_maybenull_
#define _Outref_result_nullonfailure_
#define _Points_to_data_
#define _Post_
#define _Post_bytecap_(size)
#define _Post_bytecount_(size)
#define _Post_bytecount_c_(size)
#define _Post_bytecount_x_(size)
#define _Post_cap_(size)
#define _Post_count_(size)
#define _Post_count_c_(size)
#define _Post_count_x_(size)
#define _Post_defensive_
#define _Post_equal_to_(expr)
#define _Post_invalid_
#define _Post_maybenull_
#define _Post_maybez_
#define _Post_notnull_
#define _Post_null_
#define _Post_ptr_invalid_
#define _Post_readable_byte_size_(size)
#define _Post_readable_size_(size)
#define _Post_satisfies_(cond)
#define _Post_valid_
#define _Post_writable_byte_size_(size)
#define _Post_writable_size_(size)
#define _Post_z_
#define _Post_z_bytecount_(size)
#define _Post_z_bytecount_c_(size)
#define _Post_z_bytecount_x_(size)
#define _Post_z_count_(size)
#define _Post_z_count_c_(size)
#define _Post_z_count_x_(size)
#define _Pre_
#define _Pre_bytecap_(size)
#define _Pre_bytecap_c_(size)
#define _Pre_bytecap_x_(size)
#define _Pre_bytecount_(size)
#define _Pre_bytecount_c_(size)
#define _Pre_bytecount_x_(size)
#define _Pre_cap_(size)
#define _Pre_cap_c_(size)
#define _Pre_cap_c_one_
#define _Pre_cap_for_(param)
#define _Pre_cap_m_(mult,size)
#define _Pre_cap_x_(size)
#define _Pre_count_(size)
#define _Pre_count_c_(size)
#define _Pre_count_x_(size)
#define _Pre_defensive_
#define _Pre_equal_to_(expr)
#define _Pre_invalid_
#define _Pre_maybenull_
#define _Pre_notnull_
#define _Pre_null_
#define _Pre_opt_bytecap_(size)
#define _Pre_opt_bytecap_c_(size)
#define _Pre_opt_bytecap_x_(size)
#define _Pre_opt_bytecount_(size)
#define _Pre_opt_bytecount_c_(size)
#define _Pre_opt_bytecount_x_(size)
#define _Pre_opt_cap_(size)
#define _Pre_opt_cap_c_(size)
#define _Pre_opt_cap_c_one_
#define _Pre_opt_cap_for_(param)
#define _Pre_opt_cap_m_(mult,size)
#define _Pre_opt_cap_x_(size)
#define _Pre_opt_count_(size)
#define _Pre_opt_count_c_(size)
#define _Pre_opt_count_x_(size)
#define _Pre_opt_ptrdiff_cap_(ptr)
#define _Pre_opt_ptrdiff_count_(ptr)
#define _Pre_opt_valid_
#define _Pre_opt_valid_bytecap_(size)
#define _Pre_opt_valid_bytecap_c_(size)
#define _Pre_opt_valid_bytecap_x_(size)
#define _Pre_opt_valid_cap_(size)
#define _Pre_opt_valid_cap_c_(size)
#define _Pre_opt_valid_cap_x_(size)
#define _Pre_opt_z_
#define _Pre_opt_z_bytecap_(size)
#define _Pre_opt_z_bytecap_c_(size)
#define _Pre_opt_z_bytecap_x_(size)
#define _Pre_opt_z_cap_(size)
#define _Pre_opt_z_cap_c_(size)
#define _Pre_opt_z_cap_x_(size)
#define _Pre_ptrdiff_cap_(ptr)
#define _Pre_ptrdiff_count_(ptr)
#define _Pre_readable_byte_size_(size)
#define _Pre_readable_size_(size)
#define _Pre_readonly_
#define _Pre_satisfies_(cond)
#define _Pre_unknown_
#define _Pre_valid_
#define _Pre_valid_bytecap_(size)
#define _Pre_valid_bytecap_c_(size)
#define _Pre_valid_bytecap_x_(size)
#define _Pre_valid_cap_(size)
#define _Pre_valid_cap_c_(size)
#define _Pre_valid_cap_x_(size)
#define _Pre_writable_byte_size_(size)
#define _Pre_writable_size_(size)
#define _Pre_writeonly_
#define _Pre_z_
#define _Pre_z_bytecap_(size)
#define _Pre_z_bytecap_c_(size)
#define _Pre_z_bytecap_x_(size)
#define _Pre_z_cap_(size)
#define _Pre_z_cap_c_(size)
#define _Pre_z_cap_x_(size)
#define _Prepost_bytecount_(size)
#define _Prepost_bytecount_c_(size)
#define _Prepost_bytecount_x_(size)
#define _Prepost_count_(size)
#define _Prepost_count_c_(size)
#define _Prepost_count_x_(size)
#define _Prepost_opt_bytecount_(size)
#define _Prepost_opt_bytecount_c_(size)
#define _Prepost_opt_bytecount_x_(size)
#define _Prepost_opt_count_(size)
#define _Prepost_opt_count_c_(size)
#define _Prepost_opt_count_x_(size)
#define _Prepost_opt_valid_
#define _Prepost_opt_z_
#define _Prepost_valid_
#define _Prepost_z_
#define _Printf_format_string_
#define _Raises_SEH_exception_
#define _Maybe_raises_SEH_exception_
#define _Readable_bytes_(size)
#define _Readable_elements_(size)
#define _Reserved_
#define _Result_nullonfailure_
#define _Result_zeroonfailure_
#define __inner_callback
#define _Ret_
#define _Ret_bound_
#define _Ret_bytecap_(size)
#define _Ret_bytecap_c_(size)
#define _Ret_bytecap_x_(size)
#define _Ret_bytecount_(size)
#define _Ret_bytecount_c_(size)
#define _Ret_bytecount_x_(size)
#define _Ret_cap_(size)
#define _Ret_cap_c_(size)
#define _Ret_cap_x_(size)
#define _Ret_count_(size)
#define _Ret_count_c_(size)
#define _Ret_count_x_(size)
#define _Ret_maybenull_
#define _Ret_maybenull_z_
#define _Ret_notnull_
#define _Ret_null_
#define _Ret_opt_
#define _Ret_opt_bytecap_(size)
#define _Ret_opt_bytecap_c_(size)
#define _Ret_opt_bytecap_x_(size)
#define _Ret_opt_bytecount_(size)
#define _Ret_opt_bytecount_c_(size)
#define _Ret_opt_bytecount_x_(size)
#define _Ret_opt_cap_(size)
#define _Ret_opt_cap_c_(size)
#define _Ret_opt_cap_x_(size)
#define _Ret_opt_count_(size)
#define _Ret_opt_count_c_(size)
#define _Ret_opt_count_x_(size)
#define _Ret_opt_valid_
#define _Ret_opt_z_
#define _Ret_opt_z_bytecap_(size)
#define _Ret_opt_z_bytecount_(size)
#define _Ret_opt_z_cap_(size)
#define _Ret_opt_z_count_(size)
#define _Ret_range_(lb,ub)
#define _Ret_valid_
#define _Ret_writes_(size)
#define _Ret_writes_bytes_(size)
#define _Ret_writes_bytes_maybenull_(size)
#define _Ret_writes_bytes_to_(size,count)
#define _Ret_writes_bytes_to_maybenull_(size,count)
#define _Ret_writes_maybenull_(size)
#define _Ret_writes_maybenull_z_(size)
#define _Ret_writes_to_(size,count)
#define _Ret_writes_to_maybenull_(size,count)
#define _Ret_writes_z_(size)
#define _Ret_z_
#define _Ret_z_bytecap_(size)
#define _Ret_z_bytecount_(size)
#define _Ret_z_cap_(size)
#define _Ret_z_count_(size)
#define _Return_type_success_(expr)
#define _Scanf_format_string_
#define _Scanf_s_format_string_
#define _Struct_size_bytes_(size)
#define _Success_(expr)
#define _Unchanged_(e)
#define _Use_decl_annotations_
#define _Valid_
#define _When_(expr, annos)
#define _Writable_bytes_(size)
#define _Writable_elements_(size)

#endif // ] _USE_ATTRIBUTES_FOR_SAL

#define __callback __inner_callback
