/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#ifndef __WINE_SETUPAPI_H
#define __WINE_SETUPAPI_H

#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include_next <setupapi.h>
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __declspec(dllimport)

#define FLG_ADDREG_DELREG_BIT             0x00008000
#define FLG_DELREG_KEYONLY_COMMON        FLG_ADDREG_KEYONLY_COMMON
#define FLG_DELREG_MULTI_SZ_DELSTRING    (FLG_DELREG_TYPE_MULTI_SZ | FLG_ADDREG_DELREG_BIT | 0x00000002)
#define FLG_DELREG_TYPE_MULTI_SZ         FLG_ADDREG_TYPE_MULTI_SZ
#define FLG_ADDREG_KEYONLY_COMMON         0x00002000
#define FLG_ADDREG_DELREG_BIT             0x00008000
#define SPINST_COPYINF                    0x00000200

#endif /* __WINE_SETUPAPI_H */
