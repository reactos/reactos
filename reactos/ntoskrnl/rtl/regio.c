/* $Id: regio.c,v 1.6 2003/07/11 01:23:16 royce Exp $
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

/*
 * @implemented
 */
USHORT
STDCALL
READ_REGISTER_USHORT (
	PUSHORT	Register
	)
{
	return *Register;
}

/*
 * @implemented
 */
ULONG
STDCALL
READ_REGISTER_ULONG (
	PULONG	Register
	)
{
	return *Register;
}

/*
 * @implemented
 */
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

/*
 * @implemented
 */
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

/*
 * @implemented
 */
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

/*
 * @implemented
 */
VOID
STDCALL
WRITE_REGISTER_UCHAR (
	PUCHAR	Register,
	UCHAR	Value
	)
{
	*Register = Value;
}

/*
 * @implemented
 */
VOID
STDCALL
WRITE_REGISTER_USHORT (
	PUSHORT	Register,
	USHORT	Value
	)
{
	*Register = Value;
}

/*
 * @implemented
 */
VOID
STDCALL
WRITE_REGISTER_ULONG (
	PULONG	Register,
	ULONG	Value
	)
{
	*Register = Value;
}

/*
 * @implemented
 */
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

/*
 * @implemented
 */
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

/*
 * @implemented
 */
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
