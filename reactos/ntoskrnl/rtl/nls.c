/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/nls.c
 * PURPOSE:         Bitmap functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/


static PUSHORT NlsAnsiCodePageTable = NULL;
static ULONG NlsAnsiCodePageTableSize = 0;

static PUSHORT NlsOemCodePageTable = NULL;
static ULONG NlsOemCodePageTableSize = 0;

static PUSHORT NlsUnicodeCasemapTable = NULL;
static ULONG NlsUnicodeCasemapTableSize = 0;

PSECTION_OBJECT NlsSectionObject = NULL;
static PVOID NlsSectionBase = NULL;
static ULONG NlsSectionViewSize = 0;

ULONG NlsAnsiTableOffset = 0;
ULONG NlsOemTableOffset = 0;
ULONG NlsUnicodeTableOffset = 0;


/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
STDCALL
RtlpInitNls(VOID)
{
    ULONG_PTR BaseAddress;

    /* Import NLS Data */
    BaseAddress = CachedModules[AnsiCodepage]->ModStart;
    RtlpImportAnsiCodePage((PUSHORT)BaseAddress,
                           CachedModules[AnsiCodepage]->ModEnd - BaseAddress);

    BaseAddress = CachedModules[OemCodepage]->ModStart;
    RtlpImportOemCodePage((PUSHORT)BaseAddress,
                          CachedModules[OemCodepage]->ModEnd - BaseAddress);

    BaseAddress = CachedModules[UnicodeCasemap]->ModStart;
    RtlpImportUnicodeCasemap((PUSHORT)BaseAddress,
                             CachedModules[UnicodeCasemap]->ModEnd - BaseAddress);

    /* Create initial NLS tables */
    RtlpCreateInitialNlsTables();

    /* Create the NLS section */
    RtlpCreateNlsSection();
}

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
  NTSTATUS Status;

  DPRINT("RtlpCreateNlsSection() called\n");

  NlsSectionViewSize = ROUND_UP(NlsAnsiCodePageTableSize, PAGE_SIZE) +
             ROUND_UP(NlsOemCodePageTableSize, PAGE_SIZE) +
             ROUND_UP(NlsUnicodeCasemapTableSize, PAGE_SIZE);

  DPRINT("NlsSectionViewSize %lx\n", NlsSectionViewSize);

  SectionSize.QuadPart = (LONGLONG)NlsSectionViewSize;
  Status = MmCreateSection(&NlsSectionObject,
            SECTION_ALL_ACCESS,
            NULL,
            &SectionSize,
            PAGE_READWRITE,
            SEC_COMMIT,
            NULL,
            NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmCreateSection() failed\n");
      KEBUGCHECKEX(0x32, Status, 1, 1, 0);
    }
   Status = ObInsertObject(NlsSectionObject,
                           NULL,
                           SECTION_ALL_ACCESS,
                           0,
                           NULL,
                           NULL);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(NlsSectionObject);
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
