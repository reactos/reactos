/* $Id: propvar.c,v 1.4 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/propvar.c
 * PURPOSE:         CSRSS properties and variants API
 */
#define NTOS_MODE_USER
#include <ntos.h>

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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
