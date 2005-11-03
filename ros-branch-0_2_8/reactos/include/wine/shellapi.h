/* $Id$
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <shellapi.h>

#ifndef _WINE_SHELLAPI_H
#define _WINE_SHELLAPI_H

#define SHGFI_UNKNOWN1          0x000000020
#define SHGFI_UNKNOWN2          0x000000040
#define SHGFI_UNKNOWN3          0x000000080
#endif
