#ifndef __stl_config__hpux_h
#define __stl_config__hpux_h

#define _STLP_PLATFORM "HP Unix"

#define _STLP_USE_UNIX_IO

#ifdef __GNUC__
#  define _STLP_NO_WCHAR_T
#  define _STLP_NO_CWCHAR
#  define _STLP_NO_LONG_DOUBLE
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 199506
#  endif
#endif

#endif /* __stl_config__hpux_h */
