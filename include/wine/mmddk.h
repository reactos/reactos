/* $Id $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <mmddk.h>

#ifndef __WINE_MMDDK_H
#define __WINE_MMDDK_H

#define DRV_QUERYDSOUNDIFACE		(DRV_RESERVED + 20)
#define DRV_QUERYDSOUNDDESC		(DRV_RESERVED + 21)

#endif /* __WINE_MMDDK_H */
