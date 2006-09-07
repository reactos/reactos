/* $Id$
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <shlguid.h>


#ifndef __WINE_SHLGUID_H
#define __WINE_SHLGUID_H

/* command group ids */
DEFINE_GUID(IID_IShellLinkDataList, 0x45e2b4ae, 0xb1c3, 0x11d0, 0xb9, 0x2f, 0x00, 0xa0, 0xc9, 0x03, 0x12, 0xe1);

#endif /* __WINE_SHLGUID_H */



