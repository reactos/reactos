/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#ifndef __WINE_DLGS_H
#define __WINE_DLGS_H

#define OFN_DONTADDTORECENT          0x02000000
#define OFN_ENABLEINCLUDENOTIFY      0x00400000
#define NEWFILEOPENORD 			 1547

#include_next <dlgs.h>

#endif /* __WINE_DLGS_H */
