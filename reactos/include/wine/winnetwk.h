/* $Id: winnetwk.h,v 1.2 2004/06/29 13:40:40 gvg Exp $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <winnetwk.h>

#ifndef __WINE_WINNETWK_H
#define __WINE_WINNETWK_H

/* WNetEnumCachedPasswords */
typedef struct tagPASSWORD_CACHE_ENTRY
{
	WORD cbEntry;
	WORD cbResource;
	WORD cbPassword;
	BYTE iEntry;
	BYTE nType;
	BYTE abResource[1];
} PASSWORD_CACHE_ENTRY;

typedef BOOL (CALLBACK *ENUMPASSWORDPROC)(PASSWORD_CACHE_ENTRY *, DWORD);
UINT WINAPI WNetEnumCachedPasswords( LPSTR, WORD, BYTE, ENUMPASSWORDPROC, DWORD);

#endif /* __WINE_WINNETWK_H */
