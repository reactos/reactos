/* $Id: wcstombs.c,v 1.3 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/stdlib/wcstombs.c
 * PURPOSE:         converts a unicode string to a multi byte string
 */

#include <ddk/ntddk.h>
#include <stdlib.h>
#include <string.h>

/*
 * @implemented
 */
size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = wcslen (wcstr);

	if (mbstr == NULL)
	{
		RtlUnicodeToMultiByteSize (&Size,
		                           (wchar_t *)wcstr,
		                           Length * sizeof(WCHAR));

		return (size_t)Size;
	}

	Status = RtlUnicodeToMultiByteN (mbstr,
	                                 count,
	                                 &Size,
	                                 (wchar_t *)wcstr,
	                                 Length * sizeof(WCHAR));
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}

/* EOF */
