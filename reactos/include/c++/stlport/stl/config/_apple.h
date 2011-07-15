/* STLport configuration file
 * It is internal STLport header - DO NOT include it directly
 */

/* common configuration settings for Apple MPW MrCpp / SCpp */

#define _STLP_COMPILER "spec me!"

#if defined(__MRC__) && __MRC__ < 0x500
# error Apple's MPW MrCpp v.5.0.0 or better compiler required
#endif
#if defined(__SC__) && __SC__ < 0x890
# error Apple's MPW SCpp v.8.9.0 or better compiler required
#endif

/* TODO: Check that this config is necessary for all compiler versions.
 * It is here for historical reasons for the moment.
 */
#define _STLP_NO_CONTAINERS_EXTENSION

#ifdef qMacApp
# ifndef __CONDITIONALMACROS__ /* skip including ConditionalMacros_AC.h if ConditionalMacros.h is already included */
# include <CoreSwitches_AC.h>
# include <ConditionalMacros_AC.h>
# include <Types_AC.h>
# define _STLP_FILE__ _FILE_AC
# define _STLP_DEBUG_MESSAGE
# define __stl_debug_message ProgramBreak_AC
# include <ConditionalMacros.h>
# endif
# include <Types.h>
#else
# include <ConditionalMacros.h>
# include <Types.h>
#endif

#define _STLP_UINT32_T UInt32
typedef int wint_t;

#ifndef TYPE_BOOL
# error <ConditionalMacros.h> must be included. (TYPE_BOOL)
#endif
#if !TYPE_BOOL
# define _STLP_NO_BOOL
# define _STLP_DONT_USE_BOOL_TYPEDEF
#endif

#ifndef TYPE_LONGLONG
# error <ConditionalMacros.h> must be included. (TYPE_LONGLONG)
#endif
#if TYPE_LONGLONG
# define _STLP_LONG_LONG long long
#endif

#if !__option(exceptions)
# define _STLP_HAS_NO_EXCEPTIONS
#endif

#define _STLP_DEBUG_MESSAGE_POST DebugStr("\pSTL diagnosis issued. See 'stderr' for detail.");
#define _STLP_ASSERT_MSG_TRAILER " "

#ifdef _STLP_DEBUG
#   define _STLP_THROW(x) (DebugStr("\pSTL is about to throw exception: "#x),throw x)
#endif

#if defined(__MRC__)
# ifndef __spillargs
#  define __spillargs 1 // MrCpp requires this symbol to be defined as 1 to properly handle va_start; ref.[ file stdarg.h; line 26 ]
# endif
#endif

#if defined(__SC__)
#define _STLP_VENDOR_LONG_DOUBLE_MATH        //*TY 12/03/2000 - SCpp's native math type is long double
#endif

#ifndef _STLP_NATIVE_INCLUDE_PATH
# if __option(unix_includes)
#  define _STLP_NATIVE_INCLUDE_PATH ../CIncludes   // expects the alias to {CIncludes} under the same folder as {STL}
# else
#  define _STLP_NATIVE_INCLUDE_PATH ::CIncludes   // expects the alias to {CIncludes} under the same folder as {STL}
# endif
#endif
#if !defined(_STLP_MAKE_HEADER)
# if !__option(unix_includes)
#  define _STLP_MAKE_HEADER(path, header) <path:header> // Mac uses ":" for directory delimiter
# endif
#endif

# define _STLD _DBG  // to keep the length of generated symbols within the compiler limitation

#define _STLP_USE_STDIO_IO 1       //*TY 02/24/2000 - see also ; ref.[ file _fstream.h; line 36 ]
#define _STLP_NO_THREADS           //*TY 12/17/2000 - multi-thread capability not explored, yet.
#undef _REENTRANT                  //*ty 11/24/2001 - to make sure no thread facility is activated
#define _NOTHREADS                 //*ty 12/07/2001 -

// native library limitations
#define _STLP_VENDOR_GLOBAL_STD          // mpw's c++ libs do not utilize namespace std yet
#define _STLP_NO_BAD_ALLOC               // known limitation
#define _STLP_HAS_NO_NEW_C_HEADERS       // known limitation
#define _STLP_NO_NEW_NEW_HEADER          // known limitation
#define _STLP_NO_NATIVE_MBSTATE_T        // known limitation
#define _STLP_NO_NATIVE_WIDE_FUNCTIONS   // known limitation
#define _STLP_NO_NATIVE_WIDE_STREAMS     // known limitation
#define _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT // known limitation
#define _STLP_BROKEN_EXCEPTION_CLASS     // known limitation

// compiler limitations
# define _STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS

# define _STLP_MPWFIX_TRY try{                      //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_MPWFIX_CATCH }catch(...){throw;}              //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_MPWFIX_CATCH_ACTION(action) }catch(...){action;throw;}  //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_THROW_RETURN_BUG            // known limitation
# define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION    // known limitation
# define _STLP_NO_PARTIAL_SPECIALIZATION_SYNTAX    // known limitation
# define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER    // known limitation
# define _STLP_NO_RELOPS_NAMESPACE          // known limitation
// end of stl_apple.h
