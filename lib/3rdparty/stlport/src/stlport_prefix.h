#ifndef STLPORT_PREFIX_H
#define STLPORT_PREFIX_H

#define __BUILDING_STLPORT 1

#if defined (_WIN32) || defined (WIN32)
#  ifdef __cplusplus
#    define WIN32_LEAN_AND_MEAN
#    define NOSERVICE
#  endif
#endif

#undef _STLP_NO_FORCE_INSTANTIATE

/* Please add extra compilation switches for particular compilers here */

#if defined (_MSC_VER) && !defined (__COMO__) && !defined (__MWERKS__)
#  include "warning_disable.h"
#endif

#include <stl/config/features.h>

#if defined (_STLP_USE_TEMPLATE_EXPORT) && defined (_STLP_USE_DECLSPEC) && !defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)
#  define _STLP_EXPOSE_GLOBALS_IMPLEMENTATION
#endif

#ifdef __cplusplus

#  include <ctime>
#  if defined (_STLP_USE_NAMESPACES) && !defined (_STLP_VENDOR_GLOBAL_CSTD)
using _STLP_VENDOR_CSTD::time_t;
#  endif

#  if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
#    define _STLP_OPERATOR_SPEC _STLP_DECLSPEC
#  else
#    define _STLP_OPERATOR_SPEC _STLP_TEMPLATE_NULL _STLP_DECLSPEC
#  endif

#endif /* __cplusplus */

#endif /* PREFIX */

