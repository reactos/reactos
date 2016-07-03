/* 
 * FFS File System Driver for Windows
 *
 * misc.c
 *
 * 2004.5.6 ~
 *
 * Lee Jae-Hong, http://www.pyrasis.com
 *
 * See License.txt
 *
 */

#include "ntifs.h"
#include "ffsdrv.h"

/* Globals */

extern PFFS_GLOBAL FFSGlobal;


/* Definitions */

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSLog2)
#pragma alloc_text(PAGE, FFSSysTime)
#pragma alloc_text(PAGE, FFSInodeTime)
#pragma alloc_text(PAGE, FFSOEMToUnicode)
#pragma alloc_text(PAGE, FFSUnicodeToOEM)
#endif

ULONG
FFSLog2(
	ULONG Value)
{
	ULONG Order = 0;

    PAGED_CODE();

	ASSERT(Value > 0);

	while (Value)
	{
		Order++;
		Value >>= 1;
	}

	return (Order - 1);
}


LARGE_INTEGER
FFSSysTime(
	IN ULONG i_time)
{
	LARGE_INTEGER SysTime;

    PAGED_CODE();

	RtlSecondsSince1970ToTime(i_time, &SysTime);

	return SysTime;
}

ULONG
FFSInodeTime(
	IN LARGE_INTEGER SysTime)
{
	ULONG   FFSTime;

    PAGED_CODE();

	RtlTimeToSecondsSince1970(&SysTime, &FFSTime);

	return FFSTime;
}


NTSTATUS
FFSOEMToUnicode(
	IN OUT PUNICODE_STRING Unicode,
	IN     POEM_STRING     Oem)
{
	NTSTATUS  Status;

    PAGED_CODE();

	Status = RtlOemStringToUnicodeString(
			Unicode, 
			Oem,
			FALSE);

	if (!NT_SUCCESS(Status))
	{
		FFSBreakPoint();
		goto errorout;
	}

errorout:

	return Status;
}


NTSTATUS
FFSUnicodeToOEM(
	IN OUT POEM_STRING Oem,
	IN PUNICODE_STRING Unicode)
{
	NTSTATUS Status;

    PAGED_CODE();

	Status = RtlUnicodeStringToOemString(
			Oem,
			Unicode,
			FALSE);

	if (!NT_SUCCESS(Status))
	{
		FFSBreakPoint();
		goto errorout;
	}

errorout:

	return Status;
}
