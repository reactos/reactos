/*
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/*
 * Purpose of this file :
 *
 * A list of COMPILER-SPECIFIC portion of STLport settings.
 * This file is provided to help in manual configuration
 * of STLport. This file is being included by stlcomp.h
 * when STLport is unable to identify your compiler.
 * Please remove the error diagnostic below before adjusting
 * macros.
 *
 */
#ifndef _STLP_MYCOMP_H
#define  _STLP_MYCOMP_H

#error "Your compiler version is not recognized by STLport. Please edit <stlport/stl/config/stl_mycomp.h>"

//==========================================================

// the values choosen here as defaults try to give
// maximum functionality on the most conservative settings

// Mostly correct guess, change it for Alpha (and other environments
// that has 64-bit "long")
// #  define _STLP_UINT32_T unsigned long

// Disables wchar_t functionality
// #  define _STLP_NO_WCHAR_T  1

// Define if wchar_t is not an intrinsic type, and is actually a typedef to unsigned short.
// #  define _STLP_WCHAR_T_IS_USHORT 1

// Uncomment if long long is available
// #  define _STLP_LONG_LONG long long

// Uncomment if long double is not available
// #  define _STLP_NO_LONG_DOUBLE 1

// Uncomment this if your compiler does not support "typename" keyword
// #  define _STLP_NEED_TYPENAME 1

// Uncomment this if your compiler does not support "mutable" keyword
// #  define _STLP_NEED_MUTABLE 1

// Uncomment this if your compiler does not support "explicit" keyword
// #  define _STLP_NEED_EXPLICIT 1

// Uncomment if new-style-casts like const_cast<> are not available
// #  define _STLP_NO_NEW_STYLE_CASTS 1

// Uncomment this if your compiler does not have "bool" type
// #  define  _STLP_NO_BOOL 1

// Uncomment this if your compiler does not have "bool" type, but has "bool" keyword reserved
// #  define  _STLP_DONT_USE_BOOL_TYPEDEF 1

// Uncomment this if your compiler does not have "bool" type, but defines "bool" in <yvals.h>
// #  define  _STLP_YVALS_H 1

// Uncomment this if your compiler has limited or no default template arguments for classes
// #  define _STLP_LIMITED_DEFAULT_TEMPLATES 1

// Uncomment this if your compiler support only complete (not dependent on other parameters)
// types as default parameters for class templates
// #  define _STLP_DEFAULT_TYPE_PARAM 1

// Uncomment this if your compiler do not support default parameters in template class methods
// #  define _STLP_DONT_SUP_DFLT_PARAM 1

// Uncomment this if your compiler has problem with not-type
// default template parameters
// #  define _STLP_NO_DEFAULT_NON_TYPE_PARAM 1

// Define if compiler has
// trouble with functions getting non-type-parameterized classes as parameters
// #  define _STLP_NON_TYPE_TMPL_PARAM_BUG 1

// Uncomment this if your compiler does not support namespaces
// #  define _STLP_HAS_NO_NAMESPACES 1

// Uncomment if "using" keyword does not work with template types
// # define _STLP_BROKEN_USING_DIRECTIVE 1

// Uncomment this if your compiler does not support exceptions
// #  define _STLP_HAS_NO_EXCEPTIONS 1

// Uncomment this when you are able to detect that the user do not
// want to use the exceptions feature.
// #  define _STLP_DONT_USE_EXCEPTIONS 1

// Uncomment this if your compiler does not support exception specifications
// #  define _STLP_NO_EXCEPTION_SPEC

// Define this if your compiler requires return statement after throw()
// # define _STLP_THROW_RETURN_BUG 1

// Define this if your compiler do not support return of void
// # define _STLP_DONT_RETURN_VOID 1

// Header <new> that comes with the compiler
// does not define bad_alloc exception
// #  define _STLP_NO_BAD_ALLOC  1

// Define this if your compiler do not throw bad_alloc from the new operator
// #  define _STLP_NEW_DONT_THROW_BAD_ALLOC  1

// Define this if your compiler has no rtti support or if it has been disabled
// #  define _STLP_NO_RTTI 1

// Define this if there is no native type_info definition
// #  define _STLP_NO_TYPEINFO 1

// Uncomment if member template methods are not available
// #  define _STLP_NO_MEMBER_TEMPLATES   1

// Uncomment if member template classes are not available
// #  define _STLP_NO_MEMBER_TEMPLATE_CLASSES   1

// Uncomment if your compiler do not support the std::allocator rebind technique
// This is a special case of bad member template classes support, it is automatically
// defined if _STLP_NO_MEMBER_TEMPLATE_CLASSES is defined.
// # define _STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE 1

// Uncomment if no "template" keyword should be used with member template classes
// #  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD   1

// Compiler does not accept friend declaration qualified with namespace name.
// #  define _STLP_NO_QUALIFIED_FRIENDS 1

// Uncomment if partial specialization is not available
// #  define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION 1

// Define if class being partially specialized require full name (template parameters)
// of itself for method declarations
// #  define _STLP_PARTIAL_SPEC_NEEDS_TEMPLATE_ARGS

// Compiler has problem with qualified specializations (cont int, volatile int...)
// #  define _STLP_QUALIFIED_SPECIALIZATION_BUG

// Compiler has problems specializing members of partially
// specialized class
// #  define _STLP_MEMBER_SPECIALIZATION_BUG

// Uncomment if partial order of template functions is not available
// #  define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER 1

// Uncomment if specialization of methods is not allowed
// #  define _STLP_NO_METHOD_SPECIALIZATION  1

// Uncomment if full  specialization does not use partial spec. syntax : template <> struct ....
// #  define _STLP_NO_PARTIAL_SPECIALIZATION_SYNTAX  1

// Uncomment if compiler does not support explicit template arguments for functions
// # define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS

// Uncomment this if your compiler can't inline while(), for()
// #  define _STLP_LOOP_INLINE_PROBLEMS 1

// Define if the compiler fails to match a template function argument of base
// #  define _STLP_BASE_MATCH_BUG          1

// Define if the compiler fails to match a template function argument of base
// (non-template)
//#  define  _STLP_NONTEMPL_BASE_MATCH_BUG 1

// Define if the compiler rejects outline method definition
// explicitly taking nested types/typedefs
// #  define _STLP_NESTED_TYPE_PARAM_BUG   1

// Compiler requires typename keyword on outline method definition
// explicitly taking nested types/typedefs
// #define  _STLP_TYPENAME_ON_RETURN_TYPE

// Define if the baseclass typedefs not visible from outside
// #  define _STLP_BASE_TYPEDEF_OUTSIDE_BUG 1

// if your compiler have serious problems with typedefs, try this one
// #  define _STLP_BASE_TYPEDEF_BUG          1

// Uncomment if getting errors compiling mem_fun* adaptors
// #  define _STLP_MEMBER_POINTER_PARAM_BUG 1

// Uncomment if the compiler can't handle a constant-initializer in the
// declaration of a static const data member of integer type.
// (See section 9.4.2, paragraph 4, of the C++ standard.)
// # define _STLP_STATIC_CONST_INIT_BUG

// Uncomment to indicate that the compiler do not like static constant
// definition.
// Meaningfull only if  _STLP_STATIC_CONST_INIT_BUG is not defined.
// # define _STLP_NO_STATIC_CONST_DEFINITION

// Define if default constructor for builtin integer type fails to initialize it to 0
// In expression like new(&char) char():
//# define _STLP_DEF_CONST_PLCT_NEW_BUG 1
// In default function parameter like _M_method(_Tp __x = _Tp())
//# define _STLP_DEF_CONST_DEF_PARAM_BUG 1

// Defined if constructor
// required to explicitly call member's default constructors for const objects
// #  define _STLP_CONST_CONSTRUCTOR_BUG    1

// Defined if the compiler has trouble calling POD-types constructors/destructors
// #  define _STLP_TRIVIAL_CONSTRUCTOR_BUG    1
// #  define _STLP_TRIVIAL_DESTRUCTOR_BUG    1

// Define if having problems specializing maps/sets with
// key type being const
// #  define _STLP_MULTI_CONST_TEMPLATE_ARG_BUG

// Uncomment this to disable -> operators on all iterators
// #  define   _STLP_NO_ARROW_OPERATOR 1

// Uncomment this to disble at() member functions for containers
// #  define   _STLP_NO_AT_MEMBER_FUNCTION 1

// Define this if compiler lacks <exception> header
// #  define _STLP_NO_EXCEPTION_HEADER 1

// Uncomment this if your C library has lrand48() function
// #  define _STLP_RAND48 1

// Uncomment if native new-style C library headers lile <cstddef>, etc are not available.
// #   define _STLP_HAS_NO_NEW_C_HEADERS 1

// uncomment if new-style headers <new> is available
// #  define _STLP_HAS_NEW_NEW_HEADER 1

// uncomment this if <iostream> and other STD headers put their stuff in ::namespace,
// not std::
// #  define _STLP_VENDOR_GLOBAL_STD

// uncomment this if <cstdio> and the like put stuff in ::namespace,
// not std::
// #  define _STLP_VENDOR_GLOBAL_CSTD

// uncomment this if your compiler consider as ambiguous a function imported within
// the stlport namespace and called without scope (:: or std::)
// #  define _STLP_NO_USING_FOR_GLOBAL_FUNCTIONS 1

// uncomment this if your compiler define all the C math functions C++ additional
// overloads in ::namespace and not only in std::.
// #  define _STLP_HAS_GLOBAL_C_MATH_FUNCTIONS 1

// Edit relative path below (or put full path) to get native
// compiler headers included. Default is "../include".
// C headers may reside in different directory, so separate macro is provided.
// Hint : never install STLport in the directory that ends with "include"
// # define _STLP_NATIVE_INCLUDE_PATH ../include
// # define _STLP_NATIVE_C_INCLUDE_PATH ../include
// # define _STLP_NATIVE_CPP_C_INCLUDE_PATH ../include

// This macro constructs header path from directory and name.
// You may change it if your compiler does not understand "/".
// #  define _STLP_MAKE_HEADER(path, header) <path/header>

// This macro constructs native include header path from include path and name.
// You may have do define it if experimenting problems with preprocessor
// # define _STLP_NATIVE_HEADER(header) _STLP_MAKE_HEADER(_STLP_NATIVE_INCLUDE_PATH,header)

// Same for C headers
// #define _STLP_NATIVE_C_HEADER(header)

//==========================================================
#endif
