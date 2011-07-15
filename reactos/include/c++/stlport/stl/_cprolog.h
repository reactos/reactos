/* NOTE : this header has no guards and is MEANT for multiple inclusion!
 * If you are using "header protection" option with your compiler,
 * please also find #pragma which disables it and put it here, to
 * allow reentrancy of this header.
 */

#ifdef std
#  undef std /* We undef "std" on entry , as STLport headers may include native ones. */
#endif

#ifdef _STLP_PROLOG_HEADER_INCLUDED
#  error STlport prolog header can not be reincluded as long as epilog has not be included.
#endif

#define _STLP_PROLOG_HEADER_INCLUDED

#ifndef _STLP_FEATURES_H
#  include <stl/config/features.h>
#endif

/* If the platform provides any specific prolog actions,
 * like #pragmas, do include platform-specific prolog file */
#if defined (_STLP_HAS_SPECIFIC_PROLOG_EPILOG)
#  include <stl/config/_prolog.h>
#endif
