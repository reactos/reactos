/* $Id: nls.c,v 1.14 2003/05/20 14:38:05 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/nls.c
 * PURPOSE:         National Language Support (NLS) functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Emanuele Aliberti
 *                  10/11/99 Added translation functions.
 *
 * TODO:
 *   1) Add multi-byte translation code.
 */

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/nls.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/


UCHAR NlsLeadByteInfo = 0; /* exported */

USHORT NlsOemLeadByteInfo = 0; /* exported */


USHORT NlsAnsiCodePage = 0; /* exported */
BOOLEAN NlsMbCodePageTag = FALSE; /* exported */
PWCHAR NlsAnsiToUnicodeTable = NULL;
PCHAR NlsUnicodeToAnsiTable = NULL;


USHORT NlsOemCodePage = 0;
BOOLEAN NlsMbOemCodePageTag = FALSE; /* exported */
PWCHAR NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable =NULL;


PUSHORT NlsUnicodeUpcaseTable = NULL;
PUSHORT NlsUnicodeLowercaseTable = NULL;


static PUSHORT NlsAnsiCodePageTable = NULL;
static ULONG NlsAnsiCodePageTableSize = 0;

static PUSHORT NlsOemCodePageTable = NULL;
static ULONG NlsOemCodePageTableSize = 0;

static PUSHORT NlsUnicodeCasemapTable = NULL;
static ULONG NlsUnicodeCasemapTableSize = 0;

PVOID NlsSectionObject = NULL;
static PVOID NlsSectionBase = NULL;
static ULONG NlsSectionViewSize = 0;

ULONG NlsAnsiTableOffset = 0;
ULONG NlsOemTableOffset = 0;
ULONG NlsUnicodeTableOffset = 0;


/* FUNCTIONS *****************************************************************/

VOID
RtlpImportAnsiCodePage(PUSHORT TableBase,
		       ULONG Size)
{
  NlsAnsiCodePageTable = TableBase;
  NlsAnsiCodePageTableSize = Size;
}


VOID
RtlpImportOemCodePage(PUSHORT TableBase,
		      ULONG Size)
{
  NlsOemCodePageTable = TableBase;
  NlsOemCodePageTableSize = Size;
}


VOID
RtlpImportUnicodeCasemap(PUSHORT TableBase,
			 ULONG Size)
{
  NlsUnicodeCasemapTable = TableBase;
  NlsUnicodeCasemapTableSize = Size;
}


VOID
RtlpCreateInitialNlsTables(VOID)
{
  NLSTABLEINFO NlsTable;

  if (NlsAnsiCodePageTable == NULL || NlsAnsiCodePageTableSize == 0 ||
      NlsOemCodePageTable == NULL || NlsOemCodePageTableSize == 0 ||
      NlsUnicodeCasemapTable == NULL || NlsUnicodeCasemapTableSize == 0)
    {
      KeBugCheckEx (0x32, STATUS_UNSUCCESSFUL, 1, 0, 0);
    }

  RtlInitNlsTables (NlsAnsiCodePageTable,
		    NlsOemCodePageTable,
		    NlsUnicodeCasemapTable,
		    &NlsTable);

  RtlResetRtlTranslations (&NlsTable);
}




VOID
RtlpCreateNlsSection(VOID)
{
  NLSTABLEINFO NlsTable;
  LARGE_INTEGER SectionSize;
  HANDLE SectionHandle;
  NTSTATUS Status;

  DPRINT("RtlpCreateNlsSection() called\n");

  NlsSectionViewSize = ROUND_UP(NlsAnsiCodePageTableSize, PAGE_SIZE) +
		       ROUND_UP(NlsOemCodePageTableSize, PAGE_SIZE) +
		       ROUND_UP(NlsUnicodeCasemapTableSize, PAGE_SIZE);

  DPRINT("NlsSectionViewSize %lx\n", NlsSectionViewSize);

  SectionSize.QuadPart = (LONGLONG)NlsSectionViewSize;
  Status = NtCreateSection(&SectionHandle,
			   SECTION_ALL_ACCESS,
			   NULL,
			   &SectionSize,
			   PAGE_READWRITE,
			   SEC_COMMIT,
			   NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection() failed\n");
      KeBugCheckEx(0x32, Status, 1, 1, 0);
    }

  Status = ObReferenceObjectByHandle(SectionHandle,
				     SECTION_ALL_ACCESS,
				     MmSectionObjectType,
				     KernelMode,
				     &NlsSectionObject,
				     NULL);
  NtClose(SectionHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObReferenceObjectByHandle() failed\n");
      KeBugCheckEx(0x32, Status, 1, 2, 0);
    }

  Status = MmMapViewInSystemSpace(NlsSectionObject,
				  &NlsSectionBase,
				  &NlsSectionViewSize);
  NtClose(SectionHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewInSystemSpace() failed\n");
      KeBugCheckEx(0x32, Status, 1, 3, 0);
    }

  DPRINT("NlsSection: Base %p  Size %lx\n", 
	 NlsSectionBase,
	 NlsSectionViewSize);

  NlsAnsiTableOffset = 0;
  RtlCopyMemory((PVOID)((ULONG)NlsSectionBase + NlsAnsiTableOffset),
		NlsAnsiCodePageTable,
		NlsAnsiCodePageTableSize);

  NlsOemTableOffset = NlsAnsiTableOffset + ROUND_UP(NlsAnsiCodePageTableSize, PAGE_SIZE);
  RtlCopyMemory((PVOID)((ULONG)NlsSectionBase + NlsOemTableOffset),
		NlsOemCodePageTable,
		NlsOemCodePageTableSize);

  NlsUnicodeTableOffset = NlsOemTableOffset + ROUND_UP(NlsOemCodePageTableSize, PAGE_SIZE);
  RtlCopyMemory((PVOID)((ULONG)NlsSectionBase + NlsUnicodeTableOffset),
		NlsUnicodeCasemapTable,
		NlsUnicodeCasemapTableSize);

  RtlInitNlsTables ((PVOID)((ULONG)NlsSectionBase + NlsAnsiTableOffset),
		    (PVOID)((ULONG)NlsSectionBase + NlsOemTableOffset),
		    (PVOID)((ULONG)NlsSectionBase + NlsUnicodeTableOffset),
		    &NlsTable);

  RtlResetRtlTranslations (&NlsTable);
}


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
	  *UnicodeString = CustomCP->MultiByteTable[(unsigned int)*CustomString];
	  UnicodeString++;
	  CustomString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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
  Offset = NlsUnicodeLowercaseTable[Offset];

  Offset += (((USHORT)Source & 0x00F0) >> 4);
  Offset = NlsUnicodeLowercaseTable[Offset];

  Offset += ((USHORT)Source & 0x000F);
  Offset = NlsUnicodeLowercaseTable[Offset];

  return Source + (SHORT)Offset;
}


VOID STDCALL
RtlGetDefaultCodePage(PUSHORT AnsiCodePage,
		      PUSHORT OemCodePage)
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

  RtlInitCodePageTable (AnsiTableBase,
			&NlsTable->AnsiTableInfo);

  RtlInitCodePageTable (OemTableBase,
			&NlsTable->OemTableInfo);

  NlsTable->UpperCaseTable = (PUSHORT)CaseTableBase + 2;
  NlsTable->LowerCaseTable = (PUSHORT)CaseTableBase + *((PUSHORT)CaseTableBase + 1) + 2;
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
	  *UnicodeString = NlsAnsiToUnicodeTable[(unsigned int)*MbString];
	  UnicodeString++;
	  MbString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlOemToUnicodeN(PWCHAR UnicodeString,
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
	  *UnicodeString = NlsOemToUnicodeTable[(unsigned int)*OemString];
	  UnicodeString++;
	  OemString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
}


VOID STDCALL
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable)
{
  DPRINT("RtlResetRtlTranslations() called\n");

  /* Set ANSI data */
  NlsAnsiToUnicodeTable = NlsTable->AnsiTableInfo.MultiByteTable;
  NlsUnicodeToAnsiTable = NlsTable->AnsiTableInfo.WideCharTable;
  NlsAnsiCodePage = NlsTable->AnsiTableInfo.CodePage;
  DPRINT("Ansi codepage %hu\n", NlsAnsiCodePage);

  /* Set OEM data */
  NlsOemToUnicodeTable = NlsTable->OemTableInfo.MultiByteTable;
  NlsUnicodeToOemTable = NlsTable->OemTableInfo.WideCharTable;
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
	  *CustomString = ((PCHAR)CustomCP->WideCharTable)[(unsigned int)*UnicodeString];
	  CustomString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlUnicodeToMultiByteN(PCHAR MbString,
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
	  *MbString = NlsUnicodeToAnsiTable[(unsigned int)*UnicodeString];
	  MbString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlUnicodeToOemN(PCHAR OemString,
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
	  *OemString = NlsUnicodeToOemTable[(unsigned int)*UnicodeString];
	  OemString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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
RtlUpcaseUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
			    PCHAR CustomString,
			    ULONG CustomSize,
			    PULONG ResultSize,
			    PWCHAR UnicodeString,
			    ULONG UnicodeSize)
{
  ULONG Size = 0;
  ULONG i;
  WCHAR wc;

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
	  wc = RtlUpcaseUnicodeChar(*UnicodeString);
	  *CustomString = ((PCHAR)CustomCP->WideCharTable)[(unsigned int)wc];
	  CustomString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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
  WCHAR wc;

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
	  wc = RtlUpcaseUnicodeChar(*UnicodeString);
	  *MbString = NlsUnicodeToAnsiTable[(unsigned int)wc];
	  MbString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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
  UCHAR wc;

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
	  wc = RtlUpcaseUnicodeChar(*UnicodeString);
	  *OemString = NlsUnicodeToOemTable[(unsigned int)wc];
	  OemString++;
	  UnicodeString++;
	}
    }
  else
    {
      /* multi-byte code page */
      /* FIXME */
    }

  return(STATUS_SUCCESS);
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
      Unicode = NlsAnsiToUnicodeTable[(unsigned int)Source];

      /* upcase conversion */
      Unicode = RtlUpcaseUnicodeChar (Unicode);

      /* unicode -> ansi */
      Destination = NlsUnicodeToAnsiTable[(unsigned int)Unicode];
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
