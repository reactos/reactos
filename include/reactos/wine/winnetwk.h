/* $Id: winnetwk.h 20909 2006-01-15 22:25:16Z gvg $
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
DWORD WINAPI WNetCachePassword( LPSTR, WORD, LPSTR, WORD, BYTE, WORD );
UINT WINAPI WNetEnumCachedPasswords( LPSTR, WORD, BYTE, ENUMPASSWORDPROC, DWORD);
DWORD WINAPI WNetGetCachedPassword( LPSTR, WORD, LPSTR, LPWORD, BYTE );

#endif /* __WINE_WINNETWK_H */
