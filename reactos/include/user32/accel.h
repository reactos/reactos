/* $Id: accel.h,v 1.1 2003/07/20 03:45:31 hyperion Exp $
*/

#ifndef __USER32_ACCEL_H_INCLUDED__
#define __USER32_ACCEL_H_INCLUDED__

/* RT_ACCELERATOR resources are arrays of RES_ACCEL structures */
typedef struct _RES_ACCEL
{
 WORD fVirt;
 WORD key;
 DWORD cmd;
}
RES_ACCEL;

/* ACCELERATOR TABLES CACHE */
/* Cache entry */
typedef struct _USER_ACCEL_CACHE_ENTRY
{
 struct _USER_ACCEL_CACHE_ENTRY * Next;
 ULONG_PTR Usage; /* how many times the table has been loaded */
 HACCEL Object;   /* handle to the NtUser accelerator table object */
 HGLOBAL Data;    /* base address of the resource data */
}
U32_ACCEL_CACHE_ENTRY;

/* Lock guarding the cache */
extern CRITICAL_SECTION U32AccelCacheLock;

/* Cache */
extern U32_ACCEL_CACHE_ENTRY * U32AccelCache;

extern U32_ACCEL_CACHE_ENTRY ** WINAPI U32AccelCacheFind(HANDLE, HGLOBAL);
extern void WINAPI U32AccelCacheAdd(HACCEL, HGLOBAL);

#endif

/* EOF */
