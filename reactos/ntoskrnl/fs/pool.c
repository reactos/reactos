/* $Id: pool.c,v 1.4 2002/09/08 10:23:20 chorns Exp $
 *
 * reactos/ntoskrnl/fs/pool.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>
#include <internal/ifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocatePool@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	IFS_POOL_TAG is "FSrt" in mem view.
 *
 */
PVOID
STDCALL
FsRtlAllocatePool (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	)
{
	PVOID	Address;

	Address = ExAllocatePoolWithTag (
			PoolType,
			NumberOfBytes,
			IFS_POOL_TAG
			);
	if (NULL == Address)
	{
		ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
	}
	return Address;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocatePoolWithQuota@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	IFS_POOL_TAG is "FSrt" in mem view.
 */
PVOID
STDCALL
FsRtlAllocatePoolWithQuota (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	)
{
	PVOID	Address;

	Address = ExAllocatePoolWithQuotaTag (
			PoolType,
			NumberOfBytes,
			IFS_POOL_TAG
			);
	if (NULL == Address)
	{
		ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
	}
	return Address;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocatePoolWithQuotaTag@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
PVOID
STDCALL
FsRtlAllocatePoolWithQuotaTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	)
{
	PVOID	Address;

	Address = ExAllocatePoolWithQuotaTag (
			PoolType,
			NumberOfBytes,
			Tag
			);
	if (NULL == Address)
	{
		ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
	}
	return Address;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocatePoolWithTag@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
PVOID
STDCALL
FsRtlAllocatePoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	)
{
	PVOID	Address;

	Address = ExAllocatePoolWithTag (
			PoolType,
			NumberOfBytes,
			Tag
			);
	if (NULL == Address)
	{
		ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
	}
	return Address;
}



/* EOF */
