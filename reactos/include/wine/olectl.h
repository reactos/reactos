/* $Id $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <olectl.h>

#ifndef __WINE_OLECTL_H
#define __WINE_OLECTL_H

#include <w32api.h>

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

#include <ocidl.h>

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */


#endif /* __WINE_GDI_H */
