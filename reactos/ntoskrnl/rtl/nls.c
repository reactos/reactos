/* $Id: nls.c,v 1.10 2002/11/10 13:36:59 robd Exp $
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

#ifdef WIN32_REGDBG
#include "cm_win32.h"
#else

#include <ddk/ntddk.h>
//#include <internal/nls.h>

#define NDEBUG
#include <internal/debug.h>

#endif

/* GLOBALS *******************************************************************/

BOOLEAN NlsMbCodePageTag = FALSE;
BOOLEAN NlsMbOemCodePageTag = FALSE;

BYTE NlsLeadByteInfo = 0; /* ? */

USHORT NlsOemLeadByteInfo = 0;

USHORT NlsAnsiCodePage = 0;
USHORT NlsOemCodePage = 0; /* not exported */

PWCHAR AnsiToUnicodeTable = NULL; /* size: 256*sizeof(WCHAR) */
PWCHAR OemToUnicodeTable = NULL; /* size: 256*sizeof(WCHAR) */

PCHAR UnicodeToAnsiTable = NULL; /* size: 65536*sizeof(CHAR) */
PCHAR UnicodeToOemTable =NULL; /* size: 65536*sizeof(CHAR) */

PWCHAR UnicodeUpcaseTable = NULL; /* size: 65536*sizeof(WCHAR) */
PWCHAR UnicodeLowercaseTable = NULL; /* size: 65536*sizeof(WCHAR) */


/* FUNCTIONS *****************************************************************/

VOID
RtlpInitNlsTables(VOID)
{
  INT i;
  PCHAR pc;
  PWCHAR pwc;

  /* allocate and initialize ansi->unicode table */
  AnsiToUnicodeTable = ExAllocatePool(NonPagedPool, 256 * sizeof(WCHAR));
  if (AnsiToUnicodeTable == NULL)
    {
      DbgPrint("Allocation of 'AnsiToUnicodeTable' failed\n");
      KeBugCheck(0);
    }

  pwc = AnsiToUnicodeTable;
  for (i = 0; i < 256; i++, pwc++)
    *pwc = (WCHAR)i;

  /* allocate and initialize oem->unicode table */
  OemToUnicodeTable = ExAllocatePool(NonPagedPool, 256 * sizeof(WCHAR));
  if (OemToUnicodeTable == NULL)
    {
      DbgPrint("Allocation of 'OemToUnicodeTable' failed\n");
      KeBugCheck(0);
    }

  pwc = OemToUnicodeTable;
  for (i = 0; i < 256; i++, pwc++)
    *pwc = (WCHAR)i;

  /* allocate and initialize unicode->ansi table */
  UnicodeToAnsiTable = ExAllocatePool(NonPagedPool, 65536 * sizeof(CHAR));
  if (UnicodeToAnsiTable == NULL)
    {
      DbgPrint("Allocation of 'UnicodeToAnsiTable' failed\n");
      KeBugCheck(0);
    }

  pc = UnicodeToAnsiTable;
  for (i = 0; i < 256; i++, pc++)
    *pc = (CHAR)i;
  for (; i < 65536; i++, pc++)
    *pc = 0;

  /* allocate and initialize unicode->oem table */
  UnicodeToOemTable = ExAllocatePool(NonPagedPool, 65536 * sizeof(CHAR));
  if (UnicodeToOemTable == NULL)
    {
      DbgPrint("Allocation of 'UnicodeToOemTable' failed\n");
      KeBugCheck(0);
    }

  pc = UnicodeToOemTable;
  for (i = 0; i < 256; i++, pc++)
    *pc = (CHAR)i;
  for (; i < 65536; i++, pc++)
    *pc = 0;

  /* allocate and initialize unicode upcase table */
  UnicodeUpcaseTable = ExAllocatePool(NonPagedPool, 65536 * sizeof(WCHAR));
  if (UnicodeUpcaseTable == NULL)
    {
      DbgPrint("Allocation of 'UnicodeUpcaseTable' failed\n");
      KeBugCheck(0);
    }

  pwc = UnicodeUpcaseTable;
  for (i = 0; i < 65536; i++, pwc++)
    *pwc = (WCHAR)i;
  for (i = 'a'; i < ('z'+ 1); i++)
    UnicodeUpcaseTable[i] = (WCHAR)i + (L'A' - L'a');


  /* allocate and initialize unicode lowercase table */
  UnicodeLowercaseTable = ExAllocatePool(NonPagedPool, 65536 * sizeof(WCHAR));
  if (UnicodeLowercaseTable == NULL)
    {
      DbgPrint("Allocation of 'UnicodeLowercaseTable' failed\n");
      KeBugCheck(0);
    }

  pwc = UnicodeLowercaseTable;
  for (i = 0; i < 65536; i++, pwc++)
    *pwc = (WCHAR)i;
  for (i = 'A'; i < ('Z'+ 1); i++)
    UnicodeLowercaseTable[i] = (WCHAR)i - (L'A' - L'a');

  /* FIXME: initialize codepage info */

}


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
  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
	  *UnicodeString = NlsData->MultiByteToUnicode[(unsigned int)*CustomString];
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
	  *UnicodeString = AnsiToUnicodeTable[(unsigned int)*MbString];
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
	  *UnicodeString = OemToUnicodeTable[(unsigned int)*OemString];
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


NTSTATUS STDCALL
RtlUnicodeToCustomCPN(PRTL_NLS_DATA NlsData,
		      PCHAR CustomString,
		      ULONG CustomSize,
		      PULONG ResultSize,
		      PWCHAR UnicodeString,
		      ULONG UnicodeSize)
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
	  *CustomString = NlsData->UnicodeToMultiByte[(unsigned int)*UnicodeString];
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
	  *MbString = UnicodeToAnsiTable[(unsigned int)*UnicodeString];
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
	  *OemString = UnicodeToOemTable[(unsigned int)*UnicodeString];
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


NTSTATUS STDCALL
RtlUpcaseUnicodeToCustomCPN(PRTL_NLS_DATA NlsData,
			    PCHAR CustomString,
			    ULONG CustomSize,
			    PULONG ResultSize,
			    PWCHAR UnicodeString,
			    ULONG UnicodeSize)
{
  ULONG Size = 0;
  ULONG i;
  WCHAR wc;

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
	  wc = UnicodeUpcaseTable[(unsigned int)*UnicodeString];
	  *CustomString = NlsData->UnicodeToMultiByte[(unsigned int)wc];
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
	  wc = UnicodeUpcaseTable[(unsigned int)*UnicodeString];
	  *MbString = UnicodeToAnsiTable[(unsigned int)wc];
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
	  wc = UnicodeUpcaseTable[(unsigned int)*UnicodeString];
	  *OemString = UnicodeToOemTable[(unsigned int)wc];
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

/* EOF */
