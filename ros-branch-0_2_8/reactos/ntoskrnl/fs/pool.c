/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/pool.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */

#include <ntoskrnl.h>


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
 * @implemented
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
 *
 * @implemented
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
 * @implemented
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
 * @implemented
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
