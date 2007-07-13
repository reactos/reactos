/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/frommem.c
 * PURPOSE:         Creating rossym info from an in-memory image
 * 
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#ifdef _MSC_VER
#include "ntimage.h"
#endif

#define NDEBUG
#include <debug.h>

BOOLEAN
RosSymCreateFromMem(PVOID ImageStart, ULONG_PTR ImageSize, PROSSYM_INFO *RosSymInfo)
{
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_NT_HEADERS NtHeaders;
  PIMAGE_SECTION_HEADER SectionHeader;
  unsigned SectionIndex;
  char SectionName[IMAGE_SIZEOF_SHORT_NAME];

  /* Check if MZ header is valid */
  DosHeader = (PIMAGE_DOS_HEADER) ImageStart;
  if (ImageSize < sizeof(IMAGE_DOS_HEADER)
      || ! ROSSYM_IS_VALID_DOS_HEADER(DosHeader))
    {
      DPRINT1("Image doesn't have a valid DOS header\n");
      return FALSE;
    }

  /* Locate NT header  */
  NtHeaders = (PIMAGE_NT_HEADERS)((char *) ImageStart + DosHeader->e_lfanew);
  if (ImageSize < DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)
      || ! ROSSYM_IS_VALID_NT_HEADERS(NtHeaders))
    {
      DPRINT1("Image doesn't have a valid PE header\n");
      return FALSE;
    }

  /* Search for the section header */
  SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);
  if (ImageSize < (ULONG_PTR)((char *) (SectionHeader + NtHeaders->FileHeader.NumberOfSections)
                              - (char *) ImageStart))
    {
      DPRINT1("Image doesn't have valid section headers\n");
      return FALSE;
    }
  strncpy(SectionName, ROSSYM_SECTION_NAME, IMAGE_SIZEOF_SHORT_NAME);
  for (SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections; SectionIndex++)
    {
      if (0 == memcmp(SectionName, SectionHeader->Name, IMAGE_SIZEOF_SHORT_NAME))
        {
          break;
        }
      SectionHeader++;
    }
  if (NtHeaders->FileHeader.NumberOfSections <= SectionIndex)
    {
      DPRINT("No %s section found\n", ROSSYM_SECTION_NAME);
      return FALSE;
    }

  /* Locate the section itself */
  if (ImageSize < SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData
      || SectionHeader->SizeOfRawData < sizeof(ROSSYM_HEADER))
    {
      DPRINT1("Invalid %s section\n", ROSSYM_SECTION_NAME);
      return FALSE;
    }

  /* Load it */
  return RosSymCreateFromRaw((char *) ImageStart + SectionHeader->VirtualAddress,
                             SectionHeader->SizeOfRawData, RosSymInfo);
}

/* EOF */
