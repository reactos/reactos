/* $Id: rtl.c,v 1.1 1999/09/12 21:54:16 ea Exp $
 *
 * reactos/lib/psxdll/misc/rtl.c
 *
 * ReactOS Operating System
 *
 * NOTE: These functions should be imported automatically by
 * the linker.
 *
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <wchar.h>

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar (
	CHAR	AnsiChar
	)
{
	/* FIXME: it should probably call RtlMultiByteToUnicodeN
	 * with length==1.
	 */
	return (WCHAR) AnsiChar;
}


VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	)
{
	register PCHAR	w = (PCHAR) Destination;
	register UCHAR	f = Fill;
	register ULONG	n = Length;

	while (n--) *w++ = f;
}


VOID
STDCALL
RtlMoveMemory (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	)
{
	register PCHAR	w = (PCHAR) Destination;
	register PCHAR	r = (PCHAR) Source;
	register ULONG	n = Length;

	while (n--) *w++ = *r++;
}


VOID
STDCALL
RtlMultiByteToUnicodeN (
	VOID
	)
{
}


VOID
STDCALL
RtlUnicodeToMultiByteN (
	VOID
	)
{
}


ULONG
STDCALL
RtlUnicodeToMultiByteSize (
	VOID
	)
{
	return 0;
}


VOID
STDCALL
RtlUnwind (
	VOID
	)
{
}


VOID
STDCALL
RtlUpcaseUnicodeChar (
	VOID
	)
{
}


VOID
STDCALL
RtlUpcaseUnicodeToMultiByteN (
	VOID
	)
{
}




VOID
STDCALL
RtlZeroMemory (
	PVOID	Destination,
	ULONG	Length
	)
{
	register PCHAR	z = (PCHAR) Destination;
	register UINT	n = Length;

	while (n--) *z++ = (CHAR) 0;
}


/* EOF */
