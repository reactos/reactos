/* $Id: sysinfo.c,v 1.4 2000/03/26 19:38:18 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sysinfo.c
 * PURPOSE:         System information functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/zwtypes.h>
#include <string.h>
#include <internal/ex.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtQuerySystemEnvironmentValue (
	IN	PUNICODE_STRING	Name,
	OUT	PVOID		Value,
	IN	ULONG		Length,
	IN OUT	PULONG		ReturnLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetSystemEnvironmentValue (
	IN	PUNICODE_STRING	VariableName,
	IN	PUNICODE_STRING	Value
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQuerySystemInformation (
	IN	CINT	SystemInformationClass,
	OUT	PVOID	SystemInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	)
{
	/*
	 * If called from user mode, check 
	 * possible unsafe arguments.
	 */
#if 0
        if (KernelMode != KeGetPreviousMode())
        {
		// Check arguments
		//ProbeForWrite(
		//	SystemInformation,
		//	Length
		//	);
		//ProbeForWrite(
		//	ResultLength,
		//	sizeof (ULONG)
		//	);
        }
#endif
	/*
	 * Clear the user buffer.
	 */
	memset(
		SystemInformation,
		0,
		Length
		);
	/*
	 * Check the request is valid.
	 */
	switch (SystemInformationClass)
	{
#if 0
        /*---*/
		case SystemPerformanceInformation:
			/*
			 * Check user buffer's size
			 */
                        if (Length < sizeof())
                        {
                                *ResultLength = 
                                return STATUS_INFO_LENGTH_MISMATCH;
                        }
			/* FIXME */
			return STATUS_SUCCESS;
	/*---*/		
		case SystemDriverInformation:
			/* Check user buffer's size */
			if (Length < sizeof (SYSTEM_DRIVER_INFO))
			{
				*ResultLength = sizeof (SYSTEM_DRIVER_INFO);
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			/* FIXME: */
			return STATUS_SUCCESS;
	/*---*/
		case SystemCacheInformation:
			/* Check user buffer's size */
                        if (Length < sizeof (SYSTEM_CACHE_INFORMATION))
			{
                                *ResultLength = sizeof (SYSTEM_CACHE_INFORMATION);
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			return STATUS_SUCCESS;
	/*---*/
		case SystemTimeAdjustmentInformation:
			/*
			 * Check user buffer's size
			 */
			if (Length < sizeof (SYSTEM_TIME_ADJUSTMENT))
			{
				*ResultLength = sizeof (SYSTEM_TIME_ADJUSTMENT);
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			/* FIXME: */
			return STATUS_SUCCESS;
	/*---*/
                case SystemConfigurationInformation:
		{
                        PSYSTEM_CONFIGURATION_INFO Sci 
                                = (PSYSTEM_CONFIGURATION_INFO) SystemInformation;

                        *ResultLength = sizeof (SYSTEM_CONFIGUTATION_INFO);
			/*
			 * Check user buffer's size 
			 */
                        if (Length < sizeof (SYSTEM_CONFIGURATION_INFO))
			{
				return STATUS_INFO_LENGTH_MISMATCH;
			}
			/*
			 * Fill the object with config data.
			 * FIXME: some data should come from the
			 * registry.
			 */
                        Sci->tag2.tag1.ProcessorAchitecture
				= 80586;
                        Sci->tag2.tag1.Reserved
				= 0x00000000;
                        Sci->PageSize
				= 4096;
			return STATUS_SUCCESS;
		}
#endif

                case SystemTimeZoneInformation: /* 44 */
                        *ResultLength = sizeof (TIME_ZONE_INFORMATION);

			/*
			 * Check user buffer's size 
			 */
                        if (Length < sizeof (TIME_ZONE_INFORMATION))
			{
				return STATUS_INFO_LENGTH_MISMATCH;
			}

                        /* Copy the time zone information struct */
                        memcpy (SystemInformation,
                                &SystemTimeZoneInfo,
                                sizeof (TIME_ZONE_INFORMATION));

			return STATUS_SUCCESS;

	}
	return STATUS_INVALID_INFO_CLASS;
}


NTSTATUS
STDCALL
NtSetSystemInformation (
	IN	CINT	SystemInformationClass,
	IN	PVOID	SystemInformation,
	IN	ULONG	SystemInformationLength
	)
{
	/*
	 * If called from user mode, check 
	 * possible unsafe arguments.
	 */
#if 0
        if (KernelMode != KeGetPreviousMode())
        {
		// Check arguments
		//ProbeForWrite(
		//	SystemInformation,
		//	Length
		//	);
		//ProbeForWrite(
		//	ResultLength,
		//	sizeof (ULONG)
		//	);
        }
#endif

	/*
	 * Check the request is valid.
	 */
	switch (SystemInformationClass)
	{


                case SystemTimeZoneInformation: /* 44 */
			/*
			 * Check user buffer's size 
			 */
                        if (SystemInformationLength < sizeof (TIME_ZONE_INFORMATION))
			{
				return STATUS_INFO_LENGTH_MISMATCH;
			}

                        /* Copy the time zone information struct */
                        memcpy (&SystemTimeZoneInfo,
                                &SystemInformation,
                                sizeof (TIME_ZONE_INFORMATION));

			return STATUS_SUCCESS;
	}

	return STATUS_INVALID_INFO_CLASS;
}


NTSTATUS
STDCALL
NtFlushInstructionCache (
	IN	HANDLE	ProcessHandle,
	IN	PVOID	BaseAddress,
	IN	UINT	NumberOfBytesToFlush
	)
{
	UNIMPLEMENTED;
}


/* EOF */
