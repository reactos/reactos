/* Helper header to give feedback about build environment configuration
 * thanks to #pragma message directives.
 */

#if defined (_STLP_VERBOSE)
#  define _STLP_VERBOSE_MODE_SUPPORTED

#  if defined (_STLP_COMPILER)
#    pragma message (_STLP_COMPILER)
#  endif

#  if defined (_STLP_NO_RTTI)
#    pragma message ("STLport: RTTI support         -> Disabled")
#  else
#    pragma message ("STLport: RTTI support         -> Enabled")
#  endif

#  if defined (_STLP_HAS_NO_EXCEPTIONS)
#    pragma message ("STLport: Exception support    -> Disabled")
#  else
#    pragma message ("STLport: Exception support    -> Enabled")
#  endif

#  if defined (_STLP_THREADS)
#    pragma message ("STLport: Threading model      -> Multi")
#  else
#    pragma message ("STLport: Threading model      -> Mono")
#  endif

#  if defined (_STLP_USE_DYNAMIC_LIB)
#    pragma message ("STLport: Library model        -> Dynamic")
#  else
#    pragma message ("STLport: Library model        -> Static")
#  endif

#  if defined (_STLP_USING_CROSS_NATIVE_RUNTIME_LIB)
#    if defined (_STLP_USE_DYNAMIC_LIB)
#    pragma message ("STLport: Native library model -> Static")
#    else
#    pragma message ("STLport: Native library model -> Dynamic")
#    endif
#  endif
#endif
