/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */
#ifndef __WINE_GDI_H
#define __WINE_GDI_H

#define NTM_PS_OPENTYPE     0x00020000 /* wingdi.h */
#define NTM_TT_OPENTYPE     0x00040000 /* wingdi.h */
#define NTM_TYPE1           0x00100000 /* wingdi.h */

#include_next <wingdi.h>

#endif /* __WINE_GDI_H */
