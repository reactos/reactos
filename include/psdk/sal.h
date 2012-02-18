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

/* HACK: gcc's C++ headers conflict with oldstyle macros */
#if !defined(__cplusplus) || !defined(__GNUC__)
#include <sal_old.h>
#endif

#ifdef _PREFAST_

#pragma warning(disable:6320) /* disable warning about SEH filter */
#pragma warning(disable:28247) /* duplicated model file annotations */
#pragma warning(disable:28251) /* Inconsistent annotation */


#ifndef _USE_DECLSPECS_FOR_SAL
#define _USE_DECLSPECS_FOR_SAL 1
#endif

#if !defined(_USE_ATTRIBUTES_FOR_SAL) || _USE_DECLSPECS_FOR_SAL
#define _USE_ATTRIBUTES_FOR_SAL 0
#endif

#if !_USE_DECLSPECS_FOR_SAL && !_USE_ATTRIBUTES_FOR_SAL
#if _MSC_VER >= 1400
#undef _USE_ATTRIBUTES_FOR_SAL
#define _USE_ATTRIBUTES_FOR_SAL 1
#else
#undef _USE_DECLSPECS_FOR_SAL
#define _USE_DECLSPECS_FOR_SAL  1
#endif /* _MSC_VER >= 1400 */
#endif /* !_USE_DECLSPECS_FOR_SAL && !_USE_ATTRIBUTES_FOR_SAL */

#else /* _PREFAST_ */

#undef _USE_DECLSPECS_FOR_SAL
#define _USE_DECLSPECS_FOR_SAL 0
#undef _USE_ATTRIBUTES_FOR_SAL
#define _USE_ATTRIBUTES_FOR_SAL 0

#endif /* _PREFAST_ */

#if defined(MIDL_PASS) || defined(__midl) || defined(RC_INVOKED)
#undef _USE_DECLSPECS_FOR_SAL
#define _USE_DECLSPECS_FOR_SAL 0
#undef _USE_ATTRIBUTES_FOR_SAL
#define _USE_ATTRIBUTES_FOR_SAL 0
#endif

#if !defined(_MSC_EXTENSIONS) || defined(_M_CEE_SAFE)
#undef _USE_ATTRIBUTES_FOR_SAL
#define _USE_ATTRIBUTES_FOR_SAL 0
#endif


#if _USE_ATTRIBUTES_FOR_SAL || _USE_DECLSPECS_FOR_SAL

#define _SA_SPECSTRIZE(x)           #x

#if _USE_ATTRIBUTES_FOR_SAL

#if (_MSC_VER < 1400)
#error attribute sal only works with _MSC_VER >= 1400
#endif

#if !defined(_W64)
# if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#  define _W64 __w64
# else
#  define _W64
# endif
#endif

#ifndef _SIZE_T_DEFINED
# ifdef  _WIN64
typedef unsigned __int64 size_t;
# else
typedef _W64 unsigned int size_t;
# endif
# define _SIZE_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
# define _WCHAR_T_DEFINED
#endif

#ifdef __cplusplus
# define SA(id) id
# define REPEATABLE [repeatable]
# define CONSTRUCTOR(name) name();
namespace vc_attributes {
#else
# define SA(id) SA_##id
# define REPEATABLE
# define CONSTRUCTOR(name)
#endif
#define ATTR_SA_ALL [source_annotation_attribute(SA(All))]

typedef enum SA(YesNoMaybe)
{
    SA(No) = 0x0fff0001,
    SA(Maybe) = 0x0fff0010,
    SA(Yes) = 0x0fff0100
} SA(YesNoMaybe);

typedef enum SA(AccessType)
{
    SA(NoAccess) = 0,
    SA(Read) = 1,
    SA(Write) = 2,
    SA(ReadWrite) = 3
} SA(AccessType);

#define DECLARE_ATTR(Name, ...) \
    ATTR_SA_ALL \
    typedef struct Name \
    { \
        CONSTRUCTOR(Name) \
        __VA_ARGS__ \
    } Name,

REPEATABLE DECLARE_ATTR(PreAttribute, unsigned int Deref;SA(YesNoMaybe) Valid;SA(YesNoMaybe) Null;SA(YesNoMaybe) Tainted;SA(AccessType) Access;unsigned int Notref;
    size_t ValidElementsConst;size_t ValidBytesConst;const wchar_t* ValidElements;const wchar_t* ValidBytes;const wchar_t* ValidElementsLength;const wchar_t* ValidBytesLength;
    size_t WritableElementsConst;size_t WritableBytesConst;const wchar_t* WritableElements;const wchar_t* WritableBytes;const wchar_t* WritableElementsLength;
    const wchar_t* WritableBytesLength;size_t ElementSizeConst;const wchar_t* ElementSize;SA(YesNoMaybe) NullTerminated;const wchar_t* Condition;) SA_Pre;
REPEATABLE DECLARE_ATTR(PostAttribute, unsigned int Deref;SA(YesNoMaybe) Valid;SA(YesNoMaybe) Null;SA(YesNoMaybe) Tainted;SA(AccessType) Access;
    unsigned int Notref;size_t ValidElementsConst;size_t ValidBytesConst;const wchar_t* ValidElements;const wchar_t* ValidBytes;
    const wchar_t* ValidElementsLength;const wchar_t* ValidBytesLength;size_t WritableElementsConst;size_t WritableBytesConst;
    const wchar_t* WritableElements;const wchar_t* WritableBytes;const wchar_t* WritableElementsLength;const wchar_t* WritableBytesLength;
    size_t ElementSizeConst;const wchar_t* ElementSize;SA(YesNoMaybe) NullTerminated;SA(YesNoMaybe) MustCheck;const wchar_t* Condition;) SA_Post;
DECLARE_ATTR(FormatStringAttribute, const wchar_t* Style; const wchar_t* UnformattedAlternative;) SA_FormatString;
REPEATABLE DECLARE_ATTR(PreBoundAttribute, unsigned int Deref;) SA_PreBound;
REPEATABLE DECLARE_ATTR(PostBoundAttribute, unsigned int Deref;) SA_PostBound;
REPEATABLE DECLARE_ATTR(PreRangeAttribute, unsigned int Deref;const char* MinVal;const char* MaxVal;) SA_PreRange;
REPEATABLE DECLARE_ATTR(PostRangeAttribute, unsigned int Deref;const char* MinVal;const char* MaxVal;) SA_PostRange;
REPEATABLE DECLARE_ATTR(DerefAttribute, int unused;) SAL_deref;
REPEATABLE DECLARE_ATTR(NotrefAttribute, int unused;) SAL_notref;
REPEATABLE DECLARE_ATTR(AnnotesAttribute, wchar_t *Name;wchar_t *p1;wchar_t *p2;wchar_t *p3;wchar_t *p4;wchar_t *p5;wchar_t *p6;wchar_t *p7;wchar_t *p8;wchar_t *p9;) SAL_annotes;
REPEATABLE DECLARE_ATTR(AtAttribute, wchar_t *p1;) SAL_at;
REPEATABLE DECLARE_ATTR(AtBufferAttribute, wchar_t *p1;wchar_t *p2;wchar_t *p3;) SAL_at_buffer;
REPEATABLE DECLARE_ATTR(WhenAttribute, wchar_t *p1;) SAL_when;
REPEATABLE DECLARE_ATTR(TypefixAttribute, wchar_t *p1;) SAL_typefix;
REPEATABLE DECLARE_ATTR(ContextAttribute, wchar_t *p1;) SAL_context;
REPEATABLE DECLARE_ATTR(ExceptAttribute, int unused;) SAL_except;
REPEATABLE DECLARE_ATTR(PreOpAttribute, int unused;) SAL_pre;
REPEATABLE DECLARE_ATTR(PostOpAttribute, int unused;) SAL_post;
REPEATABLE DECLARE_ATTR(BeginAttribute, int unused;) SAL_begin;
REPEATABLE DECLARE_ATTR(EndAttribute, int unused;) SAL_end;

#ifdef __cplusplus
};
#endif

#undef REPEATABLE
#undef SA
#undef DECLARE_ATTR


#if (_MSC_VER >= 1610)
#define NL_(size) "\n"#size
#else
#define NL_(size) #size
#endif

#define _SA_annotes0(n)             [SAL_annotes(Name=#n)]
#define _SA_annotes1(n,pp1)         [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1))]
#define _SA_annotes2(n,pp1,pp2)     [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1), p2=_SA_SPECSTRIZE(pp2))]
#define _SA_annotes3(n,pp1,pp2,pp3) [SAL_annotes(Name=#n, p1=_SA_SPECSTRIZE(pp1), p2=_SA_SPECSTRIZE(pp2), p3=_SA_SPECSTRIZE(pp3))]

#define _SA_Deref_pre1_(p1)         [SA_Pre(Deref=1,p1)]
#define _SA_Deref_pre2_(p1,p2)      [SA_Pre(Deref=1,p1,p2)]
#define _SA_Deref_pre3_(p1,p2,p3)   [SA_Pre(Deref=1,p1,p2,p3)]
#define _SA_Deref_post1_(p1)        [SA_Post(Deref=1,p1)]
#define _SA_Deref_post2_(p1,p2)     [SA_Post(Deref=1,p1,p2)]
#define _SA_Deref_post3_(p1,p2,p3)  [SA_Post(Deref=1,p1,p2,p3)]
#define _SA_Deref_in_bound_         [SA_PreBound(Deref=1)]
#define _SA_Pre1_(p1)               [SA_Pre(p1)]
#define _SA_Pre2_(p1,p2)            [SA_Pre(p1,p2)]
#define _SA_Pre3_(p1,p2,p3)         [SA_Pre(p1,p2,p3)]
#define _SA_Post1_(p1)              [SA_Post(p1)]
#define _SA_Post2_(p1,p2)           [SA_Post(p1,p2)]
#define _SA_Post3_(p1,p2,p3)        [SA_Post(p1,p2,p3)]
#define _SA_Deref_ret1_(p1)         [returnvalue:SA_Post(Deref=1,p1)]
#define _SA_Deref2_pre1_(p1)        [SA_Pre(Deref=2,Notref=1,p1)]
#define _SA_Ret1_(p1)               [returnvalue:SA_Post(p1)]
#define _SA_Ret2_(p1,p2)            [returnvalue:SA_Post(p1,p2)]
#define _SA_Ret3_(p1,p2,p3)         [returnvalue:SA_Post(p1,p2,p3)]

#define __null_impl                 Null=SA_Yes
#define __null_impl_notref          Null=SA_Yes,Notref=1
#define __notnull_impl              Null=SA_No
#define __notnull_impl_notref       Null=SA_No,Notref=1
#define __maybenull_impl            Null=SA_Maybe
#define __maybenull_impl_notref     Null=SA_Maybe,Notref=1
#define __valid_impl                Valid=SA_Yes
#define __notvalid_impl             Valid=SA_No
#define __maybevalid_impl           Valid=SA_Maybe
#define __readaccess_impl_notref    Access=SA_Read,Notref=1
#define __writeaccess_impl_notref   Access=SA_Write,Notref=1
#define __bytecap_impl(size)        WritableBytes=_NL_(size)
#define __bytecap_c_impl(size)      WritableBytesConst=size
#define __bytecap_x_impl(size)      WritableBytes="\n@"#size
#define __cap_impl(size)            WritableElements=_NL_(size)
#define __cap_c_impl(size)          WritableElementsConst=size
#define __cap_c_one_notref_impl     WritableElementsConst=1,Notref=1
#define __cap_for_impl(param)       WritableElementsLength=#param
#define __cap_x_impl(size)          WritableElements="\n@"#size
#define __bytecount_impl(size)      ValidBytes=_NL_(size)
#define __bytecount_c_impl(size)    ValidBytesConst=size
#define __bytecount_x_impl(size)    ValidBytes="\n@"#size
#define __count_impl(size)          ValidElements=_NL_(size)
#define __count_c_impl(size)        ValidElementsConst=size
#define __count_x_impl(size)        ValidElements="\n@"#size
#define __zterm_impl                NullTerminated=SA_Yes
#define __notzterm_impl             NullTerminated=SA_No
#define __maybezterm_impl           NullTerminated=SA_Maybe
#define __mult_impl(mult,size)      __cap_impl((mult)*(size))

#else /* #if _USE_DECLSPECS_FOR_SAL */

#define _SA_annotes0(n)             __declspec(#n)
#define _SA_annotes1(n,pp1)         __declspec(#n "(" _SA_SPECSTRIZE(pp1) ")" )
#define _SA_annotes2(n,pp1,pp2)     __declspec(#n "(" _SA_SPECSTRIZE(pp1) "," _SA_SPECSTRIZE(pp2) ")")
#define _SA_annotes3(n,pp1,pp2,pp3) __declspec(#n "(" _SA_SPECSTRIZE(pp1) "," _SA_SPECSTRIZE(pp2) "," _SA_SPECSTRIZE(pp3) ")")

#define _SA_Deref_pre_              _Pre_ _Notref_ _SA_Deref_
#define _SA_Deref_pre1_(p1)         _SA_Deref_pre_ p1
#define _SA_Deref_pre2_(p1,p2)      _SA_Deref_pre_ p1 _SA_Deref_pre_ p2
#define _SA_Deref_pre3_(p1,p2,p3)   _SA_Deref_pre_ p1 _SA_Deref_pre_ p2 _SA_Deref_pre_ p3
#define _SA_Deref_post_             _Post_ _Notref_ _SA_Deref_
#define _SA_Deref_post1_(p1)        _SA_Deref_post_ p1
#define _SA_Deref_post2_(p1,p2)     _SA_Deref_post_ p1 _SA_Deref_post_ p2
#define _SA_Deref_post3_(p1,p2,p3)  _SA_Deref_post_ p1 _SA_Deref_post_ p2 _SA_Deref_post_ p3
#define _SA_Deref_in_bound_         _Pre_ _Notref_ _SA_Deref_ _SA_Bound_
#define _SA_Pre1_(p1)               _Pre_ p1
#define _SA_Pre2_(p1,p2)            _Pre_ p1 _Pre_ p2
#define _SA_Pre3_(p1,p2,p3)         _Pre_ p1 _Pre_ p2 _Pre_ p3
#define _SA_Post1_(p1)              _Post_ p1
#define _SA_Post2_(p1,p2)           _Post_ p1 _Post_ p2
#define _SA_Post3_(p1,p2,p3)        _Post_ p1 _Post_ p2 _Post_ p3
#define _SA_Deref_ret1_(p1)         _SA_Deref_post_ p1
#define _SA_Deref2_pre1_(p1)        _SA_Deref_pre_ _Notref_ _SA_Deref_ p1
#define _SA_Ret1_(p1)               _Post_ p1
#define _SA_Ret2_(p1,p2)            _Post_ p1 _Post_ p2
#define _SA_Ret3_(p1,p2,p3)         _Post_ p1 _Post_ p2 _Post_ p3

#define __null_impl                 _Null_
#define __null_impl_notref          _Notref_ _Null_
#define __notnull_impl              _Notnull_
#define __notnull_impl_notref       _Notref_ _Notnull_
#define __maybenull_impl            __inner_exceptthat _Maybenull_
#define __maybenull_impl_notref     _Notref_ __inner_exceptthat _Maybenull_
#define __valid_impl                __declspec("SAL_valid") // __declspec("SAL_valid(__yes)")
#define __maybevalid_impl           __declspec("SAL_maybevalid") // __declspec("SAL_valid(__maybe)")
#define __notvalid_impl             __declspec("SAL_notvalid") // __declspec("SAL_valid(__no)")
#define __readaccess_impl_notref    _Notref_ __declspec("SAL_readonly") // __declspec("SAL_access(0x1)")
#define __writeaccess_impl_notref   _Notref_ __declspec("SAL_notreadonly") // __declspec("SAL_access(0x2)")
#define __bytecap_impl(size)        _SA_annotes1(SAL_writableTo,byteCount(size))
#define __bytecap_c_impl(size)      _SA_annotes1(SAL_writableTo,byteCount(size))
#define __bytecap_x_impl(size)      _SA_annotes1(SAL_writableTo,inexpressibleCount(#size))
#define __cap_impl(size)            _SA_annotes1(SAL_writableTo,elementCount(size))
#define __cap_c_impl(size)          _SA_annotes1(SAL_writableTo,elementCount(size))
#define __cap_c_one_notref_impl     _Notref_ __cap_c_impl(1)
#define __cap_for_impl(param)       _SA_annotes1(SAL_writableTo,inexpressibleCount(sizeof(param)))
#define __cap_x_impl(size)          _SA_annotes1(SAL_writableTo,inexpressibleCount(#size))
#define __bytecount_impl(size)      _SA_annotes1(SAL_readableTo,byteCount(size))
#define __bytecount_c_impl(size)    _SA_annotes1(SAL_readableTo,byteCount(size))
#define __bytecount_x_impl(size)    _SA_annotes1(SAL_readableTo,inexpressibleCount(#size))
#define __count_impl(size)          _SA_annotes1(SAL_readableTo,elementCount(size))
#define __count_c_impl(size)        _SA_annotes1(SAL_readableTo,elementCount(size))
#define __count_x_impl(size)        _SA_annotes1(SAL_readableTo,inexpressibleCount(#size))
#define __zterm_impl                _SA_annotes1(SAL_nullTerminated, __yes)
#define __notzterm_impl             _SA_annotes1(SAL_nullTerminated, __no)
#define __maybezterm_impl           _SA_annotes1(SAL_nullTerminated, __maybe)
#define __mult_impl(mult,size)      _SA_annotes1(SAL_writableTo,(mult)*(size))

#endif

/* Common internal */
#define _SA_Bound_                  _SA_annotes0(SAL_bound)
#define _SA_Deref_                  _SA_annotes0(SAL_deref)
#define _SA_Satisfies_(cond)        _SA_annotes1(SAL_satisfies, cond)
#if (_MSC_VER >= 1600)
#define _SA_Must_inspect_           _Post_ _SA_annotes0(SAL_mustInspect)
#else
#define _SA_Must_inspect_
#endif
#define _Old_(x)                    x
#define __AuToQuOtE                 _SA_annotes0(SAL_AuToQuOtE)

/******************************************************************************
 *                              Public macros                                 *
 ******************************************************************************/

#if _USE_ATTRIBUTES_FOR_SAL

#define _Check_return_                  [returnvalue:SA_Post(MustCheck=SA_Yes)]
#define _Deref_in_bound_                [SA_PreBound(Deref=1)]
#define _Deref_out_bound_               [SA_PostBound(Deref=1)]
#define _Deref_in_range_(min,max)       [SA_PreRange(Deref=1,MinVal=#min,MaxVal=#max)]
#define _Deref_out_range_(min,max)      [SA_PostRange(Deref=1,MinVal=#min,MaxVal=#max)]
#define _Deref_ret_bound_               [returnvalue:SA_PostBound(Deref=1)]
#define _Deref_ret_range_(min,max)      [returnvalue:SA_PostRange(Deref=1,MinVal=#min,MaxVal=#max)]
#define _In_bound_                      [SA_PreBound(Deref=0)]
#define _In_range_(min,max)             [SA_PreRange(MinVal=#min,MaxVal=#max)]
#define _Out_bound_                     [SA_PostBound(Deref=0)]
#define _Out_range_(min,max)            [SA_PostRange(MinVal=#min,MaxVal=#max)]
#define _Printf_format_string_          [SA_FormatString(Style="printf")]
#define _Scanf_format_string_           [SA_FormatString(Style="scanf")]
#define _Scanf_s_format_string_         [SA_FormatString(Style="scanf_s")]
#define _Ret_bound_                     [returnvalue:SA_PostBound(Deref=0)]
#define _Ret_range_(min,max)            [returnvalue:SA_PostRange(MinVal=#min,MaxVal=#max)]
#define _Use_decl_annotations_          _SA_annotes0(SAL_useHeader)
#define __inner_exceptthat              [SAL_except]
#define __inner_typefix(ctype)          [SAL_typefix(p1=_SA_SPECSTRIZE(ctype))]

#elif _USE_DECLSPECS_FOR_SAL

#define _Check_return_                  _Post_ __declspec("SAL_checkReturn")
#define _Deref_in_bound_                _SA_Deref_pre_ _SA_Bound_
#define _Deref_out_bound_               _SA_Deref_post_ _SA_Bound_
#define _Deref_in_range_(min,max)       _SA_Deref_pre_ _Field_range_(min,max)
#define _Deref_out_range_(min,max)      _SA_Deref_post_ _Field_range_(min,max)
#define _Deref_ret_bound_               _SA_Deref_post_ _SA_Bound_
#define _Deref_ret_range_(min,max)      _SA_Deref_post_ _Field_range_(min,max)
#define _In_bound_                      _Pre_ _SA_Bound_
#define _In_range_(min,max)             _Pre_ _Field_range_(min,max)
#define _Out_bound_                     _Post_ _SA_Bound_
#define _Out_range_(min,max)            _Post_ _Field_range_(min,max)
#define _Printf_format_string_          _SA_annotes1(SAL_IsFormatString, "printf")
#define _Scanf_format_string_           _SA_annotes1(SAL_IsFormatString, "scanf")
#define _Scanf_s_format_string_         _SA_annotes1(SAL_IsFormatString, "scanf_s")
#define _Ret_bound_                     _Post_ _SA_Bound_
#define _Ret_range_(min,max)            _Post_ _Field_range_(min,max)
#define _Use_decl_annotations_ _        _declspec("SAL_useHeader()")
#define __inner_exceptthat              _SA_annotes0(SAL_except)
#define __inner_typefix(ctype)          _SA_annotes1(SAL_typefix, ctype)

#endif

#define _Post_                          _SA_annotes0(SAL_post)
#define _Pre_                           _SA_annotes0(SAL_pre)
#ifdef __cplusplus
#define _Notref_                        _SA_annotes0(SAL_notref)
#else
#define _Notref_
#endif
#define _Null_                          _SA_annotes1(SAL_null, __yes)
#define _Notnull_                       _SA_annotes1(SAL_null, __no)
#define _Maybenull_                     __inner_exceptthat _SA_annotes1(SAL_null, __maybe)
#define _Valid_                         _SA_annotes1(SAL_valid, __yes)
#define _Notvalid_                      _SA_annotes1(SAL_valid, __no)
#define _Maybevalid_                    _SA_annotes1(SAL_valid, __maybe)
#define _Const_                         _SA_Pre1_(__readaccess_impl_notref)
#define _Literal_                       _Pre_ _SA_annotes1(SAL_constant, __yes)
#define _Notliteral_                    _Pre_ _SA_annotes1(SAL_constant, __no)
#define _Null_terminated_               _SA_annotes1(SAL_nullTerminated, __yes)
#define _NullNull_terminated_           _Group_(_SA_annotes1(SAL_nullTerminated, __yes) _SA_annotes1(SAL_readableTo,inexpressibleCount("NullNull terminated string")))
#define _Field_range_(min,max)          _SA_annotes2(SAL_range, min, max)
#define _Readable_bytes_(size)          _SA_annotes1(SAL_readableTo, byteCount(size))
#define _Readable_elements_(size)       _SA_annotes1(SAL_readableTo, elementCount(size))
#define _Writable_bytes_(size)          _SA_annotes1(SAL_writableTo, byteCount(size))
#define _Writable_elements_(size)       _SA_annotes1(SAL_writableTo, elementCount(size))
#define _Success_(expr)                 _SA_annotes1(SAL_success, expr)
#define _When_(expr, annos)             _SA_annotes0(SAL_when(expr)) _Group_(annos)
#define _Group_(annos)                  _SA_annotes0(SAL_begin) annos _SA_annotes0(SAL_end)
#define _On_failure_(annos)             _SA_annotes1(SAL_context, SAL_failed) _Group_(_Post_ _Group_(annos))
#define _At_(target, annos)             _SA_annotes0(SAL_at(target)) _Group_(annos)
#define _At_buffer_(t, i, b, a)         _SA_annotes3(SAL_at_buffer, t, i, b) _Group_(a)
#define _Must_inspect_result_           _SA_Must_inspect_ _Check_return_
#define _Always_(annos)                 _Group_(annos) _On_failure_(annos)

#if (_MSC_VER >= 1600)
#define _Points_to_data_                _Pre_ _At_(*_Curr_, _SA_annotes1(SAL_mayBePointer, __no))
#else
// FIXME
#define _Points_to_data_
#endif

#define _Return_type_success_(expr)     _Success_(expr)
#define _Struct_size_bytes_(size)       _Writable_bytes_(size)
#define _Unchanged_(e)                  _At_(e, _Post_equal_to_(_Old_(e)) _Const_)
#define _Analysis_noreturn_             _SA_annotes0(SAL_terminates)
#define _Function_class_(x)             _SA_annotes1(SAL_functionClassNew, #x)
#define _Raises_SEH_exception_          _Group_(_Pre_ _SA_annotes1(SAL_inTry,__yes) _Analysis_noreturn_)
#define _Maybe_raises_SEH_exception_    _Pre_ _SA_annotes1(SAL_inTry,__yes)
#define _Interlocked_operand_           _Pre_ _SA_annotes0(SAL_interlocked)

#define _Field_size_(size)              _Notnull_ _Writable_elements_(size)
#define _Field_size_bytes_(size)        _Notnull_ _Writable_bytes_(size)
#define _Field_size_bytes_full_(sz)     _Field_size_bytes_part_(sz, sz)
#define _Field_size_bytes_full_opt_(sz) _Field_size_bytes_part_opt_(sz, sz)
#define _Field_size_bytes_opt_(sz)      _Writable_bytes_(sz) _Maybenull_
#define _Field_size_bytes_part_(sz, c)  _Notnull_ _Writable_bytes_(sz) _Readable_bytes_(c)
#define _Field_size_bytes_part_opt_(size, count) _Writable_bytes_(size) _Readable_bytes_(count) _Maybenull_
#define _Field_size_full_(size)         _Field_size_part_(size, size)
#define _Field_size_full_opt_(size)     _Field_size_part_opt_(size, size)
#define _Field_size_opt_(size)          _Writable_elements_(size) _Maybenull_
#define _Field_size_part_(size, count)  _Notnull_ _Writable_elements_(size) _Readable_elements_(count)
#define _Field_size_part_opt_(sz, c)    _Writable_elements_(sz) _Readable_elements_(c) _Maybenull_
#define _Field_z_                       _Null_terminated_

#define _Pre_bytecap_(size)             _Pre_notnull_ _SA_Pre1_(__bytecap_impl(size))
#define _Pre_bytecap_c_(size)           _Pre_notnull_ _SA_Pre1_(__bytecap_c_impl(size))
#define _Pre_bytecap_x_(size)           _Pre_notnull_ _SA_Pre1_(__bytecap_x_impl(size))
#define _Pre_bytecount_(size)           _Pre_notnull_ _SA_Pre1_(__bytecount_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_bytecount_c_(size)         _Pre_notnull_ _SA_Pre1_(__bytecount_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_bytecount_x_(size)         _Pre_notnull_ _SA_Pre1_(__bytecount_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_cap_(size)                 _Pre_notnull_ _SA_Pre1_(__cap_impl(size))
#define _Pre_cap_c_(size)               _Pre_notnull_ _SA_Pre1_(__cap_c_impl(size))
#define _Pre_cap_c_one_                 _Pre_notnull_ _SA_Pre1_(__cap_c_one_notref_impl)
#define _Pre_cap_for_(param)            _Pre_notnull_ _SA_Pre1_(__cap_for_impl(param))
#define _Pre_cap_m_(mult,size)          _Pre_notnull_ _SA_Pre1_(__mult_impl(mult,size))
#define _Pre_cap_x_(size)               _Pre_notnull_ _SA_Pre1_(__cap_x_impl(size))
#define _Pre_count_(size)               _Pre_notnull_ _SA_Pre1_(__count_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_count_c_(size)             _Pre_notnull_ _SA_Pre1_(__count_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_count_x_(size)             _Pre_notnull_ _SA_Pre1_(__count_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_defensive_                 _SA_annotes0(SAL_pre_defensive)
#define _Pre_equal_to_(expr)            _In_range_(==, expr)
#define _Pre_invalid_                   _SA_Deref_pre1_(__notvalid_impl) // note: this is appied to the memory referenced by the parameter! Same as _Deref_pre_invalid_
#define _Pre_maybenull_                 _SA_Pre1_(__maybenull_impl_notref)
#define _Pre_notnull_                   _SA_Pre1_(__notnull_impl_notref)
#define _Pre_null_                      _SA_Pre1_(__null_impl_notref)
#define _Pre_opt_bytecap_(size)         _SA_Pre1_(__bytecap_impl(size)) _Pre_maybenull_
#define _Pre_opt_bytecap_c_(size)       _SA_Pre1_(__bytecap_c_impl(size)) _Pre_maybenull_
#define _Pre_opt_bytecap_x_(size)       _SA_Pre1_(__bytecap_x_impl(size)) _Pre_maybenull_
#define _Pre_opt_bytecount_(size)       _SA_Pre1_(__bytecount_impl(size)) _Pre_opt_valid_
#define _Pre_opt_bytecount_c_(size)     _SA_Pre1_(__bytecount_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_bytecount_x_(size)     _SA_Pre1_(__bytecount_x_impl(size)) _Pre_opt_valid_
#define _Pre_opt_cap_(size)             _SA_Pre1_(__cap_impl(size)) _Pre_maybenull_
#define _Pre_opt_cap_c_(size)           _SA_Pre1_(__cap_c_impl(size)) _Pre_maybenull_
#define _Pre_opt_cap_c_one_             _SA_Pre1_(__cap_c_one_notref_impl) _Pre_maybenull_
#define _Pre_opt_cap_for_(param)        _SA_Pre1_(__cap_for_impl(param)) _Pre_maybenull_
#define _Pre_opt_cap_m_(mult,size)      _SA_Pre1_(__mult_impl(mult,size)) _Pre_maybenull_
#define _Pre_opt_cap_x_(size)           _SA_Pre1_(__cap_x_impl(size)) _Pre_maybenull_
#define _Pre_opt_count_(size)           _SA_Pre1_(__count_impl(size)) _Pre_opt_valid_
#define _Pre_opt_count_c_(size)         _SA_Pre1_(__count_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_count_x_(size)         _SA_Pre1_(__count_x_impl(size)) _Pre_opt_valid_
#define _Pre_opt_ptrdiff_cap_(ptr)      _SA_Pre1_(__cap_x_impl(__ptrdiff(ptr)))
#define _Pre_opt_ptrdiff_count_(ptr)    _SA_Pre1_(__count_x_impl(__ptrdiff(ptr))) _SA_Pre1_(__valid_impl)
#define _Pre_opt_valid_                 _SA_Pre1_(__valid_impl) _Pre_maybenull_
#define _Pre_opt_valid_bytecap_(size)   _SA_Pre1_(__bytecap_impl(size)) _Pre_opt_valid_
#define _Pre_opt_valid_bytecap_c_(size) _SA_Pre1_(__bytecap_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_valid_bytecap_x_(size) _SA_Pre1_(__bytecap_x_impl(size)) _Pre_opt_valid_
#define _Pre_opt_valid_cap_(size)       _SA_Pre1_(__cap_impl(size)) _Pre_opt_valid_
#define _Pre_opt_valid_cap_c_(size)     _SA_Pre1_(__cap_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_valid_cap_x_(size)     _SA_Pre1_(__cap_x_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_                     _SA_Pre1_(__zterm_impl) _Pre_opt_valid_
#define _Pre_opt_z_bytecap_(size)       _SA_Pre2_(__zterm_impl,__bytecap_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_bytecap_c_(size)     _SA_Pre2_(__zterm_impl,__bytecap_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_bytecap_x_(size)     _SA_Pre2_(__zterm_impl,__bytecap_x_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_cap_(size)           _SA_Pre2_(__zterm_impl,__cap_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_cap_c_(size)         _SA_Pre2_(__zterm_impl,__cap_c_impl(size)) _Pre_opt_valid_
#define _Pre_opt_z_cap_x_(size)         _SA_Pre2_(__zterm_impl,__cap_x_impl(size)) _Pre_opt_valid_
#define _Pre_ptrdiff_cap_(ptr)          _Pre_notnull_ _SA_Pre1_(__cap_x_impl(__ptrdiff(ptr)))
#define _Pre_ptrdiff_count_(ptr)        _Pre_notnull_ _SA_Pre1_(__count_x_impl(__ptrdiff(ptr))) _SA_Pre1_(__valid_impl)
#define _Pre_readable_byte_size_(size)  _SA_Pre1_(__bytecount_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_readable_size_(size)       _SA_Pre1_(__count_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_readonly_                  _SA_Pre1_(__readaccess_impl_notref)
#define _Pre_satisfies_(cond)           _Pre_ _SA_Satisfies_(cond)
#define _Pre_valid_                     _Pre_notnull_ _SA_Pre1_(__valid_impl)
#define _Pre_valid_bytecap_(size)       _Pre_notnull_ _SA_Pre1_(__bytecap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_valid_bytecap_c_(size)     _Pre_notnull_ _SA_Pre1_(__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_valid_bytecap_x_(size)     _Pre_notnull_ _SA_Pre1_(__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_valid_cap_(size)           _Pre_notnull_ _SA_Pre1_(__cap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_valid_cap_c_(size)         _Pre_notnull_ _SA_Pre1_(__cap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_valid_cap_x_(size)         _Pre_notnull_ _SA_Pre1_(__cap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_writable_byte_size_(size)  _SA_Pre1_(__bytecap_impl(size))
#define _Pre_writable_size_(size)       _SA_Pre1_(__cap_impl(size))
#define _Pre_writeonly_                 _SA_Pre1_(__writeaccess_impl_notref)
#define _Pre_z_                         _Pre_notnull_ _SA_Pre1_(__zterm_impl) _SA_Pre1_(__valid_impl)
#define _Pre_z_bytecap_(size)           _Pre_notnull_ _SA_Pre2_(__zterm_impl,__bytecap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_z_bytecap_c_(size)         _Pre_notnull_ _SA_Pre2_(__zterm_impl,__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_z_bytecap_x_(size)         _Pre_notnull_ _SA_Pre2_(__zterm_impl,__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_z_cap_(size)               _Pre_notnull_ _SA_Pre2_(__zterm_impl,__cap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_z_cap_c_(size)             _Pre_notnull_ _SA_Pre2_(__zterm_impl,__cap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Pre_z_cap_x_(size)             _Pre_notnull_ _SA_Pre2_(__zterm_impl,__cap_x_impl(size)) _SA_Pre1_(__valid_impl)

#define _Post_bytecap_(size)            _SA_Post1_(__bytecap_impl(size))
#define _Post_bytecount_(size)          _SA_Post1_(__bytecount_impl(size)) _Post_valid_
#define _Post_bytecount_c_(size)        _SA_Post1_(__bytecount_c_impl(size)) _Post_valid_
#define _Post_bytecount_x_(size)        _SA_Post1_(__bytecount_x_impl(size)) _Post_valid_
#define _Post_cap_(size)                _SA_Post1_(__cap_impl(size))
#define _Post_count_(size)              _SA_Post1_(__count_impl(size)) _Post_valid_
#define _Post_count_c_(size)            _SA_Post1_(__count_c_impl(size)) _Post_valid_
#define _Post_count_x_(size)            _SA_Post1_(__count_x_impl(size)) _Post_valid_
#define _Post_defensive_                _SA_annotes0(SAL_post_defensive)
#define _Post_equal_to_(expr)           _Out_range_(==, expr)
#define _Post_invalid_                  _SA_Deref_post1_(__notvalid_impl) // note: implicitly dereferenced!
#define _Post_maybenull_                _SA_Post1_(__maybenull_impl_notref)
#define _Post_maybez_                   _SA_Post1_(__maybzterm_impl)
#define _Post_notnull_                  _SA_Post1_(__notnull_impl_notref)
#define _Post_null_                     _SA_Post1_(__null_impl_notref)
#define _Post_ptr_invalid_              _SA_Post1_(__notvalid_impl)
#define _Post_readable_byte_size_(size) _SA_Post1_(__bytecount_impl(size)) _Post_valid_
#define _Post_readable_size_(size)      _SA_Post1_(__count_impl(size)) _Post_valid_
#define _Post_satisfies_(cond)          _Post_ _SA_Satisfies_(cond)
#define _Post_valid_                    _SA_Post1_(__valid_impl)
#define _Post_writable_byte_size_(size) _SA_Post1_(__bytecap_impl(size))
#define _Post_writable_size_(size)      _SA_Post1_(__cap_impl(size))
#define _Post_z_                        _SA_Post1_(__zterm_impl) _Post_valid_
#define _Post_z_bytecount_(size)        _SA_Post2_(__zterm_impl,__bytecount_impl(size)) _Post_valid_
#define _Post_z_bytecount_c_(size)      _SA_Post2_(__zterm_impl,__bytecount_c_impl(size)) _Post_valid_
#define _Post_z_bytecount_x_(size)      _SA_Post2_(__zterm_impl,__bytecount_x_impl(size)) _Post_valid_
#define _Post_z_count_(size)            _SA_Post2_(__zterm_impl,__count_impl(size)) _Post_valid_
#define _Post_z_count_c_(size)          _SA_Post2_(__zterm_impl,__count_c_impl(size)) _Post_valid_
#define _Post_z_count_x_(size)          _SA_Post2_(__zterm_impl,__count_x_impl(size)) _Post_valid_

#define _Prepost_bytecount_(size)       _Pre_bytecount_(size) _Post_bytecount_(size)
#define _Prepost_bytecount_c_(size)     _Pre_bytecount_c_(size) _Post_bytecount_c_(size)
#define _Prepost_bytecount_x_(size)     _Pre_bytecount_x_(size) _Post_bytecount_x_(size)
#define _Prepost_count_(size)           _Pre_count_(size) _Post_count_(size)
#define _Prepost_count_c_(size)         _Pre_count_c_(size) _Post_count_c_(size)
#define _Prepost_count_x_(size)         _Pre_count_x_(size) _Post_count_x_(size)
#define _Prepost_opt_bytecount_(size)   _Post_bytecount_(size) _Pre_opt_bytecount_(size)
#define _Prepost_opt_bytecount_c_(size) _Post_bytecount_c_(size) _Pre_opt_bytecount_c_(size)
#define _Prepost_opt_bytecount_x_(size) _Post_bytecount_x_(size) _Pre_opt_bytecount_x_(size)
#define _Prepost_opt_count_(size)       _Post_count_(size) _Pre_opt_count_(size)
#define _Prepost_opt_count_c_(size)     _Post_count_c_(size) _Pre_opt_count_c_(size)
#define _Prepost_opt_count_x_(size)     _Post_count_x_(size) _Pre_opt_count_x_(size)
#define _Prepost_opt_valid_             _Post_valid_ _Pre_opt_valid_
#define _Prepost_opt_z_                 _Post_z_ _Pre_opt_z_
#define _Prepost_valid_                 _Pre_valid_ _Post_valid_
#define _Prepost_z_                     _Pre_z_ _Post_z_

#define _Deref_pre_bytecap_(size)       _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_impl(size))
#define _Deref_pre_bytecap_c_(size)     _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_c_impl(size))
#define _Deref_pre_bytecap_x_(size)     _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_x_impl(size))
#define _Deref_pre_bytecount_(size)     _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecount_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_bytecount_c_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecount_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_bytecount_x_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecount_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_cap_(size)           _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_impl(size))
#define _Deref_pre_cap_c_(size)         _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_c_impl(size))
#define _Deref_pre_cap_x_(size)         _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_x_impl(size))
#define _Deref_pre_count_(size)         _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__count_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_count_c_(size)       _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__count_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_count_x_(size)       _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__count_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_invalid_             _SA_Deref_pre1_(__notvalid_impl)
#define _Deref_pre_maybenull_           _SA_Deref_pre1_(__maybenull_impl_notref)
#define _Deref_pre_notnull_             _SA_Deref_pre1_(__notnull_impl_notref)
#define _Deref_pre_null_                _SA_Deref_pre1_(__null_impl_notref)
#define _Deref_pre_opt_bytecap_(size)   _SA_Deref_pre1_(__bytecap_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_bytecap_c_(size) _SA_Deref_pre1_(__bytecap_c_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_bytecap_x_(size) _SA_Deref_pre1_(__bytecap_x_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_bytecount_(size) _SA_Deref_pre1_(__bytecount_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_bytecount_c_(size) _SA_Deref_pre1_(__bytecount_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_bytecount_x_(size) _SA_Deref_pre1_(__bytecount_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_cap_(size)       _SA_Deref_pre1_(__cap_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_cap_c_(size)     _SA_Deref_pre1_(__cap_c_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_cap_x_(size)     _SA_Deref_pre1_(__cap_x_impl(size)) _Deref_pre_maybenull_
#define _Deref_pre_opt_count_(size)     _SA_Deref_pre1_(__count_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_count_c_(size)   _SA_Deref_pre1_(__count_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_count_x_(size)   _SA_Deref_pre1_(__count_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_           _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_bytecap_(size) _SA_Deref_pre1_(__bytecap_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_bytecap_c_(size) _SA_Deref_pre1_(__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_bytecap_x_(size) _SA_Deref_pre1_(__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_cap_(size) _SA_Deref_pre1_(__cap_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_cap_c_(size) _SA_Deref_pre1_(__cap_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_valid_cap_x_(size) _SA_Deref_pre1_(__cap_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_               _SA_Deref_pre1_(__zterm_impl) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_bytecap_(size) _SA_Deref_pre2_(__zterm_impl,__bytecap_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_bytecap_c_(size) _SA_Deref_pre2_(__zterm_impl,__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_bytecap_x_(size) _SA_Deref_pre2_(__zterm_impl,__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_cap_(size)     _SA_Deref_pre2_(__zterm_impl,__cap_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_cap_c_(size)   _SA_Deref_pre2_(__zterm_impl,__cap_c_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_opt_z_cap_x_(size)   _SA_Deref_pre2_(__zterm_impl,__cap_x_impl(size)) _SA_Pre1_(__valid_impl) _Deref_pre_maybenull_
#define _Deref_pre_readonly_            _SA_Deref_pre1_(__readaccess_impl_notref)
#define _Deref_pre_valid_               _SA_Deref_pre1_(__notnull_impl_notref) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_bytecap_(size) _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_bytecap_c_(size) _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_bytecap_x_(size) _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_cap_(size)     _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_cap_c_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_valid_cap_x_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__cap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_writeonly_           _SA_Deref_pre1_(__writeaccess_impl_notref)
#define _Deref_pre_z_                   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre1_(__zterm_impl) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_bytecap_(size)     _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__bytecap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_bytecap_c_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__bytecap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_bytecap_x_(size)   _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__bytecap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_cap_(size)         _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__cap_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_cap_c_(size)       _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__cap_c_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref_pre_z_cap_x_(size)       _SA_Deref_pre1_(__notnull_impl_notref) _SA_Deref_pre2_(__zterm_impl,__cap_x_impl(size)) _SA_Pre1_(__valid_impl)
#define _Deref2_pre_readonly_           _SA_Deref2_pre1_(__readaccess_impl_notref)

#define _Deref_post_valid_              _Pre_notnull_ _When_(1 == 1, _SA_Deref_pre1_(__maybevalid_impl)) _SA_Deref_post1_(__valid_impl)
//#define _Deref_post_valid_              _Deref_post_notnull_ _Post_valid_ // <- ms implementaton
#define _Deref_post_bytecap_(size)      _Deref_post_notnull_ _SA_Deref_post1_(__bytecap_impl(size))
#define _Deref_post_bytecap_c_(sz)      _Deref_post_notnull_ _SA_Deref_post1_(__bytecap_c_impl(sz))
#define _Deref_post_bytecap_x_(sz)      _Deref_post_notnull_ _SA_Deref_post1_(__bytecap_x_impl(sz))
#define _Deref_post_bytecount_(sz)      _Deref_post_notnull_ _SA_Deref_post1_(__bytecount_impl(sz)) _Post_valid_
#define _Deref_post_bytecount_c_(size)  _Deref_post_notnull_ _SA_Deref_post1_(__bytecount_c_impl(size)) _Post_valid_
#define _Deref_post_bytecount_x_(size)  _Deref_post_notnull_ _SA_Deref_post1_(__bytecount_x_impl(size)) _Post_valid_
#define _Deref_post_cap_(size)          _Deref_post_notnull_ _SA_Deref_post1_(__cap_impl(size))
#define _Deref_post_cap_c_(size)        _Deref_post_notnull_ _SA_Deref_post1_(__cap_c_impl(size))
#define _Deref_post_cap_x_(size)        _Deref_post_notnull_ _SA_Deref_post1_(__cap_x_impl(size))
#define _Deref_post_count_(size)        _Deref_post_notnull_ _SA_Deref_post1_(__count_impl(size)) _Post_valid_
#define _Deref_post_count_c_(size)      _Deref_post_notnull_ _SA_Deref_post1_(__count_c_impl(size)) _Post_valid_
#define _Deref_post_count_x_(size)      _Deref_post_notnull_ _SA_Deref_post1_(__count_x_impl(size)) _Post_valid_
#define _Deref_post_maybenull_          _SA_Deref_post1_(__maybenull_impl_notref)
#define _Deref_post_notnull_            _SA_Deref_post1_(__notnull_impl_notref)
#define _Deref_post_null_               _SA_Deref_post1_(__null_impl_notref)
#define _Deref_post_opt_bytecap_(size)  _SA_Deref_post1_(__bytecap_impl(size)) _Deref_post_maybenull_
#define _Deref_post_opt_bytecap_c_(sz)  _SA_Deref_post1_(__bytecap_c_impl(sz)) _Deref_post_maybenull_
#define _Deref_post_opt_bytecap_x_(sz)  _SA_Deref_post1_(__bytecap_x_impl(sz)) _Deref_post_maybenull_
#define _Deref_post_opt_bytecount_(sz)  _SA_Deref_post1_(__bytecount_impl(sz)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_bytecount_c_(size) _SA_Deref_post1_(__bytecount_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_bytecount_x_(size) _SA_Deref_post1_(__bytecount_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_cap_(size)      _SA_Deref_post1_(__cap_impl(size)) _Deref_post_maybenull_
#define _Deref_post_opt_cap_c_(size)    _SA_Deref_post1_(__cap_c_impl(size)) _Deref_post_maybenull_
#define _Deref_post_opt_cap_x_(size)    _SA_Deref_post1_(__cap_x_impl(size)) _Deref_post_maybenull_
#define _Deref_post_opt_count_(size)    _SA_Deref_post1_(__count_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_count_c_(size)  _SA_Deref_post1_(__count_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_count_x_(size)  _SA_Deref_post1_(__count_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_          _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_bytecap_(size) _SA_Deref_post1_(__bytecap_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_bytecap_c_(size) _SA_Deref_post1_(__bytecap_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_bytecap_x_(size) _SA_Deref_post1_(__bytecap_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_cap_(size) _SA_Deref_post1_(__cap_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_cap_c_(size) _SA_Deref_post1_(__cap_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_valid_cap_x_(size) _SA_Deref_post1_(__cap_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_              _SA_Deref_post1_(__zterm_impl) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_bytecap_(size) _SA_Deref_post2_(__zterm_impl,__bytecap_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_bytecap_c_(size) _SA_Deref_post2_(__zterm_impl,__bytecap_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_bytecap_x_(size) _SA_Deref_post2_(__zterm_impl,__bytecap_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_cap_(size) _SA_Deref_post2_(__zterm_impl,__cap_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_cap_c_(size)  _SA_Deref_post2_(__zterm_impl,__cap_c_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_opt_z_cap_x_(size)  _SA_Deref_post2_(__zterm_impl,__cap_x_impl(size)) _Post_valid_ _Deref_post_maybenull_
#define _Deref_post_valid_              _Deref_post_notnull_ _Post_valid_
#define _Deref_post_valid_bytecap_(size) _Deref_post_valid_ _SA_Deref_post1_(__bytecap_impl(size))
#define _Deref_post_valid_bytecap_c_(size) _Deref_post_valid_ _SA_Deref_post1_(__bytecap_c_impl(size))
#define _Deref_post_valid_bytecap_x_(size) _Deref_post_valid_ _SA_Deref_post1_(__bytecap_x_impl(size))
#define _Deref_post_valid_cap_(size)    _Deref_post_valid_ _SA_Deref_post1_(__cap_impl(size))
#define _Deref_post_valid_cap_c_(size)  _Deref_post_valid_ _SA_Deref_post1_(__cap_c_impl(size))
#define _Deref_post_valid_cap_x_(size)  _Deref_post_valid_ _SA_Deref_post1_(__cap_x_impl(size))
#define _Deref_post_z_                  _Deref_post_valid_ _SA_Deref_post1_(__zterm_impl)
#define _Deref_post_z_bytecap_(size)    _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__bytecap_impl(size))
#define _Deref_post_z_bytecap_c_(size)  _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__bytecap_c_impl(size))
#define _Deref_post_z_bytecap_x_(size)  _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__bytecap_x_impl(size))
#define _Deref_post_z_cap_(size)        _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__cap_impl(size))
#define _Deref_post_z_cap_c_(size)      _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__cap_c_impl(size))
#define _Deref_post_z_cap_x_(size)      _Deref_post_valid_ _SA_Deref_post2_(__zterm_impl,__cap_x_impl(size))

#define _Deref_prepost_bytecap_(size)   _Deref_pre_bytecap_(size) _Deref_post_bytecap_(size)
#define _Deref_prepost_bytecap_x_(size) _Deref_pre_bytecap_x_(size) _Deref_post_bytecap_x_(size)
#define _Deref_prepost_bytecount_(size) _Deref_pre_bytecount_(size) _Deref_post_bytecount_(size)
#define _Deref_prepost_bytecount_x_(size) _Deref_pre_bytecount_x_(size) _Deref_post_bytecount_x_(size)
#define _Deref_prepost_cap_(size)       _Deref_pre_cap_(size) _Deref_post_cap_(size)
#define _Deref_prepost_cap_x_(size)     _Deref_pre_cap_x_(size) _Deref_post_cap_x_(size)
#define _Deref_prepost_count_(size)     _Deref_pre_count_(size) _Deref_post_count_(size)
#define _Deref_prepost_count_x_(size)   _Deref_pre_count_x_(size) _Deref_post_count_x_(size)
#define _Deref_prepost_opt_bytecap_(size) _Deref_pre_opt_bytecap_(size) _Deref_post_opt_bytecap_(size)
#define _Deref_prepost_opt_bytecap_x_(size) _Deref_pre_opt_bytecap_x_(size) _Deref_post_opt_bytecap_x_(size)
#define _Deref_prepost_opt_bytecount_(size) _Deref_pre_opt_bytecount_(size) _Deref_post_opt_bytecount_(size)
#define _Deref_prepost_opt_bytecount_x_(size) _Deref_pre_opt_bytecount_x_(size) _Deref_post_opt_bytecount_x_(size)
#define _Deref_prepost_opt_cap_(size)   _Deref_pre_opt_cap_(size) _Deref_post_opt_cap_(size)
#define _Deref_prepost_opt_cap_x_(size) _Deref_pre_opt_cap_x_(size) _Deref_post_opt_cap_x_(size)
#define _Deref_prepost_opt_count_(size) _Deref_pre_opt_count_(size) _Deref_post_opt_count_(size)
#define _Deref_prepost_opt_count_x_(size) _Deref_pre_opt_count_x_(size) _Deref_post_opt_count_x_(size)
#define _Deref_prepost_opt_valid_       _Deref_pre_opt_valid_ _Deref_post_opt_valid_
#define _Deref_prepost_opt_valid_bytecap_(size) _Deref_pre_opt_valid_bytecap_(size) _Deref_post_opt_valid_bytecap_(size)
#define _Deref_prepost_opt_valid_bytecap_x_(size) _Deref_pre_opt_valid_bytecap_x_(size) _Deref_post_opt_valid_bytecap_x_(size)
#define _Deref_prepost_opt_valid_cap_(size) _Deref_pre_opt_valid_cap_(size) _Deref_post_opt_valid_cap_(size)
#define _Deref_prepost_opt_valid_cap_x_(size) _Deref_pre_opt_valid_cap_x_(size) _Deref_post_opt_valid_cap_x_(size)
#define _Deref_prepost_opt_z_           _Deref_pre_opt_z_ _Deref_post_opt_z_
#define _Deref_prepost_opt_z_bytecap_(size) _Deref_pre_opt_z_bytecap_(size) _Deref_post_opt_z_bytecap_(size)
#define _Deref_prepost_opt_z_cap_(size) _Deref_pre_opt_z_cap_(size) _Deref_post_opt_z_cap_(size)
#define _Deref_prepost_valid_           _Deref_pre_valid_ _Deref_post_valid_
#define _Deref_prepost_valid_bytecap_(size) _Deref_pre_valid_bytecap_(size) _Deref_post_valid_bytecap_(size)
#define _Deref_prepost_valid_bytecap_x_(size) _Deref_pre_valid_bytecap_x_(size) _Deref_post_valid_bytecap_x_(size)
#define _Deref_prepost_valid_cap_(size) _Deref_pre_valid_cap_(size) _Deref_post_valid_cap_(size)
#define _Deref_prepost_valid_cap_x_(size) _Deref_pre_valid_cap_x_(size) _Deref_post_valid_cap_x_(size)
#define _Deref_prepost_z_               _Deref_pre_z_ _Deref_post_z_
#define _Deref_prepost_z_bytecap_(size) _Deref_pre_z_bytecap_(size) _Deref_post_z_bytecap_(size)
#define _Deref_prepost_z_cap_(size)     _Deref_pre_z_cap_(size) _Deref_post_z_cap_(size)

#define _In_                            _Pre_notnull_ _Pre_valid_ _Deref_pre_readonly_
#define _In_bytecount_(size)            _Pre_bytecount_(size) _Deref_pre_readonly_
#define _In_bytecount_c_(size)          _Pre_bytecount_c_(size) _Deref_pre_readonly_
#define _In_bytecount_x_(size)          _Pre_bytecount_x_(size) _Deref_pre_readonly_
#define _In_count_(size)                _Pre_count_(size) _Deref_pre_readonly_
#define _In_count_c_(size)              _Pre_count_c_(size) _Deref_pre_readonly_
#define _In_count_x_(size)              _Pre_count_x_(size) _Deref_pre_readonly_
#define _In_defensive_(annotes)         _Pre_defensive_ _Group_(annotes)
#define _In_opt_                        _Deref_pre_readonly_ _Pre_opt_valid_
#define _In_opt_bytecount_(size)        _Deref_pre_readonly_ _Pre_opt_bytecount_(size)
#define _In_opt_bytecount_c_(size)      _Deref_pre_readonly_ _Pre_opt_bytecount_c_(size)
#define _In_opt_bytecount_x_(size)      _Deref_pre_readonly_ _Pre_opt_bytecount_x_(size)
#define _In_opt_count_(size)            _Deref_pre_readonly_ _Pre_opt_count_(size)
#define _In_opt_count_c_(size)          _Deref_pre_readonly_ _Pre_opt_count_c_(size)
#define _In_opt_count_x_(size)          _Deref_pre_readonly_ _Pre_opt_count_x_(size)
#define _In_opt_ptrdiff_count_(size)    _Deref_pre_readonly_ _Pre_opt_ptrdiff_count_(size)
#define _In_opt_z_                      _Deref_pre_readonly_ _Pre_opt_z_
#define _In_opt_z_bytecount_(size)      _Deref_pre_readonly_ _Pre_opt_z_ _Pre_opt_bytecount_(size)
#define _In_opt_z_bytecount_c_(size)    _Deref_pre_readonly_ _Pre_opt_z_ _Pre_opt_bytecount_c_(size)
#define _In_opt_z_count_(size)          _Deref_pre_readonly_ _Pre_opt_z_ _Pre_opt_count_(size)
#define _In_opt_z_count_c_(size)        _Deref_pre_readonly_ _Pre_opt_z_ _Pre_opt_count_c_(size)
#define _In_ptrdiff_count_(size)        _Deref_pre_readonly_ _Pre_ptrdiff_count_(size)
#define _In_reads_(size)                _Deref_pre_readonly_ _Pre_count_(size)
#define _In_reads_bytes_(size)          _Deref_pre_readonly_ _Pre_bytecount_(size)
#define _In_reads_bytes_opt_(size)      _Deref_pre_readonly_ _Pre_opt_bytecount_(size)
#define _In_reads_opt_(size)            _Deref_pre_readonly_ _Pre_opt_count_(size)
#define _In_reads_opt_z_(size)          _Deref_pre_readonly_ _Pre_opt_count_(size) _Pre_opt_z_
#define _In_reads_or_z_(size)           _When_(_String_length_(_Curr_) < (size), _In_z_) _When_(_String_length_(_Curr_) >= (size), _In_reads_(size))
#define _In_reads_to_ptr_(ptr)          _Deref_pre_readonly_ _Pre_ptrdiff_count_(ptr)
#define _In_reads_to_ptr_opt_(ptr)      _Deref_pre_readonly_ _Pre_opt_ptrdiff_count_(ptr)
#define _In_reads_to_ptr_opt_z_(ptr)    _Deref_pre_readonly_ _Pre_opt_ptrdiff_count_(ptr) _Pre_opt_z_
#define _In_reads_to_ptr_z_(ptr)        _Deref_pre_readonly_ _Pre_ptrdiff_count_(ptr) _Pre_z_
#define _In_reads_z_(size)              _Deref_pre_readonly_ _Pre_count_(size) _Pre_z_
#define _In_z_                          _Deref_pre_readonly_ _Pre_z_
#define _In_z_bytecount_(size)          _Deref_pre_readonly_ _Pre_z_ _Pre_bytecount_(size)
#define _In_z_bytecount_c_(size)        _Deref_pre_readonly_ _Pre_z_ _Pre_bytecount_c_(size)
#define _In_z_count_(size)              _Deref_pre_readonly_ _Pre_z_ _Pre_count_(size)
#define _In_z_count_c_(size)            _Deref_pre_readonly_ _Pre_z_ _Pre_count_c_(size)

#define _Out_                           _Pre_cap_c_one_ _Post_valid_ /* not in MS hdrs:  _Deref_post_maybenull_ */
#define _Out_bytecap_(size)             _Pre_bytecap_(size) _Post_valid_
#define _Out_bytecap_c_(size)           _Pre_bytecap_c_(size) _Post_valid_
#define _Out_bytecap_post_bytecount_(cap,count) _Pre_bytecap_(cap) _Post_valid_ _Post_bytecount_(count)
#define _Out_bytecap_x_(size)           _Pre_bytecap_x_(size) _Post_valid_
#define _Out_bytecapcount_(capcount)    _Pre_bytecap_(capcount) _Post_valid_ _Post_bytecount_(capcount)
#define _Out_bytecapcount_x_(capcount)  _Pre_bytecap_x_(capcount) _Post_valid_ _Post_bytecount_x_(capcount)
#define _Out_cap_(size)                 _Pre_cap_(size) _Post_valid_
#define _Out_cap_c_(size)               _Pre_cap_c_(size) _Post_valid_
#define _Out_cap_m_(mult,size)          _Pre_cap_m_(mult,size) _Post_valid_
#define _Out_cap_post_count_(cap,count) _Pre_cap_(cap) _Post_valid_ _Post_count_(count)
#define _Out_cap_x_(size)               _Pre_cap_x_(size) _Post_valid_
#define _Out_capcount_(capcount)        _Pre_cap_(capcount) _Post_valid_ _Post_count_(capcount)
#define _Out_capcount_x_(capcount)      _Pre_cap_x_(capcount) _Post_valid_ _Post_count_x_(capcount)
#define _Out_defensive_(annotes)        _Post_defensive_ _Group_(annotes)
#define _Out_opt_                       _Post_valid_ _Pre_opt_cap_c_one_
#define _Out_opt_bytecap_(size)         _Post_valid_ _Pre_opt_bytecap_(size)
#define _Out_opt_bytecap_c_(size)       _Post_valid_ _Pre_opt_bytecap_c_(size)
#define _Out_opt_bytecap_post_bytecount_(cap,count) _Post_valid_ _Post_bytecount_(count) _Pre_opt_bytecap_(cap)
#define _Out_opt_bytecap_x_(size)       _Post_valid_ _Pre_opt_bytecap_x_(size)
#define _Out_opt_bytecapcount_(capcount) _Post_valid_ _Post_bytecount_(capcount) _Pre_opt_bytecap_(capcount)
#define _Out_opt_bytecapcount_x_(capcount) _Post_valid_ _Post_bytecount_x_(capcount) _Pre_opt_bytecap_x_(capcount)
#define _Out_opt_cap_(size)             _Post_valid_ _Pre_opt_cap_(size)
#define _Out_opt_cap_c_(size)           _Post_valid_ _Pre_opt_cap_c_(size)
#define _Out_opt_cap_m_(mult,size)      _Post_valid_ _Pre_opt_cap_m_(mult,size)
#define _Out_opt_cap_post_count_(cap,count) _Post_valid_ _Post_count_(count) _Pre_opt_cap_(cap)
#define _Out_opt_cap_x_(size)           _Post_valid_ _Pre_opt_cap_x_(size)
#define _Out_opt_capcount_(capcount)    _Post_valid_ _Post_count_(capcount) _Pre_opt_cap_(capcount)
#define _Out_opt_capcount_x_(capcount)  _Post_valid_ _Post_count_x_(capcount) _Pre_opt_cap_x_(capcount)
#define _Out_opt_ptrdiff_cap_(size)     _Post_valid_ _Pre_opt_ptrdiff_cap_(size)
#define _Out_opt_z_bytecap_(size)       _Post_valid_ _Post_z_ _Pre_opt_bytecap_(size)
#define _Out_opt_z_bytecap_c_(size)     _Post_valid_ _Post_z_ _Pre_opt_bytecap_c_(size)
#define _Out_opt_z_bytecap_post_bytecount_(cap,count) _Post_valid_ _Post_z_bytecount_(count) _Pre_opt_bytecap_(cap)
#define _Out_opt_z_bytecap_x_(size)     _Post_valid_ _Post_z_ _Pre_opt_bytecap_x_(size)
#define _Out_opt_z_bytecapcount_(capcount) _Post_valid_ _Post_z_bytecount_(capcount) _Pre_opt_bytecap_(capcount)
#define _Out_opt_z_cap_(size)           _Post_valid_ _Post_z_ _Pre_opt_cap_(size)
#define _Out_opt_z_cap_c_(size)         _Post_valid_ _Post_z_ _Pre_opt_cap_c_(size)
#define _Out_opt_z_cap_m_(mult,size)    _Post_valid_ _Post_z_ _Pre_opt_cap_m_(mult,size)
#define _Out_opt_z_cap_post_count_(cap,count) _Post_valid_ _Post_z_count_(count) _Pre_opt_cap_(cap)
#define _Out_opt_z_cap_x_(size)         _Post_valid_ _Post_z_ _Pre_opt_cap_x_(size)
#define _Out_opt_z_capcount_(capcount)  _Post_valid_ _Post_z_count_(capcount) _Pre_opt_cap_(capcount)
#define _Out_ptrdiff_cap_(size)         _Post_valid_ _Pre_ptrdiff_cap_(size)
#define _Out_writes_(size)              _Post_valid_ _Pre_cap_(size)
#define _Out_writes_all_(size)          _Out_writes_to_(_Old_(size), _Old_(size))
#define _Out_writes_all_opt_(size)      _Out_writes_to_opt_(_Old_(size), _Old_(size))
#define _Out_writes_bytes_(size)        _Post_valid_ _Pre_bytecap_(size)
#define _Out_writes_bytes_all_(size)    _Out_writes_bytes_to_(_Old_(size), _Old_(size))
#define _Out_writes_bytes_all_opt_(size) _Out_writes_bytes_to_opt_(_Old_(size), _Old_(size))
#define _Out_writes_bytes_opt_(size)    _Post_valid_ _Pre_opt_bytecap_(size)
#define _Out_writes_bytes_to_(size,count) _Post_valid_ _Post_bytecount_(count) _Pre_bytecap_(size)
#define _Out_writes_bytes_to_opt_(size,count) _Post_valid_ _Post_bytecount_(count) _Pre_opt_bytecap_(size)
#define _Out_writes_opt_(size)          _Post_valid_ _Pre_opt_cap_(size)
#define _Out_writes_opt_z_(size)        _Post_valid_ _Post_z_ _Pre_opt_cap_(size)
#define _Out_writes_to_(size,count)     _Post_valid_ _Post_count_(count) _Pre_cap_(size)
#define _Out_writes_to_opt_(size,count) _Post_valid_ _Post_count_(count) _Pre_opt_cap_(size)
#define _Out_writes_to_ptr_(ptr)        _Post_valid_ _Pre_ptrdiff_cap_(ptr)
#define _Out_writes_to_ptr_opt_(ptr)    _Post_valid_ _Pre_opt_ptrdiff_cap_(ptr)
#define _Out_writes_to_ptr_opt_z_(ptr)  _Post_valid_ Post_z_ _Pre_opt_ptrdiff_cap_(ptr)
#define _Out_writes_to_ptr_z_(ptr)      _Post_valid_ Post_z_ _Pre_ptrdiff_cap_(ptr)
#define _Out_writes_z_(size)            _Post_valid_ _Post_z_ _Pre_cap_(size)
#define _Out_z_bytecap_(size)           _Post_valid_ _Post_z_ _Pre_bytecap_(size)
#define _Out_z_bytecap_c_(size)         _Post_valid_ _Post_z_ _Pre_bytecap_c_(size)
#define _Out_z_bytecap_post_bytecount_(cap,count) _Post_valid_ _Post_z_bytecount_(count) _Pre_bytecap_(cap)
#define _Out_z_bytecap_x_(size)         _Post_valid_ _Post_z_ _Pre_bytecap_x_(size)
#define _Out_z_bytecapcount_(capcount)  _Post_valid_ _Post_z_bytecount_(capcount) _Pre_bytecap_(capcount)
#define _Out_z_cap_(size)               _Post_valid_ _Post_z_ _Pre_cap_(size)
#define _Out_z_cap_c_(size)             _Post_valid_ _Post_z_ _Pre_cap_c_(size)
#define _Out_z_cap_m_(mult,size)        _Post_valid_ _Post_z_ _Pre_cap_m_(mult,size)
#define _Out_z_cap_post_count_(cap,count) _Post_valid_ _Post_z_count_(count) _Pre_cap_(cap)
#define _Out_z_cap_x_(size)             _Post_valid_ _Post_z_ _Pre_cap_x_(size)
#define _Out_z_capcount_(capcount)      _Post_valid_ _Post_z_count_(capcount) _Pre_cap_(capcount)

#define _Inout_                         _Prepost_valid_
#define _Inout_bytecap_(size)           _Post_valid_ _Pre_valid_bytecap_(size)
#define _Inout_bytecap_c_(size)         _Post_valid_ _Pre_valid_bytecap_c_(size)
#define _Inout_bytecap_x_(size)         _Post_valid_ _Pre_valid_bytecap_x_(size)
#define _Inout_bytecount_(size)         _Prepost_bytecount_(size)
#define _Inout_bytecount_c_(size)       _Prepost_bytecount_c_(size)
#define _Inout_bytecount_x_(size)       _Prepost_bytecount_x_(size)
#define _Inout_cap_(size)               _Post_valid_ _Pre_valid_cap_(size)
#define _Inout_cap_c_(size)             _Post_valid_ _Pre_valid_cap_c_(size)
#define _Inout_cap_x_(size)             _Post_valid_ _Pre_valid_cap_x_(size)
#define _Inout_count_(size)             _Prepost_count_(size)
#define _Inout_count_c_(size)           _Prepost_count_c_(size)
#define _Inout_count_x_(size)           _Prepost_count_x_(size)
#define _Inout_defensive_(annotes)      _Pre_defensive_ _Post_defensive_ _Group_(annotes)
#define _Inout_opt_                     _Prepost_opt_valid_
#define _Inout_opt_bytecap_(size)       _Post_valid_ _Pre_opt_valid_bytecap_(size)
#define _Inout_opt_bytecap_c_(size)     _Post_valid_ _Pre_opt_valid_bytecap_c_(size)
#define _Inout_opt_bytecap_x_(size)     _Post_valid_ _Pre_opt_valid_bytecap_x_(size)
#define _Inout_opt_bytecount_(size)     _Prepost_opt_bytecount_(size)
#define _Inout_opt_bytecount_c_(size)   _Prepost_opt_bytecount_c_(size)
#define _Inout_opt_bytecount_x_(size)   _Prepost_opt_bytecount_x_(size)
#define _Inout_opt_cap_(size)           _Post_valid_ _Pre_opt_valid_cap_(size)
#define _Inout_opt_cap_c_(size)         _Post_valid_ _Pre_opt_valid_cap_c_(size)
#define _Inout_opt_cap_x_(size)         _Post_valid_ _Pre_opt_valid_cap_x_(size)
#define _Inout_opt_count_(size)         _Prepost_opt_count_(size)
#define _Inout_opt_count_c_(size)       _Prepost_opt_count_c_(size)
#define _Inout_opt_count_x_(size)       _Prepost_opt_count_x_(size)
#define _Inout_opt_ptrdiff_count_(size) _Pre_opt_ptrdiff_count_(size)
#define _Inout_opt_z_                   _Prepost_opt_z_
#define _Inout_opt_z_bytecap_(size)     _Pre_opt_z_bytecap_(size) _Post_z_
#define _Inout_opt_z_bytecap_c_(size)   _Pre_opt_z_bytecap_c_(size) _Post_z_
#define _Inout_opt_z_bytecap_x_(size)   _Pre_opt_z_bytecap_x_(size) _Post_z_
#define _Inout_opt_z_bytecount_(size)   _Prepost_z_ _Prepost_opt_bytecount_(size)
#define _Inout_opt_z_bytecount_c_(size) _Prepost_z_ _Prepost_opt_bytecount_c_(size)
#define _Inout_opt_z_cap_(size)         _Pre_opt_z_cap_(size) _Post_z_
#define _Inout_opt_z_cap_c_(size)       _Pre_opt_z_cap_c_(size) _Post_z_
#define _Inout_opt_z_cap_x_(size)       _Pre_opt_z_cap_x_(size) _Post_z_
#define _Inout_opt_z_count_(size)       _Prepost_z_ _Prepost_opt_count_(size)
#define _Inout_opt_z_count_c_(size)     _Prepost_z_ _Prepost_opt_count_c_(size)
#define _Inout_ptrdiff_count_(size)     _Pre_ptrdiff_count_(size)
#define _Inout_updates_(size)           _Post_valid_ _Pre_cap_(size) _SA_Pre1_(__valid_impl)
#define _Inout_updates_all_(size)       _Inout_updates_to_(_Old_(size), _Old_(size))
#define _Inout_updates_all_opt_(size)   _Inout_updates_to_opt_(_Old_(size), _Old_(size))
#define _Inout_updates_bytes_(size)     _Post_valid_ _Pre_bytecap_(size) _SA_Pre1_(__valid_impl)
#define _Inout_updates_bytes_all_(size) _Inout_updates_bytes_to_(_Old_(size), _Old_(size))
#define _Inout_updates_bytes_all_opt_(size) _Inout_updates_bytes_to_opt_(_Old_(size), _Old_(size))
#define _Inout_updates_bytes_opt_(size) _Post_valid_ _SA_Pre1_(__valid_impl) _Pre_opt_bytecap_(size)
#define _Inout_updates_bytes_to_(size,count) _Out_writes_bytes_to_(size,count) _SA_Pre1_(__valid_impl) _SA_Pre1_(__bytecount_impl(count))
#define _Inout_updates_bytes_to_opt_(size,count) _SA_Pre1_(__valid_impl) _SA_Pre1_(__bytecount_impl(count)) _Out_writes_bytes_to_opt_(size,count)
#define _Inout_updates_opt_(size)       _Post_valid_ _SA_Pre1_(__valid_impl) _Pre_opt_cap_(size)
#define _Inout_updates_opt_z_(size)     _Post_valid_ _SA_Post1_(__zterm_impl)  _SA_Pre1_(__valid_impl) _SA_Pre1_(__zterm_impl) _Pre_opt_cap_(size)
#define _Inout_updates_to_(size,count)  _Out_writes_to_(size,count) _SA_Pre1_(__valid_impl) _SA_Pre1_(__count_impl(count))
#define _Inout_updates_to_opt_(size,count) _SA_Pre1_(__valid_impl) _SA_Pre1_(__count_impl(count)) _Out_writes_to_opt_(size,count)
#define _Inout_updates_z_(size)         _Pre_cap_(size) _SA_Pre1_(__valid_impl) _Post_valid_ _SA_Pre1_(__zterm_impl) _SA_Post1_(__zterm_impl)
#define _Inout_z_                       _Prepost_z_
#define _Inout_z_bytecap_(size)         _Pre_z_bytecap_(size) _Post_z_
#define _Inout_z_bytecap_c_(size)       _Pre_z_bytecap_c_(size) _Post_z_
#define _Inout_z_bytecap_x_(size)       _Pre_z_bytecap_x_(size) _Post_z_
#define _Inout_z_bytecount_(size)       _Prepost_z_ _Prepost_bytecount_(size)
#define _Inout_z_bytecount_c_(size)     _Prepost_z_ _Prepost_bytecount_c_(size)
#define _Inout_z_cap_(size)             _Pre_z_cap_(size) _Post_z_
#define _Inout_z_cap_c_(size)           _Pre_z_cap_c_(size) _Post_z_
#define _Inout_z_cap_x_(size)           _Pre_z_cap_x_(size) _Post_z_
#define _Inout_z_count_(size)           _Prepost_z_ _Prepost_count_(size)
#define _Inout_z_count_c_(size)         _Prepost_z_ _Prepost_count_c_(size)

#define _Deref_opt_out_                 _Out_opt_ _Deref_post_valid_
#define _Deref_opt_out_opt_             _Out_opt_ _Deref_post_opt_valid_
#define _Deref_opt_out_opt_z_           _Out_opt_ _Deref_post_opt_z_
#define _Deref_opt_out_z_               _Out_opt_ _Deref_post_z_
#define _Deref_out_                     _Out_ _Deref_post_valid_
#define _Deref_out_opt_                 _Out_ _Deref_post_opt_valid_
#define _Deref_out_opt_z_               _Out_ _Deref_post_opt_z_
#define _Deref_out_z_                   _Out_ _Deref_post_z_
#define _Deref_out_z_bytecap_c_(sz)     _Deref_pre_bytecap_c_(sz) _Deref_post_z_
#define _Deref_out_z_cap_c_(size)       _Deref_pre_cap_c_(size) _Deref_post_z_
#define _Deref_inout_bound_             _Deref_in_bound_ _Deref_out_bound_
#define _Deref_inout_z_                 _Deref_prepost_z_
#define _Deref_inout_z_bytecap_c_(size) _Deref_pre_z_bytecap_c_(size) _Deref_post_z_
#define _Deref_inout_z_cap_c_(size)     _Deref_pre_z_cap_c_(size) _Deref_post_z_

#define _Outptr_                        _Out_ _SA_Deref_post2_(__notnull_impl_notref, __count_impl(1)) _Post_valid_
#define _Outptr_opt_                    _Out_opt_ _SA_Deref_post2_(__notnull_impl_notref, __count_impl(1)) //_Post_valid_
#define _Outptr_opt_result_buffer_(size) _Out_opt_ _SA_Deref_post2_(__notnull_impl_notref, __cap_impl(size)) //_Post_valid_
#define _Outptr_opt_result_buffer_all_(size) _Out_opt_ _SA_Deref_post2_(__notnull_impl_notref, __count_impl(size)) //_Post_valid_
#define _Outptr_opt_result_buffer_all_maybenull_(size) _Out_opt_ _SA_Deref_post2_(__maybenull_impl_notref, __count_impl(size)) //_Post_valid_
#define _Outptr_opt_result_buffer_maybenull_(size) _Out_opt_ _SA_Deref_post2_(__maybenull_impl_notref, __cap_impl(size)) //_Post_valid_
#define _Outptr_opt_result_buffer_to_(size, count) _Out_opt_ _SA_Deref_post3_(__notnull_impl_notref, __cap_impl(size), __count_impl(count)) //_Post_valid_
#define _Outptr_opt_result_buffer_to_maybenull_(size, count) _Out_opt_ _SA_Deref_post3_(__maybenull_impl_notref, __cap_impl(size), __count_impl(count)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_(size) _Out_opt_ _SA_Deref_post2_(__notnull_impl_notref, __bytecap_impl(size)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_all_(size) _Out_opt_ _SA_Deref_post2_(__notnull_impl_notref, __bytecount_impl(size)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_all_maybenull_(size) _Out_opt_ _SA_Deref_post2_(__maybenull_impl_notref, __bytecount_impl(size)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_maybenull_(size) _Out_opt_ _SA_Deref_post2_(__maybenull_impl_notref, __bytecap_impl(size)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_to_(size, count) _Out_opt_ _SA_Deref_post3_(__notnull_impl_notref, __bytecap_impl(size), __bytecount_impl(count)) //_Post_valid_
#define _Outptr_opt_result_bytebuffer_to_maybenull_(size, count) _Out_opt_ _SA_Deref_post3_(__maybenull_impl_notref, __bytecap_impl(size), __bytecount_impl(count)) //_Post_valid_
#define _Outptr_opt_result_maybenull_   _Out_opt_ _SA_Deref_post2_(__maybenull_impl_notref, __count_impl(1)) //_Post_valid_
#define _Outptr_opt_result_maybenull_z_ _Out_opt_ _Deref_post_opt_z_
#define _Outptr_opt_result_nullonfailure_ _Outptr_opt_ _On_failure_(_Deref_post_null_)
#define _Outptr_opt_result_z_           _Out_opt_ _Deref_post_z_
#define _Outptr_result_buffer_(size)    _Out_ _SA_Deref_post2_(__notnull_impl_notref, __cap_impl(size)) _Post_valid_
#define _Outptr_result_buffer_all_(size) _Out_ _SA_Deref_post2_(__notnull_impl_notref, __count_impl(size)) _Post_valid_
#define _Outptr_result_buffer_all_maybenull_(size) _Out_ _SA_Deref_post2_(__maybenull_impl_notref, __count_impl(size)) _Post_valid_
#define _Outptr_result_buffer_maybenull_(size) _Out_ _SA_Deref_post2_(__maybenull_impl_notref, __cap_impl(size)) _Post_valid_
#define _Outptr_result_buffer_to_(size, count) _Out_ _SA_Deref_post3_(__notnull_impl_notref, __cap_impl(size), __count_impl(count)) _Post_valid_
#define _Outptr_result_buffer_to_maybenull_(size, count) _Out_ _SA_Deref_post3_(__maybenull_impl_notref, __cap_impl(size), __count_impl(count)) _Post_valid_
#define _Outptr_result_bytebuffer_(size) _Out_ _SA_Deref_post2_(__notnull_impl_notref, __bytecap_impl(size)) _Post_valid_
#define _Outptr_result_bytebuffer_all_(size) _Out_ _SA_Deref_post2_(__notnull_impl_notref, __bytecount_impl(size)) _Post_valid_
#define _Outptr_result_bytebuffer_all_maybenull_(size) _Out_ _SA_Deref_post2_(__maybenull_impl_notref, __bytecount_impl(size)) _Post_valid_
#define _Outptr_result_bytebuffer_maybenull_(size) _Out_ _SA_Deref_post2_(__maybenull_impl_notref, __bytecap_impl(size)) _Post_valid_
#define _Outptr_result_bytebuffer_to_(size, count) _Out_ _SA_Deref_post3_(__notnull_impl_notref, __bytecap_impl(size), __bytecount_impl(count)) _Post_valid_
#define _Outptr_result_bytebuffer_to_maybenull_(size, count) _Out_ _SA_Deref_post3_(__maybenull_impl_notref, __bytecap_impl(size), __bytecount_impl(count)) _Post_valid_
#define _Outptr_result_maybenull_       _Out_ _SA_Deref_post2_(__maybenull_impl_notref, __count_impl(1)) _Post_valid_
#define _Outptr_result_maybenull_z_     _Out_ _Deref_post_opt_z_
#define _Outptr_result_nullonfailure_   _Outptr_ _On_failure_(_Deref_post_null_)
#define _Outptr_result_z_               _Out_ _Deref_post_z_

#define _COM_Outptr_                    _Outptr_ _On_failure_(_Deref_post_null_)
#define _COM_Outptr_opt_                _Outptr_opt_ _On_failure_(_Deref_post_null_)
#define _COM_Outptr_opt_result_maybenull_ _Outptr_opt_result_maybenull_ _On_failure_(_Deref_post_null_)
#define _COM_Outptr_result_maybenull_   _Outptr_result_maybenull_ _On_failure_(_Deref_post_null_)

#define _Outref_                        _Group_(_Out_)
#define _Outref_result_buffer_(size)    _SA_Post1_(__cap_impl(size)) _Post_valid_
#define _Outref_result_buffer_all_(size) _SA_Post1_(__count_impl(size))
#define _Outref_result_buffer_all_maybenull_(size) _SA_Post2_(__count_impl(size), __maybenull_impl)
#define _Outref_result_buffer_maybenull_(size) _SA_Post2_(__cap_impl(size), __maybenull_impl) _Post_valid_
#define _Outref_result_buffer_to_(size, count) _SA_Post2_(__cap_impl(size), __count_impl(count))
#define _Outref_result_buffer_to_maybenull_(size, count) _SA_Post3_(__cap_impl(size), __count_impl(count), __maybenull_impl)
#define _Outref_result_bytebuffer_(size) _SA_Post1_(__bytecap_impl(size)) _Post_valid_
#define _Outref_result_bytebuffer_all_(size) _SA_Post1_(__bytecount_impl(size))
#define _Outref_result_bytebuffer_all_maybenull_(size) _SA_Post2_(__bytecount_impl(size), __maybenull_impl)
#define _Outref_result_bytebuffer_maybenull_(size) _SA_Post2_(__bytecap_impl(size), __maybenull_impl) _Post_valid_
#define _Outref_result_bytebuffer_to_(size, count) _SA_Post2_(__bytecap_impl(size), __bytecount_impl(count))
#define _Outref_result_bytebuffer_to_maybenull_(size, count) _SA_Post3_(__bytecap_impl(size), __bytecount_impl(count), __maybenull_impl)
#define _Outref_result_maybenull_       _Group_(_Pre_cap_c_one_ _Post_valid_ __maybenull_impl)
#define _Outref_result_nullonfailure_   _Group_(_Out_) _On_failure_(_Post_null_)

#define _Reserved_                      _SA_Pre1_(__null_impl)
#define _Result_nullonfailure_          _On_failure_(_Notref_ _SA_Deref_ _Post_null_)
#define _Result_zeroonfailure_          _On_failure_(_Notref_ _SA_Deref_ _Out_range_(==, 0))

#define __inner_callback                _SA_annotes0(__callback)

#define _Ret_                           _Ret_valid_
#define _Ret_bytecap_(size)             _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecap_impl(size))
#define _Ret_bytecap_c_(size)           _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecap_c_impl(size))
#define _Ret_bytecap_x_(size)           _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecap_x_impl(size))
#define _Ret_bytecount_(size)           _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_bytecount_c_(size)         _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecount_c_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_bytecount_x_(size)         _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__bytecount_x_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_cap_(size)                 _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__cap_impl(size))
#define _Ret_cap_c_(size)               _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__cap_c_impl(size))
#define _Ret_cap_x_(size)               _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__cap_x_impl(size))
#define _Ret_count_(size)               _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__count_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_count_c_(size)             _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__count_c_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_count_x_(size)             _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__count_x_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_maybenull_                 _SA_Ret1_(__maybenull_impl)
#define _Ret_maybenull_z_               _SA_Ret2_(__maybenull_impl,__zterm_impl) _SA_Ret1_(__valid_impl)
#define _Ret_notnull_                   _SA_Ret1_(__notnull_impl)
#define _Ret_null_                      _SA_Ret1_(__null_impl)
#define _Ret_opt_                       _Ret_opt_valid_
#define _Ret_opt_bytecap_(size)         _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecap_impl(size))
#define _Ret_opt_bytecap_c_(size)       _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecap_c_impl(size))
#define _Ret_opt_bytecap_x_(size)       _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecap_x_impl(size))
#define _Ret_opt_bytecount_(size)       _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_bytecount_c_(size)     _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecount_c_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_bytecount_x_(size)     _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__bytecount_x_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_cap_(size)             _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__cap_impl(size))
#define _Ret_opt_cap_c_(size)           _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__cap_c_impl(size))
#define _Ret_opt_cap_x_(size)           _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__cap_x_impl(size))
#define _Ret_opt_count_(size)           _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__count_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_count_c_(size)         _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__count_c_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_count_x_(size)         _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__count_x_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_valid_                 _SA_Ret1_(__maybenull_impl_notref) _SA_Ret1_(__valid_impl)
#define _Ret_opt_z_                     _SA_Ret2_(__maybenull_impl,__zterm_impl) _SA_Ret1_(__valid_impl)
#define _Ret_opt_z_bytecap_(size)       _SA_Ret1_(__maybenull_impl_notref) _SA_Ret2_(__zterm_impl,__bytecap_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_z_bytecount_(size)     _SA_Ret1_(__maybenull_impl_notref) _SA_Ret2_(__zterm_impl,__bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_z_cap_(size)           _SA_Ret1_(__maybenull_impl_notref) _SA_Ret2_(__zterm_impl,__cap_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_opt_z_count_(size)         _SA_Ret1_(__maybenull_impl_notref) _SA_Ret2_(__zterm_impl,__count_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_valid_                     _SA_Ret1_(__notnull_impl_notref) _SA_Ret1_(__valid_impl)
#define _Ret_writes_(size)              _SA_Ret2_(__notnull_impl, __count_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_bytes_(size)        _SA_Ret2_(__notnull_impl, __bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_bytes_maybenull_(size) _SA_Ret2_(__maybenull_impl,__bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_bytes_to_(size,count) _SA_Ret3_(__notnull_impl, __bytecap_impl(size), __bytecount_impl(count)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_bytes_to_maybenull_(size,count) _SA_Ret3_(__maybenull_impl, __bytecap_impl(size), __bytecount_impl(count)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_maybenull_(size)    _SA_Ret2_(__maybenull_impl,__count_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_maybenull_z_(size)  _SA_Ret3_(__maybenull_impl,__count_impl(size),__zterm_impl) _SA_Ret1_(__valid_impl)
#define _Ret_writes_to_(size,count)     _SA_Ret3_(__notnull_impl, __cap_impl(size), __count_impl(count)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_to_maybenull_(size,count) _SA_Ret3_(__maybenull_impl, __cap_impl(size), __count_impl(count)) _SA_Ret1_(__valid_impl)
#define _Ret_writes_z_(size)            _SA_Ret3_(__notnull_impl, __count_impl(size), __zterm_impl) _SA_Ret1_(__valid_impl)
#define _Ret_z_                         _SA_Ret2_(__notnull_impl, __zterm_impl) _SA_Ret1_(__valid_impl)
#define _Ret_z_bytecap_(size)           _SA_Ret1_(__notnull_impl_notref) _SA_Ret2_(__zterm_impl,__bytecap_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_z_bytecount_(size)         _SA_Ret1_(__notnull_impl_notref) _SA_Ret2_(__zterm_impl,__bytecount_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_z_cap_(size)               _SA_Ret1_(__notnull_impl_notref) _SA_Ret2_(__zterm_impl,__cap_impl(size)) _SA_Ret1_(__valid_impl)
#define _Ret_z_count_(size)             _SA_Ret1_(__notnull_impl_notref) _SA_Ret2_(__zterm_impl,__count_impl(size)) _SA_Ret1_(__valid_impl)

#define _Deref_ret_opt_z_               _SA_Deref_ret1_(__maybenull_impl_notref) _SA_Ret1_(__zterm_impl)
#define _Deref_ret_z_                   _SA_Deref_ret1_(__notnull_impl_notref) _SA_Deref_ret1_(__zterm_impl)

/* Additional annotation declarations */
#define __ANNOTATION(fun) _SA_annotes0(SAL_annotation) void __SA_##fun
#define __PRIMOP(type, fun) _SA_annotes0(SAL_primop) type __SA_##fun;
#if (_MSC_VER < 1600)

__ANNOTATION(SAL_satisfies(_In_ char);)

#define _Inexpressible_(x) (x)

#endif

__ANNOTATION(SAL_constant(enum __SAL_YesNo);)
__ANNOTATION(SAL_TypeName(__AuToQuOtE char *));

__ANNOTATION(SAL_functionClassNew(_In_ char*);)
__PRIMOP(int, _In_function_class_(_In_ char*);)
#define _In_function_class_(x) _In_function_class_(#x)

__ANNOTATION(SAL_interlocked(void);)

__ANNOTATION(SAL_untrusted_data_source(__AuToQuOtE char *));


#else /* _USE_ATTRIBUTES_FOR_SAL || _USE_DECLSPECS_FOR_SAL */

/* Dummys */
#define __inner_exceptthat
#define __inner_typefix(ctype)
#define _Always_(annos)
#define _Analysis_noreturn_
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

#endif /* _USE_ATTRIBUTES_FOR_SAL || _USE_DECLSPECS_FOR_SAL */

