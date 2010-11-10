/* We do not use auto link feature when:
 *  - user asked not to use it (_STLP_DONT_USE_AUTO_LINK)
 *  - STLport is used only as a STL library (_STLP_NO_IOSTREAMS || _STLP_USE_NO_IOSTREAMS)
 *  - we are building a C translation unit, STLport is a C++ Standard library implementation
 */
#if !defined (__BUILDING_STLPORT) && !defined (_STLP_DONT_USE_AUTO_LINK) && \
    !defined (_STLP_NO_IOSTREAMS) && !defined (_STLP_USE_NO_IOSTREAMS) && \
    defined (__cplusplus)

#  define _STLP_STRINGIZE(X) _STLP_STRINGIZE_AUX(X)
#  define _STLP_STRINGIZE_AUX(X) #X

#  if defined (_STLP_DEBUG)
#    define _STLP_LIB_OPTIM_MODE "stld"
#  elif defined (_DEBUG)
#    define _STLP_LIB_OPTIM_MODE "d"
#  else
#    define _STLP_LIB_OPTIM_MODE ""
#  endif

#  if defined (_STLP_LIB_NAME_MOTIF)
#    define _STLP_LIB_MOTIF "_"_STLP_LIB_NAME_MOTIF
#  else
#    define _STLP_LIB_MOTIF ""
#  endif

#  if defined (_STLP_USE_DYNAMIC_LIB)
#    if defined (_STLP_USING_CROSS_NATIVE_RUNTIME_LIB)
#      define _STLP_LIB_TYPE "_x"
#    else
#      define _STLP_LIB_TYPE ""
#    endif
#  else
#    if defined (_STLP_USING_CROSS_NATIVE_RUNTIME_LIB)
#      define _STLP_LIB_TYPE "_statix"
#    else
#      define _STLP_LIB_TYPE "_static"
#    endif
#  endif

#  if defined (_STLP_USE_DYNAMIC_LIB)
#    define _STLP_VERSION_STR "."_STLP_STRINGIZE(_STLPORT_MAJOR)"."_STLP_STRINGIZE(_STLPORT_MINOR)
#  else
#    define _STLP_VERSION_STR ""
#  endif

#  define _STLP_STLPORT_LIB "stlport"_STLP_LIB_OPTIM_MODE""_STLP_LIB_TYPE""_STLP_LIB_MOTIF""_STLP_VERSION_STR".lib"

#  if defined (_STLP_VERBOSE)
#    pragma message ("STLport: Auto linking to "_STLP_STLPORT_LIB)
#  endif
#  pragma comment (lib, _STLP_STLPORT_LIB)

#  undef _STLP_STLPORT_LIB
#  undef _STLP_LIB_OPTIM_MODE
#  undef _STLP_LIB_TYPE
#  undef _STLP_STRINGIZE_AUX
#  undef _STLP_STRINGIZE

#endif /* _STLP_DONT_USE_AUTO_LINK */

