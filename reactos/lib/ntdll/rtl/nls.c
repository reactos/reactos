/* $Id: nls.c,v 1.5 2002/09/07 15:12:40 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/nls.c
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
 *   4) Implement unicode upcase and downcase handling.
 *   5) Add multi-byte translation code.
 */

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>


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
PWCHAR NlsAnsiToUnicodeTable = NULL;
PWCHAR NlsOemToUnicodeTable = NULL;

PCHAR NlsUnicodeToAnsiTable = NULL;
PCHAR NlsUnicodeToOemTable = NULL;

PWCHAR NlsUnicodeUpcaseTable = NULL;
PWCHAR NlsUnicodeLowercaseTable = NULL;
#endif


/* FUNCTIONS *****************************************************************/

/*
 * Missing functions:
 *   RtlInitCodePageTable
 *   RtlInitNlsTables
 *   RtlResetRtlTranslations
 */

/*
 * RtlConsoleMultiByteToUnicodeN@24
 */

NTSTATUS
STDCALL
RtlCustomCPToUnicodeN (
	PRTL_NLS_DATA	NlsData,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize,
	PULONG		ResultSize,
	PCHAR		CustomString,
	ULONG		CustomSize)
{
	ULONG Size = 0;
	ULONG i;

	if (NlsData->DbcsFlag == FALSE)
	{
		/* single-byte code page */
		if (CustomSize > (UnicodeSize / sizeof(WCHAR)))
			Size = UnicodeSize / sizeof(WCHAR);
		else
			Size = CustomSize;

		if (ResultSize != NULL)
			*ResultSize = Size * sizeof(WCHAR);

		for (i = 0; i < Size; i++)
		{
			*UnicodeString = NlsData->MultiByteToUnicode[(int)*CustomString];
			UnicodeString++;
			CustomString++;
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
RtlGetDefaultCodePage (
	PUSHORT	AnsiCodePage,
	PUSHORT	OemCodePage
	)
{
	*AnsiCodePage = NlsAnsiCodePage;
	*OemCodePage = NlsOemCodePage;
}


NTSTATUS
STDCALL
RtlMultiByteToUnicodeN (
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize,
	PULONG	ResultSize,
	PCHAR	MbString,
	ULONG	MbSize
	)
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
			*UnicodeString = NlsAnsiToUnicodeTable[*MbString];
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
RtlMultiByteToUnicodeSize (
	PULONG	UnicodeSize,
	PCHAR	MbString,
	ULONG	MbSize
	)
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
RtlOemToUnicodeN (
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize,
	PULONG	ResultSize,
	PCHAR	OemString,
	ULONG	OemSize
	)
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
			*UnicodeString = NlsOemToUnicodeTable[*OemString];
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
RtlUnicodeToCustomCPN (
	PRTL_NLS_DATA	NlsData,
	PCHAR		CustomString,
	ULONG		CustomSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
	)
{
	ULONG Size = 0;
	ULONG i;

	if (NlsData->DbcsFlag == 0)
	{
		/* single-byte code page */
		if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
			Size = CustomSize;
		else
			Size = UnicodeSize / sizeof(WCHAR);

		if (ResultSize != NULL)
			*ResultSize = Size;

		for (i = 0; i < Size; i++)
		{
			*CustomString = NlsData->UnicodeToMultiByte[*UnicodeString];
			CustomString++;
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
RtlUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
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


NTSTATUS
STDCALL
RtlUpcaseUnicodeToCustomCPN (
	PRTL_NLS_DATA	NlsData,
	PCHAR		CustomString,
	ULONG		CustomSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
	)
{
	WCHAR UpcaseChar;
	ULONG Size = 0;
	ULONG i;

	if (NlsData->DbcsFlag == 0)
	{
		/* single-byte code page */
		if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
			Size = CustomSize;
		else
			Size = UnicodeSize / sizeof(WCHAR);

		if (ResultSize != NULL)
			*ResultSize = Size;

		for (i = 0; i < Size; i++)
		{
			*CustomString = NlsData->UnicodeToMultiByte[*UnicodeString];
#if 0
			UpcaseChar = NlsUnicodeUpcaseTable[*UnicodeString];
			*CustomString = NlsData->UnicodeToMultiByte[UpcaseChar];
#endif
			CustomString++;
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
RtlUpcaseUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
{
	WCHAR UpcaseChar;
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
			UpcaseChar = NlsUnicodeUpcaseTable[*UnicodeString];
			*MbString = NlsUnicodeToAnsiTable[UpcaseChar];
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
RtlUpcaseUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	)
{
	WCHAR UpcaseChar;
	ULONG Size = 0;
	ULONG i;

	if (NLS_MB_OEM_CODE_PAGE_TAG == FALSE)
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
			UpcaseChar = NlsUnicodeUpcaseTable[*UnicodeString];
			*OemString = UnicodeToOemTable[UpcaseChar];
#endif

			OemString++;
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

/* EOF */
