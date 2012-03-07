
/*
 * Compatibility section
 * This section sets new-style macros based on old-style ones, for compatibility
 */

#if defined (__STL_DEBUG) && !defined (_STLP_DEBUG)
#  define _STLP_DEBUG __STL_DEBUG
#endif
#if defined (__STL_NO_ANACHRONISMS) && !defined (_STLP_NO_ANACHRONISMS)
#  define _STLP_NO_ANACHRONISMS __STL_NO_ANACHRONISMS
#endif
#if defined (__STL_NO_EXTENSIONS) && !defined (_STLP_NO_EXTENSIONS)
#  define _STLP_NO_EXTENSIONS __STL_NO_EXTENSIONS
#endif
#if defined (__STL_NO_EXCEPTIONS) && !defined (_STLP_NO_EXCEPTIONS)
#  define _STLP_NO_EXCEPTIONS __STL_NO_EXCEPTIONS
#endif
#if defined (__STL_NO_NAMESPACES) && !defined (_STLP_NO_NAMESPACES)
#  define _STLP_NO_NAMESPACES __STL_NO_NAMESPACES
#endif
#if defined (__STL_MINIMUM_DEFAULT_TEMPLATE_PARAMS) && !defined (_STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS)
#  define _STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS __STL_MINIMUM_DEFAULT_TEMPLATE_PARAMS
#endif
#if defined (__STL_NO_OWN_NAMESPACE) && !defined (_STLP_NO_OWN_NAMESPACE)
#  define _STLP_NO_OWN_NAMESPACE __STL_NO_OWN_NAMESPACE
#endif

#if defined (__STL_NO_RELOPS_NAMESPACE) && !defined (_STLP_NO_RELOPS_NAMESPACE)
#  define _STLP_NO_RELOPS_NAMESPACE __STL_NO_RELOPS_NAMESPACE
#endif

#if defined (__STL_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_UNINITIALIZED)
#  define _STLP_DEBUG_UNINITIALIZED __STL_DEBUG_UNINITIALIZED
#endif
#if defined (__STL_SHRED_BYTE) && !defined (_STLP_SHRED_BYTE)
#  define _STLP_SHRED_BYTE __STL_SHRED_BYTE
#endif
#if defined (__STL_USE_MFC) && !defined (_STLP_USE_MFC)
#  define _STLP_USE_MFC __STL_USE_MFC
#endif

#if defined (__STL_USE_NEWALLOC) && !defined (_STLP_USE_NEWALLOC)
#  define _STLP_USE_NEWALLOC __STL_USE_NEWALLOC
#endif
#if defined (__STL_USE_MALLOC) && !defined (_STLP_USE_MALLOC)
#  define _STLP_USE_MALLOC __STL_USE_MALLOC
#endif

#if defined (__STL_DEBUG_ALLOC) && !defined (_STLP_DEBUG_ALLOC)
#  define _STLP_DEBUG_ALLOC __STL_DEBUG_ALLOC
#endif

#if defined (__STL_DEBUG_MESSAGE) && !defined (_STLP_DEBUG_MESSAGE)
#  define _STLP_DEBUG_MESSAGE __STL_DEBUG_MESSAGE
#endif

#if defined (__STL_DEBUG_TERMINATE) && !defined (_STLP_DEBUG_TERMINATE)
#  define _STLP_DEBUG_TERMINATE __STL_DEBUG_TERMINATE
#endif

#if defined (__STL_USE_ABBREVS) && !defined (_STLP_USE_ABBREVS)
#  define _STLP_USE_ABBREVS __STL_USE_ABBREVS
#endif

#if defined (__STL_NO_MSVC50_COMPATIBILITY) && !defined (_STLP_NO_MSVC50_COMPATIBILITY)
#  define _STLP_NO_MSVC50_COMPATIBILITY __STL_NO_MSVC50_COMPATIBILITY
#endif

/* STLport do not support anymore the iostream wrapper mode so this macro should
 * always been define for other libraries that was using it:
 */
#if !defined (_STLP_OWN_IOSTREAMS)
#  define _STLP_OWN_IOSTREAMS
#endif

#if defined (_STLP_NO_OWN_IOSTREAMS)
#  error STLport do not support anymore the wrapper mode. If you want to use STLport \
use its iostreams implementation or no iostreams at all.
#endif
