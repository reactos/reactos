/* $Id $
 *
 * Compatibility header
 */

#ifndef REACTOS_WINUSER_H
#define REACTOS_WINUSER_H

#include <w32api.h>

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

#ifdef WINVER
#undef WINVER
#endif

#define WINVER 0x0501

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

#include_next <winuser.h>

#endif /* REACTOS_WINUSER_H */
