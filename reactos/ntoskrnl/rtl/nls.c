/* $Id: nls.c,v 1.5 2001/06/29 20:42:47 ekohl Exp $
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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN NlsMbCodePageTag = FALSE;
BOOLEAN NlsMbOemCodePageTag = FALSE;

BYTE NlsLeadByteInfo = 0; /* ? */

USHORT NlsOemLeadByteInfo = 0;

USHORT NlsAnsiCodePage = 0;
USHORT NlsOemCodePage = 0; /* not exported */


#if 0
WCHAR AnsiToUnicodeTable[256];
WCHAR OemToUnicodeTable[256];

CHAR UnicodeToAnsiTable [65536];
CHAR UnicodeToOemTable [65536];
#endif


/* FUNCTIONS *****************************************************************/

NTSTATUS
RtlpInitNlsSections(ULONG Mod1Start,
		    ULONG Mod1End,
		    ULONG Mod2Start,
		    ULONG Mod2End,
		    ULONG Mod3Start,
		    ULONG Mod3End)
{
  UNICODE_STRING UnicodeString;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE DirectoryHandle;
  HANDLE SectionHandle;
  NTSTATUS Status;
  LARGE_INTEGER SectionSize;

  DPRINT("Ansi section start: 0x%08lX\n", Mod1Start);
  DPRINT("Ansi section end: 0x%08lX\n", Mod1End);
  DPRINT("Oem section start: 0x%08lX\n", Mod2Start);
  DPRINT("Oem section end: 0x%08lX\n", Mod2End);
  DPRINT("Upcase section start: 0x%08lX\n", Mod3Start);
  DPRINT("Upcase section end: 0x%08lX\n", Mod3End);

  /* Create the '\NLS' directory */
  RtlInitUnicodeString(&UnicodeString,
		       L"\\NLS");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     OBJ_PERMANENT,
			     NULL,
			     NULL);
  Status = NtCreateDirectoryObject(&DirectoryHandle,
				   0,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Create the 'NlsSectionUnicode' section */
  RtlInitUnicodeString(&UnicodeString,
		       L"NlsSectionUnicode");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     OBJ_PERMANENT,
			     DirectoryHandle,
			     NULL);
  SectionSize.QuadPart = (Mod1End - Mod1Start) +
    (Mod2End - Mod2Start) + (Mod3End - Mod3Start);
  DPRINT("NlsSectionUnicode size: 0x%I64X\n", SectionSize.QuadPart);

  Status = NtCreateSection(&SectionHandle,
			   SECTION_ALL_ACCESS,
			   &ObjectAttributes,
			   &SectionSize,
			   PAGE_READWRITE,
			   0,
			   NULL);
  if (!NT_SUCCESS(Status))
    return(Status);


  /* create and initialize code page table */

  /* map the nls table into the 'NlsSectionUnicode' section */


  NtClose(SectionHandle);
  NtClose(DirectoryHandle);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlCustomCPToUnicodeN(PRTL_NLS_DATA NlsData,
		      PWCHAR UnicodeString,
		      ULONG UnicodeSize,
		      PULONG ResultSize,
		      PCHAR CustomString,
		      ULONG CustomSize)
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


VOID STDCALL
RtlGetDefaultCodePage(PUSHORT AnsiCodePage,
		      PUSHORT OemCodePage)
{
   *AnsiCodePage = NlsAnsiCodePage;
   *OemCodePage = NlsOemCodePage;
}


NTSTATUS STDCALL
RtlMultiByteToUnicodeN(PWCHAR UnicodeString,
		       ULONG UnicodeSize,
		       PULONG ResultSize,
		       PCHAR MbString,
		       ULONG MbSize)
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
		}
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
}


NTSTATUS STDCALL
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
RtlOemToUnicodeN (
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize,
	PULONG	ResultSize,
	PCHAR	OemString,
	ULONG	OemSize)
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
	ULONG	UnicodeSize)
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


NTSTATUS STDCALL
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


NTSTATUS STDCALL
RtlUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize)
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
		}
	}
	else
	{
		/* multi-byte code page */
		/* FIXME */

	}

	return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUpcaseUnicodeToCustomCPN (
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
			/* FIXME: Upcase!! */
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


NTSTATUS STDCALL
RtlUpcaseUnicodeToMultiByteN(PCHAR MbString,
			     ULONG MbSize,
			     PULONG ResultSize,
			     PWCHAR UnicodeString,
			     ULONG UnicodeSize)
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
			/* FIXME: Upcase !! */
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


NTSTATUS STDCALL
RtlUpcaseUnicodeToOemN(PCHAR OemString,
		       ULONG OemSize,
		       PULONG ResultSize,
		       PWCHAR UnicodeString,
		       ULONG UnicodeSize)
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
			/* FIXME: Upcase !! */
			*OemString = *UnicodeString;
#if 0
			*OemString = UnicodeToOemTable[*UnicodeString];
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
