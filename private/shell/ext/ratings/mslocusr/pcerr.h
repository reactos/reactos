/*****************************************************************/
/**				   Microsoft Windows for Workgroups				**/
/**				Copyright (C) Microsoft Corp., 1991-1992		**/
/*****************************************************************/

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/*
	Password cache error codes.
*/

#define IERR_PCACHE_BASE		7200
#define IERR_CachingDisabled	(IERR_PCACHE_BASE + 0)
#define IERR_BadSig				(IERR_PCACHE_BASE + 1)
#define IERR_VersionChanged		(IERR_PCACHE_BASE + 2)
#define IERR_CacheNotLoaded		(IERR_PCACHE_BASE + 3)
#define IERR_CacheEntryNotFound	(IERR_PCACHE_BASE + 4)
#define IERR_CacheReadOnly		(IERR_PCACHE_BASE + 5)
#define IERR_IncorrectUsername	(IERR_PCACHE_BASE + 6)
#define IERR_CacheCorrupt		(IERR_PCACHE_BASE + 7)
#define IERR_EntryTooLarge		(IERR_PCACHE_BASE + 8)
#define IERR_CacheEnumCancelled	(IERR_PCACHE_BASE + 9)
#define IERR_UsernameNotFound	(IERR_PCACHE_BASE + 10)
#define IERR_CacheFull			(IERR_PCACHE_BASE + 11)	/* only if cache would exceed 64K */
#define IERR_CacheAlreadyOpen	(IERR_PCACHE_BASE + 12)
#define IERR_CantCreateUniqueFile	(IERR_PCACHE_BASE + 13)
#define IERR_InvalidParameter	(IERR_PCACHE_BASE + 14)

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif

