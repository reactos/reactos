/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#ifndef __WINE_DLGS_H
#define __WINE_DLGS_H

#include_next <dlgs.h>

#define NEWFILEOPENORD 1547

#endif /* __WINE_DLGS_H */
