/* STLport configuration file
 * It is internal STLport header - DO NOT include it directly
 */

#define _STLP_COMPILER "aCC"

/* system C-library dependent */
#if defined (_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
#  define _STLP_RAND48 1
#endif
/* #  define _STLP_RAND48 1 */
/* #define _STLP_NO_NATIVE_MBSTATE_T      1 */
#define _STLP_HPACC_BROKEN_BUFEND       1
#define _STLP_WCHAR_HPACC_EXCLUDE      1

/* this was reported to help, just as with SUN CC 4.2 */
#define _STLP_INLINE_STRING_LITERAL_BUG

/* specific prolog is needed to select correct threads impl */
#define _STLP_HAS_SPECIFIC_PROLOG_EPILOG

/* HP aCC with +noeh */
#ifdef __HPACC_NOEH
#  define _STLP_HAS_NO_EXCEPTIONS 1
#endif

#define _STLP_NO_FORCE_INSTANTIATE
#define _STLP_LONG_LONG long long
#define _STLP_NO_VENDOR_STDLIB_L

/* The aCC6 compiler is using the EDG Front End. Unfortunately, prior to
 * version A.06.12, defining the __EDG__ and __EDG_VERSION__ macros was
 * disabled. It was corrected in A.06.12.
 */
#if ((__HP_aCC > 60000) && (__HP_aCC < 61200))
#  define __EDG__
#  define __EDG_VERSION__ 306
#endif 

#if (__HP_aCC >= 32500 )
#  define _STLP_USE_NEW_C_HEADERS

#  define _STLP_FORCE_ALLOCATORS(t,a) \
  typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
  typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;

#  if defined (_HP_NAMESPACE_STD) // option -AA
/* from now, we have a full standard lib in namespace std
 *
 * -AA indicates that we are compiling against Rogue Wave 2.2.1
 * STL shipped with the HP aCC compiler. -AA tells the compiler
 * to use the STL defined in the include_std directory.
 */
#    define _STLP_NATIVE_INCLUDE_PATH ../include_std
#  else // option -Aa
#    define _STLP_VENDOR_GLOBAL_STD         1
#    define _STLP_VENDOR_GLOBAL_CSTD        1
#    define _STLP_DONT_THROW_RANGE_ERRORS   1
#  endif
#endif

#if (__HP_aCC >= 31400 && __HP_aCC < 32500)
#  define _STLP_FORCE_ALLOCATORS(t,a) \
typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;
#  define _STLP_NO_CWCHAR
#  if defined (_NAMESPACE_STD) // option -AA
/* from now, we have a full standard lib in namespace std */
#    define _STLP_NATIVE_INCLUDE_PATH       ../include_std
#  else /* kind of compatibility mode */
#    define _STLP_VENDOR_GLOBAL_STD         1
#    define _STLP_VENDOR_GLOBAL_CSTD        1
#    define _STLP_DONT_THROW_RANGE_ERRORS   1
#    define _STLP_NO_ROPE                   1
#  endif
#endif /* 314 */

#if ((__HP_aCC >= 30000 && __HP_aCC < 31400) || (__HP_aCC == 1)) // A.03.13: __HP_aCC == 1

#  if (__HP_aCC != 1)
#    define _STLP_HAS_NO_NEW_C_HEADERS 1
#  endif

#  define _STLP_NO_QUALIFIED_FRIENDS       1
/* aCC bug ? need explicit args on constructors of partial specialized
 * classes
 */
#  define _STLP_PARTIAL_SPEC_NEEDS_TEMPLATE_ARGS 1
/* ?? fbp : maybe present in some versions ? */
#  define _STLP_NO_MEMBER_TEMPLATE_CLASSES 1
#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1
/* <exception> and stuff is in global namespace */
#  define _STLP_VENDOR_GLOBAL_EXCEPT_STD
/* fbp : moved here */
#  define _STLP_VENDOR_GLOBAL_CSTD        1
/* #     define _INCLUDE_HPUX_SOURCE */
#  define _XPG4
#  define _INCLUDE_XOPEN_SOURCE
#  define _INCLUDE_AES_SOURCE
#endif

#if (__HP_aCC <= 30000 && __HP_aCC >= 12100)
/* Special kludge to workaround bug in aCC A.01.23, CR JAGac40634 */
#  ifdef _STLP_DEBUG
static void _STLP_dummy_literal() { const char *p = "x";}
static void _STLP_dummy_literal_2() { const char *p = "123456789"; }
static void _STLP_dummy_literal_3() { const char *p = "123456700000000000000089";}
#  endif

#  define _STLP_VENDOR_GLOBAL_STD         1
#  define _STLP_VENDOR_GLOBAL_CSTD        1
#  define _STLP_DONT_THROW_RANGE_ERRORS   1
#  define _STLP_STATIC_CONST_INIT_BUG 1
#  if (__HP_aCC  < 12700)
/* new flag: on most HP compilers cwchar is missing */
#    define _STLP_NO_CWCHAR
#  endif

#  define _STLP_FORCE_ALLOCATORS(t,a) \
  typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
  typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;
#endif

#if __HP_aCC == 1
#  define _STLP_BROKEN_USING_IN_CLASS
#  define _STLP_USING_BASE_MEMBER
#  define _STLP_NO_CWCHAR
/* #     define _STLP_NO_WCHAR_T 1 */
#endif
