/* $Id: rtl.c,v 1.3 1999/11/20 21:47:38 ekohl Exp $
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
	ULONG Size;
	WCHAR UnicodeChar;

	Size = 1;
#if 0
	Size = (NlsLeadByteInfo[AnsiChar] == 0) ? 1 : 2;
#endif

	RtlMultiByteToUnicodeN (&UnicodeChar,
	                        sizeof(WCHAR),
	                        NULL,
	                        &AnsiChar,
	                        Size);

	return UnicodeChar;
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


NTSTATUS
STDCALL
RtlMultiByteToUnicodeN(PWCHAR UnicodeString,
                       ULONG  UnicodeSize,
                       PULONG ResultSize,
                       PCHAR  MbString,
                       ULONG  MbSize)
{
	ULONG Size = 0;
	ULONG i;

	if (NLS_MB_CODE_PAGE_TAG == FALSE)
	{
		/* single-byte code page */
		if (MbSize > (UnicodeSize / sizeof(WCHAR)))
			Size = UnicodeSize / sizeof(WCHAR);
		else
			Size = MbSize;

		if (ResultSize != NULL)
			*ResultSize = Size * sizeof(WCHAR);

		for (i = 0; i < Size; i++)
		{
			*UnicodeString = *MbString;
#if 0
			*UnicodeString = AnsiToUnicodeTable[*MbString];
#endif

			UnicodeString++;
			MbString++;
		}
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
{
	ULONG Size = 0;
	ULONG i;

	if (NLS_MB_CODE_PAGE_TAG == FALSE)
	{
		/* single-byte code page */
		if (UnicodeSize > (MbSize * sizeof(WCHAR)))
			Size = MbSize;
		else
			Size = UnicodeSize / sizeof(WCHAR);

		if (ResultSize != NULL)
			*ResultSize = Size;

		for (i = 0; i < Size; i++)
		{
			*MbString = *UnicodeString;
#if 0
			*MbString = UnicodeToAnsiTable[*UnicodeString];
#endif

			MbString++;
			UnicodeString++;
		}
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlUnicodeToMultiByteSize (
	PULONG	MbSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
{
	if (NLS_MB_CODE_PAGE_TAG == FALSE)
	{
		/* single-byte code page */
		*MbSize = UnicodeSize / sizeof (WCHAR);
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */
	}

	return STATUS_SUCCESS;
}


VOID
STDCALL
RtlUnwind (
	VOID
	)
{
}


WCHAR
STDCALL
RtlUpcaseUnicodeChar (
	WCHAR Source
	)
{
	if (Source < L'a')
		return Source;

	if (Source <= L'z')
		return (Source - (L'a' - L'A'));

	/* FIXME: characters above 'z' */

	return Source;
}


NTSTATUS
STDCALL
RtlUpcaseUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
{
	ULONG Size = 0;
	ULONG i;

	if (NLS_MB_CODE_PAGE_TAG == FALSE)
	{
		/* single-byte code page */
		if (UnicodeSize > (MbSize * sizeof(WCHAR)))
			Size = MbSize;
		else
			Size = UnicodeSize / sizeof(WCHAR);

		if (ResultSize != NULL)
			*ResultSize = Size;

		for (i = 0; i < Size; i++)
		{
			/* FIXME: Upcase!! */
			*MbString = *UnicodeString;
#if 0
			*MbString = UnicodeToAnsiTable[*UnicodeString];
#endif

			MbString++;
			UnicodeString++;
		}
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
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
