/* $Id: nls.c,v 1.11 2003/07/09 10:40:50 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/nls.c
 * PURPOSE:         National Language Support (NLS) functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Emanuele Aliberti
 *                  10/11/99 Added translation functions.
 *
 * TODO:
 *   1) Add multi-byte translation code.
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

USHORT NlsAnsiCodePage = 0; /* exported */
BOOLEAN NlsMbCodePageTag = FALSE; /* exported */
PWCHAR NlsAnsiToUnicodeTable = NULL;
PCHAR NlsUnicodeToAnsiTable = NULL;
PUSHORT NlsLeadByteInfo = NULL;


USHORT NlsOemCodePage = 0;
BOOLEAN NlsMbOemCodePageTag = FALSE; /* exported */
PWCHAR NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable = NULL;
PUSHORT NlsOemLeadByteInfo = NULL;


PUSHORT NlsUnicodeUpcaseTable = NULL;
PUSHORT NlsUnicodeLowercaseTable = NULL;


/* FUNCTIONS *****************************************************************/

/*
 * RtlConsoleMultiByteToUnicodeN@24
 */


NTSTATUS STDCALL
RtlCustomCPToUnicodeN(IN PCPTABLEINFO CustomCP,
		      PWCHAR UnicodeString,
		      ULONG UnicodeSize,
		      PULONG ResultSize,
		      PCHAR CustomString,
		      ULONG CustomSize)
{
  ULONG Size = 0;
  ULONG i;

  if (CustomCP->DBCSCodePage == 0)
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
	  *UnicodeString = CustomCP->MultiByteTable[(int)*CustomString];
	  UnicodeString++;
	  CustomString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


WCHAR
RtlDowncaseUnicodeChar (IN WCHAR Source)
{
  USHORT Offset;

  if (Source < L'A')
    return Source;

  if (Source <= L'Z')
    return Source + (L'a' - L'A');

  if (Source < 0x80)
    return Source;

  Offset = ((USHORT)Source >> 8);
  DPRINT("Offset: %hx\n", Offset);

  Offset = NlsUnicodeLowercaseTable[Offset];
  DPRINT("Offset: %hx\n", Offset);

  Offset += (((USHORT)Source & 0x00F0) >> 4);
  DPRINT("Offset: %hx\n", Offset);

  Offset = NlsUnicodeLowercaseTable[Offset];
  DPRINT("Offset: %hx\n", Offset);

  Offset += ((USHORT)Source & 0x000F);
  DPRINT("Offset: %hx\n", Offset);

  Offset = NlsUnicodeLowercaseTable[Offset];
  DPRINT("Offset: %hx\n", Offset);

  DPRINT("Result: %hx\n", Source + (SHORT)Offset);

  return Source + (SHORT)Offset;
}


VOID STDCALL
RtlGetDefaultCodePage(OUT PUSHORT AnsiCodePage,
		      OUT PUSHORT OemCodePage)
{
  *AnsiCodePage = NlsAnsiCodePage;
  *OemCodePage = NlsOemCodePage;
}


VOID STDCALL
RtlInitCodePageTable(IN PUSHORT TableBase,
		     OUT PCPTABLEINFO CodePageTable)
{
  PNLS_FILE_HEADER NlsFileHeader;
  PUSHORT Ptr;
  USHORT Offset;

  DPRINT("RtlInitCodePageTable() called\n");

  NlsFileHeader = (PNLS_FILE_HEADER)TableBase;

  CodePageTable->CodePage = NlsFileHeader->CodePage;
  CodePageTable->MaximumCharacterSize = NlsFileHeader->MaximumCharacterSize;
  CodePageTable->DefaultChar = NlsFileHeader->DefaultChar;
  CodePageTable->UniDefaultChar = NlsFileHeader->UniDefaultChar;
  CodePageTable->TransDefaultChar = NlsFileHeader->TransDefaultChar;
  CodePageTable->TransUniDefaultChar = NlsFileHeader->TransUniDefaultChar;

  RtlCopyMemory(&CodePageTable->LeadByte,
		&NlsFileHeader->LeadByte,
		MAXIMUM_LEADBYTES);

  /* Set Pointer to start of multi byte table */
  Ptr = (PUSHORT)((ULONG_PTR)TableBase + 2 * NlsFileHeader->HeaderSize);

  /* Get offset to the wide char table */
  Offset = (USHORT)(*Ptr++) + NlsFileHeader->HeaderSize + 1;

  /* Set pointer to the multi byte table */
  CodePageTable->MultiByteTable = Ptr;

  /* Skip ANSI and OEM table */
  Ptr += 256;
  if (*Ptr++)
    Ptr += 256;

  /* Set pointer to DBCS ranges */
  CodePageTable->DBCSRanges = (PUSHORT)Ptr;

  if (*Ptr > 0)
    {
      CodePageTable->DBCSCodePage = 1;
      CodePageTable->DBCSOffsets = (PUSHORT)++Ptr;
    }
  else
    {
      CodePageTable->DBCSCodePage = 0;
      CodePageTable->DBCSOffsets = 0;
    }

  CodePageTable->WideCharTable = (PVOID)((ULONG_PTR)TableBase + 2 * Offset);
}


VOID STDCALL
RtlInitNlsTables(IN PUSHORT AnsiTableBase,
		 IN PUSHORT OemTableBase,
		 IN PUSHORT CaseTableBase,
		 OUT PNLSTABLEINFO NlsTable)
{
  DPRINT("RtlInitNlsTables()called\n");

  if (AnsiTableBase == NULL ||
      OemTableBase == NULL ||
      CaseTableBase == NULL)
    return;

  RtlInitCodePageTable (AnsiTableBase,
			&NlsTable->AnsiTableInfo);

  RtlInitCodePageTable (OemTableBase,
			&NlsTable->OemTableInfo);

  NlsTable->UpperCaseTable = (PUSHORT)CaseTableBase + 2;
  NlsTable->LowerCaseTable = (PUSHORT)CaseTableBase + *((PUSHORT)CaseTableBase + 1) + 2;
}


NTSTATUS STDCALL
RtlMultiByteToUnicodeN (IN OUT PWCHAR UnicodeString,
			IN ULONG UnicodeSize,
			OUT PULONG ResultSize,
			IN PCHAR MbString,
			IN ULONG MbSize)
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
	  *UnicodeString = NlsAnsiToUnicodeTable[*MbString];
	  UnicodeString++;
	  MbString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlMultiByteToUnicodeSize (OUT PULONG UnicodeSize,
			   IN PCHAR MbString,
			   IN ULONG MbSize)
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


NTSTATUS STDCALL
RtlOemToUnicodeN (PWCHAR UnicodeString,
		  ULONG UnicodeSize,
		  PULONG ResultSize,
		  PCHAR OemString,
		  ULONG OemSize)
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
	  *UnicodeString = NlsOemToUnicodeTable[*OemString];
	  UnicodeString++;
	  OemString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


VOID STDCALL
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable)
{
  DPRINT("RtlResetRtlTranslations() called\n");

  /* Set ANSI data */
  NlsAnsiToUnicodeTable = NlsTable->AnsiTableInfo.MultiByteTable;
  NlsUnicodeToAnsiTable = NlsTable->AnsiTableInfo.WideCharTable;
  NlsMbCodePageTag = (NlsTable->AnsiTableInfo.DBCSCodePage != 0);
  NlsLeadByteInfo = NlsTable->AnsiTableInfo.DBCSOffsets;
  NlsAnsiCodePage = NlsTable->AnsiTableInfo.CodePage;
  DPRINT("Ansi codepage %hu\n", NlsAnsiCodePage);

  /* Set OEM data */
  NlsOemToUnicodeTable = NlsTable->OemTableInfo.MultiByteTable;
  NlsUnicodeToOemTable = NlsTable->OemTableInfo.WideCharTable;
  NlsMbOemCodePageTag = (NlsTable->OemTableInfo.DBCSCodePage != 0);
  NlsOemLeadByteInfo = NlsTable->OemTableInfo.DBCSOffsets;
  NlsOemCodePage = NlsTable->OemTableInfo.CodePage;
  DPRINT("Oem codepage %hu\n", NlsOemCodePage);

  /* Set Unicode case map data */
  NlsUnicodeUpcaseTable = NlsTable->UpperCaseTable;
  NlsUnicodeLowercaseTable = NlsTable->LowerCaseTable;
}


NTSTATUS STDCALL
RtlUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
		      PCHAR CustomString,
		      ULONG CustomSize,
		      PULONG ResultSize,
		      PWCHAR UnicodeString,
		      ULONG UnicodeSize)
{
  ULONG Size = 0;
  ULONG i;

  if (CustomCP->DBCSCodePage == 0)
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
	  *CustomString = ((PCHAR)CustomCP->WideCharTable)[*UnicodeString];
	  CustomString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUnicodeToMultiByteN (PCHAR MbString,
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
	  *MbString = NlsUnicodeToAnsiTable[*UnicodeString];
	  MbString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUnicodeToMultiByteSize (PULONG MbSize,
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
      *MbSize = 0;
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUnicodeToOemN (PCHAR OemString,
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
	  *OemString = NlsUnicodeToOemTable[*UnicodeString];
	  OemString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


WCHAR STDCALL
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
  USHORT Offset;

  if (Source < L'a')
    return Source;

  if (Source <= L'z')
    return (Source - (L'a' - L'A'));

  Offset = ((USHORT)Source >> 8);
  Offset = NlsUnicodeUpcaseTable[Offset];

  Offset += (((USHORT)Source & 0x00F0) >> 4);
  Offset = NlsUnicodeUpcaseTable[Offset];

  Offset += ((USHORT)Source & 0x000F);
  Offset = NlsUnicodeUpcaseTable[Offset];

  return Source + (SHORT)Offset;
}


NTSTATUS STDCALL
RtlUpcaseUnicodeToCustomCPN (IN PCPTABLEINFO CustomCP,
			     PCHAR CustomString,
			     ULONG CustomSize,
			     PULONG ResultSize,
			     PWCHAR UnicodeString,
			     ULONG UnicodeSize)
{
  WCHAR UpcaseChar;
  ULONG Size = 0;
  ULONG i;

  if (CustomCP->DBCSCodePage == 0)
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
	  UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
	  *CustomString = ((PCHAR)CustomCP->WideCharTable)[UpcaseChar];
	  CustomString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUpcaseUnicodeToMultiByteN (PCHAR MbString,
			      ULONG MbSize,
			      PULONG ResultSize,
			      PWCHAR UnicodeString,
			      ULONG UnicodeSize)
{
  WCHAR UpcaseChar;
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
	  UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
	  *MbString = NlsUnicodeToAnsiTable[UpcaseChar];
	  MbString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlUpcaseUnicodeToOemN (PCHAR OemString,
			ULONG OemSize,
			PULONG ResultSize,
			PWCHAR UnicodeString,
			ULONG UnicodeSize)
{
  WCHAR UpcaseChar;
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
	  UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
	  *OemString = NlsUnicodeToOemTable[UpcaseChar];
	  OemString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
      assert(FALSE);
    }

  return STATUS_SUCCESS;
}


CHAR STDCALL
RtlUpperChar (IN CHAR Source)
{
  WCHAR Unicode;
  CHAR Destination;

  if (NlsMbCodePageTag == FALSE)
    {
      /* single-byte code page */

      /* ansi->unicode */
      Unicode = NlsAnsiToUnicodeTable[Source];

      /* upcase conversion */
      Unicode = RtlUpcaseUnicodeChar (Unicode);

      /* unicode -> ansi */
      Destination = NlsUnicodeToAnsiTable[Unicode];
    }
  else
    {
      /* single-byte code page */
      /* FIXME: implement the multi-byte stuff!! */
      Destination = Source;
    }

  return Destination;
}

/* EOF */
