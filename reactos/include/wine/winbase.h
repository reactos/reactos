/* $Id $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <winbase.h>

#ifndef __WINE_WINBASE_H
#define __WINE_WINBASE_H

#include <w32api.h>

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

#if (_WIN32_WINNT >= 0x0500)
UINT WINAPI GetSystemWindowsDirectoryW(LPWSTR,UINT);
#endif

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

#endif /* __WINE_WINSPOOL_H */
