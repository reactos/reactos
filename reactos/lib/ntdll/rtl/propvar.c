/* $Id: propvar.c,v 1.1 2001/06/17 20:05:10 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/propvar.c
 * PURPOSE:         CSRSS properties and variants API
 */
#define NTOS_MODE_USER
#include <ntos.h>

NTSTATUS
STDCALL
PropertyLengthAsVariant (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}

BOOLEAN
STDCALL
RtlCompareVariants (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return (FALSE);
}

BOOLEAN
STDCALL
RtlConvertPropertyToVariant (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return (FALSE);
}

NTSTATUS
STDCALL
RtlConvertVariantToProperty (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}

	
/* EOF */
