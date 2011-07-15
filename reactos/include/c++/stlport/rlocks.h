#ifndef _STLP_misc_rlocks_h
# define _STLP_misc_rlocks_h
# if (__SUNPRO_CC >= 0x500 )
#  include <../CCios/rlocks.h>
# elif defined (__SUNPRO_CC)
#  include <../CC/rlocks.h>
# else
#  error "This file is for SUN CC only. Please remove it if it causes any harm for other compilers."
# endif
#endif

