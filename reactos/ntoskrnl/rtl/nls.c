/* $Id: nls.c,v 1.23 2004/05/31 19:40:49 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/nls.c
 * PURPOSE:         Bitmap functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Eric Kohl
 */

#include <ddk/ntddk.h>

#include <internal/mm.h>
#include <internal/nls.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/


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



VOID INIT_FUNCTION
RtlpImportAnsiCodePage(PUSHORT TableBase,
             ULONG Size)
{
  NlsAnsiCodePageTable = TableBase;
  NlsAnsiCodePageTableSize = Size;
}


VOID INIT_FUNCTION
RtlpImportOemCodePage(PUSHORT TableBase,
            ULONG Size)
{
  NlsOemCodePageTable = TableBase;
  NlsOemCodePageTableSize = Size;
}


VOID INIT_FUNCTION
RtlpImportUnicodeCasemap(PUSHORT TableBase,
          ULONG Size)
{
  NlsUnicodeCasemapTable = TableBase;
  NlsUnicodeCasemapTableSize = Size;
}


VOID INIT_FUNCTION
RtlpCreateInitialNlsTables(VOID)
{
  NLSTABLEINFO NlsTable;

  if (NlsAnsiCodePageTable == NULL || NlsAnsiCodePageTableSize == 0 ||
      NlsOemCodePageTable == NULL || NlsOemCodePageTableSize == 0 ||
      NlsUnicodeCasemapTable == NULL || NlsUnicodeCasemapTableSize == 0)
    {
      KEBUGCHECKEX (0x32, STATUS_UNSUCCESSFUL, 1, 0, 0);
    }

  RtlInitNlsTables (NlsAnsiCodePageTable,
          NlsOemCodePageTable,
          NlsUnicodeCasemapTable,
          &NlsTable);

  RtlResetRtlTranslations (&NlsTable);
}


VOID INIT_FUNCTION
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
      KEBUGCHECKEX(0x32, Status, 1, 1, 0);
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
      KEBUGCHECKEX(0x32, Status, 1, 2, 0);
    }

  Status = MmMapViewInSystemSpace(NlsSectionObject,
              &NlsSectionBase,
              &NlsSectionViewSize);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewInSystemSpace() failed\n");
      KEBUGCHECKEX(0x32, Status, 1, 3, 0);
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
