/* $Id: regio.c,v 1.5 2002/09/08 10:23:42 chorns Exp $
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
STDCALL
READ_REGISTER_UCHAR (
	PUCHAR	Register
	)
{
	return *Register;
}

USHORT
STDCALL
READ_REGISTER_USHORT (
	PUSHORT	Register
	)
{
	return *Register;
}

ULONG
STDCALL
READ_REGISTER_ULONG (
	PULONG	Register
	)
{
	return *Register;
}

VOID
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
WRITE_REGISTER_UCHAR (
	PUCHAR	Register,
	UCHAR	Value
	)
{
	*Register = Value;
}

VOID
STDCALL
WRITE_REGISTER_USHORT (
	PUSHORT	Register,
	USHORT	Value
	)
{
	*Register = Value;
}

VOID
STDCALL
WRITE_REGISTER_ULONG (
	PULONG	Register,
	ULONG	Value
	)
{
	*Register = Value;
}

VOID
STDCALL
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
STDCALL
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
STDCALL
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
