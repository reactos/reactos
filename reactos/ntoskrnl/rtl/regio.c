/* $Id: regio.c,v 1.1 1999/12/29 01:35:53 ekohl Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/rtl/regio.c
 * PURPOSE:              Register io functions
 * PROGRAMMER:           Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * REVISION HISTORY:
 *                       29/12/1999 Created
 */

#include <ddk/ntddk.h>


/* FUNCTIONS ***************************************************************/

UCHAR
READ_REGISTER_UCHAR (
	PUCHAR	Register
	)
{
	return *Register;
}

USHORT
READ_REGISTER_USHORT (
	PUSHORT	Register
	)
{
	return *Register;
}

ULONG
READ_REGISTER_ULONG (
	PULONG	Register
	)
{
	return *Register;
}

VOID
READ_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

VOID
READ_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

VOID
READ_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
	)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

VOID
WRITE_REGISTER_UCHAR (
	PUCHAR	Register,
	UCHAR	Value
	)
{
	*Register = Value;
}

VOID
WRITE_REGISTER_USHORT (
	PUSHORT	Register,
	USHORT	Value
	)
{
	*Register = Value;
}

VOID
WRITE_REGISTER_ULONG (
	PULONG	Register,
	ULONG	Value
	)
{
	*Register = Value;
}

VOID
WRITE_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

VOID
WRITE_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

VOID
WRITE_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
)
{
	while (Count--)
	{
		*Buffer++  = *Register++;
	}
}

/* EOF */