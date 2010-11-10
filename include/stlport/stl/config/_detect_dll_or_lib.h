/* File used to define macros:
 * _STLP_USE_DYNAMIC_LIB: To signal use STLport as a dll or build of the
 *                        STLport dlls.
 * _STLP_USE_STATIC_LIB: To signal use of STLport as a static lib or build of
 *                       the STLport static libs.
 * _STLP_USING_CROSS_NATIVE_RUNTIME_LIB: Signal that we are using a native runtime
 *                                       in a version different from the STLport one.
 * If _STLP_USE_DYNAMIC_LIB or _STLP_USE_STATIC_LIB is alreday defined it means that
 * user forces use of a specific version. In this case _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
 * will surely be defined otherwise there is no need to force macro defines.
 *
 * Before including this file the compiler must define:
 * _STLP_DLL: If we are building a dll
 * _STLP_RUNTIME_DLL: If we are using native runtime as a dll
 */
#if defined (__BUILDING_STLPORT)
#  undef _STLP_USE_DYNAMIC_LIB
#  undef _STLP_USE_STATIC_LIB
#  if defined (_STLP_DLL)
/* We are building the STLport dll */
#    define _STLP_USE_DYNAMIC_LIB
#    if !defined (_STLP_RUNTIME_DLL)
#      define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#    endif
#  else
#    define _STLP_USE_STATIC_LIB
#    if defined (_STLP_RUNTIME_DLL)
#      define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#    endif
#  endif
#else
#  if !defined (_STLP_NO_IOSTREAMS)
/* STLport use only depend on _STLP_RUNTIME_DLL as we are replacing
 * the native runtime.
 */
#    if defined (_STLP_RUNTIME_DLL)
#      if !defined (_STLP_USE_STATIC_LIB)
#        if !defined (_STLP_USE_DYNAMIC_LIB)
#          define _STLP_USE_DYNAMIC_LIB
#        endif
#      else
/* The user is forcing use of STLport as a dynamic library. We signal it so
 * that the STLport namespace will be modify to report such a combination
 * and force the user to link with the rebuilt STLport library.
 */
#        define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#      endif
#    else
#      if !defined(_STLP_USE_DYNAMIC_LIB)
#        if !defined (_STLP_USE_STATIC_LIB)
#          define _STLP_USE_STATIC_LIB
#        endif
#      else
/* Idem previous remark but the user forces use of the static native runtime.
 */
#        define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#      endif
#    endif
#  else
/* If we do not build and use STLport libs we consider that we are in a static lib
 * mode as only dynamic lib needs additional export/import specifier.
 */
#    define _STLP_USE_STATIC_LIB
#  endif
#endif

/* we don't have a static native runtime library on evc3/evc4 */
#ifdef _STLP_WCE
#  undef _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#endif

#if !defined (_STLP_USE_DYNAMIC_LIB) && !defined (_STLP_USE_STATIC_LIB)
#  error Unknown STLport usage config (dll/lib?)
#endif
