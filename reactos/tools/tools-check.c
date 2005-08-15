#include "tools-check.h"

/*
 * - Binutils older than 2003/10/01 have broken windres which can't handle
 *   icons with alpha channel.
 * - Binutils between 2004/09/02 and 2004/10/08 have broken handling of
 *   forward exports in dlltool.
 */

#if (BINUTILS_VERSION_DATE >= 20040902 && BINUTILS_VERSION_DATE <= 20041008) || \
    (BINUTILS_VERSION_DATE < 20031001)
#error "Due to technical reasons your binutils version can't be used to" \
       "build ReactOS. Please consider upgrading to newer version. See" \
       "www.mingw.org for details."
#endif

/*
 * GCC 3.3.1 is lowest allowed version. Older versions have various problems
 * with C++ code.
 */

#if (__GNUC__ == 3 && __GNUC_MINOR__ < 3) || \
    (__GNUC__ < 3)
#error "Due to technical reasons your GCC version can't be used to" \
       "build ReactOS. Please consider upgrading to newer version. See" \
       "www.mingw.org for details."
#endif

/*
 * FIXME: GCC 3.4.1 has broken headers which cause Explorer to not build.
 * We should warn in this case...maybe add check for the broken headers?
 */
