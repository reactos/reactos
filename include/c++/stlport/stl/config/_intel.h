// STLport configuration file
// It is internal STLport header - DO NOT include it directly

#define _STLP_COMPILER "Intel ICL"

#define _STLP_IMPORT_TEMPLATE_KEYWORD extern

/* You need to undef following macro if your icl install is binded to MSVC 6
 * native lib and you are building with /Qvc7 or /Qvc7.1 or /Qvc8 option.
 */
/* #define _STLP_MSVC_LIB 1200 */
/* You need to undef following macro if your icl install is binded to MSVC .Net 2002
 * native lib and you are building without any /Qvc* option or with /Qvc6 or /Qvc7.1
 * or /Qvc8 option.
 */
/* #define _STLP_MSVC_LIB 1300 */
/* You need to undef following macro if your icl install is binded to MSVC .Net 2003
 * native lib and you are building without any /Qvc* option or with /Qvc6 or /Qvc7
 * or /Qvc8 option.
 */
/* #define _STLP_MSVC_LIB 1310 */
/* You need to undef following macro if your icl install is binded to MSVC 2005
 * native lib and you are building without any /Qvc* option or with /Qvc6 or /Qvc7
 * or /Qvc7.1 option.
 */
/* #define _STLP_MSVC_LIB 1400 */

#include <stl/config/_msvc.h>

#if defined (_STLP_DONT_RETURN_VOID)
#  undef _STLP_DONT_RETURN_VOID
#endif

#if (__ICL < 900)
#  define _STLP_NOTHROW
#endif

#if (__ICL <= 810)
/* If method specialization is activated, compiler do not export some
 * symbols anymore.
 */
#  define _STLP_NO_METHOD_SPECIALIZATION 1
#endif

#if (__ICL >= 800 && __ICL < 900)
#  define _STLP_STATIC_CONST_INIT_BUG 1
#endif

#if (__ICL >= 450)
#  define _STLP_DLLEXPORT_NEEDS_PREDECLARATION 1
#endif

#if (__ICL < 450)
/*    only static STLport lib works for ICL */
#  undef  _STLP_USE_STATIC_LIB
#  undef  _STLP_USE_DYNAMIC_LIB
#  define _STLP_USE_STATIC_LIB
/*    disable hook which makes template symbols to be searched for in the library */
#  undef _STLP_NO_CUSTOM_IO
#endif

#undef  _STLP_LONG_LONG
#define _STLP_LONG_LONG long long

#if defined (__cplusplus) && (__ICL >= 900) && (_STLP_MSVC_LIB < 1300)
namespace std
{
  void _STLP_CALL unexpected();
}
#endif

#include <stl/config/_feedback.h>
