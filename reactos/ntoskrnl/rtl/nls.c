/* $Id: nls.c,v 1.1 1999/11/15 15:57:01 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/nls.c
 * PURPOSE:         National Language Support (NLS) functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Emanuele Aliberti
 *                  10/11/99 Added translation functions.
 *
 * NOTE:
 *   Multi-byte code pages are not supported yet. Even single-byte code
 *   pages are not supported properly. Only stupid CHAR->WCHAR and
 *   WCHAR->CHAR (Attention: data loss!!!) translation is done.
 *
 * TODO:
 *   1) Implement code to initialize the translation tables.
 *   2) Use fixed translation table for translation.
 *   3) Add loading of translation tables (NLS files).
 *   4) Add multi-byte translation code.
 */

#include <ddk/ntddk.h>
//#include <internal/nls.h>


BOOLEAN
NlsMbCodePageTag = FALSE;

BOOLEAN
NlsMbOemCodePageTag = FALSE;

BYTE
NlsLeadByteInfo = 0; /* ? */

USHORT
NlsOemLeadByteInfo = 0;

USHORT
NlsAnsiCodePage = 0;

USHORT
NlsOemCodePage = 0; /* not exported */


#if 0
WCHAR AnsiToUnicodeTable[256];
WCHAR OemToUnicodeTable[256];

CHAR UnicodeToAnsiTable [65536];
CHAR UnicodeToOemTable [65536];
#endif


/* FUNCTIONS *****************************************************************/

VOID
STDCALL
RtlGetDefaultCodePage (PUSHORT AnsiCodePage,
                       PUSHORT OemCodePage)
{
	*AnsiCodePage = NlsAnsiCodePage;
	*OemCodePage = NlsOemCodePage;
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

	if (NlsMbCodePageTag == FALSE)
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
		};
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
RtlMultiByteToUnicodeSize(PULONG UnicodeSize,
                          PCHAR MbString,
                          ULONG MbSize)
{
	if (NlsMbCodePageTag == FALSE)
	{
		/* single-byte code page */
		*UnicodeSize = MbSize * sizeof (WCHAR);
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
RtlOemToUnicodeN(PWCHAR UnicodeString,
                 ULONG  UnicodeSize,
                 PULONG ResultSize,
                 PCHAR  OemString,
                 ULONG  OemSize)
{
	ULONG Size = 0;
	ULONG i;

	if (NlsMbOemCodePageTag == FALSE)
	{
		/* single-byte code page */
		if (OemSize > (UnicodeSize / sizeof(WCHAR)))
			Size = UnicodeSize / sizeof(WCHAR);
		else
			Size = OemSize;

		if (ResultSize != NULL)
			*ResultSize = Size * sizeof(WCHAR);

		for (i = 0; i < Size; i++)
		{
			*UnicodeString = *OemString;
#if 0
			*UnicodeString = OemToUnicodeTable[*OemString];
#endif

			UnicodeString++;
			OemString++;
		};
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
RtlUnicodeToMultiByteN(PCHAR  MbString,
                       ULONG  MbSize,
                       PULONG ResultSize,
                       PWCHAR UnicodeString,
                       ULONG  UnicodeSize)
{
	ULONG Size = 0;
	ULONG i;

	if (NlsMbCodePageTag == FALSE)
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
		};
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
RtlUnicodeToMultiByteSize(PULONG MbSize,
                          PWCHAR UnicodeString,
                          ULONG UnicodeSize)
{
	if (NlsMbCodePageTag == FALSE)
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


NTSTATUS
STDCALL
RtlUnicodeToOemN(PCHAR  OemString,
                 ULONG  OemSize,
                 PULONG ResultSize,
                 PWCHAR UnicodeString,
                 ULONG  UnicodeSize)
{
	ULONG Size = 0;
	ULONG i;

	if (NlsMbOemCodePageTag == FALSE)
	{
		/* single-byte code page */
		if (UnicodeSize > (OemSize * sizeof(WCHAR)))
			Size = OemSize;
		else
			Size = UnicodeSize / sizeof(WCHAR);

		if (ResultSize != NULL)
			*ResultSize = Size;

		for (i = 0; i < Size; i++)
		{
			*OemString = *UnicodeString;
#if 0
			*OemString = UnicodeToOemTable[*UnicodeString];
#endif

			OemString++;
			UnicodeString++;
		};
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
}


/* EOF */
