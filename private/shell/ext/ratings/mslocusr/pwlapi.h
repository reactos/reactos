/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/

/*
	pcache.h
	Definitions for password cache code

	FILE HISTORY:
		gregj	06/25/92	Created
		gregj	07/13/92	Finishing up lots more stuff, incl. classes
		gregj	04/23/93	Ported to Chicago environment
		gregj	09/16/93	Added memory-only entry support for Chicago
		gregj	11/30/95	Support for new file format
		gregj	08/13/96	Removed everything but MSPWL32 API definitions
*/

#ifndef _PWLAPI_H_
#define _PWLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char UCHAR;
typedef UINT APIERR;

#include <pcerr.h>

#ifndef _SIZE_T_DEFINED
# include <stddef.h>
#endif

#ifndef PCE_STRUCT_DEFINED

#define PCE_STRUCT_DEFINED		/* for benefit of pcache.h */

struct PASSWORD_CACHE_ENTRY {
	USHORT cbEntry;				/* size of this entry in bytes, incl. pad */
								/* high bit marks end of bucket */
	USHORT cbResource;			/* size of resource name in bytes */
	USHORT cbPassword;			/* size of password in bytes */
	UCHAR iEntry;				/* index number of this entry, for MRU */
	UCHAR nType;				/* type of entry (see below) */
	CHAR abResource[1];			/* resource name (may not be ASCIIZ at all) */
//	CHAR abPassword[cbPassword]; /* password (also may not be ASCIIZ) */
//	CHAR abPad[];				/* WORD padding */
};

typedef BOOL (*CACHECALLBACK)( struct PASSWORD_CACHE_ENTRY *pce, DWORD dwRefData );

#endif	/* PCE_STRUCT_DEFINED */


/*
    the following nType values are only for the purposes of enumerating
    entries from the cache.  note that PCE_ALL is reserved and should not
    be the nType value for any entry.
*/

// NOTE BENE!  All of the following MUST be synchronized with
//             \\flipper\wb\src\common\h\pcache.hxx!

#define PCE_DOMAIN		0x01	/* entry is for a domain */
#define PCE_SERVER		0x02	/* entry is for a server */
#define PCE_UNC			0x03	/* entry is for a server/share combo */
#define PCE_MAIL		0x04	/* entry is a mail password */
#define PCE_SECURITY	0x05	/* entry is a security entry */
#define PCE_MISC		0x06	/* entry is for some other resource */
#define PCE_NDDE_WFW	0x10	/* entry is WFW DDE password */
#define PCE_NDDE_NT		0x11	/* entry is NT DDE password */
#define PCE_NW_SERVER	0x12	/* entry is Netware server*/
#define PCE_PCONN		0x81	/* persistent connection */
#define PCE_DISKSHARE	0x82	/* persistent disk share */
#define PCE_PRINTSHARE	0x83	/* persistent print share */
#define PCE_DOSPRINTSHARE	0x84	/* persistent DOS print share */
#define	PCE_NW_PSERVER	0x85	/* for NetWare Print Server login (MSPSRV.EXE) */

#define PCE_NOTMRU	0x80		/* bit set if entry is exempt from MRU aging */
#define PCE_ALL		0xff		/* retrieve all entries */

#define MAX_ENTRY_SIZE	250	/* so total file size < 64K */

struct CACHE_ENTRY_INFO {
	USHORT cbResource;		/* size of resource name in bytes */
	USHORT cbPassword;		/* size of password in bytes */
	UCHAR iEntry;			/* index number of entry */
	UCHAR nType;			/* type of entry (see below) */
	USHORT dchResource;		/* offset in buffer to resource name */
	USHORT dchPassword;		/* offset in buffer to password */
};


/*
	Externally exposed API-like things.
*/

typedef LPVOID HPWL;
typedef HPWL *LPHPWL;

APIERR OpenPasswordCache(
	LPHPWL lphCache,
	const CHAR *pszUsername,
	const CHAR *pszPassword,
	BOOL fWritable );
APIERR ClosePasswordCache( HPWL hCache, BOOL fDiscardMemory );
APIERR CreatePasswordCache(
	LPHPWL lphCache,
	const CHAR *pszUsername,
	const CHAR *pszPassword );
APIERR DeletePasswordCache(const CHAR *pszUsername);
APIERR CheckCacheVersion( HPWL hCache, ULONG ulVersion );
APIERR LoadCacheImage( HPWL hCache );
APIERR MakeCacheDirty( HPWL hCache );
APIERR FindCacheResource(
	HPWL hCache,
	const CHAR *pbResource,
	WORD cbResource,
	CHAR *pbBuffer,
	WORD cbBuffer,
	UCHAR nType );
APIERR DeleteCacheResource(
	HPWL hCache,
	const CHAR *pbResource,
	WORD cbResource,
	UCHAR nType );
APIERR AddCacheResource(
	HPWL hCache,
	const CHAR *pbResource,
	WORD cbResource,
	const CHAR *pbPassword,
	WORD cbPassword,
	UCHAR nType,
	UINT fnFlags );
#define PCE_MEMORYONLY		0x01

APIERR EnumCacheResources(
	HPWL hCache,
	const CHAR *pbPrefix,
	WORD cbPrefix,
	UCHAR nType,
	CACHECALLBACK pfnCallback,
	DWORD dwRefData );
APIERR UpdateCacheMRU(
	HPWL hCache,
	const struct CACHE_ENTRY_INFO *pce );
APIERR SetCachePassword(
	HPWL hCache,
	const CHAR *pszNewPassword );
APIERR GetCacheFileName(
	const CHAR *pszUsername,
	CHAR *pszFilename,
	UINT cbFilename );

#ifdef __cplusplus
}	/* extern "C" */
#endif


#endif	/* _PWLAPI_H_ */
